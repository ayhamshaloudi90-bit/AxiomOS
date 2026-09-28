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
USER_PHASE20_C_OBJ := $(USER_DIR)/phase20_demo.o
USER_PHASE20_ELF := $(USER_DIR)/phase20_demo.elf
USER_PHASE20_WX_OBJ := $(USER_DIR)/phase20_wx.o
USER_PHASE20_WX_ELF := $(USER_DIR)/phase20_wx.elf
USER_PHASE21_C_OBJ := $(USER_DIR)/phase21_schedbench.o
USER_PHASE21_ELF := $(USER_DIR)/schedbench.elf
USER_PHASE22_DEMO_C_OBJ := $(USER_DIR)/phase22_ipc_demo.o
USER_PHASE22_DEMO_ELF := $(USER_DIR)/ipcdemo.elf
USER_PHASE22_PEER_C_OBJ := $(USER_DIR)/phase22_ipc_peer.o
USER_PHASE22_PEER_ELF := $(USER_DIR)/ipc-peer.elf
USER_PHASE23_C_OBJ := $(USER_DIR)/phase23_gfx_demo.o
USER_PHASE23_ELF := $(USER_DIR)/gfxdemo.elf
USER_PHASE24_IFCONFIG_C_OBJ := $(USER_DIR)/phase24_ifconfig.o
USER_PHASE24_IFCONFIG_ELF := $(USER_DIR)/ifconfig.elf
USER_PHASE24_PING_C_OBJ := $(USER_DIR)/phase24_ping.o
USER_PHASE24_PING_ELF := $(USER_DIR)/ping.elf
USER_PHASE24_DNS_C_OBJ := $(USER_DIR)/phase24_dnslookup.o
USER_PHASE24_DNS_ELF := $(USER_DIR)/dnslookup.elf
USER_PHASE24_HTTPGET_C_OBJ := $(USER_DIR)/phase24_httpget.o
USER_PHASE24_HTTPGET_ELF := $(USER_DIR)/httpget.elf
USER_PHASE24_HTTPD_C_OBJ := $(USER_DIR)/phase24_httpd.o
USER_PHASE24_HTTPD_ELF := $(USER_DIR)/httpd.elf
USER_PHASE25_SYSINFO_C_OBJ := $(USER_DIR)/phase25_sysinfo.o
USER_PHASE25_SYSINFO_ELF := $(USER_DIR)/sysinfo.elf
USER_PHASE26_DESKTOP_C_OBJ := $(USER_DIR)/phase26_desktop.o
USER_PHASE26_DESKTOP_ELF := $(USER_DIR)/desktop.elf
LIBC_DIR := $(USER_DIR)/libc
LIBC_CRT0_OBJ := $(LIBC_DIR)/crt0.o
LIBC_ARCHIVE := $(USER_DIR)/libaxiom.a
LIBC_SOURCES := libc/src/syscall.c libc/src/unistd.c libc/src/string.c libc/src/ctype.c libc/src/stdlib.c libc/src/stdio.c libc/src/graphics.c
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
	drivers/mouse/ps2_mouse.c \
	drivers/net/e1000.c \
	arch/x86_64/pci/pci.c \
	drivers/storage/block.c \
	drivers/storage/ahci.c \
	process/task.c \
	process/scheduler.c \
	kernel/ipc/ipc.c \
	kernel/graphics/framebuffer.c \
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
	filesystem/procfs.c \
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


$(USER_PHASE20_C_OBJ): userspace/phase20_demo.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE20_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE20_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE20_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 20 security demo ELF:"
	@echo "  $@"

$(USER_PHASE20_WX_OBJ): userspace/phase20_wx.S include/axiom/abi/syscall.h
	@mkdir -p $(USER_DIR)
	$(CC) $(GASFLAGS) -c $< -o $@

$(USER_PHASE20_WX_ELF): $(USER_PHASE20_WX_OBJ) userspace/phase20_wx.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack \
		-T userspace/phase20_wx.ld $(USER_PHASE20_WX_OBJ) -o $@
	@echo
	@echo "Built deliberate Phase 20 W+X rejection ELF:"
	@echo "  $@"



