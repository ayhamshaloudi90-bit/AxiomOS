# AxiomOS Phase 18 — complete new/modified source

This appendix contains every file added or modified relative to the accepted Phase-17 source tree.

## File list

- `Makefile`
- `README.md`
- `arch/x86_64/pci/pci.c`
- `docs/architecture.md`
- `docs/networking.md`
- `docs/phase18.md`
- `docs/project-state.md`
- `docs/syscalls.md`
- `docs/validation.md`
- `drivers/net/e1000.c`
- `include/axiom/abi/errno.h`
- `include/axiom/abi/network.h`
- `include/axiom/abi/syscall.h`
- `include/axiom/arch/pci.h`
- `include/axiom/drivers/e1000.h`
- `include/axiom/kernel/syscall.h`
- `include/axiom/network/net.h`
- `kernel/core/kernel.c`
- `kernel/syscall/syscall.c`
- `network/arp.c`
- `network/dns.c`
- `network/ethernet.c`
- `network/http.c`
- `network/icmp.c`
- `network/internal.h`
- `network/ipv4.c`
- `network/net.c`
- `network/tcp.c`
- `network/udp.c`
- `tests/phase18_network.py`
- `userspace/phase14_shell.c`

## `Makefile`

```text
SHELL := /bin/bash


PROJECT := AxiomOS

MODE ?= normal
BUILD_DIR := build$(if $(filter-out normal,$(MODE)),-$(MODE))
VALID_MODES := normal divide invalid gp double_fault page_fault heap_double_free heap_guard
ifeq ($(filter $(MODE),$(VALID_MODES)),)
$(error Invalid MODE: $(MODE))
endif
OBJ_DIR := $(BUILD_DIR)/obj
ISO_ROOT := $(BUILD_DIR)/iso_root

KERNEL_ELF := $(BUILD_DIR)/$(PROJECT).elf
ISO_IMAGE := $(BUILD_DIR)/$(PROJECT).iso
USER_DIR := $(BUILD_DIR)/userspace
USER_PHASE11_OBJ := $(USER_DIR)/phase11_program.o
USER_PHASE11_ELF := $(USER_DIR)/phase11_demo.elf
USER_PHASE11_LAUNCHER_OBJ := $(USER_DIR)/phase11_launcher.o
USER_PHASE11_LAUNCHER_ELF := $(USER_DIR)/phase11_launcher.elf
USER_PHASE12_OBJ := $(USER_DIR)/phase12_program.o
USER_PHASE12_ELF := $(USER_DIR)/phase12_demo.elf
USER_PHASE13_OBJ := $(USER_DIR)/phase13_program.o
USER_PHASE13_ELF := $(USER_DIR)/phase13_demo.elf
USER_PHASE14_START_OBJ := $(USER_DIR)/phase14_shell_start.o
USER_PHASE14_C_OBJ := $(USER_DIR)/phase14_shell.o
USER_PHASE14_ELF := $(USER_DIR)/axiomsh.elf
USER_PHASE15_DEMO_START_OBJ := $(USER_DIR)/phase15_demo_start.o
USER_PHASE15_DEMO_C_OBJ := $(USER_DIR)/phase15_demo.o
USER_PHASE15_DEMO_ELF := $(USER_DIR)/phase15_demo.elf
USER_PHASE15_SLEEPER_START_OBJ := $(USER_DIR)/phase15_sleeper_start.o
USER_PHASE15_SLEEPER_C_OBJ := $(USER_DIR)/phase15_sleeper.o
USER_PHASE15_SLEEPER_ELF := $(USER_DIR)/phase15_sleeper.elf
DISK_IMAGE := $(BUILD_DIR)/axiom-disk.img


CC := clang
LD := ld.lld
ASM := nasm

QEMU := qemu-system-x86_64
GDB := gdb

READELF := llvm-readelf
OBJDUMP := llvm-objdump


LIMINE_VERSION := v12.9.0
LIMINE_DIR := third_party/limine
LIMINE_ARCHIVE := $(BUILD_DIR)/limine-binary.tar.gz

LIMINE_URL := \
	https://github.com/Limine-Bootloader/Limine/releases/download/$(LIMINE_VERSION)/limine-binary.tar.gz


CFLAGS := \
	--target=x86_64-unknown-none-elf \
	-std=gnu11 \
	-Wall \
	-Wextra \
	-Werror \
	-O2 \
	-g \
	-ffreestanding \
	-fno-stack-protector \
	-fno-stack-check \
	-fno-pic \
	-fno-pie \
	-fno-lto \
	-ffunction-sections \
	-fdata-sections \
	-m64 \
	-march=x86-64 \
	-mabi=sysv \
	-mno-red-zone \
	-mno-80387 \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-mcmodel=kernel


CPPFLAGS := \
	-Iinclude

ifneq ($(MODE),normal)
CPPFLAGS += -DAXIOM_TEST_$(shell echo $(MODE) | tr a-z A-Z)
endif


ASMFLAGS := \
	-f elf64 \
	-g \
	-F dwarf \
	-Wall

GASFLAGS := \
	--target=x86_64-unknown-none-elf \
	-m64 \
	-Iinclude

USER_CFLAGS := \
	--target=x86_64-unknown-none-elf \
	-std=gnu11 \
	-Wall -Wextra -Werror \
	-O2 -g \
	-ffreestanding -fno-builtin \
	-fno-stack-protector -fno-stack-check \
	-fno-pic -fno-pie -fno-lto \
	-fno-unwind-tables -fno-asynchronous-unwind-tables \
	-ffunction-sections -fdata-sections \
	-m64 -march=x86-64 -mabi=sysv -mno-red-zone \
	-mno-80387 -mno-mmx -mno-sse -mno-sse2 \
	-Iinclude


LDFLAGS := \
	-m elf_x86_64 \
	-nostdlib \
	-static \
	-z max-page-size=0x1000 \
	-z noexecstack \
	--gc-sections \
	-T linker/x86_64.ld


C_SOURCES := \
	arch/x86_64/boot/limine_requests.c \
	drivers/serial/serial.c \
	drivers/timer/apic_timer.c \
	drivers/keyboard/ps2_keyboard.c \
	drivers/net/e1000.c \
	arch/x86_64/pci/pci.c \
	drivers/storage/block.c \
	drivers/storage/ahci.c \
	process/task.c \
	process/scheduler.c \
	kernel/syscall/syscall.c \
	kernel/elf/elf64.c \
	kernel/lib/memory.c \
	kernel/sync/spinlock.c \
	kernel/sync/wait_queue.c \
	kernel/sync/mutex.c \
	kernel/sync/semaphore.c \
	kernel/sync/selftest.c \
	network/net.c \
	network/ethernet.c \
	network/arp.c \
	network/ipv4.c \
	network/icmp.c \
	network/udp.c \
	network/dns.c \
	network/tcp.c \
	network/http.c \
	filesystem/vfs.c \
	filesystem/ramfs.c \
	filesystem/bootstrap.c \
	filesystem/diskfs.c \
	kernel/terminal/terminal.c \
	kernel/terminal/kprintf.c \
	kernel/core/kernel.c \
	kernel/core/panic.c \
	memory/pmm.c \
	memory/pmm_selftest.c \
	memory/vmm.c \
	memory/vmm_selftest.c \
	memory/heap.c \
	memory/heap_selftest.c \
	arch/x86_64/cpu/gdt.c \
	arch/x86_64/cpu/smp.c \
	arch/x86_64/cpu/selftest.c \
	arch/x86_64/interrupts/idt.c \
	arch/x86_64/interrupts/pic.c \
	arch/x86_64/interrupts/apic.c


ASM_SOURCES := \
	arch/x86_64/boot/entry.asm \
	arch/x86_64/cpu/gdt_load.asm \
	arch/x86_64/cpu/register_probe.asm \
	arch/x86_64/interrupts/isr_stubs.asm

GAS_SOURCES := \
	arch/x86_64/syscall/syscall_entry.S \
	arch/x86_64/cpu/smp_entry.S \
	userspace/phase9_program.S \
	userspace/phase10_program.S

C_OBJECTS := \
	$(patsubst %.c,$(OBJ_DIR)/%.o,$(C_SOURCES))


ASM_OBJECTS := \
	$(patsubst %.asm,$(OBJ_DIR)/%.o,$(ASM_SOURCES))

GAS_OBJECTS := \
	$(patsubst %.S,$(OBJ_DIR)/%.o,$(GAS_SOURCES))

OBJECTS := \
	$(ASM_OBJECTS) \
	$(GAS_OBJECTS) \
	$(C_OBJECTS)


.DEFAULT_GOAL := all


.PHONY: \
	all \
	deps \
	run \
	debug \
	test \
	inspect \
	clean \
	distclean \
	help


all: $(ISO_IMAGE)


deps: $(LIMINE_DIR)/limine


$(LIMINE_DIR)/limine:
	@echo "Fetching Limine $(LIMINE_VERSION)..."

	mkdir -p $(BUILD_DIR)

	curl \
		-fL \
		$(LIMINE_URL) \
		-o $(LIMINE_ARCHIVE)

	rm -rf $(LIMINE_DIR)

	mkdir -p $(LIMINE_DIR)

	tar \
		-xzf $(LIMINE_ARCHIVE) \
		-C $(LIMINE_DIR) \
		--strip-components=1

	$(MAKE) -C $(LIMINE_DIR)


$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)

	$(CC) \
		$(CFLAGS) \
		$(CPPFLAGS) \
		-MMD -MP \
		-c $< \
		-o $@


$(OBJ_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)

	$(ASM) \
		$(ASMFLAGS) \
		$< \
		-o $@

$(OBJ_DIR)/%.o: %.S
	@mkdir -p $(dir $@)

	$(CC) \
		$(GASFLAGS) \
		-c $< \
		-o $@


$(USER_PHASE11_OBJ): userspace/phase11_program.S include/axiom/abi/syscall.h include/axiom/abi/user_layout.h
	@mkdir -p $(USER_DIR)

	$(CC) \
		$(GASFLAGS) \
		-c $< \
		-o $@


$(USER_PHASE11_LAUNCHER_OBJ): userspace/phase11_launcher.S include/axiom/abi/syscall.h
	@mkdir -p $(USER_DIR)

	$(CC) \
		$(GASFLAGS) \
		-c $< \
		-o $@


$(USER_PHASE12_OBJ): userspace/phase12_program.S include/axiom/abi/syscall.h include/axiom/abi/fs.h
	@mkdir -p $(USER_DIR)

	$(CC) \
		$(GASFLAGS) \
		-c $< \
		-o $@


$(USER_PHASE11_LAUNCHER_ELF): $(USER_PHASE11_LAUNCHER_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)

	$(LD) \
		-m elf_x86_64 \
		-nostdlib \
		-static \
		-z max-page-size=0x1000 \
		-z noexecstack \
		-T userspace/x86_64_user.ld \
		$(USER_PHASE11_LAUNCHER_OBJ) \
		-o $@

	@echo
	@echo "Built Phase 11 exec launcher ELF:"
	@echo "  $@"


$(USER_PHASE11_ELF): $(USER_PHASE11_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)

	$(LD) \
		-m elf_x86_64 \
		-nostdlib \
		-static \
		-z max-page-size=0x1000 \
		-z noexecstack \
		-T userspace/x86_64_user.ld \
		$(USER_PHASE11_OBJ) \
		-o $@

	@echo
	@echo "Built Phase 11 userspace ELF:"
	@echo "  $@"


$(USER_PHASE12_ELF): $(USER_PHASE12_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)

	$(LD) \
		-m elf_x86_64 \
		-nostdlib \
		-static \
		-z max-page-size=0x1000 \
		-z noexecstack \
		-T userspace/x86_64_user.ld \
		$(USER_PHASE12_OBJ) \
		-o $@

	@echo
	@echo "Built Phase 12 VFS demo ELF:"
	@echo "  $@"



$(USER_PHASE13_OBJ): userspace/phase13_program.S include/axiom/abi/syscall.h include/axiom/abi/fs.h
	@mkdir -p $(USER_DIR)

	$(CC) \
		$(GASFLAGS) \
		-c $< \
		-o $@


$(USER_PHASE13_ELF): $(USER_PHASE13_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)

	$(LD) \
		-m elf_x86_64 \
		-nostdlib \
		-static \
		-z max-page-size=0x1000 \
		-z noexecstack \
		-T userspace/x86_64_user.ld \
		$(USER_PHASE13_OBJ) \
		-o $@

	@echo
	@echo "Built Phase 13 disk demo ELF:"
	@echo "  $@"

$(USER_PHASE14_START_OBJ): userspace/phase14_shell_start.S include/axiom/abi/syscall.h
	@mkdir -p $(USER_DIR)
	$(CC) $(GASFLAGS) -c $< -o $@

$(USER_PHASE14_C_OBJ): userspace/phase14_shell.c include/axiom/abi/syscall.h include/axiom/abi/fs.h include/axiom/abi/input.h include/axiom/abi/network.h
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE14_ELF): $(USER_PHASE14_START_OBJ) $(USER_PHASE14_C_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) \
		-m elf_x86_64 \
		-nostdlib \
		-static \
		-z max-page-size=0x1000 \
		-z noexecstack \
		--gc-sections \
		-T userspace/x86_64_user.ld \
		$(USER_PHASE14_START_OBJ) $(USER_PHASE14_C_OBJ) \
		-o $@

	@echo
	@echo "Built Phase 14 AxiomOS shell ELF:"
	@echo "  $@"

$(USER_PHASE15_DEMO_START_OBJ): userspace/phase15_demo_start.S include/axiom/abi/syscall.h
	@mkdir -p $(USER_DIR)
	$(CC) $(GASFLAGS) -c $< -o $@

$(USER_PHASE15_DEMO_C_OBJ): userspace/phase15_demo.c include/axiom/abi/syscall.h include/axiom/abi/process.h
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE15_DEMO_ELF): $(USER_PHASE15_DEMO_START_OBJ) $(USER_PHASE15_DEMO_C_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(USER_PHASE15_DEMO_START_OBJ) $(USER_PHASE15_DEMO_C_OBJ) -o $@
	@echo
	@echo "Built Phase 15 process demo ELF:"
	@echo "  $@"

$(USER_PHASE15_SLEEPER_START_OBJ): userspace/phase15_sleeper_start.S include/axiom/abi/syscall.h
	@mkdir -p $(USER_DIR)
	$(CC) $(GASFLAGS) -c $< -o $@

$(USER_PHASE15_SLEEPER_C_OBJ): userspace/phase15_sleeper.c include/axiom/abi/syscall.h include/axiom/abi/process.h
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE15_SLEEPER_ELF): $(USER_PHASE15_SLEEPER_START_OBJ) $(USER_PHASE15_SLEEPER_C_OBJ) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(USER_PHASE15_SLEEPER_START_OBJ) $(USER_PHASE15_SLEEPER_C_OBJ) -o $@
	@echo
	@echo "Built Phase 15 sleeper ELF:"
	@echo "  $@"


$(KERNEL_ELF): $(OBJECTS) linker/x86_64.ld
	@mkdir -p $(BUILD_DIR)

	$(LD) \
		$(LDFLAGS) \
		$(OBJECTS) \
		-o $(KERNEL_ELF)

	@echo
	@echo "Built kernel:"
	@echo "  $(KERNEL_ELF)"


$(ISO_IMAGE): \
	$(KERNEL_ELF) \
	$(USER_PHASE11_ELF) \
	$(USER_PHASE11_LAUNCHER_ELF) \
	$(USER_PHASE12_ELF) \
	$(USER_PHASE13_ELF) \
	$(USER_PHASE14_ELF) \
	$(USER_PHASE15_DEMO_ELF) \
	$(USER_PHASE15_SLEEPER_ELF) \
	boot/limine/limine.conf \
	$(LIMINE_DIR)/limine

	rm -rf $(ISO_ROOT)

	mkdir -p $(ISO_ROOT)/boot/limine
	mkdir -p $(ISO_ROOT)/EFI/BOOT


	cp \
		$(KERNEL_ELF) \
		$(ISO_ROOT)/boot/$(PROJECT).elf


	cp \
		$(USER_PHASE11_ELF) \
		$(ISO_ROOT)/boot/phase11_demo.elf


	cp \
		$(USER_PHASE11_LAUNCHER_ELF) \
		$(ISO_ROOT)/boot/phase11_launcher.elf


	cp \
		$(USER_PHASE12_ELF) \
		$(ISO_ROOT)/boot/phase12_demo.elf


	cp \
		$(USER_PHASE13_ELF) \
		$(ISO_ROOT)/boot/phase13_demo.elf

	cp \
		$(USER_PHASE14_ELF) \
		$(ISO_ROOT)/boot/axiomsh.elf

	cp \
		$(USER_PHASE15_DEMO_ELF) \
		$(ISO_ROOT)/boot/phase15_demo.elf

	cp \
		$(USER_PHASE15_SLEEPER_ELF) \
		$(ISO_ROOT)/boot/phase15_sleeper.elf


	cp \
		boot/limine/limine.conf \
		$(ISO_ROOT)/boot/limine/


	cp \
		$(LIMINE_DIR)/limine-bios.sys \
		$(LIMINE_DIR)/limine-bios-cd.bin \
		$(LIMINE_DIR)/limine-uefi-cd.bin \
		$(ISO_ROOT)/boot/limine/


	cp \
		$(LIMINE_DIR)/BOOTX64.EFI \
		$(ISO_ROOT)/EFI/BOOT/


	xorriso \
		-as mkisofs \
		-R \
		-r \
		-J \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot \
		-boot-load-size 4 \
		-boot-info-table \
		-hfsplus \
		-apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part \
		--efi-boot-image \
		--protective-msdos-label \
		$(ISO_ROOT) \
		-o $(ISO_IMAGE)


	$(LIMINE_DIR)/limine \
		bios-install \
		$(ISO_IMAGE)


	rm -rf $(ISO_ROOT)


	@echo
	@echo "Built bootable image:"
	@echo "  $(ISO_IMAGE)"


$(DISK_IMAGE):
	@mkdir -p $(BUILD_DIR)
	truncate -s 16M $(DISK_IMAGE)


disk-image: $(DISK_IMAGE)


run: $(ISO_IMAGE) $(DISK_IMAGE)
	$(QEMU) \
		-machine q35 \
		-smp 4 \
		-m 256M \
		-netdev user,id=net0,ipv6=off \
		-device e1000,netdev=net0,mac=52:54:00:12:34:56 \
		-cdrom $(ISO_IMAGE) \
		-boot d \
		-drive file=$(DISK_IMAGE),format=raw,if=none,id=axiomdisk \
		-device ide-hd,drive=axiomdisk,bus=ide.0 \
		-serial stdio \
		-monitor none \
		-no-reboot \
		-no-shutdown


debug: $(ISO_IMAGE) $(DISK_IMAGE)
	$(QEMU) \
		-machine q35 \
		-smp 4 \
		-m 256M \
		-netdev user,id=net0,ipv6=off \
		-device e1000,netdev=net0,mac=52:54:00:12:34:56 \
		-cdrom $(ISO_IMAGE) \
		-boot d \
		-drive file=$(DISK_IMAGE),format=raw,if=none,id=axiomdisk \
		-device ide-hd,drive=axiomdisk,bus=ide.0 \
		-serial stdio \
		-monitor none \
		-no-reboot \
		-no-shutdown \
		-S \
		-gdb tcp::1234


test: $(ISO_IMAGE)
	bash tests/phase1_boot.sh $(ISO_IMAGE)
	bash tests/phase2_terminal.sh $(ISO_IMAGE)
	$(MAKE) test-phase3
	$(MAKE) test-phase4
	$(MAKE) test-phase5
	$(MAKE) test-phase6
	$(MAKE) test-phase7
	$(MAKE) test-phase8
	$(MAKE) test-phase9
	$(MAKE) test-phase10
	$(MAKE) test-phase11
	$(MAKE) test-phase12
	$(MAKE) test-phase13
	$(MAKE) test-phase14
	$(MAKE) test-phase15
	$(MAKE) test-phase16
	$(MAKE) test-phase17
	$(MAKE) test-phase18


inspect: $(KERNEL_ELF)
	@echo "========== ELF HEADER =========="
	$(READELF) -h $(KERNEL_ELF)

	@echo
	@echo "========== PROGRAM HEADERS =========="
	$(READELF) -l $(KERNEL_ELF)

	@echo
	@echo "========== DISASSEMBLY =========="
	$(OBJDUMP) -d $(KERNEL_ELF)


clean:
	rm -rf build build-*


distclean: clean
	rm -rf $(LIMINE_DIR)


help:
	@echo "AxiomOS Phase 18 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 18 tests"
	@echo "  make test-phase6 Run heap + corruption tests"
	@echo "  make test-phase7 Run timer + keyboard IRQ/input tests"
	@echo "  make test-phase8 Run preemptive scheduler/context-switch tests"
	@echo "  make test-phase9 Run Ring 3 userspace/isolation tests"
	@echo "  make test-phase10 Run SYSCALL/SYSRET ABI and user-copy tests"
	@echo "  make test-phase11 Run ELF64 loader/executable tests"
	@echo "  make test-phase12 Run VFS/RAM filesystem/file-descriptor tests"
	@echo "  make test-phase13 Run AHCI/block-device/persistent disk tests"
	@echo "  make test-phase14 Run interactive Ring-3 shell tests"
	@echo "  make test-phase15 Run fork/wait/ps/kill process-management tests"
	@echo "  make test-phase16 Run synchronization/race/wait-queue tests"
	@echo "  make test-phase17 Run SMP/AP bring-up and parallel-core tests"
	@echo "  make test-phase18 Run E1000/Ethernet/ARP/IP/ICMP/DNS/TCP/HTTP tests"
	@echo "  make disk-image Create the 16 MiB persistent QEMU disk image"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9 test-phase10 test-phase11 test-phase12 test-phase13 test-phase14 test-phase15 test-phase16 test-phase17 test-phase18 disk-image

test-phase3:
	python3 tests/phase3_cpu.py

test-phase4:
	python3 tests/phase4_pmm.py

test-phase5:
	python3 tests/phase5_vmm.py

test-phase6:
	python3 tests/phase6_heap.py

test-phase7:
	python3 tests/phase7_devices.py

test-phase8:
	python3 tests/phase8_scheduler.py

test-phase9:
	python3 tests/phase9_userspace.py

test-phase10:
	python3 tests/phase10_syscalls.py

test-phase11:
	python3 tests/phase11_elf.py

test-phase12:
	python3 tests/phase12_vfs.py

test-phase13:
	python3 tests/phase13_disk.py

test-phase14:
	python3 tests/phase14_shell.py

test-phase15:
	python3 tests/phase15_processes.py

test-phase16:
	python3 tests/phase16_sync.py

test-phase17:
	python3 tests/phase17_smp.py

test-phase18:
	python3 tests/phase18_network.py

# Track C header changes too; each fault mode has its own object directory.
-include $(C_OBJECTS:.o=.d)
```

## `README.md`

```markdown
# AxiomOS

AxiomOS is a freestanding x86-64 hobby operating system written in C and
assembly, built with Clang/LLD/NASM and booted by Limine v12.9.0 under QEMU. It
is a higher-half ELF64 kernel and does not use the host libc.

## Current milestone

Phases 0–17 provide boot, memory management, interrupts, preemptive scheduling,
Ring-3 isolation, syscalls, ELF64 loading, VFS + persistent AHCI storage, a real
interactive shell, process management, synchronization, and SMP/AP bring-up.

Phase 18 adds the first complete networking path:

- Intel E1000 PCI/MMIO/DMA driver;
- Ethernet and ARP;
- IPv4 and ICMP echo;
- UDP and DNS A-record resolution;
- a minimal TCP active connection;
- HTTP/1.0 GET;
- `netinfo`, `ping`, `dns`, and `httpget` commands in `/bin/axiomsh`.

The network stack is intentionally small and honest: IPv4 only, static QEMU
user-network configuration, a polled NIC, one synchronous TCP connection, and
plain HTTP only. TLS/HTTPS and a general sockets API are not claimed yet.

## Common commands

```bash
make
make run
make test
make test-phase13
make test-phase14
make test-phase15
make test-phase16
make test-phase17
make test-phase18
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 18 regression suite.

When `make run` reaches the shell:

```text
AxiomOS shell ready. Type 'help' for commands.
axiom> help
```

The next milestone is Phase 19: the AxiomOS userspace standard library.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Syscalls](docs/syscalls.md) |
[Filesystem](docs/filesystem.md) | [Phase 13](docs/phase13.md) |
[Phase 14](docs/phase14.md) | [Phase 15](docs/phase15.md) | [Phase 16](docs/phase16.md) | [Phase 17](docs/phase17.md) | [Phase 18](docs/phase18.md) | [Project state](docs/project-state.md) |
[Original roadmap](docs/roadmap.md)
```

## `arch/x86_64/pci/pci.c`

```c
#include <stdint.h>

#include <axiom/arch/io.h>
#include <axiom/arch/pci.h>

#define PCI_CONFIG_ADDRESS 0xCF8u
#define PCI_CONFIG_DATA    0xCFCu
#define PCI_ENABLE         0x80000000u

static uint32_t pci_address(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
)
{
    return PCI_ENABLE |
        ((uint32_t)bus << 16) |
        ((uint32_t)(device & 0x1Fu) << 11) |
        ((uint32_t)(function & 0x07u) << 8) |
        ((uint32_t)offset & 0xFCu);
}

uint32_t pci_config_read32(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
)
{
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, function, offset));
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write32(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint32_t value
)
{
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, function, offset));
    outl(PCI_CONFIG_DATA, value);
}

uint16_t pci_config_read16(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset
)
{
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, function, offset));
    return inw((uint16_t)(PCI_CONFIG_DATA + (offset & 2u)));
}

void pci_config_write16(
    uint8_t bus,
    uint8_t device,
    uint8_t function,
    uint8_t offset,
    uint16_t value
)
{
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, device, function, offset));
    outw((uint16_t)(PCI_CONFIG_DATA + (offset & 2u)), value);
}

int pci_find_device(
    uint16_t vendor_id,
    uint16_t device_id,
    struct pci_device *device_out
)
{
    uint16_t bus;

    if (device_out == 0) return 0;

    for (bus = 0u; bus < 256u; ++bus) {
        uint8_t device;
        for (device = 0u; device < 32u; ++device) {
            uint8_t function;
            uint8_t max_functions = 1u;

            for (function = 0u; function < max_functions; ++function) {
                const uint32_t id = pci_config_read32(
                    (uint8_t)bus, device, function, 0x00u
                );
                uint32_t class_reg;
                uint8_t header_type;

                if ((id & 0xFFFFu) == 0xFFFFu) continue;

                if (function == 0u) {
                    header_type = (uint8_t)(
                        pci_config_read32((uint8_t)bus, device, 0u, 0x0Cu) >> 16
                    );
                    if ((header_type & 0x80u) != 0u) max_functions = 8u;
                }

                if ((uint16_t)(id & 0xFFFFu) != vendor_id ||
                    (uint16_t)(id >> 16) != device_id) {
                    continue;
                }

                class_reg = pci_config_read32(
                    (uint8_t)bus, device, function, 0x08u
                );
                device_out->bus = (uint8_t)bus;
                device_out->device = device;
                device_out->function = function;
                device_out->vendor_id = vendor_id;
                device_out->device_id = device_id;
                device_out->class_code = (uint8_t)(class_reg >> 24);
                device_out->subclass = (uint8_t)(class_reg >> 16);
                device_out->prog_if = (uint8_t)(class_reg >> 8);
                return 1;
            }
        }
    }
    return 0;
}

int pci_find_class(
    uint8_t class_code,
    uint8_t subclass,
    uint8_t prog_if,
    struct pci_device *device_out
)
{
    uint16_t bus;

    if (device_out == 0) {
        return 0;
    }

    for (bus = 0u; bus < 256u; ++bus) {
        uint8_t device;

        for (device = 0u; device < 32u; ++device) {
            uint8_t function;
            uint8_t max_functions = 1u;

            for (function = 0u; function < max_functions; ++function) {
                const uint32_t id = pci_config_read32(
                    (uint8_t)bus, device, function, 0x00u
                );
                uint32_t class_reg;
                uint8_t header_type;

                if ((id & 0xFFFFu) == 0xFFFFu) {
                    continue;
                }

                if (function == 0u) {
                    header_type = (uint8_t)(
                        pci_config_read32((uint8_t)bus, device, 0u, 0x0Cu) >> 16
                    );
                    if ((header_type & 0x80u) != 0u) {
                        max_functions = 8u;
                    }
                }

                class_reg = pci_config_read32(
                    (uint8_t)bus, device, function, 0x08u
                );

                if ((uint8_t)(class_reg >> 24) == class_code &&
                    (uint8_t)(class_reg >> 16) == subclass &&
                    (uint8_t)(class_reg >> 8) == prog_if) {
                    device_out->bus = (uint8_t)bus;
                    device_out->device = device;
                    device_out->function = function;
                    device_out->vendor_id = (uint16_t)(id & 0xFFFFu);
                    device_out->device_id = (uint16_t)(id >> 16);
                    device_out->class_code = class_code;
                    device_out->subclass = subclass;
                    device_out->prog_if = prog_if;
                    return 1;
                }
            }
        }
    }

    return 0;
}
```

## `docs/architecture.md`

```markdown
# Architecture at Phase 18

