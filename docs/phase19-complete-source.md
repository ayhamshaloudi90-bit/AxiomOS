# AxiomOS Phase 19 — complete new/modified source

This appendix contains every file added or modified relative to the accepted Phase-18 TCP/HTTP-fixed source tree.

## File list

- `Makefile`
- `README.md`
- `boot/limine/limine.conf`
- `docs/phase19.md`
- `docs/project-state.md`
- `docs/syscalls.md`
- `docs/validation.md`
- `filesystem/bootstrap.c`
- `include/axiom/kernel/syscall.h`
- `include/axiom/memory/vmm.h`
- `include/axiom/process/scheduler.h`
- `include/axiom/process/task.h`
- `kernel/core/kernel.c`
- `kernel/syscall/syscall.c`
- `libc/crt0.S`
- `libc/include/axiom/syscalls.h`
- `libc/include/axiom/unistd.h`
- `libc/include/ctype.h`
- `libc/include/fcntl.h`
- `libc/include/stdio.h`
- `libc/include/stdlib.h`
- `libc/include/string.h`
- `libc/include/sys/stat.h`
- `libc/include/unistd.h`
- `libc/src/ctype.c`
- `libc/src/stdio.c`
- `libc/src/stdlib.c`
- `libc/src/string.c`
- `libc/src/syscall.c`
- `libc/src/syscall_internal.h`
- `libc/src/unistd.c`
- `memory/vmm.c`
- `process/scheduler.c`
- `tests/phase19_libc.py`
- `userspace/phase14_shell.c`
- `userspace/phase15_demo.c`
- `userspace/phase15_sleeper.c`
- `userspace/phase19_demo.c`

## `Makefile`

```makefile
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
USER_PHASE14_C_OBJ := $(USER_DIR)/phase14_shell.o
USER_PHASE14_ELF := $(USER_DIR)/axiomsh.elf
USER_PHASE15_DEMO_C_OBJ := $(USER_DIR)/phase15_demo.o
USER_PHASE15_DEMO_ELF := $(USER_DIR)/phase15_demo.elf
USER_PHASE15_SLEEPER_C_OBJ := $(USER_DIR)/phase15_sleeper.o
USER_PHASE15_SLEEPER_ELF := $(USER_DIR)/phase15_sleeper.elf
USER_PHASE19_C_OBJ := $(USER_DIR)/phase19_demo.o
USER_PHASE19_ELF := $(USER_DIR)/phase19_demo.elf
LIBC_DIR := $(USER_DIR)/libc
LIBC_CRT0_OBJ := $(LIBC_DIR)/crt0.o
LIBC_ARCHIVE := $(USER_DIR)/libaxiom.a
LIBC_SOURCES := libc/src/syscall.c libc/src/unistd.c libc/src/string.c libc/src/ctype.c libc/src/stdlib.c libc/src/stdio.c
LIBC_OBJECTS := $(patsubst libc/src/%.c,$(LIBC_DIR)/%.o,$(LIBC_SOURCES))
DISK_IMAGE := $(BUILD_DIR)/axiom-disk.img


CC := clang
LD := ld.lld
ASM := nasm
AR := llvm-ar

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
	-Ilibc/include \
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

$(LIBC_DIR)/%.o: libc/src/%.c
	@mkdir -p $(LIBC_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(LIBC_CRT0_OBJ): libc/crt0.S include/axiom/abi/syscall.h
	@mkdir -p $(LIBC_DIR)
	$(CC) $(GASFLAGS) -Ilibc/include -c $< -o $@

$(LIBC_ARCHIVE): $(LIBC_OBJECTS)
	@mkdir -p $(USER_DIR)
	$(AR) rcs $@ $(LIBC_OBJECTS)
	@echo
	@echo "Built AxiomOS userspace libc:"
	@echo "  $@"

$(USER_PHASE14_C_OBJ): userspace/phase14_shell.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE14_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE14_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE14_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 14 AxiomOS shell ELF (linked with libaxiom):"
	@echo "  $@"

$(USER_PHASE15_DEMO_C_OBJ): userspace/phase15_demo.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE15_DEMO_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE15_DEMO_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE15_DEMO_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 15 process demo ELF (linked with libaxiom):"
	@echo "  $@"

$(USER_PHASE15_SLEEPER_C_OBJ): userspace/phase15_sleeper.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE15_SLEEPER_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE15_SLEEPER_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE15_SLEEPER_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 15 sleeper ELF (linked with libaxiom):"
	@echo "  $@"

$(USER_PHASE19_C_OBJ): userspace/phase19_demo.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE19_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE19_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE19_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 19 libc demo ELF:"
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
	$(USER_PHASE19_ELF) \
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
		$(USER_PHASE19_ELF) \
		$(ISO_ROOT)/boot/phase19_demo.elf


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
	$(MAKE) test-phase19


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
	@echo "AxiomOS Phase 19 targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 19 tests"
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
	@echo "  make test-phase19 Run userspace libc + mmap-backed malloc tests"
	@echo "  make disk-image Create the 16 MiB persistent QEMU disk image"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9 test-phase10 test-phase11 test-phase12 test-phase13 test-phase14 test-phase15 test-phase16 test-phase17 test-phase18 test-phase19 disk-image

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

test-phase19:
	python3 tests/phase19_libc.py

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

Phases 0–18 provide boot, memory management, interrupts, preemptive scheduling, Ring-3 isolation, syscalls, ELF64 loading, VFS + persistent AHCI storage, an interactive shell, process management, synchronization, SMP/AP bring-up, and an E1000-to-HTTP networking stack.

Phase 19 adds the first reusable AxiomOS userspace C library (`libaxiom.a`):

- shared `crt0` startup code;
- `stdio` with `printf`/`snprintf`/`puts`/`putchar`;
- `string` and memory routines;
- `ctype`;
- `stdlib` numeric conversion and dynamic allocation;
- unistd-like file/process wrappers plus AxiomOS-specific process/network wrappers;
- a real anonymous `mmap` syscall used as the backing store for `malloc`;
- fork/exec/exit lifecycle support for anonymous userspace pages.

The libc is deliberately small: no floating point formatting, locale, threads, environment, dynamic linker, or POSIX sockets yet.

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
make test-phase19
make MODE=page_fault run
make MODE=heap_double_free run
make MODE=heap_guard run
make debug
```

`make test` runs the complete Phase 1 through Phase 19 regression suite.

When `make run` reaches the shell:

```text
AxiomOS shell ready. Type 'help' for commands.
axiom> help
```

The next milestone is Phase 20: security hardening.

[Architecture](docs/architecture.md) | [Memory](docs/memory.md) |
[Tasks](docs/processes.md) | [Syscalls](docs/syscalls.md) |
[Filesystem](docs/filesystem.md) | [Phase 13](docs/phase13.md) |
[Phase 14](docs/phase14.md) | [Phase 15](docs/phase15.md) | [Phase 16](docs/phase16.md) | [Phase 17](docs/phase17.md) | [Phase 18](docs/phase18.md) | [Phase 19](docs/phase19.md) | [Project state](docs/project-state.md) |
[Original roadmap](docs/roadmap.md)
```

## `boot/limine/limine.conf`

```text
timeout: 0
serial: yes
serial_baudrate: 115200
verbose: yes

/AxiomOS
    protocol: limine
    path: boot():/boot/AxiomOS.elf
    module_path: boot():/boot/phase11_launcher.elf
    module_string: phase11-launcher
    module_path: boot():/boot/phase11_demo.elf
    module_string: phase11-demo
    module_path: boot():/boot/phase12_demo.elf
    module_string: phase12-demo
    module_path: boot():/boot/phase13_demo.elf
    module_string: phase13-demo
    module_path: boot():/boot/axiomsh.elf
    module_string: phase14-shell
    module_path: boot():/boot/phase15_demo.elf
    module_string: phase15-demo
    module_path: boot():/boot/phase15_sleeper.elf
    module_string: phase15-sleeper
    module_path: boot():/boot/phase19_demo.elf
    module_string: phase19-demo
```

## `docs/phase19.md`

```markdown
# Phase 19 — Userspace libc

Phase 19 turns the ad-hoc C helpers used by earlier Ring-3 programs into a reusable static userspace library, `libaxiom.a`.

## Library layout

- `libc/crt0.S`: common ELF entry point calling `main()` then `_exit()`.
- `libc/include/stdio.h`: `printf`, `snprintf`, `puts`, `putchar`.
- `libc/include/stdlib.h`: `malloc`, `free`, `calloc`, `realloc`, `atoi`, `strtol`.
- `libc/include/string.h`: common string/memory operations.
- `libc/include/ctype.h`: character classification/case conversion.
- `libc/include/unistd.h`: read/write/process wrappers.
- `libc/include/fcntl.h`, `sys/stat.h`: small filesystem-facing interfaces.
- `libc/include/axiom/syscalls.h`: AxiomOS-specific directory, process, network, and mmap wrappers.

The shell and the Phase-15 C programs now link against `libaxiom.a`. Assembly-only historical demos remain standalone because they intentionally demonstrate the raw syscall/ELF layers from their original phases.

## Anonymous mmap and malloc

`AXIOM_SYS_MMAP`, reserved since Phase 10, is now implemented. It allocates zero-filled, writable, NX user pages from a per-process anonymous mapping region beginning at `0x0000100000000000`.

The first allocator is a first-fit userspace allocator with 16-byte alignment, block splitting, block coalescing, and page growth through `AXIOM_SYS_MMAP`. `free()` recycles blocks inside the process; it does not yet return individual pages to the kernel. All mapped pages are reclaimed when the process exits or execs.

Anonymous mapping pages participate in Phase-15 eager `fork()`: physical pages are copied into the child, preserving process isolation. `exec()` discards old anonymous mappings with the old address space.

Current per-process anonymous mapping ceiling: 128 pages (512 KiB).

## printf subset

Supported conversions: `%c`, `%s`, `%d`, `%i`, `%u`, `%x`, `%X`, `%p`, `%%`, including `l`/`ll` integer length modifiers. Width, precision, floating point, and locale are deliberately not implemented yet.

## Acceptance

Run:

```bash
make test-phase19
make test
```

`test-phase19` launches `/bin/phase19-demo` from the real Ring-3 shell and verifies stdio, strings/memory, ctype/numeric conversion, dynamic allocation, malloc-backed fork isolation, exec cleanup of anonymous mappings, files, process wrappers, and the network-info wrapper.
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


## Phase 19 — accepted implementation pending local runtime validation

Phase 19 adds `libaxiom.a`, shared `crt0`, stdio/string/ctype/stdlib/unistd-like interfaces, anonymous `AXIOM_SYS_MMAP`, and an mmap-backed userspace allocator. The shell and Phase-15 C programs link against the library. `/bin/phase19-demo` is the runtime acceptance executable.
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

`mmap` retains syscall number 11 and is implemented by Phase 19 as a small anonymous userspace mapping primitive used by `libaxiom` heap growth. `fork` is implemented by Phase 15.

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


## Phase 19 anonymous mmap

| Number | Call | Purpose |
|---:|---|---|
| 11 | `mmap` | Allocate zero-filled anonymous writable/NX pages for the current Ring-3 process |

The Phase-19 ABI intentionally keeps this call smaller than POSIX `mmap(2)`: userspace passes only a byte length in `RDI`. The kernel rounds to 4 KiB pages and returns a page-aligned virtual base or a negative errno. Mappings are process-private, are eagerly copied by `fork()`, and are discarded by `exec()` or process teardown. There is no `munmap`, file-backed mapping, fixed-address mapping, or protection-changing API yet. `malloc()` uses this call internally and recycles freed blocks in userspace.
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


## Phase 19

`make test-phase19` boots QEMU with the normal SMP/E1000 environment, waits for the Ring-3 shell, runs `/bin/phase19-demo`, and validates libc formatting, strings/memory, ctype/conversion, dynamic allocation, anonymous-mapping fork isolation, exec cleanup of anonymous mappings, VFS wrappers, process wrappers, and networking wrappers. `make test` includes this target after Phase 18.
```

## `filesystem/bootstrap.c`

```c
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
    return vfs_write_file(vfs_path, address, (size_t)size) == 0;
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
        vfs_mkdir("/tmp") < 0) {
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
        !seed_module_file("phase19-demo", "/bin/phase19-demo")) {
        return 0;
    }

    /* A second RAM filesystem proves that path resolution honors mounts. */
    if (vfs_mount("/tmp", tmpfs) < 0) {
        return 0;
    }

    return 1;
}
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
    uint64_t mmap_calls;
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

## `include/axiom/memory/vmm.h`

```c
#ifndef AXIOM_MEMORY_VMM_H
#define AXIOM_MEMORY_VMM_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/memory/address.h>

#define VMM_PAGE_SIZE 4096ULL

/* Hardware-compatible leaf mapping flags exposed by the paging API. */
#define VMM_FLAG_WRITABLE      (1ULL << 1)
#define VMM_FLAG_USER          (1ULL << 2)
#define VMM_FLAG_WRITE_THROUGH (1ULL << 3)
#define VMM_FLAG_CACHE_DISABLE (1ULL << 4)
#define VMM_FLAG_GLOBAL        (1ULL << 8)
#define VMM_FLAG_NO_EXECUTE    (1ULL << 63)

/* Reserved lower-half virtual addresses used only by Phase-5 validation. */
#define VMM_SELFTEST_ADDRESS   0x0000600000000000ULL
#define VMM_FAULT_TEST_ADDRESS 0x0000612345600000ULL

struct vmm_stats {
    paddr_t root_table;
    uint64_t page_table_pages;
    uint64_t hhdm_offset;
};

int vmm_init(void);

/* Active-kernel-address-space compatibility API from Phase 5. */
int map_page(vaddr_t virtual_address, paddr_t physical_address, uint64_t flags);
int unmap_page(vaddr_t virtual_address);
paddr_t virt_to_phys(vaddr_t virtual_address);

/* Phase 9: isolated lower-half address spaces sharing supervisor kernel maps. */
paddr_t vmm_create_user_address_space(void);
int vmm_destroy_user_address_space(paddr_t root_table);
int vmm_map_page_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
);
int vmm_unmap_page_in_address_space(paddr_t root_table, vaddr_t virtual_address);
paddr_t vmm_virt_to_phys_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address
);
int vmm_activate_address_space(paddr_t root_table);
paddr_t vmm_current_address_space(void);
paddr_t vmm_kernel_address_space(void);

/* Phase 10: validate and copy user buffers without dereferencing them in Ring 0. */
int vmm_user_range_accessible(
    paddr_t root_table,
    vaddr_t user_address,
    size_t length,
    int write_access
);
int vmm_copy_from_user(
    paddr_t root_table,
    void *destination,
    vaddr_t user_source,
    size_t length
);
int vmm_copy_to_user(
    paddr_t root_table,
    vaddr_t user_destination,
    const void *source,
    size_t length
);

/* Phase 11: inspect the effective leaf flags for an existing 4 KiB mapping. */
int vmm_mapping_flags_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    uint64_t *flags_out
);

struct vmm_stats vmm_get_stats(void);
int phase5_vmm_selftest(void);

#endif
```

## `include/axiom/process/scheduler.h`