$(USER_PHASE21_C_OBJ): userspace/phase21_schedbench.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE21_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE21_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE21_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 21 scheduler benchmark ELF:"
	@echo "  $@"


$(USER_PHASE22_DEMO_C_OBJ): userspace/phase22_ipc_demo.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE22_DEMO_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE22_DEMO_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE22_DEMO_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 22 IPC demo ELF:"
	@echo "  $@"

$(USER_PHASE22_PEER_C_OBJ): userspace/phase22_ipc_peer.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE22_PEER_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE22_PEER_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE22_PEER_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 22 IPC peer ELF:"
	@echo "  $@"

$(USER_PHASE23_C_OBJ): userspace/phase23_gfx_demo.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE23_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE23_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	@mkdir -p $(USER_DIR)
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections \
		-T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE23_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 23 graphics demo ELF:"
	@echo "  $@"


$(USER_PHASE24_IFCONFIG_C_OBJ): userspace/phase24_ifconfig.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@
$(USER_PHASE24_IFCONFIG_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE24_IFCONFIG_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections -T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE24_IFCONFIG_C_OBJ) $(LIBC_ARCHIVE) -o $@

$(USER_PHASE24_PING_C_OBJ): userspace/phase24_ping.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@
$(USER_PHASE24_PING_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE24_PING_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections -T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE24_PING_C_OBJ) $(LIBC_ARCHIVE) -o $@

$(USER_PHASE24_DNS_C_OBJ): userspace/phase24_dnslookup.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@
$(USER_PHASE24_DNS_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE24_DNS_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections -T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE24_DNS_C_OBJ) $(LIBC_ARCHIVE) -o $@

$(USER_PHASE24_HTTPGET_C_OBJ): userspace/phase24_httpget.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@
$(USER_PHASE24_HTTPGET_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE24_HTTPGET_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections -T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE24_HTTPGET_C_OBJ) $(LIBC_ARCHIVE) -o $@