- Target: x86-64 SMP-capable machine, freestanding C plus x86-64 assembly.
- Limine v12.9.0 boots a higher-half ELF64 kernel and provides MP CPU discovery.
- Normal QEMU boots expose four virtual CPUs (`-smp 4`).
- CPU 0 is the bootstrap processor (BSP); secondary CPUs are APs.
- Each managed AP switches to the AxiomOS kernel CR3, a private 16 KiB kernel
  stack, and a private GDT/TSS/IST set before entering Phase-17 C code.
- The IDT is shared after BSP initialization; AP interrupts remain disabled in
  this foundational SMP phase.
- Serial output mirrors the framebuffer terminal/custom `kprintf()`.
- The BSP local APIC supplies the 100 Hz timer; the I/O APIC routes PS/2 IRQ1.
- PMM manages 4 KiB frames; VMM owns the kernel PML4 and isolated user roots.
- Kernel heap provides dynamic allocation.
- Scheduler is five-tick preemptive round robin with reusable BLOCKED/READY
  wait-queue hooks, still deliberately BSP-only in Phase 17.
- Ring-3 tasks use private user memory/stacks and trusted kernel stacks.
- Phase 15 provides fork/exec/wait parent-child process management.
- Phase 16 provides spinlocks, FIFO wait queues, sleeping mutexes and counting
  semaphores.
- Phase 17 proves the spinlock across truly parallel CPUs using an exact shared
  counter test; APs park after that proof rather than running general tasks.
- Phase 18 adds an Intel E1000 PCI/MMIO/DMA NIC and an in-kernel
  Ethernet -> ARP -> IPv4 stack with ICMP, UDP/DNS, and a minimal TCP/HTTP
  client. Networking is polled and synchronous in this phase.
- `SYSCALL/SYSRETQ` validates user pointers at the kernel boundary.
- ELF64 loader maps ET_EXEC PT_LOAD segments with R/W/X permissions.
- VFS provides paths, mounts, vnodes and per-task file descriptors.
- RAMFS backs `/` and `/tmp`.
- Phase 13 discovers q35 AHCI through PCI, exposes a SATA block device, and
  mounts persistent `diskfs` at `/disk`.

## CPU/SMP topology

```text
                     +-----------------------------+
                     |        Shared kernel        |
                     | CR3 / kernel image / IDT    |
                     | spinlocks / shared memory   |
                     +---------------+-------------+
                                     |
          +--------------------------+--------------------------+
          |                          |                          |
          v                          v                          v
+-------------------+      +-------------------+      +-------------------+
| CPU 0 / BSP       |      | CPU 1 / AP        |      | CPU 2+ / AP       |
| private TSS/stack |      | private TSS/stack |      | private TSS/stack |
| APIC timer        |      | IF=0              |      | IF=0              |
| scheduler         |      | SMP proof -> HLT  |      | SMP proof -> HLT  |
| Ring-3 tasks      |      |                   |      |                   |
+-------------------+      +-------------------+      +-------------------+
```

This is intentionally **not** a fully SMP scheduler yet. Moving arbitrary tasks
between CPUs requires per-CPU current-task state, run-queue locking or per-CPU
run queues, AP timers, reschedule IPIs, and TLB shootdowns.

## Storage stack

```text
Ring-3 ELF process
        |
        v
file syscalls
        |
        v
+-------------------------------+
|              VFS              |
+-------------------------------+
   |           |            |
   v           v            v
 RAMFS /    RAMFS /tmp   diskfs /disk
                              |
                              v
                       block_device API
                              |
                              v
                         AHCI driver
                              |
                              v
                     PCI / q35 ICH9 SATA
                              |
                              v
                    raw persistent image
```

AHCI currently uses a single command slot, polling and a one-sector DMA bounce
buffer. That intentionally favors correctness/visibility over throughput.


## Network stack

```text
Ring-3 axiomsh
      |
      v
netinfo / ping / dns / httpget
      |
      v
HTTP/1.0 -> TCP
DNS      -> UDP
ICMP echo
      \    |    /
          IPv4
            |
           ARP
            |
        Ethernet II
            |
   Intel E1000 82540EM
            |
   QEMU user networking
```

Phase 18 uses IPv4 only, a static QEMU guest topology, one synchronous TCP
connection, and plain HTTP. It deliberately does not claim DHCP, sockets, TCP
retransmission/congestion control, TLS/HTTPS, or IPv6 yet.
```

## `docs/networking.md`

```markdown
# AxiomOS networking

Networking is implemented as of Phase 18.

## Current stack

```text
E1000 -> Ethernet -> ARP -> IPv4
                         |-> ICMP
                         |-> UDP -> DNS
                         `-> TCP -> HTTP/1.0
```

The implementation is under `drivers/net/` and `network/`. Public kernel-facing
interfaces are in `include/axiom/network/`, and the userspace ABI structures
are in `include/axiom/abi/network.h`.

Normal QEMU boots explicitly attach an Intel E1000 to QEMU user-mode networking.
AxiomOS currently uses the Phase-18 static QEMU topology `10.0.2.15/24`, gateway
`10.0.2.2`, and DNS `10.0.2.3`.

See [Phase 18](phase18.md) for protocol behavior, tests, and limitations.
```

## `docs/phase18.md`

```markdown
# Phase 18 — Networking

Phase 18 gives AxiomOS its first real network stack and its first ability to
exchange packets with systems outside the guest.

## Architecture

```text
Ring-3 axiomsh
   |
   | netinfo / ping / dns / httpget syscalls
   v
network services
   |
   +-- HTTP/1.0
   |     `-- TCP
   +-- DNS
   |     `-- UDP
   +-- ICMP echo
   `-- IPv4
          `-- ARP routing/neighbor resolution
                 `-- Ethernet
                        `-- Intel E1000 82540EM PCI NIC
                               `-- QEMU user-mode networking
```

AxiomOS implements these protocol components itself. It does not import a host
TCP/IP stack or a userspace networking library.

## NIC choice

Phase 18 uses QEMU's emulated Intel 82540EM E1000. Compared with starting with
VirtIO-net, E1000 exposes the classic pieces we want to learn directly:

- PCI discovery;
- MMIO registers;
- DMA receive/transmit descriptor rings;
- physical DMA buffer addresses;
- Ethernet frames.

The driver is intentionally polled. NIC interrupts and asynchronous socket I/O
are future improvements.

## QEMU network configuration

Normal `make run` attaches:

```text
-netdev user,id=net0,ipv6=off
-device e1000,netdev=net0,mac=52:54:00:12:34:56
```

Phase 18 uses QEMU user-networking's stable IPv4 topology:

```text
AxiomOS     10.0.2.15/24
router      10.0.2.2
dns         10.0.2.3
```

The address is static in this phase. DHCP is not claimed yet.

## Implemented protocols

### Ethernet

Creates and parses Ethernet II frames, supports unicast to the local MAC and
broadcast reception, and dispatches EtherTypes 0x0806 (ARP) and 0x0800 (IPv4).

### ARP

Maintains a small neighbor cache, sends ARP requests, learns replies, responds
to requests for AxiomOS's own address, and resolves either the destination host
or the default gateway depending on the IPv4 subnet.

### IPv4

Creates/validates IPv4 headers and Internet checksums and dispatches ICMP, UDP,
and TCP. Phase 18 deliberately rejects fragmented packets instead of pretending
fragment reassembly exists.

### ICMP

Implements Echo Request/Echo Reply. The shell's `ping` sends one request. The
QEMU router at `10.0.2.2` is the deterministic test peer.

### UDP + DNS

UDP includes pseudo-header checksums. DNS sends recursive A-record questions to
`10.0.2.3` and understands compressed DNS names well enough to walk questions
and answers and return an IPv4 A record.

### TCP

Implements one synchronous active connection at a time:

- SYN;
- SYN/ACK;
- ACK;
- ordered data send/receive;
- checksum verification;
- ACK generation;
- RST handling;
- FIN reception/acknowledgment.

There is no retransmission engine, congestion control, out-of-order reassembly,
select/poll API, or general socket API yet.

### HTTP

`httpget` performs an HTTP/1.0 GET over TCP. It parses the response status line
and header/body boundary and returns up to 4096 body bytes to userspace.

HTTPS/TLS is **not** implemented. `https://` sites are not claimed to work.

## Shell commands

```text
axiom> netinfo
axiom> ping 10.0.2.2
axiom> dns example.com
axiom> httpget example.com /
```

`httpget` also supports a non-default port for development/testing:

```text
axiom> httpget 10.0.2.2:8080 /
```

## New syscalls

```text
23  netinfo
24  ping
25  dns
26  httpget
```

They deliberately expose useful Phase-18 operations rather than pretending a
full POSIX sockets API exists. A socket-style userspace API can be added after
the userspace libc and asynchronous I/O foundations mature.

## Acceptance test

```bash
make test-phase18
```

The test boots QEMU with four CPUs and an explicit E1000. It verifies:

1. E1000 PCI/MMIO/DMA initialization;
2. `netinfo` reports `10.0.2.15`;
3. ARP + IPv4 + ICMP by pinging `10.0.2.2`;
4. UDP + DNS by resolving `example.com` through QEMU's DNS proxy;
5. TCP + HTTP against a deterministic local HTTP server reached through
   QEMU `guestfwd` at `10.0.2.100:80`;
6. the Ring-3 shell remains usable afterward.

The DNS portion requires ordinary host Internet/DNS connectivity.

Then run the complete regression suite:

```bash
make test
```

## Important limitations

- IPv4 only;
- static QEMU user-network configuration, no DHCP yet;
- E1000 only;
- polling NIC, no NIC interrupt handler;
- synchronous network syscalls temporarily monopolize the BSP while waiting;
- ARP cache is small and has no wall-clock expiration;
- no IP fragmentation/reassembly;
- one TCP connection at a time;
- no TCP retransmission, congestion control, receive reordering or sockets;
- DNS A records only;
- HTTP/1.0 GET only;
- no TLS/HTTPS;
- no IPv6.

Those boundaries are intentional. Phase 18 proves the complete vertical path
from a real NIC DMA ring through TCP/HTTP without claiming production-network
semantics we have not built.
```

## `docs/project-state.md`

```markdown
# AxiomOS handoff — Phase 18 implementation

Development environment: Linux/Distrobox on Steam Deck. Run commands from the
directory containing `Makefile`. Roadmap: Phase 0 through Phase 25.

Accepted foundation before Phase 18:

- Phases 0–7: boot, terminal, CPU architecture, memory, heap, timer/keyboard.
- Phase 8: preemptive round-robin scheduler.
- Phase 9: isolated Ring-3 userspace.
- Phase 10: x86-64 SYSCALL/SYSRET boundary.
- Phase 11: standalone ELF64 loading and exec.
- Phase 12: VFS/RAMFS/file descriptors.
- Phase 13: PCI/AHCI persistent block storage and diskfs.
- Phase 14: `/bin/axiomsh` Ring-3 interactive shell.
- Phase 15: fork/exec/wait/process lifecycle/ps/SIGTERM.
- Phase 16: spinlocks, wait queues, mutexes and semaphores.
- Phase 17: four-CPU SMP discovery/AP bring-up/cross-core locking; normal
  process scheduling remains BSP-only.

Phase 18 adds:

- Intel E1000 82540EM PCI/MMIO driver;
- physical DMA RX/TX descriptor rings and frame buffers;
- Ethernet II framing;
- ARP requests/replies and neighbor cache;
- IPv4 send/receive/checksum/routing;
- ICMP echo request/reply;
- UDP and transport checksums;
- DNS A-record resolution;
- minimal active-open TCP state machine;
- HTTP/1.0 GET;
- Ring-3 shell commands `netinfo`, `ping`, `dns`, `httpget`;
- dedicated `make test-phase18` with ICMP, DNS and deterministic TCP/HTTP.

Important limits:

- IPv4 only and static QEMU user-network address configuration;
- E1000 is polled rather than interrupt-driven;
- no DHCP, IPv6 or IP fragmentation;
- TCP is a deliberately small synchronous single-connection implementation;
- no sockets API, retransmission/congestion control, TLS or HTTPS;
- general process scheduling remains BSP-only as established in Phase 17.

Acceptance commands:

```bash
make clean
make
make test-phase18
make test
```

Next milestone: Phase 19 — an AxiomOS userspace standard library (`stdio`,
`stdlib`, `string`, `ctype`, and unistd-like wrappers) so userspace programs no
longer hand-roll syscall and string helpers.
```

## `docs/syscalls.md`

```markdown
# AxiomOS syscall ABI

Phase 10 introduced the x86-64 `SYSCALL` / `SYSRETQ` boundary. Arguments use
`RDI, RSI, RDX, R10, R8, R9`; `RAX` contains the syscall number on entry and the
signed result on return.

Implemented calls now include:

```text
write
read
_exit
sleep
getpid
yield
open
close
exec
lseek
stat
```

``mmap` retains a stable ABI number but still returns `-ENOSYS` until its
owning virtual-memory/userspace-library work is implemented. `fork` is now
implemented by Phase 15.

All pointer-bearing calls validate Ring-3 ranges through the VMM before copying
bytes. Bad user pointers return `-EFAULT`; the kernel never trusts a raw user
pointer.

## File syscalls in Phase 12

`open()` resolves an absolute path through the VFS and installs a
`struct vfs_file` into the current task's descriptor table. Regular descriptors
start at 3 because 0/1/2 remain stdin/stdout/stderr.

`read()` and `write()` dispatch either to keyboard/terminal semantics for the
standard descriptors or to VFS file operations for regular descriptors.

`lseek()` changes a descriptor's independent file offset. `stat()` copies a
small `struct axiom_stat` containing size/type/mode back into validated user
memory. `close()` releases the open-file object and descriptor slot.

## `exec`

`SYS_exec` now copies the path from userspace, reads the target ELF bytes using
`vfs_read_all()`, and passes that kernel buffer into the existing Phase-11 ELF
image-replacement path. The temporary hardcoded boot-module executable registry
is gone.

## Phase 14 additions

The interactive Ring-3 shell extends the ABI without changing the x86-64
`SYSCALL/SYSRET` entry mechanism:

| Number | Call | Purpose |
|---:|---|---|
| 14 | `readdir` | Enumerate a VFS directory by index |
| 15 | `mkdir` | Create a directory |
| 16 | `spawn` | Load a VFS ELF into a new Ring-3 task |
| 17 | `waitpid` | Wait for a spawned child and collect exit status |
| 18 | `clear` | Clear the framebuffer terminal |
| 19 | `kbdstats` | Read PS/2 keyboard diagnostic counters |

`spawn()` is deliberately smaller than POSIX `fork()+exec()`: Phase 14 needs a
safe foreground command-launch mechanism, while full process semantics remain
Phase 15 work.

## Phase 15 process calls

| Number | Call | Purpose |
|---:|---|---|
| 9 | `fork` | Eager-copy the current Ring-3 process |
| 20 | `procinfo` | Copy one process-table entry to userspace |
| 21 | `kill` | Send the simplified SIGTERM to a Ring-3 process |
| 22 | `getppid` | Return the current process parent PID |

`waitpid` now blocks the parent in `TASK_BLOCKED` instead of polling with
`yield()`. A terminated child keeps its exit status until the parent reaps it.
Open file descriptions use reference counting so descriptors inherited across
`fork()` share the same VFS offset.

## Phase 18 networking calls

Phase 18 exposes a deliberately small high-level networking ABI while the
in-kernel Ethernet/IP stack is still young. These calls are synchronous and do
not claim to be a POSIX sockets interface.

| Number | Call | Purpose |
|---:|---|---|
| 23 | `netinfo` | Copy the configured MAC/IPv4/gateway/DNS data and counters |
| 24 | `ping` | Resolve an IPv4/host target and perform one ICMP echo exchange |
| 25 | `dns` | Resolve one hostname to an IPv4 A record using the configured DNS server |
| 26 | `httpget` | Perform one HTTP/1.0 GET over the Phase-18 TCP client |

`httpget` accepts host/path strings and copies at most
`AXIOM_NET_HTTP_USER_MAX` bytes of response body into validated Ring-3 memory.
The result structure reports peer IPv4, port, HTTP status, copied byte count,
and truncation. HTTPS/TLS is not implemented in Phase 18.

The ABI still uses the normal AxiomOS syscall register convention. The fifth
`httpget` argument therefore arrives in `R8` after `RDI`, `RSI`, `RDX`, and
`R10`.

```

## `docs/validation.md`

```markdown
# Validation

AxiomOS uses phase-specific QEMU regression tests plus a full cumulative suite.

Current acceptance commands:

```bash
make clean
make
make test-phase18
make test
```

`make test` runs Phase 1 through Phase 18. Destructive fault/corruption cases
are built in separate output directories so they do not overwrite the normal
kernel image.

Phase 12 additionally validates that:

- `phase12_demo.elf` is a standalone ELF64 `ET_EXEC` with three PT_LOAD segments;
- rootfs and `/tmp` are separate mounts;
- `/bin`, `/etc` and `/tmp` resolve as expected;
- ordinary process file descriptors begin at 3;
- userspace can read `/etc/motd`;
- userspace can create/write/seek/read/stat/close `/tmp/phase12.txt`;
- the kernel reads the file back and verifies its bytes;
- `exec("/bin/phase11-demo")` resolves through the VFS and runs the ELF target;
- the kernel reaches the existing interactive keyboard loop afterward without a panic.

## Phase 14 static/package validation

Before packaging Phase 14, the kernel C sources were compiled with the normal
freestanding flags and every existing deliberate fault/corruption define:
`divide`, `invalid`, `gp`, `double_fault`, `page_fault`, `heap_double_free`, and
`heap_guard`. The userspace shell C and assembly entry were compiled and linked
as ELF64 `ET_EXEC`, x86-64, entry `0x400000`, with three `PT_LOAD` segments.
All Python tests were syntax-checked. Runtime QEMU acceptance remains the local
`make test-phase14` / `make test` step.

## Phase 15

```bash
make test-phase15
```

The test boots QEMU with a disk, drives `/bin/axiomsh` via PS/2 input, executes
`phase15-demo` to verify fork/exec/blocking-wait/private-memory semantics, uses
`ps`, launches a sleeping process in the background, kills it with SIGTERM,
reaps status 143, and then launches another ELF to prove the task slot is
reusable.

## Phase 16

```bash
make test-phase16
```

The test boots QEMU with the ordinary Phase-16 kernel and validates four
synchronization behaviors: a deterministic unprotected lost-update race, an
exact spinlock-protected counter, a sleeping mutex that actually blocks and
wakes waiters, and a counting semaphore whose measured concurrency peak is
exactly its configured limit of two. It then types `echo phase16-ok` into the
Ring-3 shell to verify that the kernel returns to normal userspace operation.

Static package validation additionally compiles every kernel C translation unit
with `-Wall -Wextra -Werror` under normal mode and all seven existing fault /
corruption modes, syntax-checks every Python test, and compiles all existing GAS
and userspace C sources without host-libc dependencies.


## Phase 17

`make test-phase17` boots QEMU with `-smp 4`, requires at least four detected/managed/online CPUs, verifies that every released AP enters AxiomOS code, checks the per-CPU participant mask, and requires the exact locked shared-counter total (`online CPUs × 5,000`). It also requires the normal Ring-3 shell to launch after APs park. Older phase tests may still boot one CPU; Phase 17 discovery remains active but its multicore proof reports SKIPPED on those boots rather than breaking historical regression tests.


## Phase 18

```bash
make test-phase18
```

The Phase-18 test boots QEMU with an explicit Intel E1000 and QEMU user-mode
networking. It drives the Ring-3 shell through virtual PS/2 input and checks:

- PCI/MMIO/DMA E1000 initialization;
- the static Phase-18 guest IPv4 configuration;
- Ethernet + ARP + IPv4 by reaching the QEMU router;
- ICMP Echo Request/Reply with `ping 10.0.2.2`;
- UDP + DNS A-record resolution through the QEMU DNS proxy;
- a TCP three-way handshake and ordered stream receive;
- HTTP/1.0 GET against a deterministic host-side test server exposed with
  QEMU `guestfwd`;
- return to a usable Ring-3 shell without a kernel panic.

The DNS subtest needs ordinary host DNS/Internet connectivity. Static package
validation compiles all 47 kernel C translation units under normal mode and
all seven deliberate fault/corruption modes, builds the GAS/userspace sources,
syntax-checks the Python tests, and audits the combined kernel objects for
unexpected host-libc/runtime dependencies. Actual NIC/TCP runtime acceptance
remains the local QEMU test above.
```

## `drivers/net/e1000.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/pci.h>
#include <axiom/drivers/e1000.h>
#include <axiom/memory/address.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define E1000_VENDOR_INTEL 0x8086u
#define E1000_DEVICE_82540EM 0x100Eu

#define PCI_COMMAND 0x04u
#define PCI_COMMAND_MEMORY (1u << 1)
#define PCI_COMMAND_BUS_MASTER (1u << 2)
#define PCI_BAR0 0x10u
#define PCI_BAR_IO_SPACE 0x1u
#define PCI_BAR_MEMORY_TYPE_MASK 0x6u
#define PCI_BAR_MEMORY_64 0x4u
#define PCI_BAR_MEMORY_MASK 0xFFFFFFF0u

/* 82540EM exposes a 128 KiB register window. */
#define E1000_MMIO_VA 0xFFFFD00000100000ULL
#define E1000_MMIO_BYTES 0x20000u
#define E1000_MMIO_PAGES (E1000_MMIO_BYTES / VMM_PAGE_SIZE)

#define E1000_REG_CTRL  0x0000u
#define E1000_REG_STATUS 0x0008u
#define E1000_REG_ICR   0x00C0u
#define E1000_REG_IMC   0x00D8u
#define E1000_REG_RCTL  0x0100u
#define E1000_REG_TCTL  0x0400u
#define E1000_REG_TIPG  0x0410u
#define E1000_REG_RDBAL 0x2800u
#define E1000_REG_RDBAH 0x2804u
#define E1000_REG_RDLEN 0x2808u
#define E1000_REG_RDH   0x2810u
#define E1000_REG_RDT   0x2818u
#define E1000_REG_TDBAL 0x3800u
#define E1000_REG_TDBAH 0x3804u
#define E1000_REG_TDLEN 0x3808u
#define E1000_REG_TDH   0x3810u
#define E1000_REG_TDT   0x3818u
#define E1000_REG_MTA   0x5200u
#define E1000_REG_RAL0  0x5400u
#define E1000_REG_RAH0  0x5404u

#define E1000_CTRL_SLU (1u << 6)
#define E1000_RCTL_EN  (1u << 1)
#define E1000_RCTL_BAM (1u << 15)
#define E1000_RCTL_SECRC (1u << 26)
#define E1000_TCTL_EN  (1u << 1)
#define E1000_TCTL_PSP (1u << 3)

#define E1000_RX_STATUS_DD  (1u << 0)
#define E1000_RX_STATUS_EOP (1u << 1)
#define E1000_TX_STATUS_DD  (1u << 0)
#define E1000_TX_CMD_EOP    (1u << 0)
#define E1000_TX_CMD_IFCS   (1u << 1)
#define E1000_TX_CMD_RS     (1u << 3)

#define E1000_RX_COUNT 16u
#define E1000_TX_COUNT 8u
#define E1000_BUFFER_BYTES 2048u
#define E1000_TX_WAIT 5000000u

struct e1000_rx_desc {
    uint64_t address;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed));

struct e1000_tx_desc {
    uint64_t address;
    uint16_t length;
    uint8_t checksum_offset;
    uint8_t command;
    uint8_t status;
    uint8_t checksum_start;
    uint16_t special;
} __attribute__((packed));

_Static_assert(sizeof(struct e1000_rx_desc) == 16u, "E1000 RX descriptor size");
_Static_assert(sizeof(struct e1000_tx_desc) == 16u, "E1000 TX descriptor size");

struct e1000_state {
    volatile uint8_t *mmio;
    uint8_t mac[6];
    paddr_t rx_ring_phys;
    paddr_t tx_ring_phys;
    volatile struct e1000_rx_desc *rx_ring;
    volatile struct e1000_tx_desc *tx_ring;
    paddr_t rx_buffer_phys[E1000_RX_COUNT];
    paddr_t tx_buffer_phys[E1000_TX_COUNT];
    uint8_t *rx_buffers[E1000_RX_COUNT];
    uint8_t *tx_buffers[E1000_TX_COUNT];
    uint32_t rx_index;
    uint32_t tx_index;
    struct e1000_stats stats;
    int initialized;
    int available;
};

static struct e1000_state state;

static void bytes_clear(void *memory, size_t count)
{
    volatile uint8_t *out = (volatile uint8_t *)memory;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = 0u;
}

static void bytes_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = in[index];
}

static inline uint32_t mmio_read32(uint32_t offset)
{
    volatile uint32_t *reg = (volatile uint32_t *)(void *)(state.mmio + offset);
    const uint32_t value = *reg;
    __asm__ volatile ("mfence" ::: "memory");
    return value;
}

static inline void mmio_write32(uint32_t offset, uint32_t value)
{
    volatile uint32_t *reg = (volatile uint32_t *)(void *)(state.mmio + offset);
    *reg = value;
    __asm__ volatile ("mfence" ::: "memory");
}

static int map_mmio(paddr_t physical)
{
    uint32_t page;

    physical &= ~(paddr_t)(VMM_PAGE_SIZE - 1u);
    for (page = 0u; page < E1000_MMIO_PAGES; ++page) {
        const vaddr_t va = E1000_MMIO_VA + (vaddr_t)page * VMM_PAGE_SIZE;
        const paddr_t pa = physical + (paddr_t)page * VMM_PAGE_SIZE;
        const paddr_t current = virt_to_phys(va);

        if (current != PADDR_INVALID) {
            if ((current & ~(VMM_PAGE_SIZE - 1ULL)) != pa) return 0;
            continue;
        }
        if (!map_page(
                va,
                pa,
                VMM_FLAG_WRITABLE |
                VMM_FLAG_WRITE_THROUGH |
                VMM_FLAG_CACHE_DISABLE |
                VMM_FLAG_NO_EXECUTE
            )) {
            return 0;
        }
    }
    return 1;
}

static int allocate_dma_page(paddr_t *physical_out, void **virtual_out)
{
    paddr_t physical;
    void *virtual_address;

    if (physical_out == 0 || virtual_out == 0) return 0;
    physical = pmm_alloc_page();
    if (physical == PADDR_INVALID) return 0;
    virtual_address = pmm_phys_to_hhdm(physical);
    if (virtual_address == 0) {
        (void)pmm_free_page(physical);
        return 0;
    }
    bytes_clear(virtual_address, VMM_PAGE_SIZE);
    *physical_out = physical;
    *virtual_out = virtual_address;
    return 1;
}

static int read_mac(void)
{
    const uint32_t low = mmio_read32(E1000_REG_RAL0);
    const uint32_t high = mmio_read32(E1000_REG_RAH0);

    state.mac[0] = (uint8_t)low;
    state.mac[1] = (uint8_t)(low >> 8);
    state.mac[2] = (uint8_t)(low >> 16);
    state.mac[3] = (uint8_t)(low >> 24);
    state.mac[4] = (uint8_t)high;
    state.mac[5] = (uint8_t)(high >> 8);

    return (state.mac[0] | state.mac[1] | state.mac[2] |
            state.mac[3] | state.mac[4] | state.mac[5]) != 0u;
}