```c
#ifndef AXIOM_PROCESS_SCHEDULER_H
#define AXIOM_PROCESS_SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/interrupts.h>
#include <axiom/process/task.h>

#define SCHEDULER_MAX_TASKS 16u
#define SCHEDULER_TASK_STACK_SIZE (16u * 1024u)
#define SCHEDULER_DEFAULT_QUANTUM_TICKS 5u

struct syscall_frame;

struct scheduler_stats {
    uint64_t task_count;
    uint64_t runnable_tasks;
    uint64_t context_switches;
    uint64_t preemptions;
    uint64_t scheduling_ticks;
    uint64_t user_fault_terminations;
    uint64_t current_task_id;
};

int scheduler_init(void);

int task_create(
    const char *name,
    task_entry_t entry,
    void *argument,
    uint64_t *task_id_out
);

/* Create a Ring-3 task in its own address space from a <=4 KiB code image. */
int user_task_create(
    const char *name,
    const void *image,
    size_t image_size,
    uint64_t *task_id_out
);

/* Phase 11: create a Ring-3 task by parsing a standalone ELF64 ET_EXEC image. */
int user_task_create_elf(
    const char *name,
    const void *elf_image,
    size_t elf_size,
    uint64_t *task_id_out
);

/* Replace the currently running Ring-3 process image with a new ELF. IF=0. */
int scheduler_exec_current_elf(
    const void *elf_image,
    size_t elf_size,
    vaddr_t *entry_out,
    vaddr_t *stack_out
);


/* Phase 15 process-management primitives. */
int scheduler_fork_current(
    const struct syscall_frame *parent_frame,
    uint64_t *child_id_out
);
int scheduler_wait_current_child(uint64_t child_id, int64_t *status_out);
int scheduler_terminate_task(uint64_t task_id, uint32_t signal_number);

/* Phase 19: anonymous writable/NX user mappings for libc heap growth. */
int scheduler_mmap_current(size_t length, vaddr_t *address_out);

/* Phase 16: generic blocking/wakeup hooks used by kernel wait queues. IF=0. */
int scheduler_prepare_block_current(uint64_t expected_task_id);
int scheduler_park_current(uint64_t expected_task_id);
int scheduler_wake_task(uint64_t task_id);

int scheduler_start(void);
int scheduler_running(void);

/* Return the interrupt frame that the assembly epilogue should restore. */
struct interrupt_frame *scheduler_on_timer_interrupt(
    struct interrupt_frame *frame
);

/* Kill a faulting Ring-3 task and return the next runnable task's frame. */
struct interrupt_frame *scheduler_handle_user_fault(
    struct interrupt_frame *frame,
    uint64_t vector,
    uint64_t error_code,
    vaddr_t fault_address
);

int scheduler_sleep_current(uint64_t ticks);
int scheduler_yield_current(void);
_Noreturn void task_exit_current(int64_t status);

struct task *scheduler_current_task_mutable(void);
const struct task *scheduler_current_task(void);

struct scheduler_stats scheduler_get_stats(void);
size_t scheduler_task_count(void);
const struct task *scheduler_task_at(size_t index);
const struct task *scheduler_task_by_id(uint64_t id);
struct task *scheduler_task_by_id_mutable(uint64_t id);

#endif
```

## `include/axiom/process/task.h`

```c
#ifndef AXIOM_PROCESS_TASK_H
#define AXIOM_PROCESS_TASK_H

#include <stddef.h>
#include <stdint.h>

#include <axiom/abi/user_layout.h>
#include <axiom/arch/interrupts.h>
#include <axiom/memory/address.h>

#define TASK_USER_CODE_BASE   ((vaddr_t)AXIOM_USER_CODE_BASE)
#define TASK_USER_DATA_BASE   ((vaddr_t)AXIOM_USER_DATA_BASE)
#define TASK_USER_STACK_TOP   ((vaddr_t)AXIOM_USER_STACK_TOP)
#define TASK_USER_STACK_PAGES 4u
#define TASK_USER_ELF_MAX_PAGES 32u
#define TASK_USER_MMAP_MAX_PAGES 128u
#define TASK_USER_MMAP_BASE ((vaddr_t)0x0000100000000000ULL)
#define TASK_MAX_FILES 16u

#define TASK_USER_MESSAGE_MAGIC_OFFSET 64u
#define TASK_USER_MESSAGE_MAGIC 0x4158494F4D555345ULL

typedef void (*task_entry_t)(void *argument);

struct vfs_file;

enum task_state {
    TASK_RUNNING = 0,
    TASK_READY,
    TASK_BLOCKED,
    TASK_SLEEPING,
    TASK_TERMINATED,
};

enum task_privilege {
    TASK_PRIVILEGE_KERNEL = 0,
    TASK_PRIVILEGE_USER = 3,
};

struct task {
    uint64_t id;
    uint64_t parent_id;
    const char *name;
    enum task_state state;
    enum task_privilege privilege;

    /* Actual frame restored by the common ISR epilogue. */
    struct interrupt_frame *saved_frame;

    void *kernel_stack_base;
    size_t kernel_stack_size;
    uintptr_t kernel_stack_top;
    paddr_t address_space;

    task_entry_t entry;
    void *argument;

    paddr_t user_code_page;
    paddr_t user_data_page;
    paddr_t user_stack_pages[TASK_USER_STACK_PAGES];

    /* Phase 11 ELF-backed process image pages. */
    paddr_t user_elf_pages[TASK_USER_ELF_MAX_PAGES];
    vaddr_t user_elf_virtual_pages[TASK_USER_ELF_MAX_PAGES];
    uint64_t user_elf_page_flags[TASK_USER_ELF_MAX_PAGES];
    size_t user_elf_page_count;

    /* Phase 19 anonymous userspace mappings backing libc malloc(). */
    paddr_t user_mmap_pages[TASK_USER_MMAP_MAX_PAGES];
    vaddr_t user_mmap_virtual_pages[TASK_USER_MMAP_MAX_PAGES];
    size_t user_mmap_page_count;
    vaddr_t user_mmap_next;
    uint16_t elf_program_headers;
    uint16_t elf_load_segments;
    uint64_t elf_file_bytes;
    uint64_t elf_memory_bytes;
    size_t elf_image_size;
    int elf_backed;

    vaddr_t user_entry;
    vaddr_t user_stack_top;

    /* Phase 12 per-process descriptor table. 0/1/2 remain stdin/out/err. */
    struct vfs_file *file_descriptors[TASK_MAX_FILES];

    uint64_t fault_vector;
    uint64_t fault_error_code;
    vaddr_t fault_address;
    int64_t exit_code;
    uint64_t wake_tick;
    uint64_t wait_target_id;
    int wait_collected;
    uint32_t termination_signal;

    uint64_t quantum_ticks;
    uint64_t ticks_in_slice;
    uint64_t runtime_ticks;
    uint64_t context_switches;
};

const char *task_state_name(enum task_state state);
const char *task_privilege_name(enum task_privilege privilege);

int task_install_file(struct task *task, struct vfs_file *file);
struct vfs_file *task_file(struct task *task, int fd);
struct vfs_file *task_take_file(struct task *task, int fd);
void task_close_all_files(struct task *task);
int task_share_files(struct task *destination, const struct task *source);

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

    terminal_set_color(
        TERMINAL_COLOR_LIGHT_CYAN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("AxiomOS Phase 19 userspace libc available.\n");
    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );
    kprintf("libaxiom: stdio + stdlib + string + ctype + unistd-like wrappers\n");
    kprintf("Phase 19 heap backend: anonymous user mmap pages\n");
    kprintf("Phase 19 libc linkage: axiomsh + Phase 15 C programs + /bin/phase19-demo\n");
    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREEN,
        TERMINAL_COLOR_BLACK
    );
    kprintf("Phase 19 userspace library initialization complete.\n");
    terminal_set_color(
        TERMINAL_COLOR_LIGHT_GREY,
        TERMINAL_COLOR_BLACK
    );

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

static int64_t sys_mmap(uint64_t length)
{
    vaddr_t address = 0u;

    ++stats.mmap_calls;
    if (length == 0u || length > (uint64_t)TASK_USER_MMAP_MAX_PAGES * VMM_PAGE_SIZE) {
        return syscall_error(AXIOM_EINVAL);
    }
    if (!scheduler_mmap_current((size_t)length, &address)) {
        return syscall_error(AXIOM_ENOMEM);
    }
    return (int64_t)address;
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
    stats.mmap_calls = 0u;
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
            result = sys_mmap(frame->rdi);
            break;

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

## `libc/crt0.S`

```asm
#include <axiom/abi/syscall.h>
.section .text.start,"ax",@progbits
.code64
.global _start
.type _start, @function
.extern main
_start:
    movw $0x1B, %ax
    movw %ax, %ds
    movw %ax, %es
    call main
    movq %rax, %rdi
    movl $AXIOM_SYS_EXIT, %eax
    syscall
    ud2
.size _start, .-_start
.section .note.GNU-stack,"",@progbits
```

## `libc/include/axiom/syscalls.h`

```c
#ifndef AXIOM_LIBC_SYSCALLS_H
#define AXIOM_LIBC_SYSCALLS_H

#include <stddef.h>
#include <stdint.h>
#include <axiom/abi/fs.h>
#include <axiom/abi/input.h>
#include <axiom/abi/network.h>
#include <axiom/abi/process.h>

long axiom_readdir(const char *path, unsigned long index, struct axiom_dirent *entry);
long axiom_mkdir(const char *path);
long axiom_spawn(const char *path);
long axiom_clear(void);
long axiom_kbdstats(struct axiom_keyboard_stats *stats);
long axiom_procinfo(unsigned long index, struct axiom_process_info *info);
long axiom_kill(long pid, unsigned long signal_number);
long axiom_netinfo(struct axiom_net_info *info);
long axiom_ping(const char *target, struct axiom_ping_result *result);
long axiom_dns(const char *host, uint32_t *address);
long axiom_httpget(const char *host, const char *path, void *body, size_t capacity, struct axiom_http_result *result);
void *axiom_mmap(size_t length);

#endif
```

## `libc/include/axiom/unistd.h`

```c
#ifndef AXIOM_LIBC_AXIOM_UNISTD_H
#define AXIOM_LIBC_AXIOM_UNISTD_H
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <axiom/syscalls.h>
#endif
```

## `libc/include/ctype.h`

```c
#ifndef AXIOM_LIBC_CTYPE_H
#define AXIOM_LIBC_CTYPE_H

int isalpha(int c);
int isdigit(int c);
int isalnum(int c);
int isspace(int c);
int isupper(int c);
int islower(int c);
int toupper(int c);
int tolower(int c);

#endif
```

## `libc/include/fcntl.h`

```c
#ifndef AXIOM_LIBC_FCNTL_H
#define AXIOM_LIBC_FCNTL_H

#include <axiom/abi/fs.h>

#define O_RDONLY AXIOM_O_RDONLY
#define O_WRONLY AXIOM_O_WRONLY
#define O_RDWR   AXIOM_O_RDWR
#define O_CREAT  AXIOM_O_CREAT
#define O_TRUNC  AXIOM_O_TRUNC
#define O_APPEND AXIOM_O_APPEND

int open(const char *path, int flags);

#endif
```

## `libc/include/stdio.h`

```c
#ifndef AXIOM_LIBC_STDIO_H
#define AXIOM_LIBC_STDIO_H

#include <stddef.h>
#include <stdarg.h>

int putchar(int c);
int puts(const char *s);
int printf(const char *format, ...);
int snprintf(char *buffer, size_t capacity, const char *format, ...);
int vsnprintf(char *buffer, size_t capacity, const char *format, va_list args);

#endif
```

## `libc/include/stdlib.h`

```c
#ifndef AXIOM_LIBC_STDLIB_H
#define AXIOM_LIBC_STDLIB_H

#include <stddef.h>

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);
int atoi(const char *text);
long strtol(const char *text, char **endptr, int base);

#endif
```

## `libc/include/string.h`

```c
#ifndef AXIOM_LIBC_STRING_H
#define AXIOM_LIBC_STRING_H

#include <stddef.h>

size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int value, size_t n);
int memcmp(const void *a, const void *b, size_t n);
char *strchr(const char *s, int c);

#endif
```

## `libc/include/sys/stat.h`

```c
#ifndef AXIOM_LIBC_SYS_STAT_H
#define AXIOM_LIBC_SYS_STAT_H

#include <axiom/abi/fs.h>

int stat(const char *path, struct axiom_stat *status);

#endif
```

## `libc/include/unistd.h`

```c
#ifndef AXIOM_LIBC_UNISTD_H
#define AXIOM_LIBC_UNISTD_H

#include <stddef.h>
#include <stdint.h>

long write(int fd, const void *buffer, size_t count);
long read(int fd, void *buffer, size_t count);
_Noreturn void _exit(int status);
long sleep(uint64_t milliseconds);
long getpid(void);
long getppid(void);
long yield(void);
long fork(void);
long exec(const char *path);
long close(int fd);
long lseek(int fd, long offset, int whence);
long waitpid(long pid, long *status);

