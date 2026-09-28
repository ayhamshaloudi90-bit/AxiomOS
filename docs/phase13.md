# Phase 13 — Persistent block storage

Phase 13 connects the existing Phase-12 VFS to an actual emulated SATA disk.
The implementation deliberately keeps the layers separate:

```text
Ring-3 file syscalls
        |
        v
       VFS
        |
        v
     diskfs
        |
        v
 generic block-device API
        |
        v
 AHCI SATA driver
        |
        v
 q35 ICH9 AHCI controller
        |
        v
 raw QEMU disk image
```

## PCI + AHCI

`arch/x86_64/pci/pci.c` implements PCI configuration mechanism #1 through
I/O ports `0xCF8/0xCFC`. The storage driver scans for class 01/subclass 06/
programming-interface 01, enables PCI memory decoding and bus mastering, maps
BAR5 as uncached kernel MMIO, performs AHCI BIOS/OS ownership handoff when the
controller advertises it, and selects an attached ATA SATA port.

The AHCI driver uses one command slot and polled DMA. Its command list, received
FIS area, command table and 4 KiB bounce buffer are allocated from the PMM.
Current block transfers intentionally use one 512-byte sector per ATA DMA
command; this is simple and easy to validate before adding request merging or
interrupt-driven I/O later.

Supported ATA commands in Phase 13:

- IDENTIFY DEVICE (`0xEC`) to discover capacity;
- READ DMA EXT (`0x25`);
- WRITE DMA EXT (`0x35`);
- FLUSH CACHE EXT (`0xEA`).

## Block-device layer

`drivers/storage/block.c` exposes a generic `struct block_device` with read,
write and flush methods plus transfer counters. Filesystem code does not know
about AHCI registers.

## diskfs

Phase 13 introduces a deliberately small persistent filesystem backend mounted
at `/disk`. It is not intended to replace a mature filesystem such as ext2; its
purpose is to prove real sector persistence while exercising the Phase-12 VFS.

Layout:

```text
LBA 0        superblock
LBA 1..8     fixed 64-entry directory table
LBA 9..15    reserved
LBA 16..     append-only file data extents
```

`diskfs` currently supports a flat root directory, create/open/read/write/
truncate/readdir, and append-only extent allocation. Growing a file may move it
to a new extent and leave the old extent unused; deletion and free-space
reclamation are future work.

## Persistence acceptance test

`tests/phase13_disk.py` creates a blank 16 MiB raw disk, boots AxiomOS twice
against the same image, and requires:

1. first boot formats the blank disk;
2. a Ring-3 ELF creates `/disk/persist.txt` through ordinary file syscalls;
3. the file contents round-trip through the AHCI/block/diskfs/VFS stack;
4. the second QEMU boot does not reformat the disk;
5. the second boot finds the file before the userspace writer runs.

This is stronger than checking that a single write command completed because it
proves data survived destruction and recreation of the virtual machine.

## Current limits

- one AHCI controller and one ATA disk are used;
- one AHCI command slot, polling, no AHCI interrupts/NCQ;
- one-sector bounce-buffer transfers;
- diskfs has one flat directory and 64 file slots;
- diskfs does not reclaim deleted/old extents;
- no partitions/GPT parser yet;
- `/`, `/tmp`, and `/bin` still come from RAMFS/boot modules; `/disk` is the
  persistent mount;
- shell interaction remains Phase 14.

## Acceptance

```bash
make clean
make
make test-phase13
make test
```

For interactive persistence testing:

```bash
make run
# stop QEMU, then run `make run` again without `make clean`
```

The default `make run` target reuses `build/axiom-disk.img`, so `/disk` persists
between those runs.
