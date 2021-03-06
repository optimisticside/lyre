#ifndef __SCHED__SCHED_H__
#define __SCHED__SCHED_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/types.h>
#include <lib/dynarray.h>
#include <lib/lock.h>
#include <lib/elf.h>
#include <sys/cpu.h>
#include <mm/vmm.h>

struct process;

struct thread {
    tid_t tid;
    lock_t lock;
    size_t timeslice;
    struct cpu_gpr_context ctx;
    struct process *process;
    uintptr_t user_gs;
    uintptr_t kernel_stack;
};

struct process {
    pid_t pid;
    struct pagemap *pagemap;
    DYNARRAY_STRUCT(struct thread *) threads;
    uintptr_t thread_stack_top;
};

void sched_init(void);

struct thread *sched_new_thread(struct process *proc,
                                bool want_elf,
                                void *addr,
                                void *arg,
                                const char **argv,
                                const char **envp,
                                struct auxval_t *auxval);

#endif