#endif
```

## `libc/src/ctype.c`

```c
#include <ctype.h>
int isupper(int c){return c>='A'&&c<='Z';}
int islower(int c){return c>='a'&&c<='z';}
int isalpha(int c){return isupper(c)||islower(c);}
int isdigit(int c){return c>='0'&&c<='9';}
int isalnum(int c){return isalpha(c)||isdigit(c);}
int isspace(int c){return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\f'||c=='\v';}
int toupper(int c){return islower(c)?c-('a'-'A'):c;}
int tolower(int c){return isupper(c)?c+('a'-'A'):c;}
```

## `libc/src/stdio.c`

```c
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

struct out { char *buf; size_t cap; size_t pos; int fd; };
static void emit(struct out *o,char c){ if(o->buf){if(o->cap&&o->pos+1u<o->cap)o->buf[o->pos]=c;} else if(o->fd>=0){(void)write(o->fd,&c,1u);} ++o->pos; }
static void text(struct out*o,const char*s){if(!s)s="(null)";while(*s)emit(o,*s++);}
static void number(struct out*o,uint64_t v,unsigned base,int upper){char b[32];size_t n=0;const char*d=upper?"0123456789ABCDEF":"0123456789abcdef";if(v==0){emit(o,'0');return;}while(v&&n<sizeof(b)){b[n++]=d[v%base];v/=base;}while(n)emit(o,b[--n]);}
static int format(struct out*o,const char*f,va_list ap){while(*f){if(*f!='%'){emit(o,*f++);continue;}++f;if(*f=='%'){emit(o,'%');++f;continue;}int ll=0;if(*f=='l'){++f;ll=1;if(*f=='l'){++f;ll=2;}}switch(*f){case 'c':emit(o,(char)va_arg(ap,int));break;case 's':text(o,va_arg(ap,const char*));break;case 'd':case 'i':{int64_t v=ll==2?va_arg(ap,long long):(ll==1?va_arg(ap,long):va_arg(ap,int));if(v<0){emit(o,'-');number(o,(uint64_t)(-(v+1))+1u,10,0);}else number(o,(uint64_t)v,10,0);break;}case 'u':{uint64_t v=ll==2?va_arg(ap,unsigned long long):(ll==1?va_arg(ap,unsigned long):va_arg(ap,unsigned int));number(o,v,10,0);break;}case 'x':case 'X':{int up=*f=='X';uint64_t v=ll==2?va_arg(ap,unsigned long long):(ll==1?va_arg(ap,unsigned long):va_arg(ap,unsigned int));number(o,v,16,up);break;}case 'p':text(o,"0x");number(o,(uintptr_t)va_arg(ap,void*),16,0);break;default:emit(o,'%');emit(o,*f);break;}if(*f)++f;}if(o->buf&&o->cap){size_t i=o->pos<o->cap?o->pos:o->cap-1u;o->buf[i]='\0';}return (int)o->pos;}
int putchar(int c){char ch=(char)c;return write(1,&ch,1)==1?(unsigned char)ch:-1;}
int puts(const char*s){long a=write(1,s,strlen(s)),b=write(1,"\n",1);return a<0||b<0?-1:(int)(a+b);}
int vsnprintf(char*b,size_t c,const char*f,va_list a){struct out o={b,c,0u,-1};va_list cp;va_copy(cp,a);int r=format(&o,f,cp);va_end(cp);return r;}
int snprintf(char*b,size_t c,const char*f,...){va_list a;va_start(a,f);int r=vsnprintf(b,c,f,a);va_end(a);return r;}
int printf(const char*f,...){struct out o={0,0u,0u,1};va_list a;va_start(a,f);int r=format(&o,f,a);va_end(a);return r;}
```

## `libc/src/stdlib.c`

```c
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <axiom/syscalls.h>

#define ALIGN16(x) (((x)+15u)&~(size_t)15u)
#define HEAP_CHUNK_MIN 4096u
#define BLOCK_MAGIC 0xA1190C19u

struct block { size_t size; struct block *next; uint32_t magic; uint32_t free; uint64_t reserved; };
_Static_assert((sizeof(struct block) % 16u) == 0u, "malloc block header alignment");
static struct block *head;

static struct block *grow_heap(size_t need)
{
    size_t total=ALIGN16(need+sizeof(struct block));
    size_t chunk=(total+4095u)&~(size_t)4095u;
    struct block *b;
    if(chunk<HEAP_CHUNK_MIN) chunk=HEAP_CHUNK_MIN;
    b=(struct block*)axiom_mmap(chunk);
    if(!b) return 0;
    b->size=chunk-sizeof(*b); b->next=0; b->magic=BLOCK_MAGIC; b->free=1u; b->reserved=0u;
    if(!head) head=b; else { struct block *p=head; while(p->next)p=p->next; p->next=b; }
    return b;
}

static void split_block(struct block *b,size_t n)
{
    size_t wanted=ALIGN16(n);
    if(b->size>=wanted+sizeof(struct block)+16u){
        struct block *tail=(struct block*)((uint8_t*)(b+1)+wanted);
        tail->size=b->size-wanted-sizeof(*tail); tail->next=b->next; tail->magic=BLOCK_MAGIC; tail->free=1u; tail->reserved=0u;
        b->size=wanted; b->next=tail;
    }
}

static void coalesce(void)
{
    struct block *b=head;
    while(b&&b->next){
        uint8_t *end=(uint8_t*)(b+1)+b->size;
        if(b->free&&b->next->free&&end==(uint8_t*)b->next){
            b->size+=sizeof(struct block)+b->next->size; b->next=b->next->next;
        } else b=b->next;
    }
}

void *malloc(size_t size)
{
    struct block *b; if(!size) return 0; size=ALIGN16(size);
    for(;;){ for(b=head;b;b=b->next) if(b->magic==BLOCK_MAGIC&&b->free&&b->size>=size){split_block(b,size);b->free=0u;return b+1;} if(!grow_heap(size)) return 0; }
}
void free(void *ptr){struct block*b;if(!ptr)return;b=((struct block*)ptr)-1;if(b->magic!=BLOCK_MAGIC||b->free)return;b->free=1u;coalesce();}
void *calloc(size_t count,size_t size){size_t total;void*p;if(size&&count>SIZE_MAX/size)return 0;total=count*size;p=malloc(total);if(p)memset(p,0,total);return p;}
void *realloc(void *ptr,size_t size){struct block*b;void*n;if(!ptr)return malloc(size);if(!size){free(ptr);return 0;}b=((struct block*)ptr)-1;if(b->magic!=BLOCK_MAGIC)return 0;if(b->size>=size){split_block(b,size);return ptr;}n=malloc(size);if(!n)return 0;memcpy(n,ptr,b->size);free(ptr);return n;}

long strtol(const char *s,char **end,int base)
{
    long sign=1,value=0; const char *p=s; int d;
    while(isspace((unsigned char)*p))++p;
    if(*p=='-'||*p=='+'){if(*p=='-')sign=-1;++p;}
    if(base==0){ if(p[0]=='0'&&(p[1]=='x'||p[1]=='X')){base=16;p+=2;} else if(*p=='0'){base=8;++p;} else base=10; }
    else if(base==16&&p[0]=='0'&&(p[1]=='x'||p[1]=='X'))p+=2;
    if(base<2||base>36){if(end)*end=(char*)s;return 0;}
    while(*p){ if(*p>='0'&&*p<='9')d=*p-'0'; else if(*p>='a'&&*p<='z')d=*p-'a'+10; else if(*p>='A'&&*p<='Z')d=*p-'A'+10; else break; if(d>=base)break; value=value*base+d;++p; }
    if(end)*end=(char*)p; return value*sign;
}
int atoi(const char *s){return (int)strtol(s,0,10);}
```

## `libc/src/string.c`

```c
#include <string.h>
#include <stdint.h>

size_t strlen(const char *s) { size_t n = 0; if (!s) return 0; while (s[n]) ++n; return n; }
int strcmp(const char *a, const char *b) { while (*a && *a == *b) { ++a; ++b; } return (unsigned char)*a - (unsigned char)*b; }
int strncmp(const char *a, const char *b, size_t n) { size_t i; for (i=0;i<n;++i) { unsigned char ac=(unsigned char)a[i], bc=(unsigned char)b[i]; if (ac!=bc) return ac-bc; if (!ac) return 0; } return 0; }
char *strcpy(char *d, const char *s) { char *r=d; while ((*d++=*s++)!='\0') {} return r; }
char *strncpy(char *d, const char *s, size_t n) { size_t i=0; for (;i<n && s[i];++i) d[i]=s[i]; for (;i<n;++i) d[i]='\0'; return d; }
void *memcpy(void *d,const void *s,size_t n){ unsigned char *o=d; const unsigned char *i=s; size_t k; for(k=0;k<n;++k)o[k]=i[k]; return d; }
void *memmove(void *d,const void *s,size_t n){ unsigned char *o=d; const unsigned char *i=s; size_t k; if(o<i){for(k=0;k<n;++k)o[k]=i[k];}else if(o>i){for(k=n;k>0;--k)o[k-1]=i[k-1];} return d; }
void *memset(void *d,int v,size_t n){ unsigned char *o=d; size_t k; for(k=0;k<n;++k)o[k]=(unsigned char)v; return d; }
int memcmp(const void *a,const void *b,size_t n){ const unsigned char *x=a,*y=b; size_t k; for(k=0;k<n;++k) if(x[k]!=y[k]) return x[k]-y[k]; return 0; }
char *strchr(const char *s,int c){ char ch=(char)c; while(*s){ if(*s==ch) return (char*)s; ++s;} return ch=='\0'?(char*)s:0; }
```

## `libc/src/syscall.c`

```c
#include "syscall_internal.h"

long __axiom_syscall0(long n)
{
    register long rax __asm__("rax") = n;
    __asm__ volatile("syscall" : "+a"(rax) : : "rcx", "r11", "memory");
    return rax;
}

long __axiom_syscall1(long n, long a1)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi) : "rcx", "r11", "memory");
    return rax;
}

long __axiom_syscall2(long n, long a1, long a2)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    register long rsi __asm__("rsi") = a2;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi) : "rcx", "r11", "memory");
    return rax;
}

long __axiom_syscall3(long n, long a1, long a2, long a3)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    register long rsi __asm__("rsi") = a2;
    register long rdx __asm__("rdx") = a3;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx) : "rcx", "r11", "memory");
    return rax;
}

long __axiom_syscall5(long n, long a1, long a2, long a3, long a4, long a5)
{
    register long rax __asm__("rax") = n;
    register long rdi __asm__("rdi") = a1;
    register long rsi __asm__("rsi") = a2;
    register long rdx __asm__("rdx") = a3;
    register long r10 __asm__("r10") = a4;
    register long r8 __asm__("r8") = a5;
    __asm__ volatile("syscall" : "+a"(rax) : "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8) : "rcx", "r11", "memory");
    return rax;
}
```

## `libc/src/syscall_internal.h`

```c
#ifndef AXIOM_LIBC_SYSCALL_INTERNAL_H
#define AXIOM_LIBC_SYSCALL_INTERNAL_H
long __axiom_syscall0(long n);
long __axiom_syscall1(long n, long a1);
long __axiom_syscall2(long n, long a1, long a2);
long __axiom_syscall3(long n, long a1, long a2, long a3);
long __axiom_syscall5(long n, long a1, long a2, long a3, long a4, long a5);
#endif
```

## `libc/src/unistd.c`

```c
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <axiom/syscalls.h>
#include <axiom/abi/syscall.h>
#include "syscall_internal.h"

long write(int fd,const void *b,size_t n){return __axiom_syscall3(AXIOM_SYS_WRITE,fd,(long)(uintptr_t)b,(long)n);}
long read(int fd,void *b,size_t n){return __axiom_syscall3(AXIOM_SYS_READ,fd,(long)(uintptr_t)b,(long)n);}
_Noreturn void _exit(int s){(void)__axiom_syscall1(AXIOM_SYS_EXIT,s);for(;;){__asm__ volatile("ud2");}}
long sleep(uint64_t ms){return __axiom_syscall1(AXIOM_SYS_SLEEP,(long)ms);}
long getpid(void){return __axiom_syscall0(AXIOM_SYS_GETPID);}
long getppid(void){return __axiom_syscall0(AXIOM_SYS_GETPPID);}
long yield(void){return __axiom_syscall0(AXIOM_SYS_YIELD);}
long fork(void){return __axiom_syscall0(AXIOM_SYS_FORK);}
long exec(const char *p){return __axiom_syscall1(AXIOM_SYS_EXEC,(long)(uintptr_t)p);}
int open(const char *p,int f){return (int)__axiom_syscall2(AXIOM_SYS_OPEN,(long)(uintptr_t)p,f);}
long close(int fd){return __axiom_syscall1(AXIOM_SYS_CLOSE,fd);}
long lseek(int fd,long o,int w){return __axiom_syscall3(AXIOM_SYS_LSEEK,fd,o,w);}
int stat(const char *p,struct axiom_stat *s){return (int)__axiom_syscall2(AXIOM_SYS_STAT,(long)(uintptr_t)p,(long)(uintptr_t)s);}
long waitpid(long pid,long *status){return __axiom_syscall2(AXIOM_SYS_WAITPID,pid,(long)(uintptr_t)status);}
long axiom_readdir(const char*p,unsigned long i,struct axiom_dirent*e){return __axiom_syscall3(AXIOM_SYS_READDIR,(long)(uintptr_t)p,(long)i,(long)(uintptr_t)e);}
long axiom_mkdir(const char*p){return __axiom_syscall1(AXIOM_SYS_MKDIR,(long)(uintptr_t)p);}
long axiom_spawn(const char*p){return __axiom_syscall1(AXIOM_SYS_SPAWN,(long)(uintptr_t)p);}
long axiom_clear(void){return __axiom_syscall0(AXIOM_SYS_CLEAR);}
long axiom_kbdstats(struct axiom_keyboard_stats*s){return __axiom_syscall1(AXIOM_SYS_KBDSTATS,(long)(uintptr_t)s);}
long axiom_procinfo(unsigned long i,struct axiom_process_info*p){return __axiom_syscall2(AXIOM_SYS_PROCINFO,(long)i,(long)(uintptr_t)p);}
long axiom_kill(long p,unsigned long s){return __axiom_syscall2(AXIOM_SYS_KILL,p,(long)s);}
long axiom_netinfo(struct axiom_net_info*i){return __axiom_syscall1(AXIOM_SYS_NETINFO,(long)(uintptr_t)i);}
long axiom_ping(const char*t,struct axiom_ping_result*r){return __axiom_syscall2(AXIOM_SYS_PING,(long)(uintptr_t)t,(long)(uintptr_t)r);}
long axiom_dns(const char*h,uint32_t*a){return __axiom_syscall2(AXIOM_SYS_DNS,(long)(uintptr_t)h,(long)(uintptr_t)a);}
long axiom_httpget(const char*h,const char*p,void*b,size_t c,struct axiom_http_result*r){return __axiom_syscall5(AXIOM_SYS_HTTPGET,(long)(uintptr_t)h,(long)(uintptr_t)p,(long)(uintptr_t)b,(long)c,(long)(uintptr_t)r);}
void *axiom_mmap(size_t length){long r=__axiom_syscall1(AXIOM_SYS_MMAP,(long)length);return r<0?0:(void*)(uintptr_t)r;}
```

## `memory/vmm.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/boot/limine.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>

#define PAGE_ENTRY_PRESENT       (1ULL << 0)
#define PAGE_ENTRY_WRITABLE      (1ULL << 1)
#define PAGE_ENTRY_USER          (1ULL << 2)
#define PAGE_ENTRY_LARGE         (1ULL << 7)

#define PAGE_ADDRESS_MASK_4K     0x000FFFFFFFFFF000ULL
#define PAGE_ADDRESS_MASK_2M     0x000FFFFFFFE00000ULL
#define PAGE_ADDRESS_MASK_1G     0x000FFFFFC0000000ULL

#define PAGE_OFFSET_MASK_4K      (VMM_PAGE_SIZE - 1ULL)
#define PAGE_OFFSET_MASK_2M      ((1ULL << 21) - 1ULL)
#define PAGE_OFFSET_MASK_1G      ((1ULL << 30) - 1ULL)

#define PAGE_TABLE_ENTRIES       512ULL


static struct {
    paddr_t root_table;
    uint64_t hhdm_offset;
    uint64_t page_table_pages;
    int initialized;
} vmm;


static uint64_t pml4_index(vaddr_t address)
{
    return (address >> 39) & 0x1FFULL;
}


static uint64_t pdpt_index(vaddr_t address)
{
    return (address >> 30) & 0x1FFULL;
}


static uint64_t pd_index(vaddr_t address)
{
    return (address >> 21) & 0x1FFULL;
}


static uint64_t pt_index(vaddr_t address)
{
    return (address >> 12) & 0x1FFULL;
}


static int is_page_aligned(uint64_t address)
{
    return (address & PAGE_OFFSET_MASK_4K) == 0ULL;
}


static int is_canonical(vaddr_t address)
{
    const uint64_t upper = address >> 48;
    const uint64_t sign = (address >> 47) & 1ULL;

    return sign != 0ULL ? upper == 0xFFFFULL : upper == 0ULL;
}


static uint64_t read_cr3(void)
{
    uint64_t value;

    __asm__ volatile ("mov %%cr3, %0" : "=r"(value));
    return value;
}


static void write_cr3(paddr_t root)
{
    __asm__ volatile ("mov %0, %%cr3" : : "r"(root) : "memory");
}


static void invalidate_page(vaddr_t address)
{
    __asm__ volatile ("invlpg (%0)" : : "r"((uintptr_t)address) : "memory");
}


static uint64_t *raw_hhdm_table(paddr_t physical_address)
{
    if (physical_address == PADDR_INVALID ||
        UINT64_MAX - vmm.hhdm_offset < physical_address) {

        return 0;
    }

    return (uint64_t *)(uintptr_t)(vmm.hhdm_offset + physical_address);
}


static uint64_t *owned_table(paddr_t physical_address)
{
    return (uint64_t *)pmm_phys_to_hhdm(physical_address);
}


static void clear_table(uint64_t *table)
{
    uint64_t index;

    for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
        table[index] = 0ULL;
    }
}


static int table_is_empty(const uint64_t *table)
{
    uint64_t index;

    for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
        if ((table[index] & PAGE_ENTRY_PRESENT) != 0ULL) {
            return 0;
        }
    }

    return 1;
}


static int entry_is_large(uint64_t entry, unsigned level)
{
    return (level == 3U || level == 2U) &&
           (entry & PAGE_ENTRY_LARGE) != 0ULL;
}


static void free_owned_tree(paddr_t table_physical, unsigned level);
static paddr_t virt_to_phys_in_root(paddr_t root_table, vaddr_t virtual_address);


