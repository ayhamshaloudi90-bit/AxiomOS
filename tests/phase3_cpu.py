#!/usr/bin/env python3
"""Boot real ISOs; check return-state tests and independently located fault RIPs."""
from pathlib import Path
import re
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
CASES = [
    ('normal', None, None, None, None),
    ('divide', 'Divide by Zero', 0, 0, 'phase3_divide_fault'),
    ('invalid', 'Invalid Opcode', 6, 0, 'phase3_invalid_fault'),
    ('gp', 'General Protection Fault', 13, 0x38, 'phase3_gp_fault'),
    ('double_fault', 'Double Fault', 8, 0, None),
]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def symbol_address(elf, name):
    output = subprocess.check_output(['llvm-nm', str(elf)], text=True)
    for line in output.splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[2] == name:
            return int(fields[0], 16)
    raise AssertionError(f'Missing ELF symbol {name}')


def run_case(mode, name, vector, error, fault_symbol):
    directory = ROOT / ('build' if mode == 'normal' else f'build-{mode}')
    directory.mkdir(exist_ok=True)
    with (directory / 'phase3-build.log').open('w') as output:
        subprocess.run(['make', f'MODE={mode}', 'all'], cwd=ROOT,
                       stdout=output, stderr=subprocess.STDOUT, check=True)
    serial = directory / 'phase3-serial.log'
    serial.write_text('')
    marker = 'Phase 3 CPU initialization complete.' if name is None else 'CPU halted.'
    command = ['qemu-system-x86_64', '-machine', 'q35', '-accel', 'tcg',
               '-m', '256M', '-cdrom', str(directory / 'AxiomOS.iso'),
               '-boot', 'd', '-display', 'none', '-serial', f'file:{serial}',
               '-monitor', 'none', '-no-reboot', '-no-shutdown']
    with (directory / 'phase3-qemu.log').open('w') as output:
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 20
            while marker not in serial.read_text(errors='replace'):
                require(process.poll() is None, f'{mode}: QEMU exited before completion')
                require(time.monotonic() < deadline, f'{mode}: boot timed out; see {serial}')
                time.sleep(0.05)
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
    text = serial.read_text().replace('\r', '')
    for expected in ['Phase 2 terminal test complete.',
                     'GDT/TSS loaded. IDT: 256 gates installed.',
                     'Breakpoint: resumed safely.',
                     'Phase 3 register preservation: OK',
                     'Software interrupt 0x80: returned safely.',
                     'Phase 3 CPU initialization complete.']:
        require(expected in text, f'{mode}: missing {expected}')
    require('FAILED' not in text and 'NESTED KERNEL PANIC' not in text,
            f'{mode}: internal failure')
    if name is None:
        require('KERNEL PANIC' not in text, 'Normal boot panicked')
    else:
        require(text.count('KERNEL PANIC') == 1, f'{mode}: missing/repeated panic')
        require(f'Exception: {name}\n' in text, f'{mode}: wrong exception')
        require(f'Vector: {vector}  Error: 0x{error:X}\n' in text,
                f'{mode}: wrong vector/error code')
        registers = {key: int(value, 16) for key, value in
                     re.findall(r'^(R[A-Z0-9]+): 0x([0-9A-F]+)$', text, re.M)}
        for key in ['RIP', 'RSP', 'RFLAGS', 'RAX', 'RBX', 'RCX', 'RDX', 'RSI',
                    'RDI', 'RBP', 'R8', 'R9', 'R10', 'R11', 'R12', 'R13', 'R14', 'R15']:
            require(key in registers, f'{mode}: missing {key}')
        if fault_symbol:
            expected_rip = symbol_address(directory / 'AxiomOS.elf', fault_symbol)
            require(registers['RIP'] == expected_rip, f'{mode}: incorrect saved RIP')
        bottom = symbol_address(directory / 'AxiomOS.elf', 'kernel_stack_bottom')
        top = symbol_address(directory / 'AxiomOS.elf', 'kernel_stack_top')
        if mode != 'double_fault':
            require(bottom <= registers['RSP'] < top, f'{mode}: invalid interrupted RSP')
        require(registers['RFLAGS'] & 2, f'{mode}: invalid RFLAGS reserved bit')
        require((registers['RFLAGS'] & 0x200) == 0, f'{mode}: IF unexpectedly set')
        require('CS: 0x8  SS: 0x10' in text, f'{mode}: wrong segment state')
        if mode == 'divide':
            require(registers['RAX'] == 123 and registers['RCX'] == 0
                    and registers['RDX'] == 0, 'Divide operands not preserved')
        if mode == 'double_fault':
            require('Double-fault IST stack: OK' in text, 'Double fault did not use IST1')
    print(f'PASS: Phase 3 {mode}', flush=True)


if __name__ == '__main__':
    for case in CASES:
        run_case(*case)
    print('PASS: all Phase 3 CPU tests. No Phase 4 functionality added.')
