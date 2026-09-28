#ifndef AXIOM_ABI_FS_H
#define AXIOM_ABI_FS_H

/* open(2)-style access and creation flags used by the AxiomOS ABI. */
#define AXIOM_O_RDONLY  0x0000
#define AXIOM_O_WRONLY  0x0001
#define AXIOM_O_RDWR    0x0002
#define AXIOM_O_ACCMODE 0x0003
#define AXIOM_O_CREAT   0x0040
#define AXIOM_O_TRUNC   0x0200
#define AXIOM_O_APPEND  0x0400

#define AXIOM_SEEK_SET 0
#define AXIOM_SEEK_CUR 1
#define AXIOM_SEEK_END 2

#define AXIOM_DT_FILE 1
#define AXIOM_DT_DIR  2

/* Phase 20: simple ownerless Unix-style permission bits. */
#define AXIOM_S_IRUSR 0400u
#define AXIOM_S_IWUSR 0200u
#define AXIOM_S_IXUSR 0100u
#define AXIOM_S_IRGRP 0040u
#define AXIOM_S_IWGRP 0020u
#define AXIOM_S_IXGRP 0010u
#define AXIOM_S_IROTH 0004u
#define AXIOM_S_IWOTH 0002u
#define AXIOM_S_IXOTH 0001u
#define AXIOM_S_IRALL 0444u
#define AXIOM_S_IWALL 0222u
#define AXIOM_S_IXALL 0111u

#define AXIOM_DIRENT_NAME_MAX 64

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_stat {
    uint64_t size;
    uint32_t type;
    uint32_t mode;
};

struct axiom_dirent {
    char name[AXIOM_DIRENT_NAME_MAX];
    uint32_t type;
};
#endif

#endif