static paddr_t clone_table_recursive(paddr_t source_physical, unsigned level)
{
    uint64_t *source;
    uint64_t *destination;
    paddr_t destination_physical;
    uint64_t index;


    source = raw_hhdm_table(source_physical);

    if (source == 0) {
        return PADDR_INVALID;
    }


    destination_physical = pmm_alloc_page();

    if (destination_physical == PADDR_INVALID) {
        return PADDR_INVALID;
    }


    destination = owned_table(destination_physical);

    if (destination == 0) {
        (void)pmm_free_page(destination_physical);
        return PADDR_INVALID;
    }


    clear_table(destination);
    ++vmm.page_table_pages;


    for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
        const uint64_t entry = source[index];


        if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
            destination[index] = entry;
            continue;
        }


        if (level == 1U || entry_is_large(entry, level)) {
            destination[index] = entry;
            continue;
        }


        {
            const paddr_t child_source = entry & PAGE_ADDRESS_MASK_4K;
            const paddr_t child_destination =
                clone_table_recursive(child_source, level - 1U);


            if (child_destination == PADDR_INVALID) {
                free_owned_tree(destination_physical, level);
                return PADDR_INVALID;
            }


            destination[index] =
                child_destination |
                (entry & ~PAGE_ADDRESS_MASK_4K);
        }
    }


    return destination_physical;
}


static void free_owned_tree(paddr_t table_physical, unsigned level)
{
    uint64_t *table;
    uint64_t index;


    table = owned_table(table_physical);

    if (table == 0) {
        return;
    }


    if (level > 1U) {
        for (index = 0; index < PAGE_TABLE_ENTRIES; ++index) {
            const uint64_t entry = table[index];


            if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
                entry_is_large(entry, level)) {

                continue;
            }


            free_owned_tree(
                entry & PAGE_ADDRESS_MASK_4K,
                level - 1U
            );
        }
    }


    if (pmm_free_page(table_physical)) {
        if (vmm.page_table_pages > 0ULL) {
            --vmm.page_table_pages;
        }
    }
}


static int allocate_child_table(
    uint64_t *parent_entry,
    uint64_t leaf_flags,
    paddr_t *new_physical
)
{
    paddr_t physical;
    uint64_t *table;
    uint64_t flags;


    physical = pmm_alloc_page();

    if (physical == PADDR_INVALID) {
        return 0;
    }


    table = owned_table(physical);

    if (table == 0) {
        (void)pmm_free_page(physical);
        return 0;
    }


    clear_table(table);


    flags = PAGE_ENTRY_PRESENT | PAGE_ENTRY_WRITABLE;

    if ((leaf_flags & VMM_FLAG_USER) != 0ULL) {
        flags |= PAGE_ENTRY_USER;
    }


    *parent_entry = physical | flags;
    *new_physical = physical;
    ++vmm.page_table_pages;

    return 1;
}


static void rollback_new_tables(
    uint64_t **parent_entries,
    paddr_t *physical_pages,
    uint64_t count
)
{
    while (count > 0ULL) {
        --count;

        *parent_entries[count] = 0ULL;

        if (pmm_free_page(physical_pages[count]) &&
            vmm.page_table_pages > 0ULL) {

            --vmm.page_table_pages;
        }
    }
}


int vmm_init(void)
{
    struct limine_memmap_response *memory_map;
    uint64_t hhdm_offset;
    paddr_t source_root;
    paddr_t cloned_root;


    if (vmm.initialized) {
        return 1;
    }


    if (!limine_get_memory_boot_info(&memory_map, &hhdm_offset)) {
        return 0;
    }


    if (memory_map == 0) {
        return 0;
    }


    vmm.hhdm_offset = hhdm_offset;
    vmm.page_table_pages = 0ULL;


    source_root = read_cr3() & PAGE_ADDRESS_MASK_4K;

    if (source_root == 0ULL) {
        return 0;
    }


    cloned_root = clone_table_recursive(source_root, 4U);

    if (cloned_root == PADDR_INVALID) {
        return 0;
    }


    vmm.root_table = cloned_root;

    /*
     * The cloned hierarchy contains all current kernel, HHDM and framebuffer
     * mappings. Switching CR3 therefore preserves the currently executing
     * instruction stream and stack while making AxiomOS the page-table owner.
     */
    write_cr3(vmm.root_table);


    if ((read_cr3() & PAGE_ADDRESS_MASK_4K) != vmm.root_table) {
        write_cr3(source_root);
        free_owned_tree(vmm.root_table, 4U);
        vmm.root_table = PADDR_INVALID;
        return 0;
    }


    vmm.initialized = 1;
    return 1;
}


static int map_page_in_root(
    paddr_t root_table,
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    uint64_t *parent_entries[3];
    paddr_t new_tables[3];
    uint64_t *user_entries[3];
    uint64_t created;
    uint64_t user_entry_count;

    uint64_t index4;
    uint64_t index3;
    uint64_t index2;
    uint64_t index1;


    if (!vmm.initialized ||
        !is_page_aligned(virtual_address) ||
        !is_page_aligned(physical_address) ||
        physical_address == PADDR_INVALID ||
        (physical_address & ~PAGE_ADDRESS_MASK_4K) != 0ULL ||
        !is_canonical(virtual_address)) {

        return 0;
    }


    index4 = pml4_index(virtual_address);
    index3 = pdpt_index(virtual_address);
    index2 = pd_index(virtual_address);
    index1 = pt_index(virtual_address);

    created = 0ULL;
    user_entry_count = 0ULL;


    pml4 = owned_table(root_table);

    if (pml4 == 0) {
        return 0;
    }


    if ((pml4[index4] & PAGE_ENTRY_PRESENT) == 0ULL) {
        parent_entries[created] = &pml4[index4];

        if (!allocate_child_table(
                &pml4[index4],
                flags,
                &new_tables[created])) {

            return 0;
        }

        ++created;
    } else if ((flags & VMM_FLAG_USER) != 0ULL) {
        user_entries[user_entry_count++] = &pml4[index4];
    }


    pdpt = owned_table(pml4[index4] & PAGE_ADDRESS_MASK_4K);

    if (pdpt == 0) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pdpt[index3] & PAGE_ENTRY_PRESENT) != 0ULL &&
        (pdpt[index3] & PAGE_ENTRY_LARGE) != 0ULL) {

        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pdpt[index3] & PAGE_ENTRY_PRESENT) == 0ULL) {
        parent_entries[created] = &pdpt[index3];

        if (!allocate_child_table(
                &pdpt[index3],
                flags,
                &new_tables[created])) {

            rollback_new_tables(parent_entries, new_tables, created);
            return 0;
        }

        ++created;
    } else if ((flags & VMM_FLAG_USER) != 0ULL) {
        user_entries[user_entry_count++] = &pdpt[index3];
    }


    pd = owned_table(pdpt[index3] & PAGE_ADDRESS_MASK_4K);

    if (pd == 0) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pd[index2] & PAGE_ENTRY_PRESENT) != 0ULL &&
        (pd[index2] & PAGE_ENTRY_LARGE) != 0ULL) {

        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pd[index2] & PAGE_ENTRY_PRESENT) == 0ULL) {
        parent_entries[created] = &pd[index2];

        if (!allocate_child_table(
                &pd[index2],
                flags,
                &new_tables[created])) {

            rollback_new_tables(parent_entries, new_tables, created);
            return 0;
        }

        ++created;
    } else if ((flags & VMM_FLAG_USER) != 0ULL) {
        user_entries[user_entry_count++] = &pd[index2];
    }


    pt = owned_table(pd[index2] & PAGE_ADDRESS_MASK_4K);

    if (pt == 0) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    if ((pt[index1] & PAGE_ENTRY_PRESENT) != 0ULL) {
        rollback_new_tables(parent_entries, new_tables, created);
        return 0;
    }


    pt[index1] =
        physical_address |
        PAGE_ENTRY_PRESENT |
        (
            flags & (
                VMM_FLAG_WRITABLE |
                VMM_FLAG_USER |
                VMM_FLAG_WRITE_THROUGH |
                VMM_FLAG_CACHE_DISABLE |
                VMM_FLAG_GLOBAL |
                VMM_FLAG_NO_EXECUTE
            )
        );


    while (user_entry_count > 0ULL) {
        --user_entry_count;
        *user_entries[user_entry_count] |= PAGE_ENTRY_USER;
    }


    if ((read_cr3() & PAGE_ADDRESS_MASK_4K) == root_table) {
        invalidate_page(virtual_address);
    }

    return 1;
}




int map_page(
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
)
{
    return map_page_in_root(
        vmm.root_table,
        virtual_address,
        physical_address,
        flags
    );
}

int vmm_map_page_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    paddr_t physical_address,
    uint64_t flags
)
{
    if (!vmm.initialized || root_table == PADDR_INVALID) {
        return 0;
    }

    /* Non-kernel roots may only receive user-accessible lower-half mappings. */
    if (root_table != vmm.root_table) {
        if (virtual_address >= 0x0000800000000000ULL ||
            (flags & VMM_FLAG_USER) == 0ULL) {
            return 0;
        }
    }

    return map_page_in_root(
        root_table,
        virtual_address,
        physical_address,
        flags
    );
}

paddr_t vmm_create_user_address_space(void)
{
    uint64_t *kernel_root;
    uint64_t *user_root;
    paddr_t root_physical;
    uint64_t index;

    if (!vmm.initialized) {
        return PADDR_INVALID;
    }

    root_physical = pmm_alloc_page();
    if (root_physical == PADDR_INVALID) {
        return PADDR_INVALID;
    }

    user_root = owned_table(root_physical);
    kernel_root = owned_table(vmm.root_table);

    if (user_root == 0 || kernel_root == 0) {
        (void)pmm_free_page(root_physical);
        return PADDR_INVALID;
    }

    clear_table(user_root);

    /*
     * Keep the lower canonical half empty for the process. The upper half is
     * shared with the kernel, but PML4 USER is forcibly clear so Ring 3 cannot
     * traverse into any kernel/HHDM/heap/MMIO mapping.
     */
    for (index = 256ULL; index < PAGE_TABLE_ENTRIES; ++index) {
        user_root[index] = kernel_root[index] & ~PAGE_ENTRY_USER;
    }

    ++vmm.page_table_pages;
    return root_physical;
}


static void free_user_table_tree(paddr_t table_physical, unsigned level)
{
    uint64_t *table = owned_table(table_physical);
    uint64_t index;

    if (table == 0) {
        return;
    }

    if (level > 1U) {
        for (index = 0ULL; index < PAGE_TABLE_ENTRIES; ++index) {
            const uint64_t entry = table[index];

            if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
                entry_is_large(entry, level)) {
                continue;
            }

            free_user_table_tree(entry & PAGE_ADDRESS_MASK_4K, level - 1U);
        }
    }

    if (pmm_free_page(table_physical) && vmm.page_table_pages > 0ULL) {
        --vmm.page_table_pages;
    }
}

int vmm_destroy_user_address_space(paddr_t root_table)
{
    uint64_t *root;
    uint64_t index;

    if (!vmm.initialized || root_table == PADDR_INVALID ||
        root_table == vmm.root_table ||
        vmm_current_address_space() == root_table) {
        return 0;
    }

    root = owned_table(root_table);
    if (root == 0) {
        return 0;
    }

    /* Only lower-half structures belong exclusively to this user space. */
    for (index = 0ULL; index < 256ULL; ++index) {
        const uint64_t entry = root[index];

        if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
            continue;
        }

        if ((entry & PAGE_ENTRY_LARGE) != 0ULL) {
            continue;
        }

        free_user_table_tree(entry & PAGE_ADDRESS_MASK_4K, 3U);
        root[index] = 0ULL;
    }

    if (!pmm_free_page(root_table)) {
        return 0;
    }

    if (vmm.page_table_pages > 0ULL) {
        --vmm.page_table_pages;
    }

    return 1;
}

int vmm_activate_address_space(paddr_t root_table)
{
    if (!vmm.initialized || root_table == PADDR_INVALID ||
        !is_page_aligned(root_table) || owned_table(root_table) == 0) {
        return 0;
    }

    write_cr3(root_table);
    return (read_cr3() & PAGE_ADDRESS_MASK_4K) == root_table;
}

paddr_t vmm_current_address_space(void)
{
    if (!vmm.initialized) {
        return PADDR_INVALID;
    }

    return read_cr3() & PAGE_ADDRESS_MASK_4K;
}

paddr_t vmm_kernel_address_space(void)
{
    return vmm.initialized ? vmm.root_table : PADDR_INVALID;
}

paddr_t virt_to_phys(vaddr_t virtual_address)
{
    return virt_to_phys_in_root(vmm.root_table, virtual_address);
}

paddr_t vmm_virt_to_phys_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address
)
{
    if (!vmm.initialized || root_table == PADDR_INVALID) {
        return PADDR_INVALID;
    }

    return virt_to_phys_in_root(root_table, virtual_address);
}

static int unmap_page_in_root(paddr_t root_table, vaddr_t virtual_address)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    paddr_t pdpt_physical;
    paddr_t pd_physical;
    paddr_t pt_physical;
    const uint64_t index4 = pml4_index(virtual_address);
    const uint64_t index3 = pdpt_index(virtual_address);
    const uint64_t index2 = pd_index(virtual_address);
    const uint64_t index1 = pt_index(virtual_address);

    if (!vmm.initialized || root_table == PADDR_INVALID ||
        !is_page_aligned(virtual_address) || !is_canonical(virtual_address)) {
        return 0;
    }

    pml4 = owned_table(root_table);
    if (pml4 == 0 || (pml4[index4] & PAGE_ENTRY_PRESENT) == 0ULL) return 0;
    pdpt_physical = pml4[index4] & PAGE_ADDRESS_MASK_4K;
    pdpt = owned_table(pdpt_physical);
    if (pdpt == 0 || (pdpt[index3] & PAGE_ENTRY_PRESENT) == 0ULL ||
        (pdpt[index3] & PAGE_ENTRY_LARGE) != 0ULL) return 0;
    pd_physical = pdpt[index3] & PAGE_ADDRESS_MASK_4K;
    pd = owned_table(pd_physical);
    if (pd == 0 || (pd[index2] & PAGE_ENTRY_PRESENT) == 0ULL ||
        (pd[index2] & PAGE_ENTRY_LARGE) != 0ULL) return 0;
    pt_physical = pd[index2] & PAGE_ADDRESS_MASK_4K;
    pt = owned_table(pt_physical);
    if (pt == 0 || (pt[index1] & PAGE_ENTRY_PRESENT) == 0ULL) return 0;

    pt[index1] = 0ULL;
    if (vmm_current_address_space() == root_table) invalidate_page(virtual_address);

    if (table_is_empty(pt)) {
        pd[index2] = 0ULL;
        if (pmm_free_page(pt_physical) && vmm.page_table_pages > 0ULL) --vmm.page_table_pages;
        if (table_is_empty(pd)) {
            pdpt[index3] = 0ULL;
            if (pmm_free_page(pd_physical) && vmm.page_table_pages > 0ULL) --vmm.page_table_pages;
            if (table_is_empty(pdpt)) {
                pml4[index4] = 0ULL;
                if (pmm_free_page(pdpt_physical) && vmm.page_table_pages > 0ULL) --vmm.page_table_pages;
            }
        }
    }
    return 1;
}

int unmap_page(vaddr_t virtual_address)
{
    return unmap_page_in_root(vmm.root_table, virtual_address);
}

int vmm_unmap_page_in_address_space(paddr_t root_table, vaddr_t virtual_address)
{
    if (root_table != vmm.root_table && virtual_address >= 0x0000800000000000ULL) return 0;
    return unmap_page_in_root(root_table, virtual_address);
}