static int setup_rx(void)
{
    void *ring_memory;
    uint32_t index;

    if (!allocate_dma_page(&state.rx_ring_phys, &ring_memory)) return 0;
    state.rx_ring = (volatile struct e1000_rx_desc *)ring_memory;

    for (index = 0u; index < E1000_RX_COUNT; ++index) {
        void *buffer;
        if (!allocate_dma_page(&state.rx_buffer_phys[index], &buffer)) return 0;
        state.rx_buffers[index] = (uint8_t *)buffer;
        state.rx_ring[index].address = state.rx_buffer_phys[index];
        state.rx_ring[index].status = 0u;
    }

    mmio_write32(E1000_REG_RDBAL, (uint32_t)state.rx_ring_phys);
    mmio_write32(E1000_REG_RDBAH, (uint32_t)(state.rx_ring_phys >> 32));
    mmio_write32(E1000_REG_RDLEN, E1000_RX_COUNT * sizeof(struct e1000_rx_desc));
    mmio_write32(E1000_REG_RDH, 0u);
    mmio_write32(E1000_REG_RDT, E1000_RX_COUNT - 1u);
    state.rx_index = 0u;

    mmio_write32(E1000_REG_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC);
    return 1;
}

static int setup_tx(void)
{
    void *ring_memory;
    uint32_t index;

    if (!allocate_dma_page(&state.tx_ring_phys, &ring_memory)) return 0;
    state.tx_ring = (volatile struct e1000_tx_desc *)ring_memory;

    for (index = 0u; index < E1000_TX_COUNT; ++index) {
        void *buffer;
        if (!allocate_dma_page(&state.tx_buffer_phys[index], &buffer)) return 0;
        state.tx_buffers[index] = (uint8_t *)buffer;
        state.tx_ring[index].address = state.tx_buffer_phys[index];
        state.tx_ring[index].status = E1000_TX_STATUS_DD;
    }

    mmio_write32(E1000_REG_TDBAL, (uint32_t)state.tx_ring_phys);
    mmio_write32(E1000_REG_TDBAH, (uint32_t)(state.tx_ring_phys >> 32));
    mmio_write32(E1000_REG_TDLEN, E1000_TX_COUNT * sizeof(struct e1000_tx_desc));
    mmio_write32(E1000_REG_TDH, 0u);
    mmio_write32(E1000_REG_TDT, 0u);
    state.tx_index = 0u;

    mmio_write32(E1000_REG_TIPG, 0x0060200Au);
    mmio_write32(
        E1000_REG_TCTL,
        E1000_TCTL_EN | E1000_TCTL_PSP | (0x10u << 4) | (0x40u << 12)
    );
    return 1;
}

int e1000_init(void)
{
    struct pci_device device;
    uint32_t bar_low;
    uint64_t bar_address;
    uint16_t command;
    uint32_t mta_index;

    if (state.initialized) return state.available;
    state.initialized = 1;

    if (!pci_find_device(E1000_VENDOR_INTEL, E1000_DEVICE_82540EM, &device)) {
        return 0;
    }

    bar_low = pci_config_read32(device.bus, device.device, device.function, PCI_BAR0);
    if ((bar_low & PCI_BAR_IO_SPACE) != 0u) return 0;

    bar_address = (uint64_t)(bar_low & PCI_BAR_MEMORY_MASK);
    if ((bar_low & PCI_BAR_MEMORY_TYPE_MASK) == PCI_BAR_MEMORY_64) {
        const uint32_t high = pci_config_read32(
            device.bus, device.device, device.function, PCI_BAR0 + 4u
        );
        bar_address |= (uint64_t)high << 32;
    }
    if (bar_address == 0u || !map_mmio((paddr_t)bar_address)) return 0;

    command = pci_config_read16(device.bus, device.device, device.function, PCI_COMMAND);
    command |= PCI_COMMAND_MEMORY | PCI_COMMAND_BUS_MASTER;
    pci_config_write16(device.bus, device.device, device.function, PCI_COMMAND, command);

    state.mmio = (volatile uint8_t *)(uintptr_t)E1000_MMIO_VA;

    /* This phase is intentionally polled. Mask all NIC interrupts. */
    mmio_write32(E1000_REG_IMC, 0xFFFFFFFFu);
    (void)mmio_read32(E1000_REG_ICR);
    mmio_write32(E1000_REG_CTRL, mmio_read32(E1000_REG_CTRL) | E1000_CTRL_SLU);

    for (mta_index = 0u; mta_index < 128u; ++mta_index) {
        mmio_write32(E1000_REG_MTA + mta_index * 4u, 0u);
    }

    if (!read_mac() || !setup_rx() || !setup_tx()) return 0;

    state.available = 1;
    return 1;
}

int e1000_available(void)
{
    return state.available;
}

int e1000_send(const void *frame, size_t length)
{
    volatile struct e1000_tx_desc *descriptor;
    uint32_t remaining;
    const uint32_t index = state.tx_index;

    if (!state.available || frame == 0 || length < 14u ||
        length > E1000_ETHERNET_FRAME_MAX) {
        return 0;
    }

    descriptor = &state.tx_ring[index];
    remaining = E1000_TX_WAIT;
    while ((descriptor->status & E1000_TX_STATUS_DD) == 0u) {
        if (remaining-- == 0u) {
            ++state.stats.tx_errors;
            return 0;
        }
        __asm__ volatile ("pause");
    }

    bytes_copy(state.tx_buffers[index], frame, length);
    descriptor->length = (uint16_t)length;
    descriptor->checksum_offset = 0u;
    descriptor->checksum_start = 0u;
    descriptor->special = 0u;
    descriptor->status = 0u;
    descriptor->command = E1000_TX_CMD_EOP | E1000_TX_CMD_IFCS | E1000_TX_CMD_RS;
    __asm__ volatile ("mfence" ::: "memory");

    state.tx_index = (index + 1u) % E1000_TX_COUNT;
    mmio_write32(E1000_REG_TDT, state.tx_index);

    remaining = E1000_TX_WAIT;
    while ((descriptor->status & E1000_TX_STATUS_DD) == 0u) {
        if (remaining-- == 0u) {
            ++state.stats.tx_errors;
            return 0;
        }
        __asm__ volatile ("pause");
    }

    ++state.stats.tx_frames;
    state.stats.tx_bytes += length;
    return 1;
}

int e1000_receive(void *frame, size_t capacity, size_t *length_out)
{
    volatile struct e1000_rx_desc *descriptor;
    size_t length;
    uint32_t index;

    if (!state.available || frame == 0 || length_out == 0) return 0;

    index = state.rx_index;
    descriptor = &state.rx_ring[index];
    __asm__ volatile ("mfence" ::: "memory");
    if ((descriptor->status & E1000_RX_STATUS_DD) == 0u) return 0;

    length = descriptor->length;
    if ((descriptor->status & E1000_RX_STATUS_EOP) == 0u ||
        descriptor->errors != 0u || length > capacity ||
        length > E1000_BUFFER_BYTES) {
        ++state.stats.rx_dropped;
        length = 0u;
    } else {
        bytes_copy(frame, state.rx_buffers[index], length);
        ++state.stats.rx_frames;
        state.stats.rx_bytes += length;
    }

    descriptor->status = 0u;
    descriptor->errors = 0u;
    descriptor->length = 0u;
    __asm__ volatile ("mfence" ::: "memory");
    mmio_write32(E1000_REG_RDT, index);
    state.rx_index = (index + 1u) % E1000_RX_COUNT;

    *length_out = length;
    return length != 0u;
}

void e1000_mac(uint8_t mac_out[6])
{
    uint32_t index;
    if (mac_out == 0) return;
    for (index = 0u; index < 6u; ++index) mac_out[index] = state.mac[index];
}

struct e1000_stats e1000_get_stats(void)
{
    return state.stats;
}
```

## `include/axiom/abi/errno.h`

```c
#ifndef AXIOM_ABI_ERRNO_H
#define AXIOM_ABI_ERRNO_H

/* Small errno set shared by kernel and userspace. Syscalls return -errno. */
#define AXIOM_EPERM        1
#define AXIOM_ENOENT       2
#define AXIOM_ESRCH        3
#define AXIOM_EIO          5
#define AXIOM_EBADF        9
#define AXIOM_ECHILD       10
#define AXIOM_ENOMEM      12
#define AXIOM_EACCES      13
#define AXIOM_EFAULT      14
#define AXIOM_EBUSY       16
#define AXIOM_EEXIST      17
#define AXIOM_ENOTDIR     20
#define AXIOM_EISDIR      21
#define AXIOM_EINVAL      22
#define AXIOM_EMFILE      24
#define AXIOM_ENOSPC      28
#define AXIOM_ESPIPE      29
#define AXIOM_EROFS       30
#define AXIOM_ENAMETOOLONG 36
#define AXIOM_ENOSYS      38
#define AXIOM_EMSGSIZE     90
#define AXIOM_ENETDOWN    100
#define AXIOM_ECONNREFUSED 111
#define AXIOM_ETIMEDOUT   110
#define AXIOM_EHOSTUNREACH 113

#endif
```

## `include/axiom/abi/network.h`

```c
#ifndef AXIOM_ABI_NETWORK_H
#define AXIOM_ABI_NETWORK_H

#define AXIOM_NET_HOST_MAX 96u
#define AXIOM_NET_PATH_MAX 192u
#define AXIOM_NET_HTTP_USER_MAX 4096u

#ifndef __ASSEMBLER__
#include <stdint.h>

struct axiom_net_info {
    uint8_t mac[6];
    uint8_t reserved0[2];
    uint32_t address;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
    uint64_t tx_frames;
    uint64_t rx_frames;
    uint64_t arp_requests;
    uint64_t arp_replies;
    uint64_t ipv4_tx;
    uint64_t ipv4_rx;
};

struct axiom_ping_result {
    uint32_t address;
    uint32_t sequence;
    uint64_t poll_iterations;
};

struct axiom_http_result {
    uint32_t address;
    uint16_t port;
    uint16_t status_code;
    uint64_t body_bytes;
    uint32_t truncated;
    uint32_t reserved0;
};
#endif

#endif
```

## `include/axiom/abi/syscall.h`

```c
#ifndef AXIOM_ABI_SYSCALL_H
#define AXIOM_ABI_SYSCALL_H

#include <axiom/abi/errno.h>
#include <axiom/abi/fs.h>
#include <axiom/abi/input.h>
#include <axiom/abi/process.h>
#include <axiom/abi/network.h>

/* Stable syscall numbers shared by kernel and userspace. */
#define AXIOM_SYS_WRITE      1
#define AXIOM_SYS_READ       2
#define AXIOM_SYS_EXIT       3
#define AXIOM_SYS_SLEEP      4
#define AXIOM_SYS_GETPID     5
#define AXIOM_SYS_YIELD      6
#define AXIOM_SYS_OPEN       7
#define AXIOM_SYS_CLOSE      8
#define AXIOM_SYS_FORK       9
#define AXIOM_SYS_EXEC      10
#define AXIOM_SYS_MMAP      11
#define AXIOM_SYS_LSEEK     12
#define AXIOM_SYS_STAT      13
#define AXIOM_SYS_READDIR   14
#define AXIOM_SYS_MKDIR     15
#define AXIOM_SYS_SPAWN     16
#define AXIOM_SYS_WAITPID   17
#define AXIOM_SYS_CLEAR     18
#define AXIOM_SYS_KBDSTATS  19
#define AXIOM_SYS_PROCINFO  20
#define AXIOM_SYS_KILL      21
#define AXIOM_SYS_GETPPID   22
#define AXIOM_SYS_NETINFO   23
#define AXIOM_SYS_PING      24
#define AXIOM_SYS_DNS       25
#define AXIOM_SYS_HTTPGET   26

#define AXIOM_SYSCALL_MAX_NUMBER AXIOM_SYS_HTTPGET

#endif
```

## `include/axiom/arch/pci.h`

```c
#ifndef AXIOM_ARCH_PCI_H
#define AXIOM_ARCH_PCI_H

#include <stdint.h>

struct pci_device {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
};

uint32_t pci_config_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
uint16_t pci_config_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);

int pci_find_device(
    uint16_t vendor_id,
    uint16_t device_id,
    struct pci_device *device_out
);

int pci_find_class(
    uint8_t class_code,
    uint8_t subclass,
    uint8_t prog_if,
    struct pci_device *device_out
);

#endif
```

## `include/axiom/drivers/e1000.h`

```c
#ifndef AXIOM_DRIVERS_E1000_H
#define AXIOM_DRIVERS_E1000_H

#include <stddef.h>
#include <stdint.h>

#define E1000_ETHERNET_FRAME_MAX 1518u

struct e1000_stats {
    uint64_t tx_frames;
    uint64_t rx_frames;
    uint64_t tx_bytes;
    uint64_t rx_bytes;
    uint64_t tx_errors;
    uint64_t rx_dropped;
};

/* Probe and initialise the QEMU/Intel 82540EM E1000 PCI NIC. */
int e1000_init(void);
int e1000_available(void);

/* The Phase-18 driver is deliberately polled; interrupts come later. */
int e1000_send(const void *frame, size_t length);
int e1000_receive(void *frame, size_t capacity, size_t *length_out);

void e1000_mac(uint8_t mac_out[6]);
struct e1000_stats e1000_get_stats(void);

#endif
```

## `include/axiom/kernel/syscall.h`

```c
#ifndef AXIOM_KERNEL_SYSCALL_H
#define AXIOM_KERNEL_SYSCALL_H

#include <stddef.h>
#include <stdint.h>

struct syscall_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t user_rip;
    uint64_t user_rflags;
    uint64_t user_rsp;
};

_Static_assert(sizeof(struct syscall_frame) == 128, "syscall frame size");
_Static_assert(offsetof(struct syscall_frame, rax) == 96, "syscall RAX offset");
_Static_assert(offsetof(struct syscall_frame, user_rip) == 104, "syscall RIP offset");
_Static_assert(offsetof(struct syscall_frame, user_rsp) == 120, "syscall RSP offset");

struct syscall_stats {
    uint64_t total_calls;
    uint64_t write_calls;
    uint64_t read_calls;
    uint64_t exit_calls;
    uint64_t sleep_calls;
    uint64_t getpid_calls;
    uint64_t yield_calls;
    uint64_t open_calls;
    uint64_t close_calls;
    uint64_t fork_calls;
    uint64_t exec_calls;
    uint64_t exec_successes;
    uint64_t lseek_calls;
    uint64_t stat_calls;
    uint64_t readdir_calls;
    uint64_t mkdir_calls;
    uint64_t spawn_calls;
    uint64_t waitpid_calls;
    uint64_t clear_calls;
    uint64_t kbdstats_calls;
    uint64_t procinfo_calls;
    uint64_t kill_calls;
    uint64_t getppid_calls;
    uint64_t netinfo_calls;
    uint64_t ping_calls;
    uint64_t dns_calls;
    uint64_t httpget_calls;
    uint64_t rejected_pointers;
    uint64_t unimplemented_calls;
    uint64_t bytes_written;
    uint64_t bytes_read;
};

int syscall_init(void);
int syscall_initialized(void);
void syscall_set_kernel_stack(uintptr_t stack_top);
void syscall_dispatch(struct syscall_frame *frame);
struct syscall_stats syscall_get_stats(void);

#endif
```

## `include/axiom/network/net.h`

```c
#ifndef AXIOM_NETWORK_NET_H
#define AXIOM_NETWORK_NET_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/network.h>

#define NET_MTU 1500u
#define NET_IPV4(a, b, c, d) \
    (((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | \
     ((uint32_t)(c) << 8) | (uint32_t)(d))

struct net_config {
    uint8_t mac[6];
    uint32_t address;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns_server;
};

struct net_stats {
    uint64_t arp_requests;
    uint64_t arp_replies;
    uint64_t ipv4_tx;
    uint64_t ipv4_rx;
    uint64_t icmp_tx;
    uint64_t icmp_rx;
    uint64_t udp_tx;
    uint64_t udp_rx;
    uint64_t tcp_tx;
    uint64_t tcp_rx;
    uint64_t dns_queries;
    uint64_t dns_answers;
    uint64_t http_requests;
};

/* Initialise Phase-18 networking. No NIC is a non-fatal unavailable result. */
int net_init(void);
int net_available(void);
const struct net_config *net_get_config(void);
struct net_stats net_get_stats(void);

int net_parse_ipv4(const char *text, uint32_t *address_out);
int net_resolve(const char *name, uint32_t *address_out);
int net_ping(const char *target, struct axiom_ping_result *result_out);
int net_http_get(
    const char *host_spec,
    const char *path,
    uint8_t *body,
    size_t capacity,
    struct axiom_http_result *result_out
);

void net_get_abi_info(struct axiom_net_info *info_out);

#endif
```

## `kernel/core/kernel.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/syscall.h>
#include <axiom/abi/user_layout.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/arch/smp.h>

#include <axiom/boot/limine.h>
#include <axiom/drivers/keyboard.h>
#include <axiom/drivers/block.h>
#include <axiom/drivers/serial.h>
#include <axiom/drivers/timer.h>
#include <axiom/elf/elf64.h>
#include <axiom/filesystem/bootstrap.h>
#include <axiom/filesystem/diskfs.h>
#include <axiom/filesystem/vfs.h>
#include <axiom/kernel/syscall.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>
#include <axiom/network/net.h>
#include <axiom/process/scheduler.h>
#include <axiom/sync/selftest.h>

#include <axiom/terminal/kprintf.h>
#include <axiom/terminal/terminal.h>



#define PHASE8_WORKER_TARGET 1000ULL
#define PHASE8_TEST_TIMEOUT_TICKS 300ULL
#define PHASE9_TEST_TIMEOUT_TICKS 500ULL
#define PHASE10_TEST_TIMEOUT_TICKS 1500ULL
#define PHASE11_TEST_TIMEOUT_TICKS 1000ULL
#define PHASE12_TEST_TIMEOUT_TICKS 1000ULL
#define PHASE13_TEST_TIMEOUT_TICKS 1500ULL
#define PHASE9_KERNEL_PROBE_ADDRESS 0xFFFFFFFF80000000ULL

static volatile uint64_t phase8_worker_a_count;
static volatile uint64_t phase8_worker_b_count;
static volatile int phase8_workers_stop;

static const char phase13_persistent_message[] =
    "Persistent storage works across AxiomOS reboots!\n";

static int phase13_bytes_equal(
    const void *left_memory,
    const void *right_memory,
    size_t count
)
{
    const uint8_t *left = (const uint8_t *)left_memory;
    const uint8_t *right = (const uint8_t *)right_memory;
    size_t index;

    if (left == 0 || right == 0) {
        return 0;
    }

    for (index = 0u; index < count; ++index) {
        if (left[index] != right[index]) {
            return 0;
        }
    }
    return 1;
}

static int phase13_persistent_file_matches(int *exists_out)
{
    void *data = 0;
    size_t size = 0u;
    int result;

    if (exists_out == 0) {
        return 0;
    }

    result = vfs_read_all("/disk/persist.txt", &data, &size);
    if (result == -AXIOM_ENOENT) {
        *exists_out = 0;
        return 1;
    }
    if (result < 0) {
        return 0;
    }

    *exists_out = 1;
    result = size == sizeof(phase13_persistent_message) - 1u &&
        phase13_bytes_equal(
            data,
            phase13_persistent_message,
            sizeof(phase13_persistent_message) - 1u
        );
    kfree(data);
    return result;
}


extern const uint8_t phase9_user_hello_start[];
extern const uint8_t phase9_user_hello_end[];
extern const uint8_t phase9_user_fault_start[];
extern const uint8_t phase9_user_fault_end[];
extern const uint8_t phase10_user_start[];
extern const uint8_t phase10_user_end[];

static int phase9_message_ready(const struct task *task, char *message, size_t capacity)
{
    const uint8_t *data;
    const uint64_t *magic;
    size_t index;

    if (task == 0 || task->user_data_page == PADDR_INVALID ||
        message == 0 || capacity == 0u) {
        return 0;
    }

    data = (const uint8_t *)pmm_phys_to_hhdm(task->user_data_page);
    if (data == 0) {
        return 0;
    }

    magic = (const uint64_t *)(const void *)(
        data + TASK_USER_MESSAGE_MAGIC_OFFSET
    );

    if (*magic != TASK_USER_MESSAGE_MAGIC) {
        return 0;
    }

    for (index = 0u; index + 1u < capacity; ++index) {
        message[index] = (char)data[index];

        if (message[index] == '\0') {
            return 1;
        }
    }

    message[capacity - 1u] = '\0';
    return 1;
}

static int phase10_results_ready(
    const struct task *task,
    uint64_t *pid_out,
    int64_t *bad_write_out,
    int64_t *open_result_out,
    char *read_character_out
)
{
    const uint8_t *data;
    uint64_t magic;

    if (task == 0 || task->user_data_page == PADDR_INVALID ||
        pid_out == 0 || bad_write_out == 0 || open_result_out == 0 ||
        read_character_out == 0) {
        return 0;
    }

    data = (const uint8_t *)pmm_phys_to_hhdm(task->user_data_page);
    if (data == 0) {
        return 0;
    }

    magic = *(const uint64_t *)(const void *)(
        data + AXIOM_PHASE10_MAGIC_OFFSET
    );

    if (magic != AXIOM_PHASE10_MAGIC) {
        return 0;
    }

    *pid_out = *(const uint64_t *)(const void *)(
        data + AXIOM_PHASE10_PID_OFFSET
    );
    *bad_write_out = *(const int64_t *)(const void *)(
        data + AXIOM_PHASE10_BAD_WRITE_OFFSET
    );
    *open_result_out = *(const int64_t *)(const void *)(
        data + AXIOM_PHASE10_OPEN_RESULT_OFFSET
    );
    *read_character_out = (char)data[AXIOM_PHASE10_READ_CHAR_OFFSET];
    return 1;
}


static int phase11_results_ready(
    const struct task *task,
    uint64_t *pid_out,
    uint64_t *bss_zero_out,
    uint64_t *bss_probe_out
)
{
    uint64_t values[4];

    if (task == 0 || pid_out == 0 || bss_zero_out == 0 || bss_probe_out == 0 ||
        !task->elf_backed || task->address_space == PADDR_INVALID) {
        return 0;
    }

    if (!vmm_copy_from_user(
            task->address_space,
            values,
            (vaddr_t)AXIOM_PHASE11_DATA_BASE,
            sizeof(values)
        )) {
        return 0;
    }

    if (values[1] != AXIOM_PHASE11_MAGIC) {
        return 0;
    }

    *pid_out = values[0];
    *bss_zero_out = values[2];
    *bss_probe_out = values[3];
    return 1;
}

static int phase11_permissions_ok(const struct task *task)
{
    uint64_t text_flags;
    uint64_t rodata_flags;
    uint64_t data_flags;

    if (task == 0 || task->address_space == PADDR_INVALID ||
        !vmm_mapping_flags_in_address_space(
            task->address_space,
            AXIOM_PHASE11_TEXT_BASE,
            &text_flags
        ) ||
        !vmm_mapping_flags_in_address_space(
            task->address_space,
            AXIOM_PHASE11_RODATA_BASE,
            &rodata_flags
        ) ||
        !vmm_mapping_flags_in_address_space(
            task->address_space,
            AXIOM_PHASE11_DATA_BASE,
            &data_flags
        )) {
        return 0;
    }

    return
        (text_flags & VMM_FLAG_USER) != 0ULL &&
        (text_flags & VMM_FLAG_WRITABLE) == 0ULL &&
        (text_flags & VMM_FLAG_NO_EXECUTE) == 0ULL &&
        (rodata_flags & VMM_FLAG_USER) != 0ULL &&
        (rodata_flags & VMM_FLAG_WRITABLE) == 0ULL &&
        (rodata_flags & VMM_FLAG_NO_EXECUTE) != 0ULL &&
        (data_flags & VMM_FLAG_USER) != 0ULL &&
        (data_flags & VMM_FLAG_WRITABLE) != 0ULL &&
        (data_flags & VMM_FLAG_NO_EXECUTE) != 0ULL;
}


static void phase8_counter_worker(void *argument)
{
    volatile uint64_t *counter = (volatile uint64_t *)argument;

    /*
     * These workers exist only to prove timer-driven preemption in Phase 8.
     * Leaving them alive forever would burn two full scheduler quanta in every
     * later round-robin cycle and distort blocking/synchronization tests.
     */
    while (!phase8_workers_stop) {
        ++(*counter);
        __asm__ volatile ("pause" ::: "memory");
    }

    task_exit_current(0);
}


void kernel_main(void)
{
    uint64_t line;
    uint64_t scroll_lines;
    struct pmm_stats memory_stats;


    /*
     * Phase-1 early debugging output remains available.
     */
    serial_init();


    if (!limine_base_revision_supported()) {
        serial_write_string(
            "AxiomOS boot error: "
            "unsupported Limine protocol revision.\n"
        );

        return;
    }


    /*
     * Keep this exact line so the Phase-1 regression test continues to work.
     */
    serial_write_string(
        "AxiomOS kernel booted successfully.\n"
    );


    /*
     * Initialise the Phase-2 framebuffer terminal.
     */
    if (!terminal_init()) {
        serial_write_string(
            "AxiomOS terminal error: "
            "framebuffer terminal unavailable.\n"
        );

        return;
    }


    /*
     * Exercise scrolling by printing slightly more than one screen of text.
     */
    scroll_lines =
        (uint64_t)terminal_rows()
        +
        3ULL;


    for (line = 1; line <= scroll_lines; ++line) {
        kprintf(
            "Scroll exercise line %llu\n",
            (unsigned long long)line
        );
    }


    terminal_clear();


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "AxiomOS Phase 2 terminal online.\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Terminal geometry: %llux%llu cells\n",
        (unsigned long long)terminal_columns(),
        (unsigned long long)terminal_rows()
    );


    kprintf(
        "Decimal test: %u\n",
        123456789u
    );


    kprintf(
        "Hex test: 0x%X\n",
        0xDEADBEEFu
    );


    kprintf(
        "64-bit test: %llu\n",
        18446744073709551615ULL
    );


    kprintf(
        "Kernel entry: %p\n",
        (void *)(uintptr_t)&kernel_main
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Color output: OK\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_YELLOW,
        TERMINAL_COLOR_BLACK
    );


    kprintf(
        "Phase 2 terminal test complete.\n"
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    /*
     * Phase 3 CPU architecture initialization and regression tests.
     */
    gdt_init();
    interrupts_init();

    kprintf("GDT/TSS loaded. IDT: 256 gates installed.\n");
    kprintf("PIC remapped: IRQs masked; IF=0.\n");

    phase3_selftest();


    /*
     * Phase 4: discover usable physical RAM and initialise the 4 KiB page
     * allocator. Only LIMINE_MEMMAP_USABLE pages are managed at this stage.
     */
    if (!pmm_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        kprintf("Phase 4 PMM initialization: FAILED\n");
        return;
    }


    memory_stats = pmm_get_stats();


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("AxiomOS Phase 4 physical memory manager online.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    kprintf(
        "PMM memory map entries: %llu\n",
        (unsigned long long)memory_stats.memory_map_entries
    );

    kprintf(
        "PMM total RAM-like memory: %llu MiB\n",
        (unsigned long long)(
            memory_stats.total_memory_bytes /
            (1024ULL * 1024ULL)
        )
    );

    kprintf(
        "PMM usable memory: %llu MiB (%llu pages)\n",
        (unsigned long long)(
            memory_stats.usable_memory_bytes /
            (1024ULL * 1024ULL)
        ),
        (unsigned long long)memory_stats.usable_pages
    );

    kprintf(
        "PMM metadata: %llu pages at physical 0x%llX\n",
        (unsigned long long)memory_stats.metadata_pages,
        (unsigned long long)memory_stats.metadata_base
    );

    kprintf(
        "PMM allocated pages: %llu\n",
        (unsigned long long)memory_stats.allocated_pages
    );

    kprintf(
        "PMM free pages: %llu\n",
        (unsigned long long)memory_stats.free_pages
    );


    if (!phase4_pmm_selftest()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        return;
    }


    memory_stats = pmm_get_stats();

    kprintf(
        "PMM post-test allocated pages: %llu\n",
        (unsigned long long)memory_stats.allocated_pages
    );

    kprintf(
        "PMM post-test free pages: %llu\n",
        (unsigned long long)memory_stats.free_pages
    );


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 4 physical memory manager complete.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    /*
     * Phase 5: clone Limine's active 4-level paging hierarchy into physical
     * pages owned by our PMM, switch CR3, and expose 4 KiB map/unmap/translate
     * primitives. Existing kernel/HHDM/framebuffer mappings are preserved by
     * the clone.
     */
    if (!vmm_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        kprintf("Phase 5 VMM initialization: FAILED\n");
        return;
    }


    {
        const struct vmm_stats virtual_memory = vmm_get_stats();
        const paddr_t kernel_main_physical =
            virt_to_phys((vaddr_t)(uintptr_t)&kernel_main);


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );

        kprintf("AxiomOS Phase 5 virtual memory manager online.\n");


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        kprintf(
            "VMM root PML4: physical 0x%llX\n",
            (unsigned long long)virtual_memory.root_table
        );

        kprintf(
            "VMM owned page-table pages: %llu\n",
            (unsigned long long)virtual_memory.page_table_pages
        );

        kprintf(
            "VMM HHDM offset: 0x%llX\n",
            (unsigned long long)virtual_memory.hhdm_offset
        );

        kprintf(
            "kernel_main physical address: 0x%llX\n",
            (unsigned long long)kernel_main_physical
        );


        if (kernel_main_physical == PADDR_INVALID) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );

            kprintf("Phase 5 kernel translation check: FAILED\n");
            return;
        }
    }


    if (!phase5_vmm_selftest()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        return;
    }


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 5 virtual memory manager complete.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


