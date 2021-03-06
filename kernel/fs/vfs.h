#ifndef __FS__VFS_H__
#define __FS__VFS_H__

#include <stdbool.h>
#include <lib/types.h>
#include <lib/resource.h>
#include <lib/lock.h>

extern lock_t vfs_lock;

struct filesystem {
    const char *name;
    bool needs_backing_device;
    struct vfs_node *(*mount)(struct resource *device);
    struct vfs_node *(*populate)(struct vfs_node *node);
    struct resource *(*open)(struct vfs_node *node, bool new_node, mode_t mode);
};

#define VFS_ROOT_INODE ((ino_t)0xffffffffffffffff)

struct vfs_node {
    char name[NAME_MAX];
    struct resource *res;
    void *mount_data;
    dev_t backing_dev_id;
    struct filesystem *fs;
    struct vfs_node *mount_gate;
    struct vfs_node *parent;
    struct vfs_node *child;
    struct vfs_node *next;
};

struct vfs_node *vfs_new_node(struct vfs_node *parent, const char *name);
struct vfs_node *vfs_new_node_deep(struct vfs_node *parent, const char *name);
void vfs_dump_nodes(struct vfs_node *node, const char *parent);
void vfs_get_absolute_path(char *path_ptr, const char *path, const char *pwd);
bool vfs_install_fs(struct filesystem *fs);
bool vfs_mount(const char *source, const char *target, const char *fs);
struct resource *vfs_open(const char *path, int oflags, mode_t mode);
bool vfs_stat(const char *path, struct stat *st);

#endif