static paddr_t virt_to_phys_in_root(paddr_t root_table, vaddr_t virtual_address)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;

    uint64_t entry;


    if (!vmm.initialized || !is_canonical(virtual_address)) {
        return PADDR_INVALID;
    }


    pml4 = owned_table(root_table);

    if (pml4 == 0) {
        return PADDR_INVALID;
    }


    entry = pml4[pml4_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    pdpt = owned_table(entry & PAGE_ADDRESS_MASK_4K);

    if (pdpt == 0) {
        return PADDR_INVALID;
    }


    entry = pdpt[pdpt_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    if ((entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return
            (entry & PAGE_ADDRESS_MASK_1G) |
            (virtual_address & PAGE_OFFSET_MASK_1G);
    }


    pd = owned_table(entry & PAGE_ADDRESS_MASK_4K);

    if (pd == 0) {
        return PADDR_INVALID;
    }


    entry = pd[pd_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    if ((entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return
            (entry & PAGE_ADDRESS_MASK_2M) |
            (virtual_address & PAGE_OFFSET_MASK_2M);
    }


    pt = owned_table(entry & PAGE_ADDRESS_MASK_4K);

    if (pt == 0) {
        return PADDR_INVALID;
    }


    entry = pt[pt_index(virtual_address)];

    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return PADDR_INVALID;
    }


    return
        (entry & PAGE_ADDRESS_MASK_4K) |
        (virtual_address & PAGE_OFFSET_MASK_4K);
}


static int user_mapping_info(
    paddr_t root_table,
    vaddr_t virtual_address,
    int write_access,
    paddr_t *physical_out
)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    uint64_t entry;

    if (!vmm.initialized || root_table == PADDR_INVALID ||
        virtual_address >= 0x0000800000000000ULL ||
        !is_canonical(virtual_address) || physical_out == 0) {
        return 0;
    }

    pml4 = owned_table(root_table);
    if (pml4 == 0) {
        return 0;
    }

    entry = pml4[pml4_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL)) {
        return 0;
    }

    pdpt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pdpt == 0) {
        return 0;
    }

    entry = pdpt[pdpt_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL) ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }

    pd = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pd == 0) {
        return 0;
    }

    entry = pd[pd_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL) ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }

    pt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pt == 0) {
        return 0;
    }

    entry = pt[pt_index(virtual_address)];
    if ((entry & (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER)) !=
        (PAGE_ENTRY_PRESENT | PAGE_ENTRY_USER) ||
        (write_access && (entry & PAGE_ENTRY_WRITABLE) == 0ULL)) {
        return 0;
    }

    *physical_out =
        (entry & PAGE_ADDRESS_MASK_4K) |
        (virtual_address & PAGE_OFFSET_MASK_4K);
    return 1;
}

int vmm_user_range_accessible(
    paddr_t root_table,
    vaddr_t user_address,
    size_t length,
    int write_access
)
{
    size_t checked = 0u;

    if (length == 0u) {
        return 1;
    }

    if (user_address >= 0x0000800000000000ULL ||
        UINT64_MAX - user_address < (uint64_t)(length - 1u)) {
        return 0;
    }

    while (checked < length) {
        const vaddr_t address = user_address + (vaddr_t)checked;
        const size_t page_remaining =
            (size_t)(VMM_PAGE_SIZE - (address & PAGE_OFFSET_MASK_4K));
        const size_t chunk =
            (length - checked < page_remaining) ?
            (length - checked) : page_remaining;
        paddr_t physical;

        if (!user_mapping_info(root_table, address, write_access, &physical)) {
            return 0;
        }

        checked += chunk;
    }

    return 1;
}

int vmm_copy_from_user(
    paddr_t root_table,
    void *destination,
    vaddr_t user_source,
    size_t length
)
{
    uint8_t *out = (uint8_t *)destination;
    size_t copied = 0u;

    if ((length != 0u && destination == 0) ||
        !vmm_user_range_accessible(root_table, user_source, length, 0)) {
        return 0;
    }

    while (copied < length) {
        const vaddr_t address = user_source + (vaddr_t)copied;
        const size_t page_remaining =
            (size_t)(VMM_PAGE_SIZE - (address & PAGE_OFFSET_MASK_4K));
        const size_t chunk =
            (length - copied < page_remaining) ?
            (length - copied) : page_remaining;
        paddr_t physical;
        const uint8_t *source;
        size_t index;

        if (!user_mapping_info(root_table, address, 0, &physical)) {
            return 0;
        }

        source = (const uint8_t *)pmm_phys_to_hhdm(physical);
        if (source == 0) {
            return 0;
        }

        for (index = 0u; index < chunk; ++index) {
            out[copied + index] = source[index];
        }

        copied += chunk;
    }

    return 1;
}

int vmm_copy_to_user(
    paddr_t root_table,
    vaddr_t user_destination,
    const void *source,
    size_t length
)
{
    const uint8_t *in = (const uint8_t *)source;
    size_t copied = 0u;

    if ((length != 0u && source == 0) ||
        !vmm_user_range_accessible(root_table, user_destination, length, 1)) {
        return 0;
    }

    while (copied < length) {
        const vaddr_t address = user_destination + (vaddr_t)copied;
        const size_t page_remaining =
            (size_t)(VMM_PAGE_SIZE - (address & PAGE_OFFSET_MASK_4K));
        const size_t chunk =
            (length - copied < page_remaining) ?
            (length - copied) : page_remaining;
        paddr_t physical;
        uint8_t *destination;
        size_t index;

        if (!user_mapping_info(root_table, address, 1, &physical)) {
            return 0;
        }

        destination = (uint8_t *)pmm_phys_to_hhdm(physical);
        if (destination == 0) {
            return 0;
        }

        for (index = 0u; index < chunk; ++index) {
            destination[index] = in[copied + index];
        }

        copied += chunk;
    }

    return 1;
}


int vmm_mapping_flags_in_address_space(
    paddr_t root_table,
    vaddr_t virtual_address,
    uint64_t *flags_out
)
{
    uint64_t *pml4;
    uint64_t *pdpt;
    uint64_t *pd;
    uint64_t *pt;
    uint64_t entry;
    uint64_t flags = 0ULL;

    if (!vmm.initialized || root_table == PADDR_INVALID || flags_out == 0 ||
        !is_canonical(virtual_address)) {
        return 0;
    }

    pml4 = owned_table(root_table);
    if (pml4 == 0) {
        return 0;
    }

    entry = pml4[pml4_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return 0;
    }
    if ((entry & PAGE_ENTRY_USER) != 0ULL) {
        flags |= VMM_FLAG_USER;
    }
    if ((entry & PAGE_ENTRY_WRITABLE) == 0ULL) {
        /* A read-only parent makes the whole path read-only. */
    }

    pdpt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pdpt == 0) {
        return 0;
    }

    entry = pdpt[pdpt_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }
    if ((entry & PAGE_ENTRY_USER) != 0ULL) {
        flags |= VMM_FLAG_USER;
    }

    pd = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pd == 0) {
        return 0;
    }

    entry = pd[pd_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL ||
        (entry & PAGE_ENTRY_LARGE) != 0ULL) {
        return 0;
    }
    if ((entry & PAGE_ENTRY_USER) != 0ULL) {
        flags |= VMM_FLAG_USER;
    }

    pt = owned_table(entry & PAGE_ADDRESS_MASK_4K);
    if (pt == 0) {
        return 0;
    }

    entry = pt[pt_index(virtual_address)];
    if ((entry & PAGE_ENTRY_PRESENT) == 0ULL) {
        return 0;
    }

    flags = entry & (
        VMM_FLAG_WRITABLE |
        VMM_FLAG_USER |
        VMM_FLAG_WRITE_THROUGH |
        VMM_FLAG_CACHE_DISABLE |
        VMM_FLAG_GLOBAL |
        VMM_FLAG_NO_EXECUTE
    );

    *flags_out = flags;
    return 1;
}

struct vmm_stats vmm_get_stats(void)
{
    struct vmm_stats stats;

    stats.root_table = vmm.root_table;
    stats.page_table_pages = vmm.page_table_pages;
    stats.hhdm_offset = vmm.hhdm_offset;

    return stats;
}
```

## `process/scheduler.c`

```c
#include <stddef.h>
#include <stdint.h>

#include <axiom/arch/gdt.h>
#include <axiom/arch/interrupts.h>
#include <axiom/elf/elf64.h>
#include <axiom/kernel/panic.h>
#include <axiom/kernel/syscall.h>
#include <axiom/memory/heap.h>
#include <axiom/memory/pmm.h>
#include <axiom/memory/vmm.h>
#include <axiom/process/scheduler.h>

static struct task tasks[SCHEDULER_MAX_TASKS];
static size_t task_count_value;
static size_t current_index;
static uint64_t next_task_id;
static uint64_t total_context_switches;
static uint64_t total_preemptions;
static uint64_t total_scheduling_ticks;
static uint64_t total_user_fault_terminations;
static int initialized;
static int running;

extern uint8_t kernel_stack_bottom[];
extern uint8_t kernel_stack_top[];

static void free_user_task_resources(struct task *task);

static void bytes_clear(void *memory, size_t count)
{
    uint8_t *bytes = (uint8_t *)memory;
    size_t index;

    for (index = 0u; index < count; ++index) {
        bytes[index] = 0u;
    }
}

static void bytes_copy(void *destination, const void *source, size_t count)
{
    uint8_t *out = (uint8_t *)destination;
    const uint8_t *in = (const uint8_t *)source;
    size_t index;

    for (index = 0u; index < count; ++index) {
        out[index] = in[index];
    }
}

static void frame_clear(struct interrupt_frame *frame)
{
    bytes_clear(frame, sizeof(*frame));
}

static void task_reset(struct task *task)
{
    size_t index;

    task->id = 0u;
    task->parent_id = UINT64_MAX;
    task->name = 0;
    task->state = TASK_TERMINATED;
    task->privilege = TASK_PRIVILEGE_KERNEL;
    task->saved_frame = 0;
    task->kernel_stack_base = 0;
    task->kernel_stack_size = 0u;
    task->kernel_stack_top = 0u;
    task->address_space = PADDR_INVALID;
    task->entry = 0;
    task->argument = 0;
    task->user_code_page = PADDR_INVALID;
    task->user_data_page = PADDR_INVALID;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        task->user_stack_pages[index] = PADDR_INVALID;
    }

    for (index = 0u; index < TASK_USER_ELF_MAX_PAGES; ++index) {
        task->user_elf_pages[index] = PADDR_INVALID;
        task->user_elf_virtual_pages[index] = 0u;
        task->user_elf_page_flags[index] = 0u;
    }

    for (index = 0u; index < TASK_USER_MMAP_MAX_PAGES; ++index) {
        task->user_mmap_pages[index] = PADDR_INVALID;
        task->user_mmap_virtual_pages[index] = 0u;
    }

    for (index = 0u; index < TASK_MAX_FILES; ++index) {
        task->file_descriptors[index] = 0;
    }

    task->user_elf_page_count = 0u;
    task->user_mmap_page_count = 0u;
    task->user_mmap_next = TASK_USER_MMAP_BASE;
    task->elf_program_headers = 0u;
    task->elf_load_segments = 0u;
    task->elf_file_bytes = 0u;
    task->elf_memory_bytes = 0u;
    task->elf_image_size = 0u;
    task->elf_backed = 0;

    task->user_entry = 0u;
    task->user_stack_top = 0u;
    task->fault_vector = 0u;
    task->fault_error_code = 0u;
    task->fault_address = 0u;
    task->exit_code = 0;
    task->wake_tick = 0u;
    task->wait_target_id = UINT64_MAX;
    task->wait_collected = 0;
    task->termination_signal = 0u;
    task->quantum_ticks = SCHEDULER_DEFAULT_QUANTUM_TICKS;
    task->ticks_in_slice = 0u;
    task->runtime_ticks = 0u;
    task->context_switches = 0u;
}

static int allocate_kernel_stack(struct task *task)
{
    uintptr_t top;
    void *stack = kmalloc(SCHEDULER_TASK_STACK_SIZE);

    if (stack == 0) {
        return 0;
    }

    top = ((uintptr_t)stack + SCHEDULER_TASK_STACK_SIZE) & ~(uintptr_t)0xFu;
    task->kernel_stack_base = stack;
    task->kernel_stack_size = SCHEDULER_TASK_STACK_SIZE;
    task->kernel_stack_top = top;
    return 1;
}

static _Noreturn void task_bootstrap(struct task *task)
{
    if (task == 0 || task->entry == 0) {
        kernel_panic("scheduler entered an invalid task bootstrap");
    }

    task->entry(task->argument);
    task_exit_current(0);
}

static struct interrupt_frame *build_kernel_initial_frame(struct task *task)
{
    uintptr_t address;
    uintptr_t initial_rsp;
    struct interrupt_frame *frame;

    /*
     * In 64-bit mode the CPU saves SS:RSP for every interrupt frame, even
     * without a privilege change, and IRETQ restores them. Build the synthetic
     * kernel-thread frame with a real private-stack RSP and kernel-data SS.
     *
     * A normal SysV C function sees RSP == 8 (mod 16) at entry because CALL
     * pushed a return address. IRETQ does not do that, so reserve one dummy
     * quadword at the top of the task stack and restore RSP to that slot.
     */
    initial_rsp = task->kernel_stack_top - sizeof(uint64_t);
    *(uint64_t *)initial_rsp = 0u;

    address = initial_rsp - sizeof(struct interrupt_frame);
    frame = (struct interrupt_frame *)address;
    frame_clear(frame);

    frame->rip = (uint64_t)(uintptr_t)&task_bootstrap;
    frame->cs = GDT_KERNEL_CODE;
    frame->rflags = 0x202ULL;
    frame->rsp = (uint64_t)initial_rsp;
    frame->ss = GDT_KERNEL_DATA;
    frame->rdi = (uint64_t)(uintptr_t)task;
    return frame;
}

static struct interrupt_frame *build_user_initial_frame(struct task *task)
{
    uintptr_t address;
    struct interrupt_frame *frame;

    address = task->kernel_stack_top - sizeof(struct interrupt_frame);
    frame = (struct interrupt_frame *)address;
    frame_clear(frame);

    frame->rip = task->user_entry;
    frame->cs = GDT_USER_CODE;
    frame->rflags = 0x202ULL;
    frame->rsp = task->user_stack_top - 8ULL;
    frame->ss = GDT_USER_DATA;
    return frame;
}

static size_t find_next_ready(size_t after)
{
    size_t step;

    for (step = 1u; step <= task_count_value; ++step) {
        const size_t index = (after + step) % task_count_value;

        if (tasks[index].state == TASK_READY) {
            return index;
        }
    }

    return SCHEDULER_MAX_TASKS;
}

static struct interrupt_frame *switch_to(size_t next_index)
{
    struct task *next;

    if (next_index >= task_count_value) {
        kernel_panic("scheduler selected invalid task index");
    }

    next = &tasks[next_index];

    if (next->saved_frame == 0 ||
        next->address_space == PADDR_INVALID ||
        next->kernel_stack_top == 0u) {
        kernel_panic("scheduler selected incomplete task context");
    }

    if (vmm_current_address_space() != next->address_space &&
        !vmm_activate_address_space(next->address_space)) {
        kernel_panic("scheduler failed to switch address spaces");
    }

    gdt_set_kernel_stack(next->kernel_stack_top);
    syscall_set_kernel_stack(next->kernel_stack_top);
    next->state = TASK_RUNNING;
    next->ticks_in_slice = 0u;
    ++next->context_switches;