#ifdef AXIOM_TEST_PAGE_FAULT
    kprintf("Test: triggering unmapped page read.\n");

    {
        volatile const uint64_t *fault_address =
            (volatile const uint64_t *)(uintptr_t)VMM_FAULT_TEST_ADDRESS;

        volatile uint64_t ignored = *fault_address;
        (void)ignored;
    }

    kprintf("Phase 5 page-fault test: FAILED to fault\n");
#endif

    /*
     * Phase 6: higher-half kernel heap backed by Phase-4 physical pages and
     * Phase-5 virtual mappings. The normal self-test leaves no live
     * allocations but may leave additional heap pages mapped for reuse.
     */
    if (!heap_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        kprintf("Phase 6 heap initialization: FAILED\n");
        return;
    }


    {
        const struct heap_stats initial_heap = heap_get_stats();


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );

        kprintf("AxiomOS Phase 6 kernel heap online.\n");


        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        kprintf(
            "Heap base: 0x%llX\n",
            (unsigned long long)HEAP_BASE_ADDRESS
        );

        kprintf(
            "Heap mapped pages: %llu\n",
            (unsigned long long)initial_heap.mapped_pages
        );

        kprintf(
            "Heap initial free bytes: %llu\n",
            (unsigned long long)initial_heap.free_bytes
        );
    }


    if (!phase6_heap_selftest()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );

        return;
    }


    {
        const struct heap_stats final_heap = heap_get_stats();

        kprintf(
            "Heap mapped pages after test: %llu\n",
            (unsigned long long)final_heap.mapped_pages
        );

        kprintf(
            "Heap active allocations: %llu\n",
            (unsigned long long)final_heap.active_allocations
        );

        kprintf(
            "Heap bytes in use: %llu\n",
            (unsigned long long)final_heap.bytes_in_use
        );

        kprintf(
            "Heap free bytes: %llu\n",
            (unsigned long long)final_heap.free_bytes
        );

        kprintf(
            "Heap largest free block: %llu\n",
            (unsigned long long)final_heap.largest_free_block
        );

        kprintf(
            "Heap total allocations: %llu\n",
            (unsigned long long)final_heap.total_allocations
        );

        kprintf(
            "Heap total frees: %llu\n",
            (unsigned long long)final_heap.total_frees
        );

        kprintf(
            "Heap reallocations: %llu\n",
            (unsigned long long)final_heap.total_reallocations
        );

        kprintf(
            "Heap failed allocations: %llu\n",
            (unsigned long long)final_heap.failed_allocations
        );
    }


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );

    kprintf("Phase 6 kernel heap complete.\n");


    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


#ifdef AXIOM_TEST_HEAP_DOUBLE_FREE
    {
        void *first = kmalloc(64);
        void *second = kmalloc(64);

        if (first == 0 || second == 0) {
            kprintf("Phase 6 double-free setup: FAILED\n");
            return;
        }

        kfree(first);
        kprintf("Test: triggering heap double free.\n");
        kfree(first);

        kprintf("Phase 6 double-free test: FAILED to panic\n");
        kfree(second);
    }
#endif


#ifdef AXIOM_TEST_HEAP_GUARD
    {
        uint8_t *buffer = (uint8_t *)kmalloc(32);

        if (buffer == 0) {
            kprintf("Phase 6 guard-corruption setup: FAILED\n");
            return;
        }

        buffer[32] = 0xA5u;

        kprintf("Test: triggering heap tail-guard corruption.\n");
        kfree(buffer);

        kprintf("Phase 6 guard-corruption test: FAILED to panic\n");
    }
