#include <stdint.h>
#include <stddef.h>
#include <mm/pmm.h>
#include <mm/vmm.h>
#include <lib/bitmap.h>
#include <lib/math.h>
#include <lib/lock.h>
#include <lib/builtins.h>

static void *bitmap;
static size_t last_used_index = 0;
static uintptr_t highest_page = 0;

static lock_t pmm_lock;

void pmm_init(struct stivale2_mmap_entry *memmap, size_t memmap_entries) {
    // First, calculate how big the bitmap needs to be.
    for (size_t i = 0; i < memmap_entries; i++) {
        if (memmap[i].type != STIVALE2_MMAP_USABLE)
            continue;

        uintptr_t top = memmap[i].base + memmap[i].length;

        if (top > highest_page)
            highest_page = top;
    }

    size_t bitmap_size = DIV_ROUNDUP(highest_page, PAGE_SIZE) / 8;

    // Second, find a location with enough free pages to host the bitmap.
    for (size_t i = 0; i < memmap_entries; i++) {
        if (memmap[i].type != STIVALE2_MMAP_USABLE)
            continue;

        if (memmap[i].length >= bitmap_size) {
            bitmap = (void *)(memmap[i].base + MEM_PHYS_OFFSET);

            // Initialise entire bitmap to 1 (non-free)
            memset(bitmap, 0xff, bitmap_size);

            memmap[i].length -= bitmap_size;
            memmap[i].base += bitmap_size;

            break;
        }
    }

    // Third, populate free bitmap entries according to memory map.
    for (size_t i = 0; i < memmap_entries; i++) {
        if (memmap[i].type != STIVALE2_MMAP_USABLE)
            continue;

        for (uintptr_t j = 0; j < memmap[i].length; j += PAGE_SIZE)
            bitmap_unset(bitmap, (memmap[i].base + j) / PAGE_SIZE);
    }
}

static void *inner_alloc(size_t count, size_t limit) {
    size_t p = 0;

    while (last_used_index < limit) {
        if (!bitmap_test(bitmap, last_used_index++)) {
            if (++p == count) {
                size_t page = last_used_index - count;
                for (size_t i = page; i < last_used_index; i++) {
                    bitmap_set(bitmap, i);
                }
                return (void *)(page * PAGE_SIZE);
            }
        } else {
            p = 0;
        }
    }

    return NULL;
}

void *pmm_alloc(size_t count) {
    SPINLOCK_ACQUIRE(pmm_lock);

    size_t l = last_used_index;
    void *ret = inner_alloc(count, highest_page / PAGE_SIZE);
    if (ret == NULL) {
        last_used_index = 0;
        ret = inner_alloc(count, l);
    }

    LOCK_RELEASE(pmm_lock);
    return ret;
}

void *pmm_allocz(size_t count) {
    char *ret = (char *)pmm_alloc(count);

    if (ret == NULL)
        return NULL;

    uint64_t *ptr = (uint64_t *)(ret + MEM_PHYS_OFFSET);

    for (size_t i = 0; i < count * (PAGE_SIZE / sizeof(uint64_t)); i++)
        ptr[i] = 0;

    return ret;
}

void pmm_free(void *ptr, size_t count) {
    SPINLOCK_ACQUIRE(pmm_lock);
    size_t page = (size_t)ptr / PAGE_SIZE;
    for (size_t i = page; i < page + count; i++)
        bitmap_unset(bitmap, i);
    LOCK_RELEASE(pmm_lock);
}