    current_index = next_index;
    ++total_context_switches;
    return next->saved_frame;
}

int scheduler_init(void)
{
    size_t index;
    struct task *bootstrap;
    const paddr_t kernel_root = vmm_kernel_address_space();

    if (initialized || interrupts_enabled() || kernel_root == PADDR_INVALID) {
        return 0;
    }

    for (index = 0u; index < SCHEDULER_MAX_TASKS; ++index) {
        task_reset(&tasks[index]);
    }

    bootstrap = &tasks[0];
    bootstrap->id = 0u;
    bootstrap->name = "bootstrap";
    bootstrap->state = TASK_RUNNING;
    bootstrap->privilege = TASK_PRIVILEGE_KERNEL;
    bootstrap->kernel_stack_base = kernel_stack_bottom;
    bootstrap->kernel_stack_size = (size_t)(kernel_stack_top - kernel_stack_bottom);
    bootstrap->kernel_stack_top = (uintptr_t)kernel_stack_top;
    bootstrap->address_space = kernel_root;

    gdt_set_kernel_stack(bootstrap->kernel_stack_top);
    syscall_set_kernel_stack(bootstrap->kernel_stack_top);

    task_count_value = 1u;
    current_index = 0u;
    next_task_id = 1u;
    total_context_switches = 0u;
    total_preemptions = 0u;
    total_scheduling_ticks = 0u;
    total_user_fault_terminations = 0u;
    running = 0;
    initialized = 1;
    return 1;
}

static void release_terminated_task_slot(struct task *task)
{
    if (task == 0) {
        return;
    }

    if (task->privilege == TASK_PRIVILEGE_USER) {
        free_user_task_resources(task);
        return;
    }

    /* Slot zero is the permanent bootstrap task and is never reclaimed. */
    if (task->kernel_stack_base != 0 &&
        task->kernel_stack_base != (void *)kernel_stack_bottom) {
        kfree(task->kernel_stack_base);
    }

    task_reset(task);
}

static int task_parent_is_alive(const struct task *task)
{
    size_t index;

    if (task == 0 || task->parent_id == UINT64_MAX) {
        return 0;
    }

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].id == task->parent_id &&
            tasks[index].state != TASK_TERMINATED) {
            return 1;
        }
    }

    return 0;
}

static int task_slot_reclaimable(const struct task *task)
{
    if (task == 0 || task->state != TASK_TERMINATED) {
        return 0;
    }

    /*
     * Parentless historical phase tasks can be reused immediately. A real
     * child stays as a small zombie until waitpid() collects its status, unless
     * its parent has already died.
     */
    return task->parent_id == UINT64_MAX || task->wait_collected ||
        !task_parent_is_alive(task);
}

static struct task *prepare_creation_slot(size_t *slot_index_out)
{
    size_t index;

    if (slot_index_out == 0) {
        return 0;
    }

    for (index = 1u; index < task_count_value; ++index) {
        if (index != current_index && task_slot_reclaimable(&tasks[index])) {
            release_terminated_task_slot(&tasks[index]);
            *slot_index_out = index;
            return &tasks[index];
        }
    }

    if (task_count_value >= SCHEDULER_MAX_TASKS) {
        return 0;
    }

    index = task_count_value;
    task_reset(&tasks[index]);
    *slot_index_out = index;
    return &tasks[index];
}

static void commit_creation_slot(size_t slot_index)
{
    if (slot_index == task_count_value) {
        ++task_count_value;
    }
}

int task_create(
    const char *name,
    task_entry_t entry,
    void *argument,
    uint64_t *task_id_out
)
{
    struct task *task;
    size_t slot_index;

    if (!initialized || interrupts_enabled() || entry == 0) {
        return 0;
    }

    task = prepare_creation_slot(&slot_index);
    if (task == 0) {
        return 0;
    }

    if (!allocate_kernel_stack(task)) {
        return 0;
    }

    task->id = next_task_id++;
    task->name = name != 0 ? name : "kernel-thread";
    task->state = TASK_READY;
    task->privilege = TASK_PRIVILEGE_KERNEL;
    task->address_space = vmm_kernel_address_space();
    task->entry = entry;
    task->argument = argument;
    task->saved_frame = build_kernel_initial_frame(task);

    if (task_id_out != 0) {
        *task_id_out = task->id;
    }

    commit_creation_slot(slot_index);
    return 1;
}

static void free_user_task_resources(struct task *task)
{
    size_t index;

    task_close_all_files(task);

    if (task->address_space != PADDR_INVALID &&
        task->address_space != vmm_kernel_address_space()) {
        (void)vmm_destroy_user_address_space(task->address_space);
    }

    if (task->user_code_page != PADDR_INVALID) {
        (void)pmm_free_page(task->user_code_page);
    }

    if (task->user_data_page != PADDR_INVALID) {
        (void)pmm_free_page(task->user_data_page);
    }

    for (index = 0u; index < task->user_elf_page_count; ++index) {
        if (task->user_elf_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(task->user_elf_pages[index]);
        }
    }

    for (index = 0u; index < task->user_mmap_page_count; ++index) {
        if (task->user_mmap_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(task->user_mmap_pages[index]);
        }
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        if (task->user_stack_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(task->user_stack_pages[index]);
        }
    }

    if (task->kernel_stack_base != 0) {
        kfree(task->kernel_stack_base);
    }

    task_reset(task);
}

int user_task_create(
    const char *name,
    const void *image,
    size_t image_size,
    uint64_t *task_id_out
)
{
    struct task *task;
    uint8_t *code_bytes;
    uint8_t *data_bytes;
    size_t index;
    size_t slot_index;

    if (!initialized || interrupts_enabled() || image == 0 || image_size == 0u ||
        image_size > VMM_PAGE_SIZE) {
        return 0;
    }

    task = prepare_creation_slot(&slot_index);
    if (task == 0) {
        return 0;
    }

    if (!allocate_kernel_stack(task)) {
        return 0;
    }

    task->address_space = vmm_create_user_address_space();
    if (task->address_space == PADDR_INVALID) {
        free_user_task_resources(task);
        return 0;
    }

    task->user_code_page = pmm_alloc_page();
    task->user_data_page = pmm_alloc_page();

    if (task->user_code_page == PADDR_INVALID ||
        task->user_data_page == PADDR_INVALID) {
        free_user_task_resources(task);
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        task->user_stack_pages[index] = pmm_alloc_page();

        if (task->user_stack_pages[index] == PADDR_INVALID) {
            free_user_task_resources(task);
            return 0;
        }
    }

    code_bytes = (uint8_t *)pmm_phys_to_hhdm(task->user_code_page);
    data_bytes = (uint8_t *)pmm_phys_to_hhdm(task->user_data_page);

    if (code_bytes == 0 || data_bytes == 0) {
        free_user_task_resources(task);
        return 0;
    }

    bytes_clear(code_bytes, VMM_PAGE_SIZE);
    bytes_clear(data_bytes, VMM_PAGE_SIZE);
    bytes_copy(code_bytes, image, image_size);

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        void *stack_page = pmm_phys_to_hhdm(task->user_stack_pages[index]);

        if (stack_page == 0) {
            free_user_task_resources(task);
            return 0;
        }

        bytes_clear(stack_page, VMM_PAGE_SIZE);
    }

    if (!vmm_map_page_in_address_space(
            task->address_space,
            TASK_USER_CODE_BASE,
            task->user_code_page,
            VMM_FLAG_USER
        ) ||
        !vmm_map_page_in_address_space(
            task->address_space,
            TASK_USER_DATA_BASE,
            task->user_data_page,
            VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
        )) {
        free_user_task_resources(task);
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        if (!vmm_map_page_in_address_space(
                task->address_space,
                address,
                task->user_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) {
            free_user_task_resources(task);
            return 0;
        }
    }

    task->id = next_task_id++;
    task->name = name != 0 ? name : "user-task";
    task->state = TASK_READY;
    task->privilege = TASK_PRIVILEGE_USER;
    task->user_entry = TASK_USER_CODE_BASE;
    task->user_stack_top = TASK_USER_STACK_TOP;
    task->saved_frame = build_user_initial_frame(task);

    if (task_id_out != 0) {
        *task_id_out = task->id;
    }

    commit_creation_slot(slot_index);
    return 1;
}


int user_task_create_elf(
    const char *name,
    const void *elf_image,
    size_t elf_size,
    uint64_t *task_id_out
)
{
    struct task *task;
    struct elf64_load_result loaded;
    size_t index;
    size_t slot_index;

    if (!initialized || interrupts_enabled() || elf_image == 0 ||
        elf_size == 0u) {
        return 0;
    }

    task = prepare_creation_slot(&slot_index);
    if (task == 0) {
        return 0;
    }

    if (!allocate_kernel_stack(task)) {
        return 0;
    }

    task->address_space = vmm_create_user_address_space();
    if (task->address_space == PADDR_INVALID) {
        free_user_task_resources(task);
        return 0;
    }

    if (!elf64_load_executable(
            task->address_space,
            elf_image,
            elf_size,
            &loaded
        )) {
        free_user_task_resources(task);
        return 0;
    }

    if (loaded.page_count > TASK_USER_ELF_MAX_PAGES) {
        free_user_task_resources(task);
        return 0;
    }

    task->user_elf_page_count = loaded.page_count;
    for (index = 0u; index < loaded.page_count; ++index) {
        task->user_elf_pages[index] = loaded.pages[index].physical_address;
        task->user_elf_virtual_pages[index] = loaded.pages[index].virtual_address;
        task->user_elf_page_flags[index] = loaded.pages[index].vmm_flags;
    }
    for (index = 0u; index < TASK_USER_MMAP_MAX_PAGES; ++index) {
        task->user_mmap_pages[index] = PADDR_INVALID;
        task->user_mmap_virtual_pages[index] = 0u;
    }
    task->user_mmap_page_count = 0u;
    task->user_mmap_next = TASK_USER_MMAP_BASE;

    task->elf_program_headers = loaded.program_headers;
    task->elf_load_segments = loaded.load_segments;
    task->elf_file_bytes = loaded.file_bytes;
    task->elf_memory_bytes = loaded.memory_bytes;
    task->elf_image_size = elf_size;
    task->elf_backed = 1;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        void *stack_page;
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        task->user_stack_pages[index] = pmm_alloc_page();
        if (task->user_stack_pages[index] == PADDR_INVALID) {
            free_user_task_resources(task);
            return 0;
        }

        stack_page = pmm_phys_to_hhdm(task->user_stack_pages[index]);
        if (stack_page == 0) {
            free_user_task_resources(task);
            return 0;
        }
        bytes_clear(stack_page, VMM_PAGE_SIZE);

        if (!vmm_map_page_in_address_space(
                task->address_space,
                address,
                task->user_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) {
            free_user_task_resources(task);
            return 0;
        }
    }

    task->id = next_task_id++;
    task->name = name != 0 ? name : "elf-user-task";
    task->state = TASK_READY;
    task->privilege = TASK_PRIVILEGE_USER;
    task->user_entry = loaded.entry;
    task->user_stack_top = TASK_USER_STACK_TOP;
    task->saved_frame = build_user_initial_frame(task);

    if (task_id_out != 0) {
        *task_id_out = task->id;
    }

    commit_creation_slot(slot_index);
    return 1;
}


int scheduler_exec_current_elf(
    const void *elf_image,
    size_t elf_size,
    vaddr_t *entry_out,
    vaddr_t *stack_out
)
{
    struct task *task;
    struct elf64_load_result loaded;
    paddr_t new_root = PADDR_INVALID;
    paddr_t new_stack_pages[TASK_USER_STACK_PAGES];
    paddr_t old_stack_pages[TASK_USER_STACK_PAGES];
    paddr_t old_elf_pages[TASK_USER_ELF_MAX_PAGES];
    paddr_t old_mmap_pages[TASK_USER_MMAP_MAX_PAGES];
    paddr_t old_root;
    paddr_t old_code;
    paddr_t old_data;
    size_t old_elf_count;
    size_t old_mmap_count;
    size_t index;

    if (!initialized || !running || interrupts_enabled() ||
        current_index >= task_count_value || elf_image == 0 || elf_size == 0u ||
        entry_out == 0 || stack_out == 0) {
        return 0;
    }

    task = &tasks[current_index];
    if (task->privilege != TASK_PRIVILEGE_USER ||
        task->address_space == PADDR_INVALID) {
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        new_stack_pages[index] = PADDR_INVALID;
    }

    new_root = vmm_create_user_address_space();
    if (new_root == PADDR_INVALID) {
        return 0;
    }

    if (!elf64_load_executable(new_root, elf_image, elf_size, &loaded)) {
        (void)vmm_destroy_user_address_space(new_root);
        return 0;
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        void *stack_page;
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        new_stack_pages[index] = pmm_alloc_page();
        if (new_stack_pages[index] == PADDR_INVALID) {
            goto fail_new_image;
        }

        stack_page = pmm_phys_to_hhdm(new_stack_pages[index]);
        if (stack_page == 0) {
            goto fail_new_image;
        }
        bytes_clear(stack_page, VMM_PAGE_SIZE);

        if (!vmm_map_page_in_address_space(
                new_root,
                address,
                new_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) {
            goto fail_new_image;
        }
    }

    old_root = task->address_space;
    old_code = task->user_code_page;
    old_data = task->user_data_page;
    old_elf_count = task->user_elf_page_count;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        old_stack_pages[index] = task->user_stack_pages[index];
    }
    for (index = 0u; index < TASK_USER_ELF_MAX_PAGES; ++index) {
        old_elf_pages[index] = task->user_elf_pages[index];
    }
    old_mmap_count = task->user_mmap_page_count;
    for (index = 0u; index < TASK_USER_MMAP_MAX_PAGES; ++index) {
        old_mmap_pages[index] = task->user_mmap_pages[index];
    }

    if (!vmm_activate_address_space(new_root)) {
        goto fail_new_image;
    }

    task->address_space = new_root;
    task->user_code_page = PADDR_INVALID;
    task->user_data_page = PADDR_INVALID;

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        task->user_stack_pages[index] = new_stack_pages[index];
    }
    for (index = 0u; index < TASK_USER_ELF_MAX_PAGES; ++index) {
        task->user_elf_pages[index] = PADDR_INVALID;
        task->user_elf_virtual_pages[index] = 0u;
        task->user_elf_page_flags[index] = 0u;
    }
    task->user_elf_page_count = loaded.page_count;
    for (index = 0u; index < loaded.page_count; ++index) {
        task->user_elf_pages[index] = loaded.pages[index].physical_address;
        task->user_elf_virtual_pages[index] = loaded.pages[index].virtual_address;
        task->user_elf_page_flags[index] = loaded.pages[index].vmm_flags;
    }
    for (index = 0u; index < TASK_USER_MMAP_MAX_PAGES; ++index) {
        task->user_mmap_pages[index] = PADDR_INVALID;
        task->user_mmap_virtual_pages[index] = 0u;
    }
    task->user_mmap_page_count = 0u;
    task->user_mmap_next = TASK_USER_MMAP_BASE;

    task->elf_program_headers = loaded.program_headers;
    task->elf_load_segments = loaded.load_segments;
    task->elf_file_bytes = loaded.file_bytes;
    task->elf_memory_bytes = loaded.memory_bytes;
    task->elf_image_size = elf_size;
    task->elf_backed = 1;
    task->user_entry = loaded.entry;
    task->user_stack_top = TASK_USER_STACK_TOP;
    task->fault_vector = 0u;
    task->fault_error_code = 0u;
    task->fault_address = 0u;
    task->exit_code = 0;
    task->wake_tick = 0u;

    /* The new root is active, so the old user page tables can now be released. */
    if (old_root != vmm_kernel_address_space()) {
        (void)vmm_destroy_user_address_space(old_root);
    }
    if (old_code != PADDR_INVALID) {
        (void)pmm_free_page(old_code);
    }
    if (old_data != PADDR_INVALID) {
        (void)pmm_free_page(old_data);
    }
    for (index = 0u; index < old_elf_count; ++index) {
        if (old_elf_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(old_elf_pages[index]);
        }
    }
    for (index = 0u; index < old_mmap_count; ++index) {
        if (old_mmap_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(old_mmap_pages[index]);
        }
    }
    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        if (old_stack_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(old_stack_pages[index]);
        }
    }

    *entry_out = loaded.entry;
    *stack_out = TASK_USER_STACK_TOP - 8ULL;
    return 1;