#endif


    /*
     * Phase 7: IRQ-driven time and keyboard input. q35 is an APIC-era
     * platform, so the local APIC provides the periodic timer and the I/O
     * APIC routes the PS/2 keyboard. The legacy PIC remains fully masked.
     */
    if (!timer_init(100u)) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 7 timer initialization: FAILED\n");
        return;
    }

    if (!keyboard_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 7 keyboard initialization: FAILED\n");
        return;
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("AxiomOS Phase 7 timer + keyboard online.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );
    kprintf("APIC timer frequency: %u Hz\n", timer_frequency());
    kprintf("Interrupt controller: Local APIC + I/O APIC\n");

    interrupts_enable();

    if (!interrupts_enabled()) {
        kprintf("Phase 7 interrupt enable: FAILED\n");
        return;
    }

    {
        const uint64_t before = timer_ticks();
        uint64_t elapsed;

        timer_wait_ticks(10u);
        elapsed = timer_ticks() - before;

        if (elapsed < 10u) {
            kprintf("Phase 7 timer test: FAILED\n");
            return;
        }

        kprintf(
            "Timer self-test ticks: %llu\n",
            (unsigned long long)elapsed
        );
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 7 timer test: OK\n");
    kprintf("Phase 7 timer + keyboard drivers complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 8: preemptive kernel-thread multitasking. Build every task while
     * IF=0 because task creation allocates private stacks from the Phase-6
     * heap, which is intentionally not interrupt-safe yet.
     */
    interrupts_disable();

    if (!scheduler_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 8 scheduler initialization: FAILED\n");
        return;
    }

    phase8_worker_a_count = 0u;
    phase8_worker_b_count = 0u;
    phase8_workers_stop = 0;

    {
        uint64_t worker_a_id;
        uint64_t worker_b_id;

        if (!task_create(
                "phase8-worker-a",
                phase8_counter_worker,
                (void *)&phase8_worker_a_count,
                &worker_a_id
            ) ||
            !task_create(
                "phase8-worker-b",
                phase8_counter_worker,
                (void *)&phase8_worker_b_count,
                &worker_b_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 8 task creation: FAILED\n");
            return;
        }

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 8 preemptive scheduler online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Scheduler policy: preemptive round-robin\n");
        kprintf(
            "Scheduler quantum: %u timer ticks\n",
            SCHEDULER_DEFAULT_QUANTUM_TICKS
        );
        kprintf(
            "Phase 8 worker task IDs: %llu, %llu\n",
            (unsigned long long)worker_a_id,
            (unsigned long long)worker_b_id
        );
    }

    if (!scheduler_start()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 8 scheduler start: FAILED\n");
        return;
    }

    interrupts_enable();

    {
        const uint64_t start = timer_ticks();

        while ((phase8_worker_a_count < PHASE8_WORKER_TARGET ||
                phase8_worker_b_count < PHASE8_WORKER_TARGET) &&
               (timer_ticks() - start) < PHASE8_TEST_TIMEOUT_TICKS) {
            /*
             * The bootstrap thread sleeps. Worker A and worker B never yield;
             * only timer preemption can move execution between all three.
             */
            __asm__ volatile ("hlt" ::: "memory");
        }
    }

    {
        const struct scheduler_stats scheduler = scheduler_get_stats();

        if (phase8_worker_a_count < PHASE8_WORKER_TARGET ||
            phase8_worker_b_count < PHASE8_WORKER_TARGET ||
            scheduler.context_switches < 3u ||
            scheduler.preemptions < 3u ||
            scheduler.task_count != 3u) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 8 preemptive multitasking test: FAILED\n");
            return;
        }

        kprintf(
            "Phase 8 task count: %llu\n",
            (unsigned long long)scheduler.task_count
        );
        kprintf(
            "Phase 8 worker A count: %llu\n",
            (unsigned long long)phase8_worker_a_count
        );
        kprintf(
            "Phase 8 worker B count: %llu\n",
            (unsigned long long)phase8_worker_b_count
        );
        kprintf(
            "Phase 8 context switches: %llu\n",
            (unsigned long long)scheduler.context_switches
        );
        kprintf(
            "Phase 8 timer preemptions: %llu\n",
            (unsigned long long)scheduler.preemptions
        );
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 8 preemptive multitasking test: OK\n");
    kprintf("Phase 8 multitasking complete.\n");

    /*
     * The two Phase-8 workers are test fixtures, not kernel services. Stop
     * them after their acceptance criteria have been measured so later phases
     * are not forced to share the CPU with permanent synthetic busy loops.
     */
    phase8_workers_stop = 1;

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 9: create two Ring-3 tasks while interrupts are disabled. The first
     * writes a message into its own user data page. The second deliberately
     * reads a supervisor-only kernel address and must be killed by #PF.
     */
    interrupts_disable();

    {
        uint64_t hello_id;
        uint64_t fault_id;
        const size_t hello_size =
            (size_t)(phase9_user_hello_end - phase9_user_hello_start);
        const size_t fault_size =
            (size_t)(phase9_user_fault_end - phase9_user_fault_start);

        if (!user_task_create(
                "phase9-user-hello",
                phase9_user_hello_start,
                hello_size,
                &hello_id
            ) ||
            !user_task_create(
                "phase9-user-protection",
                phase9_user_fault_start,
                fault_size,
                &fault_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 9 user task creation: FAILED\n");
            return;
        }

        {
            const struct task *hello = scheduler_task_by_id(hello_id);
            const struct task *fault = scheduler_task_by_id(fault_id);
            const paddr_t kernel_space = vmm_kernel_address_space();

            if (hello == 0 || fault == 0 ||
                hello->address_space == PADDR_INVALID ||
                fault->address_space == PADDR_INVALID ||
                hello->address_space == fault->address_space ||
                hello->address_space == kernel_space ||
                fault->address_space == kernel_space) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 9 address-space creation: FAILED\n");
                return;
            }

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_CYAN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("AxiomOS Phase 9 Ring 3 userspace online.\n");

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
            kprintf(
                "Phase 9 user hello task ID: %llu\n",
                (unsigned long long)hello_id
            );
            kprintf(
                "Phase 9 protection task ID: %llu\n",
                (unsigned long long)fault_id
            );
            kprintf("Phase 9 user privilege: Ring 3\n");
            kprintf(
                "Phase 9 hello address space: 0x%llX\n",
                (unsigned long long)hello->address_space
            );
            kprintf(
                "Phase 9 protection address space: 0x%llX\n",
                (unsigned long long)fault->address_space
            );
            kprintf("Phase 9 separate address spaces: OK\n");
        }

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            char user_message[64];
            int hello_ready = 0;
            int protection_ready = 0;

            user_message[0] = '\0';

            while ((timer_ticks() - start) < PHASE9_TEST_TIMEOUT_TICKS) {
                const struct task *hello = scheduler_task_by_id(hello_id);
                const struct task *fault = scheduler_task_by_id(fault_id);

                hello_ready = phase9_message_ready(
                    hello,
                    user_message,
                    sizeof(user_message)
                );

                protection_ready =
                    fault != 0 &&
                    fault->state == TASK_TERMINATED &&
                    fault->fault_vector == 14u &&
                    fault->fault_address == PHASE9_KERNEL_PROBE_ADDRESS &&
                    (fault->fault_error_code & 0x5ULL) == 0x5ULL;

                if (hello_ready && protection_ready) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            if (!hello_ready || !protection_ready) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 9 userspace isolation test: FAILED\n");
                return;
            }

            {
                const struct task *fault = scheduler_task_by_id(fault_id);
                const struct scheduler_stats stats = scheduler_get_stats();

                kprintf("%s\n", user_message);
                kprintf("Phase 9 kernel protection fault: OK\n");
                kprintf(
                    "Phase 9 protection fault vector: %llu\n",
                    (unsigned long long)fault->fault_vector
                );
                kprintf(
                    "Phase 9 protection fault address: 0x%llX\n",
                    (unsigned long long)fault->fault_address
                );
                kprintf(
                    "Phase 9 protection fault error: 0x%llX\n",
                    (unsigned long long)fault->fault_error_code
                );
                kprintf(
                    "Phase 9 user fault terminations: %llu\n",
                    (unsigned long long)stats.user_fault_terminations
                );
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 9 userspace isolation test: OK\n");
    kprintf("Phase 9 userspace complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 10: install the x86-64 SYSCALL/SYSRET ABI and launch a Ring-3
     * program that exercises write/read/getpid/sleep/yield/exit. open() is
     * intentionally reserved but returns -ENOSYS until the VFS exists.
     */
    interrupts_disable();

    if (!syscall_init()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 10 syscall initialization: FAILED\n");
        return;
    }

    {
        uint64_t syscall_task_id;
        const size_t image_size =
            (size_t)(phase10_user_end - phase10_user_start);

        if (!user_task_create(
                "phase10-syscall-demo",
                phase10_user_start,
                image_size,
                &syscall_task_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 10 user task creation: FAILED\n");
            return;
        }

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 10 syscall interface online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Syscall mechanism: x86-64 SYSCALL/SYSRET\n");
        kprintf(
            "Phase 10 user task ID: %llu\n",
            (unsigned long long)syscall_task_id
        );
        kprintf("Phase 10 demo: type one key when userspace asks for input.\n");

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            int results_ready = 0;
            uint64_t pid_result = 0u;
            int64_t bad_write_result = 0;
            int64_t open_result = 0;
            char read_character = '\0';

            while ((timer_ticks() - start) < PHASE10_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(syscall_task_id);

                results_ready = phase10_results_ready(
                    task,
                    &pid_result,
                    &bad_write_result,
                    &open_result,
                    &read_character
                );

                if (results_ready && task != 0 &&
                    task->state == TASK_TERMINATED) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            {
                const struct task *task = scheduler_task_by_id(syscall_task_id);
                const struct syscall_stats syscall_stats = syscall_get_stats();

                if (!results_ready || task == 0 ||
                    task->state != TASK_TERMINATED ||
                    task->exit_code != AXIOM_PHASE10_EXIT_STATUS ||
                    pid_result != syscall_task_id ||
                    bad_write_result != -(int64_t)AXIOM_EFAULT ||
                    open_result != -(int64_t)AXIOM_ENOSYS ||
                    syscall_stats.write_calls < 2u ||
                    syscall_stats.read_calls < 1u ||
                    syscall_stats.sleep_calls < 1u ||
                    syscall_stats.getpid_calls < 1u ||
                    syscall_stats.yield_calls < 1u ||
                    syscall_stats.exit_calls < 1u ||
                    syscall_stats.rejected_pointers < 1u ||
                    syscall_stats.unimplemented_calls < 1u) {
                    terminal_set_color(
                        TERMINAL_COLOR_LIGHT_RED,
                        TERMINAL_COLOR_BLACK
                    );
                    kprintf("Phase 10 syscall self-test: FAILED\n");
                    return;
                }

                kprintf(
                    "Phase 10 getpid result: %llu\n",
                    (unsigned long long)pid_result
                );
                kprintf(
                    "Phase 10 invalid pointer result: %lld\n",
                    (long long)bad_write_result
                );
                kprintf(
                    "Phase 10 open result: %lld\n",
                    (long long)open_result
                );
                if (read_character != '\0') {
                    kprintf(
                        "Phase 10 read character: %c\n",
                        read_character
                    );
                } else {
                    kprintf("Phase 10 read character: <none>\n");
                }
                kprintf(
                    "Phase 10 exit status: %lld\n",
                    (long long)task->exit_code
                );
                kprintf(
                    "Phase 10 syscall calls: %llu\n",
                    (unsigned long long)syscall_stats.total_calls
                );
                kprintf(
                    "Phase 10 bytes written/read: %llu/%llu\n",
                    (unsigned long long)syscall_stats.bytes_written,
                    (unsigned long long)syscall_stats.bytes_read
                );
                kprintf(
                    "Phase 10 rejected user pointers: %llu\n",
                    (unsigned long long)syscall_stats.rejected_pointers
                );
                kprintf("Phase 10 user-copy validation: OK\n");
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 10 syscall self-test: OK\n");
    kprintf("Phase 10 system calls complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );


    /*
     * Phase 12's VFS is initialized here so Phase 11's accepted exec() test
     * now resolves /bin/phase11-demo through normal VFS lookup rather than a
     * syscall-local boot-module registry. The visible Phase-12 acceptance test
     * still runs after Phase 11, preserving the roadmap/output order.
     */
    interrupts_disable();
    if (!filesystem_phase12_bootstrap()) {
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_RED,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 12 VFS bootstrap: FAILED\n");
        return;
    }

    /*
     * Phase 11: start from a standalone ELF launcher, then prove SYS_exec by
     * replacing that process with /bin/phase11-demo, another standalone ELF.
     * Phase 12 is already bootstrapped, so exec now resolves the target through
     * the VFS while the launcher itself remains a Limine-delivered test input.
     */
    interrupts_disable();

    {
        const void *launcher_image;
        uint64_t launcher_size_u64;
        const char *launcher_path;
        const void *demo_image;
        uint64_t demo_size_u64;
        const char *demo_path;
        uint64_t elf_task_id;

        if (!limine_get_module(
                "phase11-launcher",
                &launcher_image,
                &launcher_size_u64,
                &launcher_path
            ) ||
            !limine_get_module(
                "phase11-demo",
                &demo_image,
                &demo_size_u64,
                &demo_path
            ) ||
            launcher_size_u64 > (uint64_t)SIZE_MAX ||
            demo_size_u64 > (uint64_t)SIZE_MAX ||
            !user_task_create_elf(
                "phase11-exec-launcher",
                launcher_image,
                (size_t)launcher_size_u64,
                &elf_task_id
            )) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 11 ELF task creation: FAILED (%s)\n", elf64_last_error());
            return;
        }

        (void)demo_image;
        (void)demo_size_u64;

        {
            const struct task *task = scheduler_task_by_id(elf_task_id);

            if (task == 0 || !task->elf_backed ||
                task->user_entry != AXIOM_PHASE11_TEXT_BASE ||
                !phase11_permissions_ok(task)) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 11 ELF mapping validation: FAILED\n");
                return;
            }

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_CYAN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("AxiomOS Phase 11 ELF64 loader online.\n");

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
            kprintf("ELF launcher source: %s (Limine module)\n", launcher_path);
            kprintf("ELF exec target: /bin/phase11-demo -> %s\n", demo_path);
            kprintf("ELF type: ET_EXEC x86-64\n");
            kprintf(
                "Phase 11 ELF task ID: %llu\n",
                (unsigned long long)elf_task_id
            );
        }

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            int results_ready = 0;
            uint64_t pid_result = 0u;
            uint64_t bss_was_zero = 0u;
            uint64_t bss_probe = UINT64_MAX;

            while ((timer_ticks() - start) < PHASE11_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(elf_task_id);

                results_ready = phase11_results_ready(
                    task,
                    &pid_result,
                    &bss_was_zero,
                    &bss_probe
                );

                if (results_ready && task != 0 &&
                    task->state == TASK_TERMINATED) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            {
                const struct task *task = scheduler_task_by_id(elf_task_id);
                const struct syscall_stats syscall_stats = syscall_get_stats();

                if (!results_ready || task == 0 ||
                    task->state != TASK_TERMINATED ||
                    task->exit_code != AXIOM_PHASE11_EXIT_STATUS ||
                    pid_result != elf_task_id ||
                    bss_was_zero != 1u || bss_probe != 0u ||
                    !task->elf_backed ||
                    task->user_entry != AXIOM_PHASE11_TEXT_BASE ||
                    task->elf_program_headers != 3u ||
                    task->elf_load_segments != 3u ||
                    task->user_elf_page_count != 3u ||
                    !phase11_permissions_ok(task) ||
                    syscall_stats.exec_calls < 1u ||
                    syscall_stats.exec_successes < 1u) {
                    terminal_set_color(
                        TERMINAL_COLOR_LIGHT_RED,
                        TERMINAL_COLOR_BLACK
                    );
                    kprintf("Phase 11 ELF executable test: FAILED\n");
                    return;
                }

                kprintf(
                    "ELF entry point: 0x%llX\n",
                    (unsigned long long)task->user_entry
                );
                kprintf(
                    "ELF program headers: %u\n",
                    (unsigned)task->elf_program_headers
                );
                kprintf(
                    "ELF PT_LOAD segments: %u\n",
                    (unsigned)task->elf_load_segments
                );
                kprintf(
                    "ELF mapped pages: %llu\n",
                    (unsigned long long)task->user_elf_page_count
                );
                kprintf(
                    "ELF file/memory bytes: %llu/%llu\n",
                    (unsigned long long)task->elf_file_bytes,
                    (unsigned long long)task->elf_memory_bytes
                );
                kprintf("Phase 11 segment permissions: RX/R/RW+NX OK\n");
                kprintf(
                    "Phase 11 exec transitions: %llu/%llu\n",
                    (unsigned long long)syscall_stats.exec_successes,
                    (unsigned long long)syscall_stats.exec_calls
                );
                kprintf(
                    "Phase 11 getpid result: %llu\n",
                    (unsigned long long)pid_result
                );
                kprintf("Phase 11 exec preserved task ID: OK\n");
                kprintf("Phase 11 BSS zero-fill: OK\n");
                kprintf(
                    "Phase 11 exit status: %lld\n",
                    (long long)task->exit_code
                );
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 11 ELF executable test: OK\n");
    kprintf("Phase 11 ELF loader complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 12: load a standalone ELF through the VFS itself. The program
     * exercises open/read/write/close/lseek/stat on rootfs and the separately
     * mounted /tmp RAM filesystem, then execs /bin/phase11-demo through VFS.
     */
    interrupts_disable();

    {
        struct axiom_stat bin_stat;
        struct axiom_stat tmp_stat;
        void *phase12_image = 0;
        size_t phase12_image_size = 0u;
        uint64_t phase12_task_id;
        const struct syscall_stats syscall_before = syscall_get_stats();
        const struct vfs_stats vfs_before = vfs_get_stats();

        if (vfs_stat("/bin", &bin_stat) < 0 ||
            vfs_stat("/tmp", &tmp_stat) < 0 ||
            bin_stat.type != AXIOM_DT_DIR ||
            tmp_stat.type != AXIOM_DT_DIR ||
            vfs_read_all(
                "/bin/phase12-demo",
                &phase12_image,
                &phase12_image_size
            ) < 0 ||
            !user_task_create_elf(
                "phase12-vfs-demo",
                phase12_image,
                phase12_image_size,
                &phase12_task_id
            )) {
            if (phase12_image != 0) {
                kfree(phase12_image);
            }
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 12 VFS task creation: FAILED\n");
            return;
        }

        kfree(phase12_image);

        {
            const struct vfs_stats mounted = vfs_get_stats();

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_CYAN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("AxiomOS Phase 12 virtual filesystem online.\n");

            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
            kprintf("VFS root filesystem: ramfs mounted at /\n");
            kprintf("VFS secondary mount: ramfs mounted at /tmp\n");
            kprintf(
                "VFS mount count: %llu\n",
                (unsigned long long)mounted.mount_count
            );
            kprintf("VFS /bin type: DIRECTORY\n");
            kprintf("VFS /tmp type: DIRECTORY (mount point)\n");
            kprintf(
                "Phase 12 ELF task ID: %llu\n",
                (unsigned long long)phase12_task_id
            );
            kprintf("Phase 12 ELF source: /bin/phase12-demo (VFS)\n");
        }

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();
            int target_ready = 0;
            uint64_t pid_result = 0u;
            uint64_t bss_was_zero = 0u;
            uint64_t bss_probe = UINT64_MAX;

            while ((timer_ticks() - start) < PHASE12_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(phase12_task_id);

                target_ready = phase11_results_ready(
                    task,
                    &pid_result,
                    &bss_was_zero,
                    &bss_probe
                );

                if (target_ready && task != 0 &&
                    task->state == TASK_TERMINATED) {
                    break;
                }

                __asm__ volatile ("hlt" ::: "memory");
            }

            interrupts_disable();

            {
                const struct task *task = scheduler_task_by_id(phase12_task_id);
                const struct syscall_stats syscall_after = syscall_get_stats();
                const struct vfs_stats vfs_after = vfs_get_stats();
                void *roundtrip_data = 0;
                size_t roundtrip_size = 0u;
                static const char expected_roundtrip[] =
                    "RAM filesystem round-trip works!\n";
                int roundtrip_ok = 0;

                if (vfs_read_all(
                        "/tmp/phase12.txt",
                        &roundtrip_data,
                        &roundtrip_size
                    ) == 0 &&
                    roundtrip_size == sizeof(expected_roundtrip) - 1u) {
                    size_t index;

                    roundtrip_ok = 1;
                    for (index = 0u; index < roundtrip_size; ++index) {
                        if (((const char *)roundtrip_data)[index] !=
                            expected_roundtrip[index]) {
                            roundtrip_ok = 0;
                            break;
                        }
                    }
                }

                if (roundtrip_data != 0) {
                    kfree(roundtrip_data);
                }

                if (!target_ready || task == 0 ||
                    task->state != TASK_TERMINATED ||
                    task->exit_code != AXIOM_PHASE11_EXIT_STATUS ||
                    pid_result != phase12_task_id ||
                    bss_was_zero != 1u || bss_probe != 0u ||
                    !roundtrip_ok ||
                    vfs_after.mount_count != 2u ||
                    syscall_after.open_calls < syscall_before.open_calls + 2u ||
                    syscall_after.close_calls < syscall_before.close_calls + 2u ||
                    syscall_after.lseek_calls < syscall_before.lseek_calls + 1u ||
                    syscall_after.stat_calls < syscall_before.stat_calls + 2u ||
                    syscall_after.exec_successes < syscall_before.exec_successes + 1u ||
                    vfs_after.reads <= vfs_before.reads ||
                    vfs_after.writes <= vfs_before.writes) {
                    terminal_set_color(
                        TERMINAL_COLOR_LIGHT_RED,
                        TERMINAL_COLOR_BLACK
                    );
                    kprintf("Phase 12 VFS self-test: FAILED\n");
                    return;
                }

                kprintf("Phase 12 file descriptor base: 3\n");
                kprintf("Phase 12 /tmp file round-trip: OK\n");
                kprintf("Phase 12 seek/stat operations: OK\n");
                kprintf("Phase 12 exec through VFS: OK\n");
                kprintf(
                    "Phase 12 VFS opens/closes: %llu/%llu\n",
                    (unsigned long long)vfs_after.opens,
                    (unsigned long long)vfs_after.closes
                );
                kprintf(
                    "Phase 12 VFS bytes read/written: %llu/%llu\n",
                    (unsigned long long)vfs_after.bytes_read,
                    (unsigned long long)vfs_after.bytes_written
                );
            }
        }
    }

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 12 VFS self-test: OK\n");
    kprintf("Phase 12 virtual filesystem complete.\n");

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

    /*
     * Phase 13: discover the q35 AHCI controller through PCI, expose its SATA
     * disk as a 512-byte block device, mount a tiny persistent filesystem at
     * /disk, and run a Ring-3 program against that mount. Earlier regression
     * tests intentionally boot without a hard disk, so absence is non-fatal.
     */
    interrupts_disable();

    if (block_init()) {
        struct block_device *disk = block_primary();
        struct diskfs_info disk_info;
        struct vfs_filesystem *persistent_fs;
        int persistent_before = 0;
        int mkdir_result;
        void *phase13_image = 0;
        size_t phase13_image_size = 0u;
        uint64_t phase13_task_id = 0u;

        mkdir_result = vfs_mkdir("/disk");
        if (mkdir_result < 0 && mkdir_result != -AXIOM_EEXIST) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 diskfs mount: FAILED\n");
            return;
        }

        persistent_fs = diskfs_create(disk, &disk_info);
        if (persistent_fs == 0 || vfs_mount("/disk", persistent_fs) < 0) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 diskfs mount: FAILED\n");
            return;
        }

        if (!phase13_persistent_file_matches(&persistent_before)) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 persistent file validation: FAILED\n");
            return;
        }

        if (vfs_read_all(
                "/bin/phase13-demo",
                &phase13_image,
                &phase13_image_size
            ) < 0 ||
            !user_task_create_elf(
                "phase13-disk-demo",
                phase13_image,
                phase13_image_size,
                &phase13_task_id
            )) {
            if (phase13_image != 0) {
                kfree(phase13_image);
            }
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 13 disk task creation: FAILED\n");
            return;
        }
        kfree(phase13_image);

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 13 persistent storage online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Storage controller: PCI AHCI (q35 SATA)\n");
        kprintf("Block device: %s (512-byte sectors)\n", disk->name);
        kprintf(
            "Phase 13 disk sectors: %llu\n",
            (unsigned long long)disk->sector_count
        );
        kprintf("VFS disk mount: diskfs mounted at /disk\n");
        kprintf(
            "Phase 13 disk freshly formatted: %s\n",
            disk_info.freshly_formatted ? "yes" : "no"
        );
        kprintf(
            "Phase 13 persistent file before user: %s\n",
            persistent_before ? "present" : "absent"
        );
        kprintf(
            "Phase 13 disk task ID: %llu\n",
            (unsigned long long)phase13_task_id
        );

        interrupts_enable();

        {
            const uint64_t start = timer_ticks();

            while ((timer_ticks() - start) < PHASE13_TEST_TIMEOUT_TICKS) {
                const struct task *task = scheduler_task_by_id(phase13_task_id);
                if (task != 0 && task->state == TASK_TERMINATED) {
                    break;
                }
                __asm__ volatile ("hlt" ::: "memory");
            }
        }

        interrupts_disable();

        {
            const struct task *task = scheduler_task_by_id(phase13_task_id);
            struct block_stats block_stats;
            struct diskfs_info final_disk_info;
            int persistent_after = 0;

            if (task == 0 || task->state != TASK_TERMINATED ||
                task->exit_code != 13 ||
                !phase13_persistent_file_matches(&persistent_after) ||
                !persistent_after) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 13 disk round-trip: FAILED\n");
                return;
            }

            if (!block_flush(disk)) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 13 disk flush: FAILED\n");
                return;
            }

            block_stats = block_get_stats();
            final_disk_info = diskfs_get_info();

            kprintf("Phase 13 disk round-trip: OK\n");
            kprintf(
                "Phase 13 diskfs files: %u\n",
                (unsigned)final_disk_info.file_count
            );
            kprintf(
                "Phase 13 block sectors read: %llu\n",
                (unsigned long long)block_stats.sectors_read
            );
            kprintf(
                "Phase 13 block sectors written: %llu\n",
                (unsigned long long)block_stats.sectors_written
            );
            kprintf(
                "Phase 13 block flushes: %llu\n",
                (unsigned long long)block_stats.flush_commands
            );
        }

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREEN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 13 persistent storage complete.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
    } else {
        kprintf("Phase 13 block device: not present; persistent demo skipped.\n");
    }

    interrupts_enable();

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("AxiomOS Phase 15 process management online.\n");
    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Process table capacity: %u tasks\n", (unsigned)SCHEDULER_MAX_TASKS);
    kprintf("fork policy: eager private-page copy\n");
    kprintf("wait policy: blocking parent + zombie reap\n");
    kprintf("signal policy: SIGTERM immediate termination\n");

    /*
     * Phase 16: synchronization primitives. First demonstrate a lost-update
     * race on purpose, then prove spinlocks, sleeping mutexes, semaphores, and
     * FIFO wait queues repair/control the same kind of concurrency.
     */
    {
        struct sync_selftest_result sync_result;

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 16 synchronization online.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Synchronization CPU model: BSP only; APs remain parked until Phase 17\n");
        kprintf("Spinlock policy: IRQ-save busy lock for short critical sections\n");
        kprintf("Mutex policy: blocking FIFO handoff\n");
        kprintf("Semaphore policy: counting tokens + blocking FIFO waiters\n");

        if (!sync_run_selftest(&sync_result)) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf(
                "Phase 16 synchronization failure stage: %llu\n",
                (unsigned long long)sync_result.failure_stage
            );
            if (sync_result.failure_stage == SYNC_SELFTEST_STAGE_MUTEX) {
                kprintf(
                    "Phase 16 failure mutex expected/actual: %llu/%llu\n",
                    (unsigned long long)sync_result.mutex_expected,
                    (unsigned long long)sync_result.mutex_actual
                );
                kprintf(
                    "Phase 16 failure mutex blocks/wakeups: %llu/%llu\n",
                    (unsigned long long)sync_result.mutex_blocks,
                    (unsigned long long)sync_result.mutex_wakeups
                );
            }
            kprintf("Phase 16 synchronization self-test: FAILED\n");
            return;
        }

        kprintf(
            "Phase 16 race expected/observed: %llu/%llu\n",
            (unsigned long long)sync_result.race_expected,
            (unsigned long long)sync_result.race_actual
        );
        kprintf("Phase 16 race condition demonstrated: OK\n");
        kprintf(
            "Phase 16 spinlock counter expected/actual: %llu/%llu\n",
            (unsigned long long)sync_result.spin_expected,
            (unsigned long long)sync_result.spin_actual
        );
        kprintf(
            "Phase 16 spinlock acquisitions: %llu\n",
            (unsigned long long)sync_result.spin_acquisitions
        );
        kprintf("Phase 16 spinlock protected counter: OK\n");
        kprintf(
            "Phase 16 mutex counter expected/actual: %llu/%llu\n",
            (unsigned long long)sync_result.mutex_expected,
            (unsigned long long)sync_result.mutex_actual
        );
        kprintf(
            "Phase 16 mutex blocks/wakeups: %llu/%llu\n",
            (unsigned long long)sync_result.mutex_blocks,
            (unsigned long long)sync_result.mutex_wakeups
        );
        kprintf("Phase 16 mutex protected counter: OK\n");
        kprintf(
            "Phase 16 semaphore limit/peak: %llu/%llu\n",
            (unsigned long long)sync_result.semaphore_limit,
            (unsigned long long)sync_result.semaphore_peak
        );
        kprintf(
            "Phase 16 semaphore blocks/wakeups: %llu/%llu\n",
            (unsigned long long)sync_result.semaphore_blocks,
            (unsigned long long)sync_result.semaphore_wakeups
        );
        kprintf("Phase 16 semaphore limit: OK\n");
        kprintf("Phase 16 wait queue wakeups: OK\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREEN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 16 synchronization self-test: OK\n");
        kprintf("Phase 16 synchronization complete.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
    }

    /*
     * Phase 17: release Limine-parked application processors.  APs switch to
     * the AxiomOS kernel CR3, private kernel stacks, and private TSS instances.
     * The general scheduler deliberately remains BSP-only in this foundation
     * phase; APs run a parallel spinlock proof and then park with IF=0.
     */
    {
        const struct smp_stats *smp;
        uint64_t cpu_index;

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 17 SMP / multicore online.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        if (!smp_init()) {
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 17 SMP initialization/self-test: FAILED\n");
            return;
        }

        smp = smp_get_stats();
        kprintf(
            "Phase 17 CPUs detected/managed/online: %llu/%llu/%llu\n",
            (unsigned long long)smp->detected_cpus,
            (unsigned long long)smp->managed_cpus,
            (unsigned long long)smp->online_cpus
        );
        kprintf(
            "Phase 17 APs released/completed: %llu/%llu\n",
            (unsigned long long)smp->aps_released,
            (unsigned long long)smp->aps_completed
        );

        for (cpu_index = 0u; cpu_index < smp->managed_cpus; ++cpu_index) {
            const struct smp_cpu *cpu = smp_cpu_at(cpu_index);

            if (cpu == 0) {
                continue;
            }

            kprintf(
                "Phase 17 CPU %llu: processor=%u LAPIC=%u role=%s online=%u\n",
                (unsigned long long)cpu->logical_index,
                (unsigned)cpu->processor_id,
                (unsigned)cpu->lapic_id,
                cpu->role == SMP_CPU_BSP ? "BSP" : "AP",
                (unsigned)cpu->online
            );
        }

        if (smp->multicore_test_ran) {
            kprintf(
                "Phase 17 parallel participant mask: 0x%llX\n",
                (unsigned long long)smp->participant_mask
            );
            kprintf(
                "Phase 17 locked counter expected/actual: %llu/%llu\n",
                (unsigned long long)smp->expected_locked_count,
                (unsigned long long)smp->actual_locked_count
            );
            kprintf(
                "Phase 17 cross-CPU spinlock acquisitions/contentions: %llu/%llu\n",
                (unsigned long long)smp->spinlock_acquisitions,
                (unsigned long long)smp->spinlock_contentions
            );

            if (!smp->multicore_test_passed) {
                terminal_set_color(
                    TERMINAL_COLOR_LIGHT_RED,
                    TERMINAL_COLOR_BLACK
                );
                kprintf("Phase 17 multicore parallel-work test: FAILED\n");
                return;
            }

            kprintf("Phase 17 AP bring-up: OK\n");
            kprintf("Phase 17 cross-CPU spinlock: OK\n");
            kprintf("Phase 17 multicore parallel-work test: OK\n");
        } else {
            kprintf(
                "Phase 17 multicore self-test: SKIPPED (single CPU boot)\n"
            );
        }

        kprintf("Phase 17 scheduler policy: BSP-only; APs parked after self-test\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREEN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Phase 17 SMP initialization complete.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
    }

    /*
     * Phase 18: initialise the first real network interface and protocol stack.
     * Networking is optional for older regression boots: if no supported E1000
     * NIC is attached, the shell still launches and network syscalls return
     * ENETDOWN. Normal `make run` and test-phase18 attach an E1000 explicitly.
     */
    {
        const struct net_config *network_config;

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 18 networking online.\n");
        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );

        if (!net_init()) {
            kprintf("Phase 18 E1000 NIC: not present; networking unavailable.\n");
            kprintf("Phase 18 networking initialization: SKIPPED\n");
        } else {
            network_config = net_get_config();
            kprintf("Network device: Intel E1000 82540EM (PCI/MMIO, polled DMA)\n");
            kprintf(
                "Phase 18 MAC: %X:%X:%X:%X:%X:%X\n",
                (unsigned)network_config->mac[0],
                (unsigned)network_config->mac[1],
                (unsigned)network_config->mac[2],
                (unsigned)network_config->mac[3],
                (unsigned)network_config->mac[4],
                (unsigned)network_config->mac[5]
            );
            kprintf(
                "Phase 18 IPv4: %u.%u.%u.%u/24 gateway %u.%u.%u.%u DNS %u.%u.%u.%u\n",
                (unsigned)((network_config->address >> 24) & 0xFFu),
                (unsigned)((network_config->address >> 16) & 0xFFu),
                (unsigned)((network_config->address >> 8) & 0xFFu),
                (unsigned)(network_config->address & 0xFFu),
                (unsigned)((network_config->gateway >> 24) & 0xFFu),
                (unsigned)((network_config->gateway >> 16) & 0xFFu),
                (unsigned)((network_config->gateway >> 8) & 0xFFu),
                (unsigned)(network_config->gateway & 0xFFu),
                (unsigned)((network_config->dns_server >> 24) & 0xFFu),
                (unsigned)((network_config->dns_server >> 16) & 0xFFu),
                (unsigned)((network_config->dns_server >> 8) & 0xFFu),
                (unsigned)(network_config->dns_server & 0xFFu)
            );
            kprintf("Phase 18 stack: Ethernet -> ARP -> IPv4 -> ICMP/UDP/TCP -> DNS/HTTP\n");
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREEN,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 18 networking initialization complete.\n");
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_GREY,
                TERMINAL_COLOR_BLACK
            );
        }
    }

    /*
     * Phase 14: launch the interactive command shell as a normal Ring-3 ELF.
     * The shell consumes keyboard input through SYS_read, uses the VFS through
     * file syscalls, and launches other ELF programs through spawn/waitpid.
     */
    {
        void *shell_image = 0;
        size_t shell_image_size = 0u;
        uint64_t shell_task_id = 0u;

        interrupts_disable();

        if (vfs_read_all(
                "/bin/axiomsh",
                &shell_image,
                &shell_image_size
            ) < 0 ||
            !user_task_create_elf(
                "axiomsh",
                shell_image,
                shell_image_size,
                &shell_task_id
            )) {
            if (shell_image != 0) {
                kfree(shell_image);
            }
            terminal_set_color(
                TERMINAL_COLOR_LIGHT_RED,
                TERMINAL_COLOR_BLACK
            );
            kprintf("Phase 14 shell task creation: FAILED\n");
            return;
        }

        kfree(shell_image);

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_CYAN,
            TERMINAL_COLOR_BLACK
        );
        kprintf("AxiomOS Phase 14 interactive shell online.\n");

        terminal_set_color(
            TERMINAL_COLOR_LIGHT_GREY,
            TERMINAL_COLOR_BLACK
        );
        kprintf("Shell executable: /bin/axiomsh\n");
        kprintf("Shell privilege: Ring 3\n");
        kprintf(
            "Phase 14 shell task ID: %llu\n",
            (unsigned long long)shell_task_id
        );
        kprintf("Phase 14 shell launch complete.\n");

        interrupts_enable();
    }

    /* Bootstrap task becomes an idle host while the Ring-3 shell runs. */
    for (;;) {
        __asm__ volatile ("hlt" ::: "memory");
    }
}
```

## `kernel/syscall/syscall.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/syscall.h>
#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/drivers/keyboard.h>
#include <axiom/drivers/timer.h>
#include <axiom/filesystem/vfs.h>
#include <axiom/kernel/panic.h>
#include <axiom/kernel/syscall.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/vmm.h>
#include <axiom/network/net.h>
#include <axiom/process/scheduler.h>
#include <axiom/terminal/terminal.h>

#define IA32_EFER  0xC0000080u
#define IA32_STAR  0xC0000081u
#define IA32_LSTAR 0xC0000082u
#define IA32_FMASK 0xC0000084u

#define EFER_SCE (1ULL << 0)
#define RFLAGS_TF (1ULL << 8)
#define RFLAGS_IF (1ULL << 9)
#define RFLAGS_DF (1ULL << 10)

#define SYSCALL_WRITE_MAX 4096u
#define SYSCALL_READ_MAX  4096u
#define SYSCALL_SLEEP_MAX_MS 60000ULL
#define SYSCALL_HTTP_MAX AXIOM_NET_HTTP_USER_MAX

static uint8_t syscall_http_buffer[AXIOM_NET_HTTP_USER_MAX];

/* Assembly reads this immediately after SYSCALL while IF is masked. */
uintptr_t syscall_kernel_stack_top;

static struct syscall_stats stats;
static int initialized;

extern void syscall_entry(void);

static uint64_t rdmsr(uint32_t msr)
{
    uint32_t low;
    uint32_t high;

    __asm__ volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );

    return ((uint64_t)high << 32) | low;
}

static void wrmsr(uint32_t msr, uint64_t value)
{
    __asm__ volatile (
        "wrmsr"
        :
        : "c"(msr),
          "a"((uint32_t)value),
          "d"((uint32_t)(value >> 32))
        : "memory"
    );
}

static int cpu_supports_syscall(void)
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    eax = 0x80000000u;
    __asm__ volatile (
        "cpuid"
        : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
    );

    if (eax < 0x80000001u) {
        return 0;
    }

    eax = 0x80000001u;
    __asm__ volatile (
        "cpuid"
        : "+a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
    );

    return (edx & (1u << 11)) != 0u;
}

static int64_t syscall_error(int error)
{
    return -(int64_t)error;
}

static struct task *current_user_task(void)
{
    struct task *task = scheduler_current_task_mutable();

    if (task == 0 || task->privilege != TASK_PRIVILEGE_USER ||
        task->address_space == PADDR_INVALID) {
        return 0;
    }

    return task;
}

static int copy_user_path(
    const struct task *task,
    vaddr_t user_path,
    char *destination,
    size_t capacity
)
{
    size_t index;

    if (task == 0 || destination == 0 || capacity < 2u) {
        return -AXIOM_EINVAL;
    }

    for (index = 0u; index + 1u < capacity; ++index) {
        if (!vmm_copy_from_user(
                task->address_space,
                &destination[index],
                user_path + (vaddr_t)index,
                1u
            )) {
            return -AXIOM_EFAULT;
        }

        if (destination[index] == '\0') {
            return 0;
        }
    }

    destination[capacity - 1u] = '\0';
    return -AXIOM_ENAMETOOLONG;
}

static int64_t sys_write(uint64_t fd, vaddr_t buffer, uint64_t count)
{
    struct task *task = current_user_task();
    struct vfs_file *file = 0;
    uint64_t transferred = 0u;
    uint8_t local[128];

    ++stats.write_calls;

    if (count > SYSCALL_WRITE_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (count == 0u) {
        return 0;
    }
    if (task == 0 ||
        !vmm_user_range_accessible(task->address_space, buffer, (size_t)count, 0)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    if (fd != 1u && fd != 2u) {
        if (fd > (uint64_t)INT32_MAX) {
            return syscall_error(AXIOM_EBADF);
        }
        file = task_file(task, (int)fd);
        if (file == 0) {
            return syscall_error(AXIOM_EBADF);
        }
    }

    while (transferred < count) {
        const size_t chunk =
            (count - transferred < sizeof(local)) ?
            (size_t)(count - transferred) : sizeof(local);

        if (!vmm_copy_from_user(
                task->address_space,
                local,
                buffer + transferred,
                chunk
            )) {
            ++stats.rejected_pointers;
            return transferred != 0u ?
                (int64_t)transferred : syscall_error(AXIOM_EFAULT);
        }

        if (file == 0) {
            size_t index;

            for (index = 0u; index < chunk; ++index) {
                terminal_putchar((char)local[index]);
            }
            transferred += chunk;
        } else {
            const int64_t written = vfs_write(file, local, chunk);

            if (written < 0) {
                return transferred != 0u ? (int64_t)transferred : written;
            }
            if (written == 0) {
                break;
            }
            transferred += (uint64_t)written;
            if ((size_t)written < chunk) {
                break;
            }
        }
    }

    stats.bytes_written += transferred;
    return (int64_t)transferred;
}

static int64_t sys_read(uint64_t fd, vaddr_t buffer, uint64_t count)
{
    struct task *task = current_user_task();
    struct vfs_file *file = 0;
    uint64_t transferred = 0u;
    uint8_t local[128];

    ++stats.read_calls;

    if (count > SYSCALL_READ_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (count == 0u) {
        return 0;
    }
    if (task == 0 ||
        !vmm_user_range_accessible(task->address_space, buffer, (size_t)count, 1)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    if (fd != 0u) {
        if (fd > (uint64_t)INT32_MAX) {
            return syscall_error(AXIOM_EBADF);
        }
        file = task_file(task, (int)fd);
        if (file == 0) {
            return syscall_error(AXIOM_EBADF);
        }
    }

    if (file == 0) {
        while (transferred < count) {
            char character;

            if (!keyboard_read_char(&character)) {
                break;
            }

            if (!vmm_copy_to_user(
                    task->address_space,
                    buffer + transferred,
                    &character,
                    1u
                )) {
                ++stats.rejected_pointers;
                return transferred != 0u ?
                    (int64_t)transferred : syscall_error(AXIOM_EFAULT);
            }
            ++transferred;
        }
    } else {
        while (transferred < count) {
            const size_t chunk =
                (count - transferred < sizeof(local)) ?
                (size_t)(count - transferred) : sizeof(local);
            const int64_t got = vfs_read(file, local, chunk);

            if (got < 0) {
                return transferred != 0u ? (int64_t)transferred : got;
            }
            if (got == 0) {
                break;
            }

            if (!vmm_copy_to_user(
                    task->address_space,
                    buffer + transferred,
                    local,
                    (size_t)got
                )) {
                ++stats.rejected_pointers;
                return transferred != 0u ?
                    (int64_t)transferred : syscall_error(AXIOM_EFAULT);
            }

            transferred += (uint64_t)got;
            if ((size_t)got < chunk) {
                break;
            }
        }
    }

    stats.bytes_read += transferred;
    return (int64_t)transferred;
}

static int64_t sys_open(vaddr_t user_path, uint64_t flags)
{
    struct task *task = current_user_task();
    struct vfs_file *file = 0;
    char path[VFS_PATH_MAX + 1u];
    int copied;
    int result;
    int fd;

    ++stats.open_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || flags > UINT32_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    result = vfs_open(path, (uint32_t)flags, &file);
    if (result < 0) {
        return result;
    }

    fd = task_install_file(task, file);
    if (fd < 0) {
        (void)vfs_close(file);
        return syscall_error(AXIOM_EMFILE);
    }

    return fd;
}

static int64_t sys_close(uint64_t fd)
{
    struct task *task = current_user_task();
    struct vfs_file *file;

    ++stats.close_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || fd > (uint64_t)INT32_MAX) {
        return syscall_error(AXIOM_EBADF);
    }

    file = task_take_file(task, (int)fd);
    if (file == 0) {
        return syscall_error(AXIOM_EBADF);
    }

    return vfs_close(file);
}

static int64_t sys_fork(struct syscall_frame *frame)
{
    uint64_t child_id = 0u;

    ++stats.fork_calls;

    if (frame == 0 || current_user_task() == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (!scheduler_fork_current(frame, &child_id)) {
        return syscall_error(AXIOM_ENOMEM);
    }

    return (int64_t)child_id;
}

static int64_t sys_exec(vaddr_t user_path, struct syscall_frame *frame)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    void *image = 0;
    size_t image_size = 0u;
    vaddr_t entry;
    vaddr_t stack;
    int copied;
    int result;

    ++stats.exec_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || frame == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    result = vfs_read_all(path, &image, &image_size);
    if (result < 0) {
        return result;
    }

    if (!scheduler_exec_current_elf(
            image,
            image_size,
            &entry,
            &stack
        )) {
        kfree(image);
        return syscall_error(AXIOM_EINVAL);
    }
    kfree(image);

    /* exec() starts the new image with a clean user register set. */
    frame->r15 = 0u;
    frame->r14 = 0u;
    frame->r13 = 0u;
    frame->r12 = 0u;
    frame->r10 = 0u;
    frame->r9 = 0u;
    frame->r8 = 0u;
    frame->rbp = 0u;
    frame->rdi = 0u;
    frame->rsi = 0u;
    frame->rdx = 0u;
    frame->rbx = 0u;
    frame->user_rip = entry;
    frame->user_rflags = 0x202ULL;
    frame->user_rsp = stack;

    ++stats.exec_successes;
    return 0;
}

static int64_t sys_lseek(uint64_t fd, int64_t offset, uint64_t whence)
{
    struct task *task = current_user_task();
    struct vfs_file *file;

    ++stats.lseek_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || fd > (uint64_t)INT32_MAX || whence > INT32_MAX) {
        return syscall_error(AXIOM_EBADF);
    }

    file = task_file(task, (int)fd);
    if (file == 0) {
        return syscall_error(AXIOM_EBADF);
    }

    return vfs_seek(file, offset, (int)whence);
}

static int64_t sys_stat(vaddr_t user_path, vaddr_t user_stat)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    struct axiom_stat status;
    int copied;
    int result;

    ++stats.stat_calls;

    if (!vfs_initialized()) {
        ++stats.unimplemented_calls;
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    if (!vmm_user_range_accessible(
            task->address_space,
            user_stat,
            sizeof(status),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = vfs_stat(path, &status);
    if (result < 0) {
        return result;
    }

    if (!vmm_copy_to_user(
            task->address_space,
            user_stat,
            &status,
            sizeof(status)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 0;
}


static int64_t sys_readdir(vaddr_t user_path, uint64_t index, vaddr_t user_entry)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    struct vfs_dirent entry;
    struct axiom_dirent user_value;
    int copied;
    int result;

    ++stats.readdir_calls;

    if (!vfs_initialized()) {
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0 || index > (uint64_t)SIZE_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    if (!vmm_user_range_accessible(
            task->address_space,
            user_entry,
            sizeof(user_value),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = vfs_readdir(path, (size_t)index, &entry);
    if (result <= 0) {
        return result;
    }

    for (size_t i = 0u; i < sizeof(user_value.name); ++i) {
        user_value.name[i] = entry.name[i];
        if (entry.name[i] == '\0') {
            for (++i; i < sizeof(user_value.name); ++i) {
                user_value.name[i] = '\0';
            }
            break;
        }
    }
    user_value.type = entry.type;

    if (!vmm_copy_to_user(
            task->address_space,
            user_entry,
            &user_value,
            sizeof(user_value)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 1;
}

static int64_t sys_mkdir(vaddr_t user_path)
{
    struct task *task = current_user_task();
    char path[VFS_PATH_MAX + 1u];
    int copied;

    ++stats.mkdir_calls;

    if (!vfs_initialized()) {
        return syscall_error(AXIOM_ENOSYS);
    }
    if (task == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    return vfs_mkdir(path);
}

static int64_t sys_spawn(vaddr_t user_path)
{
    struct task *parent = current_user_task();
    struct task *child;
    char path[VFS_PATH_MAX + 1u];
    void *image = 0;
    size_t image_size = 0u;
    uint64_t child_id;
    int copied;
    int result;

    ++stats.spawn_calls;

    if (!vfs_initialized()) {
        return syscall_error(AXIOM_ENOSYS);
    }
    if (parent == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    copied = copy_user_path(parent, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) {
            ++stats.rejected_pointers;
        }
        return copied;
    }

    result = vfs_read_all(path, &image, &image_size);
    if (result < 0) {
        return result;
    }

    if (!user_task_create_elf("spawned-elf", image, image_size, &child_id)) {
        kfree(image);
        return syscall_error(AXIOM_EINVAL);
    }
    kfree(image);

    child = scheduler_task_by_id_mutable(child_id);
    if (child == 0) {
        return syscall_error(AXIOM_EINVAL);
    }
    child->parent_id = parent->id;
    return (int64_t)child_id;
}

static int64_t sys_waitpid(uint64_t pid, vaddr_t user_status)
{
    struct task *parent = current_user_task();
    int64_t status;

    ++stats.waitpid_calls;

    if (parent == 0) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (user_status != 0u &&
        !vmm_user_range_accessible(
            parent->address_space,
            user_status,
            sizeof(status),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    if (!scheduler_wait_current_child(pid, &status)) {
        return syscall_error(AXIOM_ECHILD);
    }

    if (user_status != 0u &&
        !vmm_copy_to_user(
            parent->address_space,
            user_status,
            &status,
            sizeof(status)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return (int64_t)pid;
}

static int64_t sys_clear(void)
{
    ++stats.clear_calls;
    terminal_clear();
    return 0;
}

static int64_t sys_kbdstats(vaddr_t user_stats)
{
    struct task *task = current_user_task();
    const struct keyboard_stats source = keyboard_get_stats();
    struct axiom_keyboard_stats destination;

    ++stats.kbdstats_calls;

    if (task == 0 ||
        !vmm_user_range_accessible(
            task->address_space,
            user_stats,
            sizeof(destination),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    destination.irq_count = source.irq_count;
    destination.scancode_count = source.scancode_count;
    destination.character_count = source.character_count;
    destination.dropped_characters = source.dropped_characters;

    if (!vmm_copy_to_user(
            task->address_space,
            user_stats,
            &destination,
            sizeof(destination)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 0;
}

static int64_t sys_procinfo(uint64_t index, vaddr_t user_info)
{
    struct task *caller = current_user_task();
    const struct task *task;
    struct axiom_process_info info;
    size_t character;

    ++stats.procinfo_calls;

    if (caller == 0 || index > (uint64_t)SIZE_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!vmm_user_range_accessible(
            caller->address_space,
            user_info,
            sizeof(info),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    task = scheduler_task_at((size_t)index);
    if (task == 0) {
        return 0;
    }

    info.pid = task->id;
    info.ppid = task->parent_id;
    info.runtime_ticks = task->runtime_ticks;
    info.exit_status = task->exit_code;
    info.state = (uint32_t)task->state;
    info.privilege = (uint32_t)task->privilege;
    info.termination_signal = task->termination_signal;
    info.reserved = 0u;

    for (character = 0u; character + 1u < sizeof(info.name); ++character) {
        if (task->name == 0 || task->name[character] == '\0') {
            break;
        }
        info.name[character] = task->name[character];
    }
    info.name[character++] = '\0';
    while (character < sizeof(info.name)) {
        info.name[character++] = '\0';
    }

    if (!vmm_copy_to_user(
            caller->address_space,
            user_info,
            &info,
            sizeof(info)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    return 1;
}

static int64_t sys_kill(uint64_t pid, uint64_t signal_number)
{
    struct task *caller = current_user_task();
    const struct task *target;

    ++stats.kill_calls;

    if (caller == 0 || signal_number != AXIOM_SIGTERM) {
        return syscall_error(AXIOM_EINVAL);
    }

    target = scheduler_task_by_id(pid);
    if (target == 0 || target->state == TASK_TERMINATED) {
        return syscall_error(AXIOM_ESRCH);
    }
    if (target->privilege != TASK_PRIVILEGE_USER) {
        return syscall_error(AXIOM_EPERM);
    }

    if (!scheduler_terminate_task(pid, (uint32_t)signal_number)) {
        return syscall_error(AXIOM_ESRCH);
    }
    return 0;
}

static int64_t sys_getppid(void)
{
    const struct task *task = current_user_task();

    ++stats.getppid_calls;
    if (task == 0) {
        return syscall_error(AXIOM_EINVAL);
    }
    return task->parent_id == UINT64_MAX ? 0 : (int64_t)task->parent_id;
}


static int64_t sys_netinfo(vaddr_t user_info)
{
    struct task *task = current_user_task();
    struct axiom_net_info info;

    ++stats.netinfo_calls;
    if (task == 0) return syscall_error(AXIOM_EINVAL);
    if (!net_available()) return syscall_error(AXIOM_ENETDOWN);
    if (!vmm_user_range_accessible(
            task->address_space,
            user_info,
            sizeof(info),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    net_get_abi_info(&info);
    if (!vmm_copy_to_user(task->address_space, user_info, &info, sizeof(info))) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_ping(vaddr_t user_target, vaddr_t user_result)
{
    struct task *task = current_user_task();
    struct axiom_ping_result result_info;
    char target[AXIOM_NET_HOST_MAX];
    int copied;
    int result;

    ++stats.ping_calls;
    if (task == 0) return syscall_error(AXIOM_EINVAL);
    if (!net_available()) return syscall_error(AXIOM_ENETDOWN);

    copied = copy_user_path(task, user_target, target, sizeof(target));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) ++stats.rejected_pointers;
        return copied;
    }
    if (!vmm_user_range_accessible(
            task->address_space,
            user_result,
            sizeof(result_info),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = net_ping(target, &result_info);
    if (result < 0) return result;
    if (!vmm_copy_to_user(
            task->address_space,
            user_result,
            &result_info,
            sizeof(result_info)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_dns(vaddr_t user_name, vaddr_t user_address)
{
    struct task *task = current_user_task();
    char name[AXIOM_NET_HOST_MAX];
    uint32_t address;
    int copied;
    int result;

    ++stats.dns_calls;
    if (task == 0) return syscall_error(AXIOM_EINVAL);
    if (!net_available()) return syscall_error(AXIOM_ENETDOWN);

    copied = copy_user_path(task, user_name, name, sizeof(name));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) ++stats.rejected_pointers;
        return copied;
    }
    if (!vmm_user_range_accessible(task->address_space, user_address, sizeof(address), 1)) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = net_resolve(name, &address);
    if (result < 0) return result;
    if (!vmm_copy_to_user(task->address_space, user_address, &address, sizeof(address))) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_httpget(
    vaddr_t user_host,
    vaddr_t user_path,
    vaddr_t user_buffer,
    uint64_t capacity,
    vaddr_t user_result
)
{
    struct task *task = current_user_task();
    struct axiom_http_result result_info;
    char host[AXIOM_NET_HOST_MAX];
    char path[AXIOM_NET_PATH_MAX];
    int copied;
    int result;

    ++stats.httpget_calls;
    if (task == 0 || capacity == 0u || capacity > SYSCALL_HTTP_MAX) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!net_available()) return syscall_error(AXIOM_ENETDOWN);

    copied = copy_user_path(task, user_host, host, sizeof(host));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) ++stats.rejected_pointers;
        return copied;
    }
    copied = copy_user_path(task, user_path, path, sizeof(path));
    if (copied < 0) {
        if (copied == -AXIOM_EFAULT) ++stats.rejected_pointers;
        return copied;
    }

    if (!vmm_user_range_accessible(
            task->address_space,
            user_buffer,
            (size_t)capacity,
            1
        ) ||
        !vmm_user_range_accessible(
            task->address_space,
            user_result,
            sizeof(result_info),
            1
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }

    result = net_http_get(
        host,
        path,
        syscall_http_buffer,
        (size_t)capacity,
        &result_info
    );
    if (result < 0) return result;

    if (result_info.body_bytes != 0u &&
        !vmm_copy_to_user(
            task->address_space,
            user_buffer,
            syscall_http_buffer,
            (size_t)result_info.body_bytes
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    if (!vmm_copy_to_user(
            task->address_space,
            user_result,
            &result_info,
            sizeof(result_info)
        )) {
        ++stats.rejected_pointers;
        return syscall_error(AXIOM_EFAULT);
    }
    return 0;
}

static int64_t sys_sleep(uint64_t milliseconds)
{
    uint64_t ticks;
    const uint64_t frequency = timer_frequency();

    ++stats.sleep_calls;

    if (milliseconds > SYSCALL_SLEEP_MAX_MS || frequency == 0u) {
        return syscall_error(AXIOM_EINVAL);
    }

    if (milliseconds == 0u) {
        return 0;
    }

    if (milliseconds > (UINT64_MAX - 999ULL) / frequency) {
        return syscall_error(AXIOM_EINVAL);
    }

    ticks = (milliseconds * frequency + 999ULL) / 1000ULL;
    if (ticks == 0u) {
        ticks = 1u;
    }

    return scheduler_sleep_current(ticks) ? 0 : syscall_error(AXIOM_EINVAL);
}

static int64_t sys_getpid(void)
{
    const struct task *task = current_user_task();

    ++stats.getpid_calls;
    return task != 0 ? (int64_t)task->id : syscall_error(AXIOM_EINVAL);
}

static int64_t sys_yield(void)
{
    ++stats.yield_calls;
    return scheduler_yield_current() ? 0 : syscall_error(AXIOM_EINVAL);
}

int syscall_init(void)
{
    uint64_t efer;
    uint64_t star;

    if (initialized || interrupts_enabled() || !cpu_supports_syscall()) {
        return 0;
    }

    /*
     * SYSCALL: CS=0x08, SS=0x10.
     * SYSRET:  STAR.user+16 => 0x23 user code, STAR.user+8 => 0x1B user data.
     */
    star = ((uint64_t)0x10u << 48) | ((uint64_t)GDT_KERNEL_CODE << 32);

    efer = rdmsr(IA32_EFER);
    wrmsr(IA32_EFER, efer | EFER_SCE);
    wrmsr(IA32_STAR, star);
    wrmsr(IA32_LSTAR, (uint64_t)(uintptr_t)&syscall_entry);
    wrmsr(IA32_FMASK, RFLAGS_TF | RFLAGS_IF | RFLAGS_DF);

    syscall_kernel_stack_top = gdt_kernel_stack();

    stats.total_calls = 0u;
    stats.write_calls = 0u;
    stats.read_calls = 0u;
    stats.exit_calls = 0u;
    stats.sleep_calls = 0u;
    stats.getpid_calls = 0u;
    stats.yield_calls = 0u;
    stats.open_calls = 0u;
    stats.close_calls = 0u;
    stats.fork_calls = 0u;
    stats.exec_calls = 0u;
    stats.exec_successes = 0u;
    stats.lseek_calls = 0u;
    stats.stat_calls = 0u;
    stats.readdir_calls = 0u;
    stats.mkdir_calls = 0u;
    stats.spawn_calls = 0u;
    stats.waitpid_calls = 0u;
    stats.clear_calls = 0u;
    stats.kbdstats_calls = 0u;
    stats.procinfo_calls = 0u;
    stats.kill_calls = 0u;
    stats.getppid_calls = 0u;
    stats.netinfo_calls = 0u;
    stats.ping_calls = 0u;
    stats.dns_calls = 0u;
    stats.httpget_calls = 0u;
    stats.rejected_pointers = 0u;
    stats.unimplemented_calls = 0u;
    stats.bytes_written = 0u;
    stats.bytes_read = 0u;
    initialized = 1;
    return 1;
}

int syscall_initialized(void)
{
    return initialized;
}

void syscall_set_kernel_stack(uintptr_t stack_top)
{
    syscall_kernel_stack_top = stack_top;
}

void syscall_dispatch(struct syscall_frame *frame)
{
    int64_t result;

    if (!initialized || frame == 0) {
        return;
    }

    ++stats.total_calls;

    if (current_user_task() == 0) {
        kernel_panic("SYSCALL entered without a Ring-3 current task");
    }

    /* Never let SYSRETQ carry a malicious non-canonical user stack to CPL3. */
    if (frame->user_rip >= 0x0000800000000000ULL ||
        frame->user_rsp >= 0x0000800000000000ULL) {
        ++stats.rejected_pointers;
        task_exit_current(-(int64_t)AXIOM_EFAULT);
    }

    switch (frame->rax) {
        case AXIOM_SYS_WRITE:
            result = sys_write(frame->rdi, (vaddr_t)frame->rsi, frame->rdx);
            break;

        case AXIOM_SYS_READ:
            result = sys_read(frame->rdi, (vaddr_t)frame->rsi, frame->rdx);
            break;

        case AXIOM_SYS_EXIT:
            ++stats.exit_calls;
            task_exit_current((int64_t)frame->rdi);

        case AXIOM_SYS_SLEEP:
            result = sys_sleep(frame->rdi);
            break;

        case AXIOM_SYS_GETPID:
            result = sys_getpid();
            break;

        case AXIOM_SYS_YIELD:
            result = sys_yield();
            break;

        case AXIOM_SYS_OPEN:
            result = sys_open((vaddr_t)frame->rdi, frame->rsi);
            break;

        case AXIOM_SYS_CLOSE:
            result = sys_close(frame->rdi);
            break;

        case AXIOM_SYS_FORK:
            result = sys_fork(frame);
            break;

        case AXIOM_SYS_EXEC:
            result = sys_exec((vaddr_t)frame->rdi, frame);
            break;

        case AXIOM_SYS_LSEEK:
            result = sys_lseek(
                frame->rdi,
                (int64_t)frame->rsi,
                frame->rdx
            );
            break;

        case AXIOM_SYS_STAT:
            result = sys_stat((vaddr_t)frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_READDIR:
            result = sys_readdir(
                (vaddr_t)frame->rdi,
                frame->rsi,
                (vaddr_t)frame->rdx
            );
            break;

        case AXIOM_SYS_MKDIR:
            result = sys_mkdir((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_SPAWN:
            result = sys_spawn((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_WAITPID:
            result = sys_waitpid(frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_CLEAR:
            result = sys_clear();
            break;

        case AXIOM_SYS_KBDSTATS:
            result = sys_kbdstats((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_PROCINFO:
            result = sys_procinfo(frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_KILL:
            result = sys_kill(frame->rdi, frame->rsi);
            break;

        case AXIOM_SYS_GETPPID:
            result = sys_getppid();
            break;

        case AXIOM_SYS_NETINFO:
            result = sys_netinfo((vaddr_t)frame->rdi);
            break;

        case AXIOM_SYS_PING:
            result = sys_ping((vaddr_t)frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_DNS:
            result = sys_dns((vaddr_t)frame->rdi, (vaddr_t)frame->rsi);
            break;

        case AXIOM_SYS_HTTPGET:
            result = sys_httpget(
                (vaddr_t)frame->rdi,
                (vaddr_t)frame->rsi,
                (vaddr_t)frame->rdx,
                frame->r10,
                (vaddr_t)frame->r8
            );
            break;

        case AXIOM_SYS_MMAP:
        default:
            ++stats.unimplemented_calls;
            result = syscall_error(AXIOM_ENOSYS);
            break;
    }

    frame->rax = (uint64_t)result;
}

struct syscall_stats syscall_get_stats(void)
{
    return stats;
}
```

## `network/arp.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define ARP_PACKET_BYTES 28u
#define ARP_HTYPE_ETHERNET 1u
#define ARP_PTYPE_IPV4 0x0800u
#define ARP_OP_REQUEST 1u
#define ARP_OP_REPLY 2u
#define ARP_CACHE_SIZE 8u

struct arp_entry {
    uint32_t address;
    uint8_t mac[6];
    uint32_t age;
    int valid;
};

static struct arp_entry cache[ARP_CACHE_SIZE];
static uint32_t age_counter;

static void cache_store(uint32_t address, const uint8_t mac[6])
{
    unsigned index;
    unsigned victim = 0u;
    uint32_t oldest = UINT32_MAX;

    if (address == 0u || mac == 0) return;

    for (index = 0u; index < ARP_CACHE_SIZE; ++index) {
        if (cache[index].valid && cache[index].address == address) {
            net_copy(cache[index].mac, mac, 6u);
            cache[index].age = ++age_counter;
            return;
        }
        if (!cache[index].valid) {
            victim = index;
            oldest = 0u;
            break;
        }
        if (cache[index].age < oldest) {
            oldest = cache[index].age;
            victim = index;
        }
    }

    cache[victim].address = address;
    net_copy(cache[victim].mac, mac, 6u);
    cache[victim].age = ++age_counter;
    cache[victim].valid = 1;
}

static int cache_lookup(uint32_t address, uint8_t mac_out[6])
{
    unsigned index;
    for (index = 0u; index < ARP_CACHE_SIZE; ++index) {
        if (cache[index].valid && cache[index].address == address) {
            net_copy(mac_out, cache[index].mac, 6u);
            cache[index].age = ++age_counter;
            return 1;
        }
    }
    return 0;
}

static int send_request(uint32_t target)
{
    uint8_t packet[ARP_PACKET_BYTES];
    static const uint8_t broadcast[6] = {
        0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu
    };
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();

    if (config == 0) return 0;
    net_clear(packet, sizeof(packet));
    net_write_be16(&packet[0], ARP_HTYPE_ETHERNET);
    net_write_be16(&packet[2], ARP_PTYPE_IPV4);
    packet[4] = 6u;
    packet[5] = 4u;
    net_write_be16(&packet[6], ARP_OP_REQUEST);
    net_copy(&packet[8], config->mac, 6u);
    net_write_be32(&packet[14], config->address);
    net_write_be32(&packet[24], target);

    if (!ethernet_send(broadcast, ETHERTYPE_ARP, packet, sizeof(packet))) return 0;
    ++stats->arp_requests;
    return 1;
}

static int send_reply(const uint8_t destination_mac[6], uint32_t destination_ip)
{
    uint8_t packet[ARP_PACKET_BYTES];
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();

    if (config == 0 || destination_mac == 0) return 0;
    net_clear(packet, sizeof(packet));
    net_write_be16(&packet[0], ARP_HTYPE_ETHERNET);
    net_write_be16(&packet[2], ARP_PTYPE_IPV4);
    packet[4] = 6u;
    packet[5] = 4u;
    net_write_be16(&packet[6], ARP_OP_REPLY);
    net_copy(&packet[8], config->mac, 6u);
    net_write_be32(&packet[14], config->address);
    net_copy(&packet[18], destination_mac, 6u);
    net_write_be32(&packet[24], destination_ip);

    if (!ethernet_send(destination_mac, ETHERTYPE_ARP, packet, sizeof(packet))) return 0;
    ++stats->arp_replies;
    return 1;
}

void arp_handle(const uint8_t *packet, size_t length)
{
    const struct net_config *config = net_config_internal();
    uint16_t operation;
    uint32_t sender_ip;
    uint32_t target_ip;

    if (packet == 0 || config == 0 || length < ARP_PACKET_BYTES) return;
    if (net_read_be16(&packet[0]) != ARP_HTYPE_ETHERNET ||
        net_read_be16(&packet[2]) != ARP_PTYPE_IPV4 ||
        packet[4] != 6u || packet[5] != 4u) {
        return;
    }

    operation = net_read_be16(&packet[6]);
    sender_ip = net_read_be32(&packet[14]);
    target_ip = net_read_be32(&packet[24]);

    cache_store(sender_ip, &packet[8]);

    if (operation == ARP_OP_REQUEST && target_ip == config->address) {
        (void)send_reply(&packet[8], sender_ip);
    }
}

int arp_resolve(uint32_t address, uint8_t mac_out[6])
{
    const struct net_config *config = net_config_internal();
    uint32_t next_hop;
    unsigned attempt;

    if (config == 0 || mac_out == 0) return -AXIOM_EINVAL;

    next_hop = ((address & config->netmask) == (config->address & config->netmask)) ?
        address : config->gateway;

    if (cache_lookup(next_hop, mac_out)) return 0;

    for (attempt = 0u; attempt < NET_ARP_ATTEMPTS; ++attempt) {
        uint32_t poll;
        if (!send_request(next_hop)) return -AXIOM_EIO;

        for (poll = 0u; poll < NET_POLL_BUDGET / 8u; ++poll) {
            (void)net_poll(8u);
            if (cache_lookup(next_hop, mac_out)) return 0;
            __asm__ volatile ("pause");
        }
    }
    return -AXIOM_EHOSTUNREACH;
}
```

## `network/dns.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define DNS_PORT 53u
#define DNS_PACKET_MAX 512u
#define DNS_TYPE_A 1u
#define DNS_CLASS_IN 1u

struct dns_pending_state {
    uint16_t transaction;
    uint16_t source_port;
    uint32_t answer;
    int waiting;
    int done;
    int error;
};

static struct dns_pending_state pending;
static uint16_t next_transaction = 0x1801u;

static int dns_skip_name(const uint8_t *packet, size_t length, size_t *offset)
{
    size_t position;
    unsigned labels = 0u;

    if (packet == 0 || offset == 0 || *offset >= length) return 0;
    position = *offset;
    while (position < length && labels++ < 128u) {
        const uint8_t size = packet[position];
        if ((size & 0xC0u) == 0xC0u) {
            if (position + 1u >= length) return 0;
            *offset = position + 2u;
            return 1;
        }
        if (size == 0u) {
            *offset = position + 1u;
            return 1;
        }
        if ((size & 0xC0u) != 0u || size > 63u ||
            position + 1u + size > length) {
            return 0;
        }
        position += 1u + size;
    }
    return 0;
}

static int dns_encode_name(
    const char *name,
    uint8_t *output,
    size_t capacity,
    size_t *length_out
)
{
    size_t input = 0u;
    size_t output_index = 0u;
    size_t total_length;

    if (name == 0 || output == 0 || length_out == 0) return 0;
    total_length = net_text_length(name);
    if (total_length == 0u || total_length > 253u) return 0;

    while (input < total_length) {
        size_t label_start = input;
        size_t label_length;

        while (input < total_length && name[input] != '.') ++input;
        label_length = input - label_start;
        if (label_length == 0u || label_length > 63u ||
            output_index + 1u + label_length + 1u > capacity) {
            return 0;
        }
        output[output_index++] = (uint8_t)label_length;
        net_copy(&output[output_index], &name[label_start], label_length);
        output_index += label_length;
        if (input < total_length && name[input] == '.') ++input;
    }

    output[output_index++] = 0u;
    *length_out = output_index;
    return 1;
}

void dns_handle_udp(
    uint32_t source,
    uint16_t source_port,
    uint16_t destination_port,
    const uint8_t *payload,
    size_t length
)
{
    const struct net_config *config = net_config_internal();
    uint16_t flags;
    uint16_t questions;
    uint16_t answers;
    size_t offset;
    unsigned index;

    if (!pending.waiting || config == 0 || payload == 0 || length < 12u ||
        source != config->dns_server || source_port != DNS_PORT ||
        destination_port != pending.source_port ||
        net_read_be16(&payload[0]) != pending.transaction) {
        return;
    }

    flags = net_read_be16(&payload[2]);
    if ((flags & 0x8000u) == 0u) return;
    if ((flags & 0x000Fu) != 0u) {
        pending.error = -AXIOM_ENOENT;
        pending.done = 1;
        return;
    }

    questions = net_read_be16(&payload[4]);
    answers = net_read_be16(&payload[6]);
    offset = 12u;

    for (index = 0u; index < questions; ++index) {
        if (!dns_skip_name(payload, length, &offset) || offset + 4u > length) {
            pending.error = -AXIOM_EIO;
            pending.done = 1;
            return;
        }
        offset += 4u;
    }

    for (index = 0u; index < answers; ++index) {
        uint16_t type;
        uint16_t class_code;
        uint16_t data_length;

        if (!dns_skip_name(payload, length, &offset) || offset + 10u > length) {
            pending.error = -AXIOM_EIO;
            pending.done = 1;
            return;
        }
        type = net_read_be16(&payload[offset]);
        class_code = net_read_be16(&payload[offset + 2u]);
        data_length = net_read_be16(&payload[offset + 8u]);
        offset += 10u;
        if (offset + data_length > length) {
            pending.error = -AXIOM_EIO;
            pending.done = 1;
            return;
        }

        if (type == DNS_TYPE_A && class_code == DNS_CLASS_IN && data_length == 4u) {
            pending.answer = net_read_be32(&payload[offset]);
            pending.error = 0;
            pending.done = 1;
            ++net_stats_mutable()->dns_answers;
            return;
        }
        offset += data_length;
    }

    pending.error = -AXIOM_ENOENT;
    pending.done = 1;
}

int dns_resolve(const char *name, uint32_t *address_out)
{
    uint8_t packet[DNS_PACKET_MAX];
    size_t name_length;
    size_t length;
    uint32_t poll;
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    int result;

    if (name == 0 || address_out == 0 || config == 0) return -AXIOM_EINVAL;
    net_clear(packet, sizeof(packet));

    pending.transaction = next_transaction++;
    if (next_transaction == 0u) next_transaction = 1u;
    pending.source_port = net_next_ephemeral_port();
    pending.answer = 0u;
    pending.waiting = 1;
    pending.done = 0;
    pending.error = 0;

    net_write_be16(&packet[0], pending.transaction);
    net_write_be16(&packet[2], 0x0100u); /* recursion desired */
    net_write_be16(&packet[4], 1u);

    if (!dns_encode_name(name, &packet[12], sizeof(packet) - 16u, &name_length)) {
        pending.waiting = 0;
        return -AXIOM_EINVAL;
    }
    length = 12u + name_length;
    net_write_be16(&packet[length], DNS_TYPE_A);
    net_write_be16(&packet[length + 2u], DNS_CLASS_IN);
    length += 4u;

    result = udp_send(
        config->dns_server,
        pending.source_port,
        DNS_PORT,
        packet,
        length
    );
    if (result < 0) {
        pending.waiting = 0;
        return result;
    }
    ++stats->dns_queries;

    for (poll = 0u; poll < NET_POLL_BUDGET; ++poll) {
        (void)net_poll(8u);
        if (pending.done) {
            pending.waiting = 0;
            if (pending.error < 0) return pending.error;
            *address_out = pending.answer;
            return 0;
        }
        __asm__ volatile ("pause");
    }

    pending.waiting = 0;
    return -AXIOM_ETIMEDOUT;
}
```

## `network/ethernet.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/drivers/e1000.h>

#include "internal.h"

#define ETHERNET_HEADER_BYTES 14u

int ethernet_send(
    const uint8_t destination[6],
    uint16_t ether_type,
    const void *payload,
    size_t payload_length
)
{
    uint8_t frame[E1000_ETHERNET_FRAME_MAX];
    const struct net_config *config = net_config_internal();

    if (destination == 0 || payload == 0 || config == 0 ||
        payload_length + ETHERNET_HEADER_BYTES > sizeof(frame)) {
        return 0;
    }

    net_copy(&frame[0], destination, 6u);
    net_copy(&frame[6], config->mac, 6u);
    net_write_be16(&frame[12], ether_type);
    net_copy(&frame[14], payload, payload_length);
    return e1000_send(frame, payload_length + ETHERNET_HEADER_BYTES);
}

void ethernet_handle(const uint8_t *frame, size_t length)
{
    const struct net_config *config = net_config_internal();
    const uint16_t type = length >= ETHERNET_HEADER_BYTES ?
        net_read_be16(&frame[12]) : 0u;
    static const uint8_t broadcast[6] = {
        0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu
    };

    if (config == 0 || frame == 0 || length < ETHERNET_HEADER_BYTES) return;
    if (!net_bytes_equal(&frame[0], config->mac, 6u) &&
        !net_bytes_equal(&frame[0], broadcast, 6u)) {
        return;
    }

    if (type == ETHERTYPE_ARP) {
        arp_handle(&frame[ETHERNET_HEADER_BYTES], length - ETHERNET_HEADER_BYTES);
    } else if (type == ETHERTYPE_IPV4) {
        ipv4_handle(&frame[ETHERNET_HEADER_BYTES], length - ETHERNET_HEADER_BYTES);
    }
}
```

## `network/http.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define HTTP_RESPONSE_MAX 8192u
#define HTTP_REQUEST_MAX 768u

static uint8_t response_storage[HTTP_RESPONSE_MAX];

static int append_text(char *buffer, size_t capacity, size_t *used, const char *text)
{
    size_t index;
    size_t length;
    if (buffer == 0 || used == 0 || text == 0) return 0;
    length = net_text_length(text);
    if (*used + length >= capacity) return 0;
    for (index = 0u; index < length; ++index) buffer[(*used)++] = text[index];
    buffer[*used] = '\0';
    return 1;
}

static int parse_port(const char *text, uint16_t *port_out)
{
    uint32_t value = 0u;
    size_t index = 0u;
    if (text == 0 || port_out == 0 || text[0] == '\0') return 0;
    while (text[index] != '\0') {
        if (text[index] < '0' || text[index] > '9') return 0;
        value = value * 10u + (uint32_t)(text[index] - '0');
        if (value > 65535u) return 0;
        ++index;
    }
    if (value == 0u) return 0;
    *port_out = (uint16_t)value;
    return 1;
}

static int split_host_port(
    const char *host_spec,
    char *host,
    size_t host_capacity,
    uint16_t *port_out
)
{
    size_t length;
    size_t colon = SIZE_MAX;
    size_t index;

    if (host_spec == 0 || host == 0 || host_capacity < 2u || port_out == 0) return 0;
    length = net_text_length(host_spec);
    if (length == 0u || length >= host_capacity) return 0;

    for (index = 0u; index < length; ++index) {
        if (host_spec[index] == ':') colon = index;
    }

    *port_out = 80u;
    if (colon != SIZE_MAX) {
        if (colon == 0u || colon + 1u >= length || colon >= host_capacity) return 0;
        for (index = 0u; index < colon; ++index) host[index] = host_spec[index];
        host[colon] = '\0';
        if (!parse_port(&host_spec[colon + 1u], port_out)) return 0;
        return 1;
    }

    for (index = 0u; index < length; ++index) host[index] = host_spec[index];
    host[length] = '\0';
    return 1;
}

static int parse_status(const uint8_t *response, size_t length, uint16_t *status_out)
{
    size_t index;
    if (response == 0 || status_out == 0 || length < 12u) return 0;
    if (response[0] != 'H' || response[1] != 'T' || response[2] != 'T' ||
        response[3] != 'P' || response[4] != '/') {
        return 0;
    }
    for (index = 5u; index + 3u < length && index < 16u; ++index) {
        if (response[index] == ' ' &&
            response[index + 1u] >= '0' && response[index + 1u] <= '9' &&
            response[index + 2u] >= '0' && response[index + 2u] <= '9' &&
            response[index + 3u] >= '0' && response[index + 3u] <= '9') {
            *status_out = (uint16_t)(
                (response[index + 1u] - '0') * 100u +
                (response[index + 2u] - '0') * 10u +
                (response[index + 3u] - '0')
            );
            return 1;
        }
    }
    return 0;
}

static int find_body(const uint8_t *response, size_t length, size_t *offset_out)
{
    size_t index;
    if (response == 0 || offset_out == 0) return 0;
    for (index = 0u; index + 3u < length; ++index) {
        if (response[index] == '\r' && response[index + 1u] == '\n' &&
            response[index + 2u] == '\r' && response[index + 3u] == '\n') {
            *offset_out = index + 4u;
            return 1;
        }
    }
    return 0;
}

int http_get(
    const char *host_spec,
    const char *path,
    uint8_t *body,
    size_t capacity,
    struct axiom_http_result *result_out
)
{
    char host[AXIOM_NET_HOST_MAX];
    char request[HTTP_REQUEST_MAX];
    uint16_t port;
    uint32_t address;
    size_t request_length = 0u;
    size_t response_length = 0u;
    size_t body_offset;
    size_t body_length;
    size_t copied;
    uint16_t status;
    int truncated = 0;
    int result;

    if (host_spec == 0 || path == 0 || body == 0 || capacity == 0u ||
        result_out == 0) {
        return -AXIOM_EINVAL;
    }
    if (!split_host_port(host_spec, host, sizeof(host), &port)) return -AXIOM_EINVAL;
    if (path[0] != '/') return -AXIOM_EINVAL;

    result = net_resolve(host, &address);
    if (result < 0) return result;

    request[0] = '\0';
    if (!append_text(request, sizeof(request), &request_length, "GET ") ||
        !append_text(request, sizeof(request), &request_length, path) ||
        !append_text(request, sizeof(request), &request_length, " HTTP/1.0\r\nHost: ") ||
        !append_text(request, sizeof(request), &request_length, host_spec) ||
        !append_text(request, sizeof(request), &request_length,
                     "\r\nUser-Agent: AxiomOS/0.18\r\nConnection: close\r\n\r\n")) {
        return -AXIOM_ENAMETOOLONG;
    }

    net_clear(response_storage, sizeof(response_storage));
    result = tcp_exchange_http(
        address,
        port,
        request,
        request_length,
        response_storage,
        sizeof(response_storage),
        &response_length,
        &truncated
    );
    if (result < 0) return result;

    if (!parse_status(response_storage, response_length, &status) ||
        !find_body(response_storage, response_length, &body_offset) ||
        body_offset > response_length) {
        return -AXIOM_EIO;
    }

    body_length = response_length - body_offset;
    copied = body_length < capacity ? body_length : capacity;
    net_copy(body, &response_storage[body_offset], copied);

    result_out->address = address;
    result_out->port = port;
    result_out->status_code = status;
    result_out->body_bytes = copied;
    result_out->truncated = (uint32_t)(truncated || copied < body_length);
    result_out->reserved0 = 0u;
    ++net_stats_mutable()->http_requests;
    return 0;
}
```

## `network/icmp.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define ICMP_ECHO_REPLY 0u
#define ICMP_ECHO_REQUEST 8u
#define ICMP_HEADER_BYTES 8u
#define ICMP_IDENTIFIER 0xA118u

struct ping_state {
    uint32_t address;
    uint16_t identifier;
    uint16_t sequence;
    int waiting;
    int received;
};

static struct ping_state ping_state;
static uint16_t next_sequence = 1u;

static int send_echo(
    uint32_t destination,
    uint8_t type,
    uint16_t identifier,
    uint16_t sequence,
    const uint8_t *data,
    size_t data_length
)
{
    uint8_t packet[64];
    struct net_stats *stats = net_stats_mutable();
    const size_t length = ICMP_HEADER_BYTES + data_length;
    int result;

    if (length > sizeof(packet)) return -AXIOM_EMSGSIZE;
    net_clear(packet, length);
    packet[0] = type;
    packet[1] = 0u;
    net_write_be16(&packet[4], identifier);
    net_write_be16(&packet[6], sequence);
    if (data_length != 0u) net_copy(&packet[8], data, data_length);
    net_write_be16(&packet[2], net_checksum(packet, length));

    result = ipv4_send(IPV4_PROTOCOL_ICMP, destination, packet, length);
    if (result == 0) ++stats->icmp_tx;
    return result;
}

void icmp_handle(
    uint32_t source,
    uint32_t destination,
    const uint8_t *packet,
    size_t length
)
{
    struct net_stats *stats = net_stats_mutable();
    uint16_t identifier;
    uint16_t sequence;
    (void)destination;

    if (packet == 0 || length < ICMP_HEADER_BYTES ||
        net_checksum(packet, length) != 0u) {
        return;
    }

    ++stats->icmp_rx;
    identifier = net_read_be16(&packet[4]);
    sequence = net_read_be16(&packet[6]);

    if (packet[0] == ICMP_ECHO_REPLY && ping_state.waiting &&
        source == ping_state.address &&
        identifier == ping_state.identifier &&
        sequence == ping_state.sequence) {
        ping_state.received = 1;
        return;
    }

    if (packet[0] == ICMP_ECHO_REQUEST) {
        (void)send_echo(
            source,
            ICMP_ECHO_REPLY,
            identifier,
            sequence,
            &packet[8],
            length - ICMP_HEADER_BYTES
        );
    }
}

int icmp_ping(uint32_t address, struct axiom_ping_result *result_out)
{
    static const uint8_t payload[] = {
        'A','x','i','o','m','O','S','-','P','h','a','s','e','1','8'
    };
    uint32_t poll;
    int result;

    if (result_out == 0 || address == 0u) return -AXIOM_EINVAL;

    ping_state.address = address;
    ping_state.identifier = ICMP_IDENTIFIER;
    ping_state.sequence = next_sequence++;
    if (next_sequence == 0u) next_sequence = 1u;
    ping_state.waiting = 1;
    ping_state.received = 0;

    result = send_echo(
        address,
        ICMP_ECHO_REQUEST,
        ping_state.identifier,
        ping_state.sequence,
        payload,
        sizeof(payload)
    );
    if (result < 0) {
        ping_state.waiting = 0;
        return result;
    }

    for (poll = 0u; poll < NET_POLL_BUDGET; ++poll) {
        (void)net_poll(8u);
        if (ping_state.received) {
            result_out->address = address;
            result_out->sequence = ping_state.sequence;
            result_out->poll_iterations = poll + 1u;
            ping_state.waiting = 0;
            return 0;
        }
        __asm__ volatile ("pause");
    }

    ping_state.waiting = 0;
    return -AXIOM_ETIMEDOUT;
}
```

## `network/internal.h`

```c
#ifndef AXIOM_NETWORK_INTERNAL_H
#define AXIOM_NETWORK_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/network/net.h>

#define ETHERTYPE_IPV4 0x0800u
#define ETHERTYPE_ARP  0x0806u

#define IPV4_PROTOCOL_ICMP 1u
#define IPV4_PROTOCOL_TCP  6u
#define IPV4_PROTOCOL_UDP  17u

#define NET_POLL_BUDGET 12000000u
#define NET_ARP_ATTEMPTS 3u

uint16_t net_read_be16(const uint8_t *bytes);
uint32_t net_read_be32(const uint8_t *bytes);
void net_write_be16(uint8_t *bytes, uint16_t value);
void net_write_be32(uint8_t *bytes, uint32_t value);
uint16_t net_checksum(const void *data, size_t length);
uint16_t net_transport_checksum(
    uint32_t source,
    uint32_t destination,
    uint8_t protocol,
    const void *segment,
    size_t length
);
void net_copy(void *destination, const void *source, size_t count);
void net_clear(void *destination, size_t count);
int net_bytes_equal(const void *left, const void *right, size_t count);
size_t net_text_length(const char *text);
int net_text_equal(const char *left, const char *right);

const struct net_config *net_config_internal(void);
struct net_stats *net_stats_mutable(void);
uint16_t net_next_ipv4_id(void);
uint16_t net_next_ephemeral_port(void);

int ethernet_send(
    const uint8_t destination[6],
    uint16_t ether_type,
    const void *payload,
    size_t payload_length
);
void ethernet_handle(const uint8_t *frame, size_t length);

int arp_resolve(uint32_t address, uint8_t mac_out[6]);
void arp_handle(const uint8_t *packet, size_t length);

int ipv4_send(
    uint8_t protocol,
    uint32_t destination,
    const void *payload,
    size_t payload_length
);
void ipv4_handle(const uint8_t *packet, size_t length);

void icmp_handle(uint32_t source, uint32_t destination, const uint8_t *packet, size_t length);
int icmp_ping(uint32_t address, struct axiom_ping_result *result_out);

int udp_send(
    uint32_t destination,
    uint16_t source_port,
    uint16_t destination_port,
    const void *payload,
    size_t payload_length
);
void udp_handle(uint32_t source, uint32_t destination, const uint8_t *segment, size_t length);

void dns_handle_udp(
    uint32_t source,
    uint16_t source_port,
    uint16_t destination_port,
    const uint8_t *payload,
    size_t length
);
int dns_resolve(const char *name, uint32_t *address_out);

void tcp_handle(uint32_t source, uint32_t destination, const uint8_t *segment, size_t length);
int tcp_exchange_http(
    uint32_t address,
    uint16_t port,
    const void *request,
    size_t request_length,
    uint8_t *response,
    size_t response_capacity,
    size_t *response_length_out,
    int *truncated_out
);

int http_get(
    const char *host_spec,
    const char *path,
    uint8_t *body,
    size_t capacity,
    struct axiom_http_result *result_out
);

/* Poll one or more received frames. Returns the number processed. */
unsigned net_poll(unsigned frame_budget);

#endif
```

## `network/ipv4.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define IPV4_HEADER_BYTES 20u

int ipv4_send(
    uint8_t protocol,
    uint32_t destination,
    const void *payload,
    size_t payload_length
)
{
    uint8_t packet[NET_MTU];
    uint8_t destination_mac[6];
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    const size_t total_length = IPV4_HEADER_BYTES + payload_length;
    int result;

    if (config == 0 || payload == 0 || total_length > sizeof(packet) ||
        total_length > UINT16_MAX) {
        return -AXIOM_EINVAL;
    }

    result = arp_resolve(destination, destination_mac);
    if (result < 0) return result;

    net_clear(packet, IPV4_HEADER_BYTES);
    packet[0] = 0x45u;
    packet[1] = 0u;
    net_write_be16(&packet[2], (uint16_t)total_length);
    net_write_be16(&packet[4], net_next_ipv4_id());
    net_write_be16(&packet[6], 0x4000u); /* Don't Fragment. */
    packet[8] = 64u;
    packet[9] = protocol;
    net_write_be32(&packet[12], config->address);
    net_write_be32(&packet[16], destination);
    net_write_be16(&packet[10], net_checksum(packet, IPV4_HEADER_BYTES));
    net_copy(&packet[IPV4_HEADER_BYTES], payload, payload_length);

    if (!ethernet_send(destination_mac, ETHERTYPE_IPV4, packet, total_length)) {
        return -AXIOM_EIO;
    }
    ++stats->ipv4_tx;
    return 0;
}

void ipv4_handle(const uint8_t *packet, size_t length)
{
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    size_t header_length;
    uint16_t total_length;
    uint16_t fragment;
    uint32_t source;
    uint32_t destination;
    const uint8_t *payload;
    size_t payload_length;

    if (packet == 0 || config == 0 || length < IPV4_HEADER_BYTES) return;
    if ((packet[0] >> 4) != 4u) return;
    header_length = (size_t)(packet[0] & 0x0Fu) * 4u;
    if (header_length < IPV4_HEADER_BYTES || header_length > length) return;

    total_length = net_read_be16(&packet[2]);
    if (total_length < header_length || total_length > length) return;
    if (net_checksum(packet, header_length) != 0u) return;

    fragment = net_read_be16(&packet[6]);
    if ((fragment & 0x3FFFu) != 0u) return;

    source = net_read_be32(&packet[12]);
    destination = net_read_be32(&packet[16]);
    if (destination != config->address && destination != 0xFFFFFFFFu) return;

    payload = &packet[header_length];
    payload_length = (size_t)total_length - header_length;
    ++stats->ipv4_rx;

    if (packet[9] == IPV4_PROTOCOL_ICMP) {
        icmp_handle(source, destination, payload, payload_length);
    } else if (packet[9] == IPV4_PROTOCOL_UDP) {
        udp_handle(source, destination, payload, payload_length);
    } else if (packet[9] == IPV4_PROTOCOL_TCP) {
        tcp_handle(source, destination, payload, payload_length);
    }
}
```

## `network/net.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/drivers/e1000.h>
#include <axiom/network/net.h>

#include "internal.h"

static struct net_config config;
static struct net_stats stats;
static uint16_t ipv4_id = 1u;
static uint16_t ephemeral_port = 49152u;
static int initialized;
static int available;

uint16_t net_read_be16(const uint8_t *bytes)
{
    return (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
}

uint32_t net_read_be32(const uint8_t *bytes)
{
    return ((uint32_t)bytes[0] << 24) |
        ((uint32_t)bytes[1] << 16) |
        ((uint32_t)bytes[2] << 8) |
        (uint32_t)bytes[3];
}

void net_write_be16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)value;
}

void net_write_be32(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

void net_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = in[index];
}

void net_clear(void *destination, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    size_t index;
    for (index = 0u; index < count; ++index) out[index] = 0u;
}

int net_bytes_equal(const void *left, const void *right, size_t count)
{
    const uint8_t *lhs = (const uint8_t *)left;
    const uint8_t *rhs = (const uint8_t *)right;
    size_t index;
    for (index = 0u; index < count; ++index) {
        if (lhs[index] != rhs[index]) return 0;
    }
    return 1;
}

size_t net_text_length(const char *text)
{
    size_t length = 0u;
    if (text == 0) return 0u;
    while (text[length] != '\0') ++length;
    return length;
}

int net_text_equal(const char *left, const char *right)
{
    size_t index = 0u;
    if (left == 0 || right == 0) return 0;
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) return 0;
        ++index;
    }
    return left[index] == right[index];
}

static uint32_t checksum_accumulate(uint32_t sum, const uint8_t *bytes, size_t length)
{
    while (length >= 2u) {
        sum += (uint32_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
        bytes += 2;
        length -= 2u;
    }
    if (length != 0u) sum += (uint32_t)((uint16_t)bytes[0] << 8);
    return sum;
}

static uint16_t checksum_finish(uint32_t sum)
{
    while ((sum >> 16) != 0u) sum = (sum & 0xFFFFu) + (sum >> 16);
    return (uint16_t)~sum;
}

uint16_t net_checksum(const void *data, size_t length)
{
    return checksum_finish(checksum_accumulate(0u, (const uint8_t *)data, length));
}

uint16_t net_transport_checksum(
    uint32_t source,
    uint32_t destination,
    uint8_t protocol,
    const void *segment,
    size_t length
)
{
    uint8_t pseudo[12];
    uint32_t sum;

    net_write_be32(&pseudo[0], source);
    net_write_be32(&pseudo[4], destination);
    pseudo[8] = 0u;
    pseudo[9] = protocol;
    net_write_be16(&pseudo[10], (uint16_t)length);

    sum = checksum_accumulate(0u, pseudo, sizeof(pseudo));
    sum = checksum_accumulate(sum, (const uint8_t *)segment, length);
    return checksum_finish(sum);
}

const struct net_config *net_config_internal(void)
{
    return &config;
}

struct net_stats *net_stats_mutable(void)
{
    return &stats;
}

uint16_t net_next_ipv4_id(void)
{
    const uint16_t result = ipv4_id++;
    if (ipv4_id == 0u) ipv4_id = 1u;
    return result;
}

uint16_t net_next_ephemeral_port(void)
{
    const uint16_t result = ephemeral_port++;
    if (ephemeral_port < 49152u || ephemeral_port == 0u) ephemeral_port = 49152u;
    return result;
}

unsigned net_poll(unsigned frame_budget)
{
    uint8_t frame[E1000_ETHERNET_FRAME_MAX];
    unsigned processed = 0u;

    if (!available) return 0u;
    while (processed < frame_budget) {
        size_t length = 0u;
        if (!e1000_receive(frame, sizeof(frame), &length)) break;
        ethernet_handle(frame, length);
        ++processed;
    }
    return processed;
}

int net_init(void)
{
    if (initialized) return available;
    initialized = 1;

    if (!e1000_init()) return 0;

    e1000_mac(config.mac);
    /*
     * Phase 18 uses QEMU user networking's documented default IPv4 topology.
     * DHCP is intentionally deferred; the NIC and protocol stack are the focus
     * of this phase and the address remains a normal configurable net_config.
     */
    config.address = NET_IPV4(10, 0, 2, 15);
    config.netmask = NET_IPV4(255, 255, 255, 0);
    config.gateway = NET_IPV4(10, 0, 2, 2);
    config.dns_server = NET_IPV4(10, 0, 2, 3);
    available = 1;
    return 1;
}

int net_available(void)
{
    return available;
}

const struct net_config *net_get_config(void)
{
    return available ? &config : 0;
}

struct net_stats net_get_stats(void)
{
    return stats;
}

int net_parse_ipv4(const char *text, uint32_t *address_out)
{
    uint32_t parts[4];
    size_t part = 0u;
    size_t index = 0u;

    if (text == 0 || address_out == 0 || text[0] == '\0') return 0;
    parts[0] = parts[1] = parts[2] = parts[3] = 0u;

    while (part < 4u) {
        unsigned digits = 0u;
        uint32_t value = 0u;

        while (text[index] >= '0' && text[index] <= '9') {
            value = value * 10u + (uint32_t)(text[index] - '0');
            if (value > 255u || ++digits > 3u) return 0;
            ++index;
        }
        if (digits == 0u) return 0;
        parts[part++] = value;

        if (part == 4u) {
            if (text[index] != '\0') return 0;
        } else {
            if (text[index] != '.') return 0;
            ++index;
        }
    }

    *address_out = NET_IPV4(parts[0], parts[1], parts[2], parts[3]);
    return 1;
}

int net_resolve(const char *name, uint32_t *address_out)
{
    if (!available) return -AXIOM_ENETDOWN;
    if (name == 0 || address_out == 0) return -AXIOM_EINVAL;
    if (net_parse_ipv4(name, address_out)) return 0;
    return dns_resolve(name, address_out);
}

int net_ping(const char *target, struct axiom_ping_result *result_out)
{
    uint32_t address;
    int result;

    if (!available) return -AXIOM_ENETDOWN;
    if (target == 0 || result_out == 0) return -AXIOM_EINVAL;
    result = net_resolve(target, &address);
    if (result < 0) return result;
    return icmp_ping(address, result_out);
}

int net_http_get(
    const char *host_spec,
    const char *path,
    uint8_t *body,
    size_t capacity,
    struct axiom_http_result *result_out
)
{
    if (!available) return -AXIOM_ENETDOWN;
    return http_get(host_spec, path, body, capacity, result_out);
}

void net_get_abi_info(struct axiom_net_info *info_out)
{
    struct e1000_stats driver;
    unsigned index;

    if (info_out == 0) return;
    net_clear(info_out, sizeof(*info_out));
    if (!available) return;

    for (index = 0u; index < 6u; ++index) info_out->mac[index] = config.mac[index];
    info_out->address = config.address;
    info_out->netmask = config.netmask;
    info_out->gateway = config.gateway;
    info_out->dns_server = config.dns_server;
    driver = e1000_get_stats();
    info_out->tx_frames = driver.tx_frames;
    info_out->rx_frames = driver.rx_frames;
    info_out->arp_requests = stats.arp_requests;
    info_out->arp_replies = stats.arp_replies;
    info_out->ipv4_tx = stats.ipv4_tx;
    info_out->ipv4_rx = stats.ipv4_rx;
}
```

## `network/tcp.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define TCP_HEADER_BYTES 20u
#define TCP_FLAG_FIN 0x01u
#define TCP_FLAG_SYN 0x02u
#define TCP_FLAG_RST 0x04u
#define TCP_FLAG_PSH 0x08u
#define TCP_FLAG_ACK 0x10u

#define TCP_STATE_CLOSED 0u
#define TCP_STATE_SYN_SENT 1u
#define TCP_STATE_ESTABLISHED 2u
#define TCP_STATE_DONE 3u
#define TCP_STATE_ERROR 4u

struct tcp_connection {
    uint32_t remote_address;
    uint16_t remote_port;
    uint16_t local_port;
    uint32_t send_next;
    uint32_t receive_next;
    uint8_t state;
    uint8_t *receive_buffer;
    size_t receive_capacity;
    size_t receive_length;
    int truncated;
    int error;
};

static struct tcp_connection connection;
static uint32_t initial_sequence = 0x18000000u;

static int send_segment(uint8_t flags, const void *payload, size_t payload_length)
{
    uint8_t segment[NET_MTU - 20u];
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    const size_t length = TCP_HEADER_BYTES + payload_length;
    uint32_t sequence;
    uint16_t checksum;
    int result;

    if (config == 0 || length > sizeof(segment)) return -AXIOM_EMSGSIZE;

    sequence = connection.send_next;
    net_clear(segment, TCP_HEADER_BYTES);
    net_write_be16(&segment[0], connection.local_port);
    net_write_be16(&segment[2], connection.remote_port);
    net_write_be32(&segment[4], sequence);
    net_write_be32(&segment[8], connection.receive_next);
    segment[12] = (uint8_t)(5u << 4);
    segment[13] = flags;
    net_write_be16(&segment[14], 65535u);
    if (payload_length != 0u) net_copy(&segment[20], payload, payload_length);

    checksum = net_transport_checksum(
        config->address,
        connection.remote_address,
        IPV4_PROTOCOL_TCP,
        segment,
        length
    );
    net_write_be16(&segment[16], checksum);

    result = ipv4_send(
        IPV4_PROTOCOL_TCP,
        connection.remote_address,
        segment,
        length
    );
    if (result < 0) return result;

    ++stats->tcp_tx;
    if ((flags & TCP_FLAG_SYN) != 0u) ++connection.send_next;
    if ((flags & TCP_FLAG_FIN) != 0u) ++connection.send_next;
    connection.send_next += (uint32_t)payload_length;
    return 0;
}

static int wait_for_state(uint8_t wanted)
{
    uint32_t poll;

    for (poll = 0u; poll < NET_POLL_BUDGET; ++poll) {
        (void)net_poll(16u);
        if (connection.state == wanted) return 0;
        if (connection.state == TCP_STATE_ERROR) {
            return connection.error != 0 ? connection.error : -AXIOM_EIO;
        }
        __asm__ volatile ("pause");
    }
    return -AXIOM_ETIMEDOUT;
}

void tcp_handle(
    uint32_t source,
    uint32_t destination,
    const uint8_t *segment,
    size_t length
)
{
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    uint16_t source_port;
    uint16_t destination_port;
    size_t header_length;
    uint32_t sequence;
    uint32_t acknowledgment;
    uint8_t flags;
    const uint8_t *payload;
    size_t payload_length;

    if (segment == 0 || config == 0 || length < TCP_HEADER_BYTES ||
        connection.state == TCP_STATE_CLOSED || connection.state == TCP_STATE_DONE) {
        return;
    }

    source_port = net_read_be16(&segment[0]);
    destination_port = net_read_be16(&segment[2]);
    if (source != connection.remote_address ||
        source_port != connection.remote_port ||
        destination_port != connection.local_port) {
        return;
    }

    header_length = (size_t)(segment[12] >> 4) * 4u;
    if (header_length < TCP_HEADER_BYTES || header_length > length) return;
    if (net_transport_checksum(
            source,
            destination,
            IPV4_PROTOCOL_TCP,
            segment,
            length
        ) != 0u) {
        return;
    }

    sequence = net_read_be32(&segment[4]);
    acknowledgment = net_read_be32(&segment[8]);
    flags = segment[13];
    payload = &segment[header_length];
    payload_length = length - header_length;
    ++stats->tcp_rx;

    if ((flags & TCP_FLAG_RST) != 0u) {
        connection.error = -AXIOM_ECONNREFUSED;
        connection.state = TCP_STATE_ERROR;
        return;
    }

    if (connection.state == TCP_STATE_SYN_SENT) {
        if ((flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) ==
                (TCP_FLAG_SYN | TCP_FLAG_ACK) &&
            acknowledgment == connection.send_next) {
            connection.receive_next = sequence + 1u;
            connection.state = TCP_STATE_ESTABLISHED;
            (void)send_segment(TCP_FLAG_ACK, 0, 0u);
        }
        return;
    }

    if (connection.state != TCP_STATE_ESTABLISHED) return;

    /* Ignore/ack duplicate or out-of-order data; Phase 18 has no reassembly. */
    if ((payload_length != 0u || (flags & TCP_FLAG_FIN) != 0u) &&
        sequence != connection.receive_next) {
        (void)send_segment(TCP_FLAG_ACK, 0, 0u);
        return;
    }

    if (payload_length != 0u) {
        const size_t space = connection.receive_capacity > connection.receive_length ?
            connection.receive_capacity - connection.receive_length : 0u;
        const size_t copied = payload_length < space ? payload_length : space;

        if (copied != 0u) {
            net_copy(
                connection.receive_buffer + connection.receive_length,
                payload,
                copied
            );
            connection.receive_length += copied;
        }
        if (copied < payload_length) connection.truncated = 1;
        connection.receive_next += (uint32_t)payload_length;
        (void)send_segment(TCP_FLAG_ACK, 0, 0u);
    }

    if ((flags & TCP_FLAG_FIN) != 0u) {
        ++connection.receive_next;
        (void)send_segment(TCP_FLAG_ACK, 0, 0u);
        connection.state = TCP_STATE_DONE;
    }
}

int tcp_exchange_http(
    uint32_t address,
    uint16_t port,
    const void *request,
    size_t request_length,
    uint8_t *response,
    size_t response_capacity,
    size_t *response_length_out,
    int *truncated_out
)
{
    int result;

    if (address == 0u || port == 0u || request == 0 || request_length == 0u ||
        response == 0 || response_capacity == 0u ||
        response_length_out == 0 || truncated_out == 0) {
        return -AXIOM_EINVAL;
    }
    if (request_length + TCP_HEADER_BYTES > NET_MTU - 20u) {
        return -AXIOM_EMSGSIZE;
    }

    net_clear(&connection, sizeof(connection));
    connection.remote_address = address;
    connection.remote_port = port;
    connection.local_port = net_next_ephemeral_port();
    connection.send_next = initial_sequence;
    initial_sequence += 0x10000u;
    if (initial_sequence < 0x18000000u) initial_sequence = 0x18000000u;
    connection.receive_buffer = response;
    connection.receive_capacity = response_capacity;
    connection.state = TCP_STATE_SYN_SENT;

    result = send_segment(TCP_FLAG_SYN, 0, 0u);
    if (result < 0) {
        connection.state = TCP_STATE_CLOSED;
        return result;
    }

    result = wait_for_state(TCP_STATE_ESTABLISHED);
    if (result < 0) {
        connection.state = TCP_STATE_CLOSED;
        return result;
    }

    result = send_segment(TCP_FLAG_ACK | TCP_FLAG_PSH, request, request_length);
    if (result < 0) {
        connection.state = TCP_STATE_CLOSED;
        return result;
    }

    result = wait_for_state(TCP_STATE_DONE);
    if (result < 0) {
        connection.state = TCP_STATE_CLOSED;
        return result;
    }

    *response_length_out = connection.receive_length;
    *truncated_out = connection.truncated;
    connection.state = TCP_STATE_CLOSED;
    return 0;
}
```

## `network/udp.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>

#include "internal.h"

#define UDP_HEADER_BYTES 8u

int udp_send(
    uint32_t destination,
    uint16_t source_port,
    uint16_t destination_port,
    const void *payload,
    size_t payload_length
)
{
    uint8_t segment[NET_MTU - 20u];
    const struct net_config *config = net_config_internal();
    struct net_stats *stats = net_stats_mutable();
    const size_t length = UDP_HEADER_BYTES + payload_length;
    uint16_t checksum;
    int result;

    if (config == 0 || payload == 0 || length > sizeof(segment) ||
        length > UINT16_MAX) {
        return -AXIOM_EMSGSIZE;
    }

    net_clear(segment, UDP_HEADER_BYTES);
    net_write_be16(&segment[0], source_port);
    net_write_be16(&segment[2], destination_port);
    net_write_be16(&segment[4], (uint16_t)length);
    net_copy(&segment[UDP_HEADER_BYTES], payload, payload_length);

    checksum = net_transport_checksum(
        config->address, destination, IPV4_PROTOCOL_UDP, segment, length
    );
    if (checksum == 0u) checksum = 0xFFFFu;
    net_write_be16(&segment[6], checksum);

    result = ipv4_send(IPV4_PROTOCOL_UDP, destination, segment, length);
    if (result == 0) ++stats->udp_tx;
    return result;
}

void udp_handle(
    uint32_t source,
    uint32_t destination,
    const uint8_t *segment,
    size_t length
)
{
    struct net_stats *stats = net_stats_mutable();
    uint16_t source_port;
    uint16_t destination_port;
    uint16_t udp_length;
    uint16_t checksum;

    if (segment == 0 || length < UDP_HEADER_BYTES) return;
    source_port = net_read_be16(&segment[0]);
    destination_port = net_read_be16(&segment[2]);
    udp_length = net_read_be16(&segment[4]);
    checksum = net_read_be16(&segment[6]);
    if (udp_length < UDP_HEADER_BYTES || udp_length > length) return;

    if (checksum != 0u &&
        net_transport_checksum(
            source,
            destination,
            IPV4_PROTOCOL_UDP,
            segment,
            udp_length
        ) != 0u) {
        return;
    }

    ++stats->udp_rx;
    dns_handle_udp(
        source,
        source_port,
        destination_port,
        &segment[UDP_HEADER_BYTES],
        (size_t)udp_length - UDP_HEADER_BYTES
    );
}
```

## `tests/phase18_network.py`

```python
#!/usr/bin/env python3
"""Validate Phase-18 E1000 plus Ethernet/ARP/IPv4/ICMP/DNS/TCP/HTTP."""

from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
import re
import socket
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def serial_text(path):
    return path.read_text(errors="replace").replace("\r", "")


def wait_for_text(process, serial, marker, deadline):
    while True:
        text = serial_text(serial)
        if marker in text:
            return text
        if "KERNEL PANIC" in text:
            tail = "\n".join(text.splitlines()[-180:])
            raise AssertionError(f"phase18 kernel panicked\n--- serial tail ---\n{tail}")
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-180:])
            raise AssertionError(
                f"phase18 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )
        time.sleep(0.05)


def wait_for_prompt_count(process, serial, count, deadline):
    while True:
        text = serial_text(serial)
        if text.count("axiom> ") >= count:
            return text
        require(process.poll() is None, "QEMU exited while waiting for shell prompt")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-160:])
            raise AssertionError(
                f"phase18 timed out waiting for prompt #{count}\n"
                f"--- serial tail ---\n{tail}"
            )
        time.sleep(0.05)


def send_hmp_keys(monitor_path, keys):
    deadline = time.monotonic() + 5
    while not monitor_path.exists():
        require(time.monotonic() < deadline, "QEMU monitor socket was not created")
        time.sleep(0.05)

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.settimeout(2)
        connection.connect(str(monitor_path))
        try:
            connection.recv(4096)
        except socket.timeout:
            pass
        for key in keys:
            connection.sendall(f"sendkey {key} 20\n".encode("ascii"))
            time.sleep(0.035)


def key_for_character(character):
    if "a" <= character <= "z" or "0" <= character <= "9":
        return character
    mapping = {
        " ": "spc",
        "/": "slash",
        ".": "dot",
        "-": "minus",
        "_": "shift-minus",
    }
    require(character in mapping, f"phase18: no HMP key mapping for {character!r}")
    return mapping[character]


def send_command(monitor_path, command):
    keys = [key_for_character(character) for character in command]
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


class Phase18Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"

    def do_GET(self):
        body = b"AXIOMOS_PHASE18_HTTP_OK\n"
        if self.path != "/phase18":
            self.send_response(404)
            body = b"not found\n"
        else:
            self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)
        self.wfile.flush()

    def log_message(self, fmt, *args):
        pass


def test_phase18():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase18-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    server = HTTPServer(("127.0.0.1", 0), Phase18Handler)
    host_port = server.server_address[1]
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()

    serial = directory / "phase18-serial.log"
    qemu_log = directory / "phase18-qemu.log"
    monitor = directory / "phase18-monitor.sock"
    serial.write_text("")
    monitor.unlink(missing_ok=True)

    netdev = (
        "user,id=net0,ipv6=off,"
        f"guestfwd=tcp:10.0.2.100:80-tcp:127.0.0.1:{host_port}"
    )

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-smp", "4",
        "-m", "256M",
        "-cdrom", str(directory / "AxiomOS.iso"),
        "-boot", "d",
        "-netdev", netdev,
        "-device", "e1000,netdev=net0,mac=52:54:00:12:34:56",
        "-display", "none",
        "-serial", f"file:{serial}",
        "-monitor", f"unix:{monitor},server=on,wait=off",
        "-no-reboot",
        "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
        try:
            text = wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 240,
            )
            require("AxiomOS Phase 18 networking online." in text,
                    "phase18: networking banner missing")
            require("Network device: Intel E1000 82540EM" in text,
                    "phase18: E1000 did not initialize")
            require("Phase 18 networking initialization complete." in text,
                    "phase18: networking did not initialize")
            require("networking unavailable" not in text,
                    "phase18: kernel skipped networking despite attached E1000")

            prompt = text.count("axiom> ")

            send_command(monitor, "netinfo")
            wait_for_text(process, serial, "IPv4: 10.0.2.15", time.monotonic() + 20)
            wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 20)
            prompt += 1

            send_command(monitor, "ping 10.0.2.2")
            wait_for_text(process, serial, "reply from 10.0.2.2", time.monotonic() + 40)
            wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 40)
            prompt += 1

            # QEMU's user-mode DNS proxy is 10.0.2.3. This test needs the host
            # to have normal Internet/DNS access, just like downloading Limine.
            send_command(monitor, "dns example.com")
            text = wait_for_text(process, serial, "example.com -> ", time.monotonic() + 60)
            wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 60)
            prompt += 1
            require(re.search(r"example\.com -> \d+\.\d+\.\d+\.\d+", text) is not None,
                    "phase18: DNS did not return an IPv4 A record")

            # QEMU guestfwd gives us a deterministic TCP/HTTP peer at
            # 10.0.2.100:80 without root/TAP networking.
            send_command(monitor, "httpget 10.0.2.100 /phase18")
            text = wait_for_text(process, serial, "AXIOMOS_PHASE18_HTTP_OK", time.monotonic() + 60)
            wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 60)
            require("HTTP 200 from 10.0.2.100:80" in text,
                    "phase18: HTTP status/peer output missing")

            final = serial_text(serial)
            require("ping: error" not in final, "phase18: ICMP ping failed")
            require("dns: error" not in final, "phase18: DNS query failed")
            require("httpget: error" not in final, "phase18: HTTP GET failed")
            require("KERNEL PANIC" not in final, "phase18: kernel panicked")

            print("PASS: Phase 18 Intel E1000 PCI/MMIO/DMA driver")
            print("PASS: Phase 18 Ethernet + ARP + IPv4")
            print("PASS: Phase 18 ICMP echo to QEMU router")
            print("PASS: Phase 18 UDP + DNS A-record resolution")
            print("PASS: Phase 18 TCP three-way handshake and stream receive")
            print("PASS: Phase 18 HTTP/1.0 GET from Ring-3 shell")
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            monitor.unlink(missing_ok=True)
            server.shutdown()
            server.server_close()
            server_thread.join(timeout=2)


if __name__ == "__main__":
    test_phase18()
    print("PASS: all Phase 18 networking tests.")
```

## `userspace/phase14_shell.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/errno.h>
#include <axiom/abi/fs.h>
#include <axiom/abi/input.h>
#include <axiom/abi/network.h>
#include <axiom/abi/process.h>
#include <axiom/abi/syscall.h>

#define SHELL_LINE_MAX 256u
#define SHELL_PATH_MAX 256u
#define SHELL_IO_CHUNK 256u

static char cwd[SHELL_PATH_MAX] = "/";
static char http_body[AXIOM_NET_HTTP_USER_MAX];

static long syscall0(long number)
{
    register long rax __asm__("rax") = number;
    __asm__ volatile("syscall" : "+a"(rax) : : "rcx", "r11", "memory");
    return rax;
}

static long syscall1(long number, long first)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi) : "rcx", "r11", "memory");
    return rax;
}

static long syscall2(long number, long first, long second)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi) : "rcx", "r11", "memory");
    return rax;
}

static long syscall3(long number, long first, long second, long third)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    register long rdx __asm__("rdx") = third;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx) : "rcx", "r11", "memory");
    return rax;
}

static long syscall5(
    long number,
    long first,
    long second,
    long third,
    long fourth,
    long fifth
)
{
    register long rax __asm__("rax") = number;
    register long rdi __asm__("rdi") = first;
    register long rsi __asm__("rsi") = second;
    register long rdx __asm__("rdx") = third;
    register long r10 __asm__("r10") = fourth;
    register long r8 __asm__("r8") = fifth;
    __asm__ volatile(
        "syscall"
        : "+a"(rax)
        : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
    return rax;
}

static size_t text_length(const char *text)
{
    size_t length = 0u;
    if (text == 0) return 0u;
    while (text[length] != '\0') ++length;
    return length;
}

static int text_equal(const char *left, const char *right)
{
    size_t index = 0u;
    if (left == 0 || right == 0) return 0;
    while (left[index] != '\0' && right[index] != '\0') {
        if (left[index] != right[index]) return 0;
        ++index;
    }
    return left[index] == right[index];
}

static void copy_text(char *destination, const char *source, size_t capacity)
{
    size_t index = 0u;
    if (destination == 0 || capacity == 0u) return;
    if (source != 0) {
        while (index + 1u < capacity && source[index] != '\0') {
            destination[index] = source[index];
            ++index;
        }
    }
    destination[index] = '\0';
}

static void print_n(const char *text, size_t count)
{
    if (count != 0u) {
        (void)syscall3(AXIOM_SYS_WRITE, 1, (long)(uintptr_t)text, (long)count);
    }
}

static void print(const char *text)
{
    print_n(text, text_length(text));
}

static void print_u64(uint64_t value)
{
    char buffer[32];
    size_t used = 0u;
    if (value == 0u) {
        print("0");
        return;
    }
    while (value != 0u && used < sizeof(buffer)) {
        buffer[used++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (used != 0u) {
        --used;
        print_n(&buffer[used], 1u);
    }
}

static void print_i64(int64_t value)
{
    if (value < 0) {
        print("-");
        print_u64((uint64_t)(-(value + 1)) + 1u);
    } else {
        print_u64((uint64_t)value);
    }
}

static void print_hex_byte(uint8_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    char pair[2];
    pair[0] = digits[value >> 4];
    pair[1] = digits[value & 0x0Fu];
    print_n(pair, 2u);
}

static void print_ipv4(uint32_t address)
{
    print_u64((address >> 24) & 0xFFu); print(".");
    print_u64((address >> 16) & 0xFFu); print(".");
    print_u64((address >> 8) & 0xFFu); print(".");
    print_u64(address & 0xFFu);
}

static char *skip_spaces(char *text)
{
    while (*text == ' ' || *text == '\t') ++text;
    return text;
}

static char *next_token(char **cursor)
{
    char *start;
    char *position;
    if (cursor == 0 || *cursor == 0) return 0;
    position = skip_spaces(*cursor);
    if (*position == '\0') {
        *cursor = position;
        return 0;
    }
    start = position;
    while (*position != '\0' && *position != ' ' && *position != '\t') ++position;
    if (*position != '\0') *position++ = '\0';
    *cursor = position;
    return start;
}

static int normalize_path(const char *input, char *output, size_t capacity)
{
    char combined[SHELL_PATH_MAX * 2u];
    char *parts[32];
    size_t count = 0u;
    size_t length = 0u;
    char *cursor;

    if (input == 0 || output == 0 || capacity < 2u) return 0;

    if (input[0] == '/') {
        copy_text(combined, input, sizeof(combined));
    } else {
        size_t cwd_len = text_length(cwd);
        size_t input_len = text_length(input);
        if (cwd_len + input_len + 2u > sizeof(combined)) return 0;
        copy_text(combined, cwd, sizeof(combined));
        length = text_length(combined);
        if (length > 1u && combined[length - 1u] != '/') combined[length++] = '/';
        if (length == 1u && combined[0] == '/') { }
        for (size_t i = 0u; i < input_len; ++i) combined[length++] = input[i];
        combined[length] = '\0';
    }

    cursor = combined;
    while (*cursor != '\0') {
        char *start;
        while (*cursor == '/') ++cursor;
        if (*cursor == '\0') break;
        start = cursor;
        while (*cursor != '\0' && *cursor != '/') ++cursor;
        if (*cursor != '\0') *cursor++ = '\0';
        if (text_equal(start, ".")) continue;
        if (text_equal(start, "..")) {
            if (count != 0u) --count;
            continue;
        }
        if (count >= 32u) return 0;
        parts[count++] = start;
    }

    length = 0u;
    output[length++] = '/';
    for (size_t i = 0u; i < count; ++i) {
        const size_t part_len = text_length(parts[i]);
        if (length + part_len + 1u >= capacity) return 0;
        for (size_t j = 0u; j < part_len; ++j) output[length++] = parts[i][j];
        if (i + 1u < count) output[length++] = '/';
    }
    output[length] = '\0';
    return 1;
}

static long open_file(const char *path, long flags)
{
    return syscall2(AXIOM_SYS_OPEN, (long)(uintptr_t)path, flags);
}

static long close_file(long fd) { return syscall1(AXIOM_SYS_CLOSE, fd); }
static long stat_path(const char *path, struct axiom_stat *status)
{
    return syscall2(AXIOM_SYS_STAT, (long)(uintptr_t)path, (long)(uintptr_t)status);
}

static void print_error(const char *operation, long error)
{
    print(operation);
    print(": error ");
    print_i64(error);
    print("\n");
}

static int parse_u64(const char *text, uint64_t *value_out)
{
    uint64_t value = 0u;
    size_t index = 0u;

    if (text == 0 || value_out == 0 || text[0] == '\0') return 0;
    while (text[index] != '\0') {
        const unsigned digit = (unsigned)(text[index] - '0');
        if (digit > 9u || value > (UINT64_MAX - digit) / 10u) return 0;
        value = value * 10u + digit;
        ++index;
    }
    *value_out = value;
    return 1;
}

static const char *process_state_name(uint32_t state)
{
    switch (state) {
        case AXIOM_PROC_RUNNING: return "RUNNING";
        case AXIOM_PROC_READY: return "READY";
        case AXIOM_PROC_BLOCKED: return "BLOCKED";
        case AXIOM_PROC_SLEEPING: return "SLEEPING";
        case AXIOM_PROC_TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

static void command_ps(void)
{
    uint64_t index = 0u;
    long result;
    struct axiom_process_info info;

    print("PID  PPID  STATE       PRIV  TICKS  NAME\n");
    for (;;) {
        result = syscall2(
            AXIOM_SYS_PROCINFO,
            (long)index,
            (long)(uintptr_t)&info
        );
        if (result == 0) break;
        if (result < 0) {
            print_error("ps", result);
            return;
        }

        print_u64(info.pid); print("  ");
        if (info.ppid == UINT64_MAX) print("-"); else print_u64(info.ppid);
        print("  "); print(process_state_name(info.state)); print("  ");
        print(info.privilege == AXIOM_PRIV_USER ? "USER" : "KERN");
        print("  "); print_u64(info.runtime_ticks); print("  ");
        print(info.name); print("\n");
        ++index;
    }
}

static int resolve_program_path(const char *command, char *path, size_t capacity)
{
    if (command == 0 || path == 0 || capacity == 0u) return 0;

    if (command[0] == '/' || command[0] == '.') {
        return normalize_path(command, path, capacity);
    }

    {
        static const char prefix[] = "/bin/";
        const size_t prefix_length = text_length(prefix);
        const size_t command_length = text_length(command);
        size_t index;

        if (prefix_length + command_length + 1u > capacity) return 0;
        copy_text(path, prefix, capacity);
        for (index = 0u; index < command_length; ++index) {
            path[prefix_length + index] = command[index];
        }
        path[prefix_length + command_length] = '\0';
    }
    return 1;
}

static void command_spawn_background(const char *program, const char *extra)
{
    char path[SHELL_PATH_MAX];
    long pid;

    if (program == 0) {
        print("spawn: missing program\n");
        return;
    }
    if (extra != 0 && *skip_spaces((char *)extra) != '\0') {
        print("spawn: program arguments are not supported yet\n");
        return;
    }
    if (!resolve_program_path(program, path, sizeof(path))) {
        print("spawn: program path too long\n");
        return;
    }

    pid = syscall1(AXIOM_SYS_SPAWN, (long)(uintptr_t)path);
    if (pid < 0) {
        print_error("spawn", pid);
        return;
    }
    print("[started pid "); print_u64((uint64_t)pid); print("]\n");
}

static void command_wait(const char *argument)
{
    uint64_t pid;
    int64_t status = 0;
    long result;

    if (!parse_u64(argument, &pid)) {
        print("wait: usage: wait <pid>\n");
        return;
    }
    result = syscall2(AXIOM_SYS_WAITPID, (long)pid, (long)(uintptr_t)&status);
    if (result < 0) {
        print_error("wait", result);
        return;
    }
    print("[process "); print_u64(pid); print(" exited "); print_i64(status); print("]\n");
}

static void command_kill(const char *argument)
{
    uint64_t pid;
    long result;

    if (!parse_u64(argument, &pid)) {
        print("kill: usage: kill <pid>\n");
        return;
    }
    result = syscall2(AXIOM_SYS_KILL, (long)pid, AXIOM_SIGTERM);
    if (result < 0) {
        print_error("kill", result);
        return;
    }
    print("sent SIGTERM to "); print_u64(pid); print("\n");
}

static void command_netinfo(void)
{
    struct axiom_net_info info;
    long result = syscall1(AXIOM_SYS_NETINFO, (long)(uintptr_t)&info);
    unsigned index;

    if (result < 0) { print_error("netinfo", result); return; }
    print("NIC: Intel E1000 (polled)\nMAC: ");
    for (index = 0u; index < 6u; ++index) {
        if (index != 0u) print(":");
        print_hex_byte(info.mac[index]);
    }
    print("\nIPv4: "); print_ipv4(info.address);
    print("\nNetmask: "); print_ipv4(info.netmask);
    print("\nGateway: "); print_ipv4(info.gateway);
    print("\nDNS: "); print_ipv4(info.dns_server);
    print("\nFrames TX/RX: "); print_u64(info.tx_frames); print("/"); print_u64(info.rx_frames);
    print("\nARP requests/replies: "); print_u64(info.arp_requests); print("/"); print_u64(info.arp_replies);
    print("\nIPv4 TX/RX: "); print_u64(info.ipv4_tx); print("/"); print_u64(info.ipv4_rx);
    print("\n");
}

static void command_ping(const char *target)
{
    struct axiom_ping_result ping;
    long result;
    if (target == 0) { print("ping: usage: ping <IPv4-or-host>\n"); return; }
    result = syscall2(
        AXIOM_SYS_PING,
        (long)(uintptr_t)target,
        (long)(uintptr_t)&ping
    );
    if (result < 0) { print_error("ping", result); return; }
    print("reply from "); print_ipv4(ping.address);
    print(": icmp_seq="); print_u64(ping.sequence);
    print(" polls="); print_u64(ping.poll_iterations); print("\n");
}

static void command_dns(const char *name)
{
    uint32_t address = 0u;
    long result;
    if (name == 0) { print("dns: usage: dns <hostname>\n"); return; }
    result = syscall2(
        AXIOM_SYS_DNS,
        (long)(uintptr_t)name,
        (long)(uintptr_t)&address
    );
    if (result < 0) { print_error("dns", result); return; }
    print(name); print(" -> "); print_ipv4(address); print("\n");
}

static void command_httpget(const char *host, const char *path)
{
    struct axiom_http_result result_info;
    long result;

    if (host == 0) {
        print("httpget: usage: httpget <host[:port]> [path]\n");
        return;
    }
    if (path == 0 || path[0] == '\0') path = "/";
    if (path[0] != '/') {
        print("httpget: path must begin with /\n");
        return;
    }

    result = syscall5(
        AXIOM_SYS_HTTPGET,
        (long)(uintptr_t)host,
        (long)(uintptr_t)path,
        (long)(uintptr_t)http_body,
        (long)sizeof(http_body),
        (long)(uintptr_t)&result_info
    );
    if (result < 0) { print_error("httpget", result); return; }

    print("HTTP "); print_u64(result_info.status_code);
    print(" from "); print_ipv4(result_info.address);
    print(":"); print_u64(result_info.port);
    print(" (body "); print_u64(result_info.body_bytes); print(" bytes");
    if (result_info.truncated != 0u) print(", truncated");
    print(")\n");
    if (result_info.body_bytes != 0u) {
        print_n(http_body, (size_t)result_info.body_bytes);
        if (http_body[result_info.body_bytes - 1u] != '\n') print("\n");
    }
}

static void command_help(void)
{
    print("AxiomOS shell commands:\n");
    print("  help                 show this help\n");
    print("  clear                clear framebuffer terminal\n");
    print("  echo <text>          print text\n");
    print("  pwd                  print current directory\n");
    print("  cd <dir>             change shell directory\n");
    print("  ls [dir]             list a directory\n");
    print("  cat <file>           print a file\n");
    print("  stat <path>          show file type and size\n");
    print("  touch <file>         create a file\n");
    print("  write <file> <text>  replace file contents\n");
    print("  mkdir <dir>          create a directory\n");
    print("  kbdstats             show PS/2 keyboard counters\n");
    print("  ps                   show process table\n");
    print("  spawn <program>      start a program without waiting\n");
    print("  wait <pid>           wait for one of this shell's children\n");
    print("  kill <pid>           terminate a userspace process (SIGTERM)\n");
    print("  netinfo              show E1000/IPv4 configuration and counters\n");
    print("  ping <host>          send one ICMP echo request\n");
    print("  dns <host>           resolve an IPv4 A record using UDP/DNS\n");
    print("  httpget <host> [p]   HTTP/1.0 GET over TCP (plain HTTP only)\n");
    print("  <program>            run /bin/<program> and wait\n");
    print("  /path/program        run an ELF by VFS path\n");
}

static void command_ls(const char *argument)
{
    char path[SHELL_PATH_MAX];
    struct axiom_dirent entry;
    uint64_t index = 0u;
    long result;

    if (!normalize_path(argument != 0 ? argument : ".", path, sizeof(path))) {
        print("ls: path too long\n");
        return;
    }

    for (;;) {
        result = syscall3(
            AXIOM_SYS_READDIR,
            (long)(uintptr_t)path,
            (long)index,
            (long)(uintptr_t)&entry
        );
        if (result == 0) break;
        if (result < 0) {
            print_error("ls", result);
            return;
        }
        print(entry.name);
        if (entry.type == AXIOM_DT_DIR) print("/");
        print("\n");
        ++index;
    }
}

static void command_cat(const char *argument)
{
    char path[SHELL_PATH_MAX];
    char buffer[SHELL_IO_CHUNK];
    long fd;
    long got;

    if (argument == 0) { print("cat: missing file\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("cat: path too long\n"); return; }
    fd = open_file(path, AXIOM_O_RDONLY);
    if (fd < 0) { print_error("cat", fd); return; }
    for (;;) {
        got = syscall3(AXIOM_SYS_READ, fd, (long)(uintptr_t)buffer, sizeof(buffer));
        if (got < 0) { print_error("cat", got); break; }
        if (got == 0) break;
        print_n(buffer, (size_t)got);
    }
    (void)close_file(fd);
}

static void command_stat(const char *argument)
{
    char path[SHELL_PATH_MAX];
    struct axiom_stat status;
    long result;
    if (argument == 0) { print("stat: missing path\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("stat: path too long\n"); return; }
    result = stat_path(path, &status);
    if (result < 0) { print_error("stat", result); return; }
    print("type: ");
    print(status.type == AXIOM_DT_DIR ? "directory" : "file");
    print("\nsize: ");
    print_u64(status.size);
    print(" bytes\n");
}

static void command_touch(const char *argument)
{
    char path[SHELL_PATH_MAX];
    long fd;
    if (argument == 0) { print("touch: missing file\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("touch: path too long\n"); return; }
    fd = open_file(path, AXIOM_O_CREAT | AXIOM_O_RDWR);
    if (fd < 0) { print_error("touch", fd); return; }
    (void)close_file(fd);
}

static void command_write(const char *argument, const char *text)
{
    char path[SHELL_PATH_MAX];
    long fd;
    long result;
    if (argument == 0 || text == 0 || *text == '\0') {
        print("write: usage: write <file> <text>\n");
        return;
    }
    if (!normalize_path(argument, path, sizeof(path))) { print("write: path too long\n"); return; }
    fd = open_file(path, AXIOM_O_CREAT | AXIOM_O_WRONLY | AXIOM_O_TRUNC);
    if (fd < 0) { print_error("write", fd); return; }
    result = syscall3(AXIOM_SYS_WRITE, fd, (long)(uintptr_t)text, (long)text_length(text));
    if (result >= 0) {
        static const char newline = '\n';
        result = syscall3(AXIOM_SYS_WRITE, fd, (long)(uintptr_t)&newline, 1);
    }
    if (result < 0) print_error("write", result);
    (void)close_file(fd);
}

static void command_mkdir(const char *argument)
{
    char path[SHELL_PATH_MAX];
    long result;
    if (argument == 0) { print("mkdir: missing directory\n"); return; }
    if (!normalize_path(argument, path, sizeof(path))) { print("mkdir: path too long\n"); return; }
    result = syscall1(AXIOM_SYS_MKDIR, (long)(uintptr_t)path);
    if (result < 0) print_error("mkdir", result);
}

static void command_cd(const char *argument)
{
    char path[SHELL_PATH_MAX];
    struct axiom_stat status;
    long result;
    if (argument == 0) argument = "/";
    if (!normalize_path(argument, path, sizeof(path))) { print("cd: path too long\n"); return; }
    result = stat_path(path, &status);
    if (result < 0) { print_error("cd", result); return; }
    if (status.type != AXIOM_DT_DIR) { print("cd: not a directory\n"); return; }
    copy_text(cwd, path, sizeof(cwd));
}

static void command_kbdstats(void)
{
    struct axiom_keyboard_stats stats;
    long result = syscall1(AXIOM_SYS_KBDSTATS, (long)(uintptr_t)&stats);
    if (result < 0) { print_error("kbdstats", result); return; }
    print("Keyboard IRQs/scancodes/chars/dropped: ");
    print_u64(stats.irq_count); print("/");
    print_u64(stats.scancode_count); print("/");
    print_u64(stats.character_count); print("/");
    print_u64(stats.dropped_characters); print("\n");
}

static void run_program(const char *command, const char *extra)
{
    char path[SHELL_PATH_MAX];
    int64_t status = 0;
    long pid;
    long waited;

    if (extra != 0 && *skip_spaces((char *)extra) != '\0') {
        print("shell: external program arguments are not supported yet\n");
        return;
    }

    if (!resolve_program_path(command, path, sizeof(path))) {
        print("shell: program path too long\n");
        return;
    }

    pid = syscall1(AXIOM_SYS_SPAWN, (long)(uintptr_t)path);
    if (pid < 0) {
        if (pid == -AXIOM_ENOENT) {
            print("shell: command not found: "); print(command); print("\n");
        } else {
            print_error("spawn", pid);
        }
        return;
    }

    waited = syscall2(AXIOM_SYS_WAITPID, pid, (long)(uintptr_t)&status);
    if (waited < 0) {
        print_error("waitpid", waited);
        return;
    }
    print("[process "); print_u64((uint64_t)pid); print(" exited "); print_i64(status); print("]\n");
}

static void execute_line(char *line)
{
    char *cursor = line;
    char *command = next_token(&cursor);
    char *first;
    char *rest;

    if (command == 0) return;
    first = next_token(&cursor);
    rest = skip_spaces(cursor);

    if (text_equal(command, "help")) command_help();
    else if (text_equal(command, "clear")) (void)syscall0(AXIOM_SYS_CLEAR);
    else if (text_equal(command, "echo")) { if (first != 0) { print(first); if (*rest != '\0') { print(" "); print(rest); } } print("\n"); }
    else if (text_equal(command, "pwd")) { print(cwd); print("\n"); }
    else if (text_equal(command, "cd")) command_cd(first);
    else if (text_equal(command, "ls")) command_ls(first);
    else if (text_equal(command, "cat")) command_cat(first);
    else if (text_equal(command, "stat")) command_stat(first);
    else if (text_equal(command, "touch")) command_touch(first);
    else if (text_equal(command, "write")) command_write(first, rest);
    else if (text_equal(command, "mkdir")) command_mkdir(first);
    else if (text_equal(command, "kbdstats")) command_kbdstats();
    else if (text_equal(command, "ps")) command_ps();
    else if (text_equal(command, "spawn")) command_spawn_background(first, rest);
    else if (text_equal(command, "wait")) command_wait(first);
    else if (text_equal(command, "kill")) command_kill(first);
    else if (text_equal(command, "netinfo")) command_netinfo();
    else if (text_equal(command, "ping")) command_ping(first);
    else if (text_equal(command, "dns")) command_dns(first);
    else if (text_equal(command, "httpget")) command_httpget(first, *rest != '\0' ? rest : 0);
    else run_program(command, first != 0 ? first : 0);
}

static size_t read_line(char *line, size_t capacity)
{
    size_t length = 0u;
    for (;;) {
        char character = '\0';
        long got = syscall3(AXIOM_SYS_READ, 0, (long)(uintptr_t)&character, 1);
        if (got < 0) return 0u;
        if (got == 0) {
            (void)syscall1(AXIOM_SYS_SLEEP, 10);
            continue;
        }
        if (character == '\n') {
            print("\n");
            line[length] = '\0';
            return length;
        }
        if (character == '\b') {
            if (length != 0u) {
                --length;
                line[length] = '\0';
                print("\b");
            }
            continue;
        }
        if (character >= 32 && character < 127 && length + 1u < capacity) {
            line[length++] = character;
            line[length] = '\0';
            print_n(&character, 1u);
        }
    }
}

__attribute__((noreturn)) void shell_main(void)
{
    char line[SHELL_LINE_MAX];

    print("AxiomOS shell ready. Type 'help' for commands.\n");
    for (;;) {
        print("axiom> ");
        (void)read_line(line, sizeof(line));
        execute_line(line);
    }
}
```