$(USER_PHASE24_HTTPD_C_OBJ): userspace/phase24_httpd.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@
$(USER_PHASE24_HTTPD_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE24_HTTPD_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections -T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE24_HTTPD_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 24 networking application ELFs."


$(USER_PHASE25_SYSINFO_C_OBJ): userspace/phase25_sysinfo.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE25_SYSINFO_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE25_SYSINFO_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections -T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE25_SYSINFO_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 25 developer tooling ELF."


$(USER_PHASE26_DESKTOP_C_OBJ): userspace/phase26_desktop.c $(LIBC_ARCHIVE)
	@mkdir -p $(USER_DIR)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(USER_PHASE26_DESKTOP_ELF): $(LIBC_CRT0_OBJ) $(USER_PHASE26_DESKTOP_C_OBJ) $(LIBC_ARCHIVE) userspace/x86_64_user.ld
	$(LD) -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -z noexecstack --gc-sections -T userspace/x86_64_user.ld $(LIBC_CRT0_OBJ) $(USER_PHASE26_DESKTOP_C_OBJ) $(LIBC_ARCHIVE) -o $@
	@echo
	@echo "Built Phase 26 desktop GUI ELF."

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
	$(USER_PHASE20_ELF) \
	$(USER_PHASE20_WX_ELF) \
	$(USER_PHASE21_ELF) \
	$(USER_PHASE22_DEMO_ELF) \
	$(USER_PHASE22_PEER_ELF) \
	$(USER_PHASE23_ELF) \
	$(USER_PHASE24_IFCONFIG_ELF) \
	$(USER_PHASE24_PING_ELF) \
	$(USER_PHASE24_DNS_ELF) \
	$(USER_PHASE24_HTTPGET_ELF) \
	$(USER_PHASE24_HTTPD_ELF) \
	$(USER_PHASE25_SYSINFO_ELF) \
	$(USER_PHASE26_DESKTOP_ELF) \
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
		$(USER_PHASE20_ELF) \
		$(ISO_ROOT)/boot/phase20_demo.elf

	cp \
		$(USER_PHASE20_WX_ELF) \
		$(ISO_ROOT)/boot/phase20_wx.elf

	cp \
		$(USER_PHASE21_ELF) \
		$(ISO_ROOT)/boot/schedbench.elf

	cp \
		$(USER_PHASE22_DEMO_ELF) \
		$(ISO_ROOT)/boot/ipcdemo.elf

	cp \
		$(USER_PHASE22_PEER_ELF) \
		$(ISO_ROOT)/boot/ipc-peer.elf

	cp \
		$(USER_PHASE23_ELF) \
		$(ISO_ROOT)/boot/gfxdemo.elf

	cp $(USER_PHASE24_IFCONFIG_ELF) $(ISO_ROOT)/boot/ifconfig.elf
	cp $(USER_PHASE24_PING_ELF) $(ISO_ROOT)/boot/ping.elf
	cp $(USER_PHASE24_DNS_ELF) $(ISO_ROOT)/boot/dnslookup.elf
	cp $(USER_PHASE24_HTTPGET_ELF) $(ISO_ROOT)/boot/httpget.elf
	cp $(USER_PHASE24_HTTPD_ELF) $(ISO_ROOT)/boot/httpd.elf
	cp $(USER_PHASE25_SYSINFO_ELF) $(ISO_ROOT)/boot/sysinfo.elf
	cp $(USER_PHASE26_DESKTOP_ELF) $(ISO_ROOT)/boot/desktop.elf


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
		-netdev user,id=net0,ipv6=off,hostfwd=tcp:127.0.0.1:18080-:8080 \
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
		-netdev user,id=net0,ipv6=off,hostfwd=tcp:127.0.0.1:18080-:8080 \
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
	$(MAKE) test-phase20
	$(MAKE) test-phase21
	$(MAKE) test-phase22
	$(MAKE) test-phase23
	$(MAKE) test-phase24
	$(MAKE) test-phase25
	$(MAKE) test-phase26


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
	@echo "AxiomOS Phase 26 desktop targets:"
	@echo
	@echo "  make          Build kernel and bootable ISO"
	@echo "  make run      Boot AxiomOS using QEMU"
	@echo "  make debug    Boot QEMU paused for GDB"
	@echo "  make test     Run Phase 1 through Phase 26 tests"
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
	@echo "  make test-phase20 Run security hardening and attack tests"
	@echo "  make test-phase21 Run priority/aging/affinity scheduler tests"
	@echo "  make test-phase22 Run pipes/shared-memory/message-queue IPC tests"
	@echo "  make test-phase23 Run framebuffer graphics primitive/library tests"
	@echo "  make test-phase24 Run userspace networking application/server tests"
	@echo "  make test-phase25 Run /proc observability + developer tooling tests"
	@echo "  make test-phase26 Run desktop/terminal-return integration tests"
	@echo "  make disk-image Create the 16 MiB persistent QEMU disk image"
	@echo "  make MODE=heap_double_free run  Trigger double-free panic"
	@echo "  make MODE=heap_guard run        Trigger tail-guard panic"
	@echo "  make inspect  Inspect generated ELF64 kernel"
	@echo "  make clean    Remove generated build files"
	@echo "  make distclean Remove build files and Limine"

.PHONY: test-phase3 test-phase4 test-phase5 test-phase6 test-phase7 test-phase8 test-phase9 test-phase10 test-phase11 test-phase12 test-phase13 test-phase14 test-phase15 test-phase16 test-phase17 test-phase18 test-phase19 test-phase20 test-phase21 test-phase22 test-phase23 test-phase24 test-phase25 test-phase26 disk-image

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

test-phase20:
	python3 tests/phase20_security.py

test-phase21:
	python3 tests/phase21_scheduler.py

test-phase22:
	python3 tests/phase22_ipc.py

test-phase23:
	python3 tests/phase23_graphics.py

test-phase24:
	python3 tests/phase24_network_apps.py

test-phase25:
	python3 tests/phase25_tooling.py

test-phase26:
	python3 tests/phase26_desktop.py

# Track C header changes too; each fault mode has its own object directory.
-include $(C_OBJECTS:.o=.d)