fail_new_image:
    /* Tear down page tables first, then release the leaf physical frames. */
    if (vmm_current_address_space() == new_root) {
        (void)vmm_activate_address_space(task->address_space);
    }
    (void)vmm_destroy_user_address_space(new_root);

    for (index = 0u; index < loaded.page_count; ++index) {
        if (loaded.pages[index].physical_address != PADDR_INVALID) {
            (void)pmm_free_page(loaded.pages[index].physical_address);
        }
    }
    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        if (new_stack_pages[index] != PADDR_INVALID) {
            (void)pmm_free_page(new_stack_pages[index]);
        }
    }
    return 0;
}


static int clone_leaf_page(
    paddr_t destination_root,
    vaddr_t virtual_address,
    paddr_t source_page,
    uint64_t flags,
    paddr_t *destination_page_out
)
{
    paddr_t destination_page;
    void *source_bytes;
    void *destination_bytes;

    if (source_page == PADDR_INVALID || destination_page_out == 0) {
        return 0;
    }

    destination_page = pmm_alloc_page();
    if (destination_page == PADDR_INVALID) {
        return 0;
    }

    source_bytes = pmm_phys_to_hhdm(source_page);
    destination_bytes = pmm_phys_to_hhdm(destination_page);
    if (source_bytes == 0 || destination_bytes == 0) {
        (void)pmm_free_page(destination_page);
        return 0;
    }

    bytes_copy(destination_bytes, source_bytes, VMM_PAGE_SIZE);

    if (!vmm_map_page_in_address_space(
            destination_root,
            virtual_address,
            destination_page,
            flags
        )) {
        (void)pmm_free_page(destination_page);
        return 0;
    }

    *destination_page_out = destination_page;
    return 1;
}

int scheduler_mmap_current(size_t length, vaddr_t *address_out)
{
    struct task *task;
    size_t pages;
    size_t start_index;
    size_t mapped = 0u;
    vaddr_t base;
    size_t index;

    if (!initialized || !running || interrupts_enabled() || length == 0u ||
        address_out == 0 || current_index >= task_count_value) {
        return 0;
    }

    task = &tasks[current_index];
    if (task->privilege != TASK_PRIVILEGE_USER || task->address_space == PADDR_INVALID) {
        return 0;
    }

    if (length > (size_t)TASK_USER_MMAP_MAX_PAGES * VMM_PAGE_SIZE) return 0;
    pages = (length + VMM_PAGE_SIZE - 1u) / VMM_PAGE_SIZE;
    if (pages == 0u || task->user_mmap_page_count + pages > TASK_USER_MMAP_MAX_PAGES) return 0;
    if (task->user_mmap_next > TASK_USER_STACK_TOP - (vaddr_t)(pages * VMM_PAGE_SIZE)) return 0;

    base = task->user_mmap_next;
    start_index = task->user_mmap_page_count;

    for (index = 0u; index < pages; ++index) {
        paddr_t page = pmm_alloc_page();
        void *bytes;
        vaddr_t address = base + (vaddr_t)(index * VMM_PAGE_SIZE);
        if (page == PADDR_INVALID) goto fail;
        bytes = pmm_phys_to_hhdm(page);
        if (bytes == 0) { (void)pmm_free_page(page); goto fail; }
        bytes_clear(bytes, VMM_PAGE_SIZE);
        if (!vmm_map_page_in_address_space(
                task->address_space,
                address,
                page,
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE
            )) {
            (void)pmm_free_page(page);
            goto fail;
        }
        task->user_mmap_pages[start_index + index] = page;
        task->user_mmap_virtual_pages[start_index + index] = address;
        ++mapped;
    }

    task->user_mmap_page_count += pages;
    task->user_mmap_next += (vaddr_t)(pages * VMM_PAGE_SIZE);
    *address_out = base;
    return 1;

fail:
    for (index = 0u; index < mapped; ++index) {
        size_t slot = start_index + index;
        (void)vmm_unmap_page_in_address_space(task->address_space, task->user_mmap_virtual_pages[slot]);
        if (task->user_mmap_pages[slot] != PADDR_INVALID) (void)pmm_free_page(task->user_mmap_pages[slot]);
        task->user_mmap_pages[slot] = PADDR_INVALID;
        task->user_mmap_virtual_pages[slot] = 0u;
    }
    return 0;
}

static struct interrupt_frame *build_fork_child_frame(
    struct task *child,
    const struct syscall_frame *parent_frame
)
{
    uintptr_t address;
    struct interrupt_frame *frame;

    address = child->kernel_stack_top - sizeof(struct interrupt_frame);
    frame = (struct interrupt_frame *)address;
    frame_clear(frame);

    frame->r15 = parent_frame->r15;
    frame->r14 = parent_frame->r14;
    frame->r13 = parent_frame->r13;
    frame->r12 = parent_frame->r12;
    frame->r11 = parent_frame->user_rflags;
    frame->r10 = parent_frame->r10;
    frame->r9 = parent_frame->r9;
    frame->r8 = parent_frame->r8;
    frame->rbp = parent_frame->rbp;
    frame->rdi = parent_frame->rdi;
    frame->rsi = parent_frame->rsi;
    frame->rdx = parent_frame->rdx;
    frame->rcx = parent_frame->user_rip;
    frame->rbx = parent_frame->rbx;
    frame->rax = 0u; /* fork() returns zero in the child. */
    frame->rip = parent_frame->user_rip;
    frame->cs = GDT_USER_CODE;
    frame->rflags = parent_frame->user_rflags;
    frame->rsp = parent_frame->user_rsp;
    frame->ss = GDT_USER_DATA;
    return frame;
}

int scheduler_fork_current(
    const struct syscall_frame *parent_frame,
    uint64_t *child_id_out
)
{
    struct task *parent;
    struct task *child;
    size_t slot_index;
    size_t index;

    if (!initialized || !running || interrupts_enabled() ||
        parent_frame == 0 || current_index >= task_count_value) {
        return 0;
    }

    parent = &tasks[current_index];
    if (parent->privilege != TASK_PRIVILEGE_USER ||
        parent->address_space == PADDR_INVALID) {
        return 0;
    }

    child = prepare_creation_slot(&slot_index);
    if (child == 0 || !allocate_kernel_stack(child)) {
        return 0;
    }

    child->address_space = vmm_create_user_address_space();
    if (child->address_space == PADDR_INVALID) {
        free_user_task_resources(child);
        return 0;
    }

    if (parent->user_code_page != PADDR_INVALID &&
        !clone_leaf_page(
            child->address_space,
            TASK_USER_CODE_BASE,
            parent->user_code_page,
            VMM_FLAG_USER,
            &child->user_code_page
        )) {
        free_user_task_resources(child);
        return 0;
    }

    if (parent->user_data_page != PADDR_INVALID &&
        !clone_leaf_page(
            child->address_space,
            TASK_USER_DATA_BASE,
            parent->user_data_page,
            VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE,
            &child->user_data_page
        )) {
        free_user_task_resources(child);
        return 0;
    }

    child->user_elf_page_count = parent->user_elf_page_count;
    for (index = 0u; index < parent->user_elf_page_count; ++index) {
        if (!clone_leaf_page(
                child->address_space,
                parent->user_elf_virtual_pages[index],
                parent->user_elf_pages[index],
                parent->user_elf_page_flags[index],
                &child->user_elf_pages[index]
            )) {
            free_user_task_resources(child);
            return 0;
        }
        child->user_elf_virtual_pages[index] =
            parent->user_elf_virtual_pages[index];
        child->user_elf_page_flags[index] = parent->user_elf_page_flags[index];
    }

    child->user_mmap_page_count = parent->user_mmap_page_count;
    child->user_mmap_next = parent->user_mmap_next;
    for (index = 0u; index < parent->user_mmap_page_count; ++index) {
        if (!clone_leaf_page(
                child->address_space,
                parent->user_mmap_virtual_pages[index],
                parent->user_mmap_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE,
                &child->user_mmap_pages[index]
            )) {
            free_user_task_resources(child);
            return 0;
        }
        child->user_mmap_virtual_pages[index] = parent->user_mmap_virtual_pages[index];
    }

    for (index = 0u; index < TASK_USER_STACK_PAGES; ++index) {
        const vaddr_t address =
            TASK_USER_STACK_TOP -
            (vaddr_t)((TASK_USER_STACK_PAGES - index) * VMM_PAGE_SIZE);

        if (!clone_leaf_page(
                child->address_space,
                address,
                parent->user_stack_pages[index],
                VMM_FLAG_USER | VMM_FLAG_WRITABLE | VMM_FLAG_NO_EXECUTE,
                &child->user_stack_pages[index]
            )) {
            free_user_task_resources(child);
            return 0;
        }
    }

    if (!task_share_files(child, parent)) {
        free_user_task_resources(child);
        return 0;
    }

    child->id = next_task_id++;
    child->parent_id = parent->id;
    child->name = parent->name != 0 ? parent->name : "fork-child";
    child->state = TASK_READY;
    child->privilege = TASK_PRIVILEGE_USER;
    child->elf_program_headers = parent->elf_program_headers;
    child->elf_load_segments = parent->elf_load_segments;
    child->elf_file_bytes = parent->elf_file_bytes;
    child->elf_memory_bytes = parent->elf_memory_bytes;
    child->elf_image_size = parent->elf_image_size;
    child->elf_backed = parent->elf_backed;
    child->user_entry = parent->user_entry;
    child->user_stack_top = parent->user_stack_top;
    child->saved_frame = build_fork_child_frame(child, parent_frame);
    child->wait_collected = 0;

    if (child_id_out != 0) {
        *child_id_out = child->id;
    }

    commit_creation_slot(slot_index);
    return 1;
}

static void wake_waiters_for_child(uint64_t child_id)
{
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].state == TASK_BLOCKED &&
            tasks[index].wait_target_id == child_id) {
            tasks[index].state = TASK_READY;
            tasks[index].wait_target_id = UINT64_MAX;
        }
    }
}

int scheduler_wait_current_child(uint64_t child_id, int64_t *status_out)
{
    struct task *parent;
    struct task *child;
    const uint64_t parent_id =
        current_index < task_count_value ? tasks[current_index].id : UINT64_MAX;

    if (!initialized || !running || interrupts_enabled() ||
        current_index >= task_count_value || status_out == 0) {
        return 0;
    }

    parent = &tasks[current_index];

    for (;;) {
        child = scheduler_task_by_id_mutable(child_id);
        if (child == 0 || child->parent_id != parent_id || child->wait_collected) {
            return 0;
        }

        if (child->state == TASK_TERMINATED) {
            *status_out = child->exit_code;
            child->wait_collected = 1;
            return 1;
        }

        parent->state = TASK_BLOCKED;
        parent->wait_target_id = child_id;
        parent->ticks_in_slice = parent->quantum_ticks;

        interrupts_enable();
        for (;;) {
            __asm__ volatile ("hlt" ::: "memory");

            if (current_index < task_count_value &&
                tasks[current_index].id == parent_id &&
                tasks[current_index].state == TASK_RUNNING) {
                break;
            }
        }
        interrupts_disable();
    }
}

int scheduler_prepare_block_current(uint64_t expected_task_id)
{
    struct task *current;

    if (!initialized || !running || interrupts_enabled() ||
        current_index >= task_count_value) {
        return 0;
    }

    current = &tasks[current_index];
    if (current->id != expected_task_id || current->state != TASK_RUNNING) {
        return 0;
    }

    current->state = TASK_BLOCKED;
    current->wait_target_id = UINT64_MAX;
    current->ticks_in_slice = current->quantum_ticks;
    return 1;
}

int scheduler_park_current(uint64_t expected_task_id)
{
    if (!initialized || !running || interrupts_enabled()) {
        return 0;
    }

    /*
     * The caller has already made the current task non-runnable while IF=0.
     * Enabling interrupts lets the next APIC timer interrupt switch away. A
     * wakeup changes this task to READY; once scheduled again it is RUNNING
     * and execution resumes here.
     */
    interrupts_enable();

    for (;;) {
        if (current_index < task_count_value &&
            tasks[current_index].id == expected_task_id &&
            tasks[current_index].state == TASK_RUNNING) {
            break;
        }

        __asm__ volatile ("hlt" ::: "memory");
    }

    interrupts_disable();
    return 1;
}

int scheduler_wake_task(uint64_t task_id)
{
    struct task *task;

    if (!initialized || !running || interrupts_enabled()) {
        return 0;
    }

    task = scheduler_task_by_id_mutable(task_id);
    if (task == 0 || task->state != TASK_BLOCKED) {
        return 0;
    }

    task->state = TASK_READY;
    task->wait_target_id = UINT64_MAX;
    return 1;
}

int scheduler_terminate_task(uint64_t task_id, uint32_t signal_number)
{
    struct task *target;

    if (!initialized || !running || interrupts_enabled() || signal_number == 0u) {
        return 0;
    }

    target = scheduler_task_by_id_mutable(task_id);
    if (target == 0 || target->privilege != TASK_PRIVILEGE_USER ||
        target->state == TASK_TERMINATED) {
        return 0;
    }

    if (current_index < task_count_value && target == &tasks[current_index]) {
        task_exit_current((int64_t)(128u + signal_number));
    }

    task_close_all_files(target);
    target->termination_signal = signal_number;
    target->exit_code = (int64_t)(128u + signal_number);
    target->state = TASK_TERMINATED;
    target->wake_tick = 0u;
    target->wait_target_id = UINT64_MAX;
    wake_waiters_for_child(target->id);
    return 1;
}


int scheduler_start(void)
{
    if (!initialized || running || interrupts_enabled() || task_count_value < 2u) {
        return 0;
    }

    running = 1;
    return 1;
}

int scheduler_running(void)
{
    return running;
}

static void wake_sleeping_tasks(void)
{
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        struct task *task = &tasks[index];

        if (task->state == TASK_SLEEPING &&
            task->wake_tick <= total_scheduling_ticks) {
            task->state = TASK_READY;
            task->wake_tick = 0u;
        }
    }
}


struct interrupt_frame *scheduler_on_timer_interrupt(struct interrupt_frame *frame)
{
    struct task *current;
    size_t next_index;

    if (!running || frame == 0 || task_count_value == 0u) {
        return frame;
    }

    current = &tasks[current_index];
    current->saved_frame = frame;

    ++total_scheduling_ticks;
    wake_sleeping_tasks();
    ++current->runtime_ticks;
    ++current->ticks_in_slice;

    if (current->state == TASK_RUNNING &&
        current->ticks_in_slice < current->quantum_ticks) {
        return frame;
    }

    next_index = find_next_ready(current_index);

    if (next_index == SCHEDULER_MAX_TASKS) {
        if (current->state == TASK_RUNNING) {
            current->ticks_in_slice = 0u;
            return frame;
        }

        kernel_panic("scheduler has no runnable task");
    }

