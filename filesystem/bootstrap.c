#include <stddef.h>
#include <stdint.h>

#include <axiom/boot/limine.h>
#include <axiom/filesystem/bootstrap.h>
#include <axiom/filesystem/ramfs.h>
#include <axiom/filesystem/vfs.h>

static const char motd[] =
    "Welcome to the AxiomOS virtual filesystem!\n";

static const char readme[] =
    "AxiomOS Phase 12: files now live behind the VFS abstraction.\n";

static int seed_module_file(const char *module_string, const char *vfs_path)
{
    const void *address;
    uint64_t size;
    const char *boot_path;

    if (!limine_get_module(module_string, &address, &size, &boot_path) ||
        size > (uint64_t)SIZE_MAX) {
        return 0;
    }

    (void)boot_path;
    if (vfs_write_file(vfs_path, address, (size_t)size) != 0) {
        return 0;
    }
    /* Boot-seeded system programs are readable + executable, not writable. */
    return vfs_set_mode(vfs_path, 0555u) == 0;
}

int filesystem_phase12_bootstrap(void)
{
    struct vfs_filesystem *rootfs;
    struct vfs_filesystem *tmpfs;

    if (!vfs_init()) {
        return 0;
    }

    rootfs = ramfs_create("rootfs");
    tmpfs = ramfs_create("tmpfs");
    if (rootfs == 0 || tmpfs == 0) {
        return 0;
    }

    if (vfs_mount("/", rootfs) < 0 ||
        vfs_mkdir("/bin") < 0 ||
        vfs_mkdir("/etc") < 0 ||
        vfs_mkdir("/dev") < 0 ||
        vfs_mkdir("/home") < 0 ||
        vfs_mkdir("/tmp") < 0 ||
        vfs_mkdir("/proc") < 0) {
        return 0;
    }

    if (vfs_write_file("/etc/motd", motd, sizeof(motd) - 1u) < 0 ||
        vfs_write_file("/home/readme.txt", readme, sizeof(readme) - 1u) < 0) {
        return 0;
    }

    /*
     * System executables are boot-seeded into the root RAMFS for now. The VFS
     * and ELF layers do not care whether a later backend supplies these bytes.
     */
    if (!seed_module_file("phase11-launcher", "/bin/phase11-launcher") ||
        !seed_module_file("phase11-demo", "/bin/phase11-demo") ||
        !seed_module_file("phase12-demo", "/bin/phase12-demo") ||
        !seed_module_file("phase13-demo", "/bin/phase13-demo") ||
        !seed_module_file("phase14-shell", "/bin/axiomsh") ||
        !seed_module_file("phase15-demo", "/bin/phase15-demo") ||
        !seed_module_file("phase15-sleeper", "/bin/phase15-sleeper") ||
        !seed_module_file("phase19-demo", "/bin/phase19-demo") ||
        !seed_module_file("phase20-demo", "/bin/phase20-demo") ||
        !seed_module_file("phase20-wx", "/bin/phase20-wx") ||
        !seed_module_file("phase21-schedbench", "/bin/schedbench") ||
        !seed_module_file("phase22-ipcdemo", "/bin/ipcdemo") ||
        !seed_module_file("phase22-ipc-peer", "/bin/ipc-peer") ||
        !seed_module_file("phase23-gfxdemo", "/bin/gfxdemo") ||
        !seed_module_file("phase24-ifconfig", "/bin/ifconfig") ||
        !seed_module_file("phase24-ping", "/bin/ping") ||
        !seed_module_file("phase24-dnslookup", "/bin/dnslookup") ||
        !seed_module_file("phase24-httpget", "/bin/httpget") ||
        !seed_module_file("phase24-httpd", "/bin/httpd") ||
        !seed_module_file("phase25-sysinfo", "/bin/sysinfo") ||
        !seed_module_file("phase26-desktop", "/bin/desktop")) {
        return 0;
    }

    /* A second RAM filesystem proves that path resolution honors mounts. */
    if (vfs_mount("/tmp", tmpfs) < 0) {
        return 0;
    }

    return 1;
}