    if (current->state == TASK_RUNNING) {
        current->state = TASK_READY;
    }
    current->ticks_in_slice = 0u;
    ++total_preemptions;
    return switch_to(next_index);
}

struct interrupt_frame *scheduler_handle_user_fault(
    struct interrupt_frame *frame,
    uint64_t vector,
    uint64_t error_code,
    vaddr_t fault_address
)
{
    struct task *current;
    size_t next_index;

    if (!running || frame == 0 || current_index >= task_count_value) {
        return 0;
    }

    current = &tasks[current_index];

    if (current->privilege != TASK_PRIVILEGE_USER ||
        (frame->cs & 3ULL) != 3ULL) {
        return 0;
    }

    current->saved_frame = frame;
    task_close_all_files(current);
    current->state = TASK_TERMINATED;
    current->fault_vector = vector;
    current->fault_error_code = error_code;
    current->fault_address = fault_address;
    current->exit_code = (int64_t)(128u + vector);
    current->termination_signal = 0u;
    wake_waiters_for_child(current->id);
    ++total_user_fault_terminations;

    next_index = find_next_ready(current_index);
    if (next_index == SCHEDULER_MAX_TASKS) {
        return 0;
    }

    return switch_to(next_index);
}

int scheduler_sleep_current(uint64_t ticks)
{
    struct task *current;
    const uint64_t task_id =
        (current_index < task_count_value) ? tasks[current_index].id : UINT64_MAX;

    if (!running || !initialized || current_index >= task_count_value ||
        interrupts_enabled()) {
        return 0;
    }

    if (ticks == 0u) {
        return scheduler_yield_current();
    }

    current = &tasks[current_index];
    current->state = TASK_SLEEPING;
    current->wake_tick = total_scheduling_ticks + ticks;
    current->ticks_in_slice = current->quantum_ticks;

    interrupts_enable();

    for (;;) {
        __asm__ volatile ("hlt" ::: "memory");

        if (current_index < task_count_value &&
            tasks[current_index].id == task_id &&
            tasks[current_index].state == TASK_RUNNING &&
            total_scheduling_ticks >= current->wake_tick) {
            break;
        }
    }

    interrupts_disable();
    current->wake_tick = 0u;
    return 1;
}

int scheduler_yield_current(void)
{
    struct task *current;
    const uint64_t before_switches = total_context_switches;
    const uint64_t task_id =
        (current_index < task_count_value) ? tasks[current_index].id : UINT64_MAX;

    if (!running || !initialized || current_index >= task_count_value ||
        interrupts_enabled()) {
        return 0;
    }

    current = &tasks[current_index];
    current->ticks_in_slice = current->quantum_ticks;

    interrupts_enable();

    while (total_context_switches == before_switches ||
           current_index >= task_count_value ||
           tasks[current_index].id != task_id) {
        __asm__ volatile ("hlt" ::: "memory");
    }

    interrupts_disable();
    return 1;
}

_Noreturn void task_exit_current(int64_t status)
{
    if (!running || !initialized || current_index >= task_count_value) {
        kernel_panic("task_exit_current called without a running scheduler");
    }

    interrupts_disable();
    task_close_all_files(&tasks[current_index]);
    tasks[current_index].exit_code = status;
    tasks[current_index].termination_signal = 0u;
    tasks[current_index].state = TASK_TERMINATED;
    tasks[current_index].ticks_in_slice = tasks[current_index].quantum_ticks;
    wake_waiters_for_child(tasks[current_index].id);

    for (;;) {
        __asm__ volatile ("sti; hlt" ::: "memory");
    }
}

struct task *scheduler_current_task_mutable(void)
{
    if (!initialized || current_index >= task_count_value) {
        return 0;
    }

    return &tasks[current_index];
}

const struct task *scheduler_current_task(void)
{
    return scheduler_current_task_mutable();
}

struct scheduler_stats scheduler_get_stats(void)
{
    struct scheduler_stats stats;
    uint64_t runnable = 0u;
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].state == TASK_RUNNING ||
            tasks[index].state == TASK_READY) {
            ++runnable;
        }
    }

    stats.task_count = task_count_value;
    stats.runnable_tasks = runnable;
    stats.context_switches = total_context_switches;
    stats.preemptions = total_preemptions;
    stats.scheduling_ticks = total_scheduling_ticks;
    stats.user_fault_terminations = total_user_fault_terminations;
    stats.current_task_id =
        task_count_value != 0u ? tasks[current_index].id : UINT64_MAX;
    return stats;
}

size_t scheduler_task_count(void)
{
    return task_count_value;
}

const struct task *scheduler_task_at(size_t index)
{
    if (index >= task_count_value) {
        return 0;
    }

    return &tasks[index];
}

struct task *scheduler_task_by_id_mutable(uint64_t id)
{
    size_t index;

    for (index = 0u; index < task_count_value; ++index) {
        if (tasks[index].id == id) {
            return &tasks[index];
        }
    }

    return 0;
}

const struct task *scheduler_task_by_id(uint64_t id)
{
    return scheduler_task_by_id_mutable(id);
}
```

## `tests/phase19_libc.py`

```python
#!/usr/bin/env python3
"""Validate Phase-19 libaxiom, anonymous mmap, and libc-linked Ring-3 programs."""
from pathlib import Path
import socket
import subprocess
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
            raise AssertionError(f"phase19 kernel panicked\n--- serial tail ---\n{tail}")
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-180:])
            raise AssertionError(f"phase19 timed out waiting for {marker!r}\n--- serial tail ---\n{tail}")
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


def send_command(monitor_path, command):
    mapping = {" ": "spc", "/": "slash", ".": "dot", "-": "minus", "_": "shift-minus"}
    keys = []
    for character in command:
        if "a" <= character <= "z" or "0" <= character <= "9":
            keys.append(character)
        else:
            require(character in mapping, f"phase19: no HMP key mapping for {character!r}")
            keys.append(mapping[character])
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def test_phase19():
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    with (build / "phase19-build.log").open("w") as output:
        subprocess.run(["make", "MODE=normal", "all"], cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, check=True)

    serial = build / "phase19-serial.log"
    monitor = build / "phase19-monitor.sock"
    qemu_log = build / "phase19-qemu.log"
    serial.write_text("")
    monitor.unlink(missing_ok=True)

    command = [
        "qemu-system-x86_64", "-machine", "q35", "-accel", "tcg", "-smp", "4", "-m", "256M",
        "-cdrom", str(build / "AxiomOS.iso"), "-boot", "d",
        "-netdev", "user,id=net0,ipv6=off",
        "-device", "e1000,netdev=net0,mac=52:54:00:12:34:56",
        "-display", "none", "-serial", f"file:{serial}",
        "-monitor", f"unix:{monitor},server=on,wait=off", "-no-reboot", "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(command, cwd=ROOT, stdout=output, stderr=subprocess.STDOUT)
        try:
            text = wait_for_text(process, serial, "AxiomOS shell ready. Type 'help' for commands.", time.monotonic() + 240)
            require("AxiomOS Phase 19 userspace libc available." in text, "phase19: boot banner missing")
            require("Phase 19 userspace library initialization complete." in text, "phase19: libc init marker missing")
            send_command(monitor, "phase19-demo")
            text = wait_for_text(process, serial, "Phase 19 libc demo complete:", time.monotonic() + 90)
            text = wait_for_text(process, serial, "[process ", time.monotonic() + 20)
            final = serial_text(serial)
            for marker in [
                "Phase 19 stdio formatting: OK",
                "Phase 19 string/memory routines: OK",
                "Phase 19 ctype/numeric conversion: OK",
                "Phase 19 malloc/calloc/realloc/free: OK",
                "Phase 19 malloc-backed fork isolation: OK",
                "Phase 19 mmap-backed exec cleanup: OK",
                "Phase 19 unistd/file wrappers: OK",
                "Phase 19 process wrappers: OK",
                "Phase 19 network syscall wrapper: OK",
            ]:
                require(marker in final, f"phase19: missing {marker!r}")
            require("exited 19]" in final, "phase19: demo did not exit with status 19")
            require("FAILED" not in "\n".join(line for line in final.splitlines() if line.startswith("Phase 19")), "phase19: demo reported failure")
            require("KERNEL PANIC" not in final, "phase19: kernel panicked")
            print("PASS: Phase 19 libaxiom stdio/string/ctype/stdlib")
            print("PASS: Phase 19 anonymous mmap-backed malloc/calloc/realloc/free")
            print("PASS: Phase 19 malloc pages preserve fork isolation")
            print("PASS: Phase 19 exec discards inherited anonymous mappings safely")
            print("PASS: Phase 19 unistd/file/process/network syscall wrappers")
            print("PASS: Phase 19 existing shell remains interactive with libc linkage")
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill(); process.wait()
            monitor.unlink(missing_ok=True)


if __name__ == "__main__":
    test_phase19()
    print("PASS: all Phase 19 userspace libc tests.")
```

## `userspace/phase14_shell.c`

```c
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

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
    return strlen(text);
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
    if (count != 0u) (void)write(1, text, count);
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
        long got = read(0, &character, 1u);
        if (got < 0) return 0u;
        if (got == 0) {
            (void)sleep(10u);
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

int main(void)
{
    char line[SHELL_LINE_MAX];

    print("AxiomOS shell ready. Type 'help' for commands.\n");
    for (;;) {
        print("axiom> ");
        (void)read_line(line, sizeof(line));
        execute_line(line);
    }
    return 0;
}
```

## `userspace/phase15_demo.c`

```c
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static volatile uint64_t fork_marker = 0x1111111111111111ULL;

int main(void)
{
    const long parent_pid = getpid();
    long child;
    long status = -1;

    puts("Phase 15 fork demo starting.");
    child = fork();
    if (child < 0) { puts("Phase 15 fork: FAILED"); return 80; }
    if (child == 0) {
        if (getppid() != parent_pid) _exit(81);
        puts("Phase 15 child: fork returned 0; exec follows.");
        if (exec("/bin/phase11-demo") < 0) _exit(82);
        _exit(83);
    }
    if (waitpid(child, &status) != child || status != 11) { puts("Phase 15 fork/exec/wait: FAILED"); return 84; }
    puts("Phase 15 fork/exec/wait: OK");

    status = -1;
    child = fork();
    if (child < 0) { puts("Phase 15 second fork: FAILED"); return 85; }
    if (child == 0) { fork_marker = 0x2222222222222222ULL; (void)sleep(150); _exit(42); }
    if (waitpid(child, &status) != child || status != 42) { puts("Phase 15 blocked wait/sleep: FAILED"); return 86; }
    puts("Phase 15 blocked wait/sleep: OK");
    if (fork_marker != 0x1111111111111111ULL) { puts("Phase 15 fork memory isolation: FAILED"); return 87; }
    puts("Phase 15 fork memory isolation: OK");
    puts("Phase 15 process management demo complete.");
    return 15;
}
```

## `userspace/phase15_sleeper.c`

```c
#include <stdio.h>
#include <unistd.h>
int main(void)
{
    puts("Phase 15 sleeper started; waiting for SIGTERM.");
    (void)sleep(60000);
    puts("Phase 15 sleeper woke naturally.");
    return 0;
}
```

## `userspace/phase19_demo.c`

```c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <axiom/syscalls.h>
#include <axiom/abi/fs.h>

static int check(int condition, const char *name)
{
    if (!condition) {
        printf("Phase 19 %s: FAILED\n", name);
        return 0;
    }
    printf("Phase 19 %s: OK\n", name);
    return 1;
}

int main(void)
{
    char format[128];
    char moved[16] = "abcdef";
    char filebuf[64];
    const char payload[] = "libaxiom file round-trip";
    struct axiom_stat st;
    struct axiom_net_info net;
    int fd;
    long child;
    long status = -1;
    uint8_t *heap;
    uint8_t *grown;
    uint8_t *zeros;
    size_t i;

    printf("AxiomOS Phase 19 userspace libc demo.\n");

    snprintf(format, sizeof(format), "printf %s %d %u %x %p", "works", -19, 19u, 0x19u, (void *)(uintptr_t)0x4019u);
    if (!check(strcmp(format, "printf works -19 19 19 0x4019") == 0, "stdio formatting")) return 91;

    memmove(moved + 2, moved, 6u);
    if (!check(strlen("axiom") == 5u && strcmp("abc", "abc") == 0 &&
               strncmp("abcdef", "abcxyz", 3u) == 0 &&
               memcmp(moved, "ababcdef", 8u) == 0,
               "string/memory routines")) return 92;

    if (!check(isalpha('A') && isdigit('7') && isspace(' ') &&
               toupper('q') == 'Q' && tolower('Z') == 'z' &&
               atoi("-123") == -123 && strtol("0x2A", 0, 0) == 42,
               "ctype/numeric conversion")) return 93;

    heap = (uint8_t *)malloc(6000u);
    zeros = (uint8_t *)calloc(128u, 1u);
    if (heap == 0 || zeros == 0 || ((uintptr_t)heap & 15u) != 0u || ((uintptr_t)zeros & 15u) != 0u) return 94;
    for (i = 0u; i < 6000u; ++i) heap[i] = (uint8_t)(i ^ 0x5Au);
    for (i = 0u; i < 128u; ++i) if (zeros[i] != 0u) return 95;
    grown = (uint8_t *)realloc(heap, 12000u);
    if (grown == 0) return 96;
    for (i = 0u; i < 6000u; ++i) if (grown[i] != (uint8_t)(i ^ 0x5Au)) return 97;
    free(zeros);
    free(grown);
    heap = (uint8_t *)malloc(256u);
    if (!check(heap != 0, "malloc/calloc/realloc/free")) return 98;
    memset(heap, 0x3Cu, 256u);

    child = fork();
    if (child < 0) return 99;
    if (child == 0) {
        heap[0] = 0x99u;
        _exit(19);
    }
    if (waitpid(child, &status) != child || status != 19 || heap[0] != 0x3Cu) return 100;
    if (!check(1, "malloc-backed fork isolation")) return 101;

    status = -1;
    child = fork();
    if (child < 0) return 112;
    if (child == 0) {
        if (exec("/bin/phase11-demo") < 0) _exit(113);
        _exit(114);
    }
    if (waitpid(child, &status) != child || status != 11 || heap[0] != 0x3Cu) return 115;
    if (!check(1, "mmap-backed exec cleanup")) return 116;
    free(heap);

    fd = open("/tmp/phase19-libc.txt", O_CREAT | O_RDWR | O_TRUNC);
    if (fd < 0) return 102;
    if (write(fd, payload, sizeof(payload) - 1u) != (long)(sizeof(payload) - 1u)) return 103;
    if (lseek(fd, 0, AXIOM_SEEK_SET) != 0) return 104;
    memset(filebuf, 0, sizeof(filebuf));
    if (read(fd, filebuf, sizeof(payload) - 1u) != (long)(sizeof(payload) - 1u)) return 105;
    if (close(fd) < 0 || stat("/tmp/phase19-libc.txt", &st) < 0) return 106;
    if (!check(strcmp(filebuf, payload) == 0 && st.size == sizeof(payload) - 1u, "unistd/file wrappers")) return 107;

    if (getpid() <= 0 || getppid() < 0 || yield() < 0 || sleep(1u) < 0) return 108;
    if (!check(1, "process wrappers")) return 109;

    if (axiom_netinfo(&net) < 0 || net.address == 0u) return 110;
    if (!check(1, "network syscall wrapper")) return 111;

    printf("Phase 19 libc demo complete: pid=%ld IPv4=%u.%u.%u.%u\n",
           getpid(),
           (net.address >> 24) & 0xFFu,
           (net.address >> 16) & 0xFFu,
           (net.address >> 8) & 0xFFu,
           net.address & 0xFFu);
    return 19;
}
```
