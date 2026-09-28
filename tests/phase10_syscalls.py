#!/usr/bin/env python3
"""Boot AxiomOS and validate the Phase-10 x86-64 syscall ABI."""

from pathlib import Path
import re
import socket
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def wait_for_text(process, serial, marker, deadline):
    while True:
        text = serial.read_text(errors="replace")

        if marker in text:
            return

        require(process.poll() is None, f"QEMU exited before {marker!r}")

        if time.monotonic() >= deadline:
            tail = "\n".join(text.replace("\r", "").splitlines()[-60:])
            raise AssertionError(
                f"phase10 timed out waiting for {marker!r}; see {serial}\n"
                f"--- serial tail ---\n{tail}"
            )

        time.sleep(0.05)


def send_keys(monitor_path, keys):
    deadline = time.monotonic() + 5

    while not monitor_path.exists():
        require(time.monotonic() < deadline,
                "QEMU monitor socket was not created")
        time.sleep(0.05)

    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
        connection.settimeout(2)
        connection.connect(str(monitor_path))

        try:
            connection.recv(4096)
        except socket.timeout:
            pass

        for key in keys:
            connection.sendall(f"sendkey {key} 30\n".encode("ascii"))
            time.sleep(0.08)


def parse_decimal(text, label, signed=False):
    pattern = r"(-?\d+)" if signed else r"(\d+)"
    match = re.search(rf"^{re.escape(label)}: {pattern}$", text, re.MULTILINE)
    require(match is not None, f"Missing numeric field: {label}")
    return int(match.group(1))


def test_phase10():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase10-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    serial = directory / "phase10-serial.log"
    qemu_log = directory / "phase10-qemu.log"
    monitor = directory / "phase10-monitor.sock"

    serial.write_text("")
    monitor.unlink(missing_ok=True)

    command = [
        "qemu-system-x86_64",
        "-machine", "q35",
        "-accel", "tcg",
        "-m", "256M",
        "-cdrom", str(directory / "AxiomOS.iso"),
        "-boot", "d",
        "-display", "none",
        "-serial", f"file:{serial}",
        "-monitor", f"unix:{monitor},server=on,wait=off",
        "-no-reboot",
        "-no-shutdown",
    ]

    with qemu_log.open("w") as output:
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT)

        try:
            wait_for_text(
                process,
                serial,
                "Phase 10 userspace waiting for keyboard input...",
                time.monotonic() + 60,
            )

            # Exercise SYS_read through the actual IRQ-driven keyboard buffer.
            send_keys(monitor, ["x"])

            wait_for_text(
                process,
                serial,
                "Phase 10 system calls complete.",
                time.monotonic() + 20,
            )

            # Later phases now run before the shell. Keep this regression
            # timeout large enough for the integrated modern boot path.
            wait_for_text(
                process,
                serial,
                "AxiomOS shell ready. Type 'help' for commands.",
                time.monotonic() + 90,
            )

            text = serial.read_text(errors="replace").replace("\r", "")

            for expected in [
                "Phase 9 userspace complete.",
                "AxiomOS Phase 10 syscall interface online.",
                "Syscall mechanism: x86-64 SYSCALL/SYSRET",
                "Hello via SYS_write from Ring 3!",
                "Phase 10 userspace read: x",
                "Phase 10 user-copy validation: OK",
                "Phase 10 syscall self-test: OK",
                "Phase 10 system calls complete.",
            ]:
                require(expected in text, f"phase10: missing output: {expected}")

            require("KERNEL PANIC" not in text, "phase10: kernel panicked")
            require("Phase 10 syscall self-test: FAILED" not in text,
                    "phase10: syscall self-test failed")

            task_id = parse_decimal(text, "Phase 10 user task ID")
            pid = parse_decimal(text, "Phase 10 getpid result")
            bad_pointer = parse_decimal(
                text, "Phase 10 invalid pointer result", signed=True
            )
            open_result = parse_decimal(text, "Phase 10 open result", signed=True)
            exit_status = parse_decimal(text, "Phase 10 exit status", signed=True)
            calls = parse_decimal(text, "Phase 10 syscall calls")
            rejected = parse_decimal(text, "Phase 10 rejected user pointers")

            bytes_match = re.search(
                r"^Phase 10 bytes written/read: (\d+)/(\d+)$",
                text,
                re.MULTILINE,
            )
            require(bytes_match is not None,
                    "phase10: missing written/read byte statistics")
            bytes_written, bytes_read = (int(value) for value in bytes_match.groups())

            require(pid == task_id,
                    f"phase10: getpid returned {pid}, task id is {task_id}")
            require(bad_pointer == -14,
                    f"phase10: invalid pointer returned {bad_pointer}, expected -14")
            require(open_result == -38,
                    f"phase10: open returned {open_result}, expected -38/ENOSYS")
            require(exit_status == 37,
                    f"phase10: exit status {exit_status}, expected 37")
            require(calls >= 8, "phase10: too few syscall transitions observed")
            require(rejected >= 1, "phase10: bad user pointer was not rejected")
            require(bytes_written > 0, "phase10: SYS_write transferred no bytes")
            require(bytes_read >= 1, "phase10: SYS_read transferred no bytes")

            # The older kernel input loop must still work after syscall activity.
            send_keys(monitor, ["k", "b", "d", "s", "t", "a", "t", "s", "ret"])
            wait_for_text(
                process,
                serial,
                "Keyboard IRQs/scancodes/chars/dropped:",
                time.monotonic() + 10,
            )

            final_text = serial.read_text(errors="replace").replace("\r", "")
            require("Keyboard IRQs/scancodes/chars/dropped:" in final_text,
                    "phase10: keyboard input broke after syscall execution")
            require("KERNEL PANIC" not in final_text,
                    "phase10: kernel panicked after syscall execution")

            print("PASS: Phase 10 x86-64 SYSCALL/SYSRET ABI")
            print(f"  user task id/getpid: {task_id}/{pid}")
            print(f"  syscall calls:       {calls}")
            print("PASS: Phase 10 validated user copies")
            print(f"  bad pointer result:  {bad_pointer}")
            print(f"  bytes written/read:  {bytes_written}/{bytes_read}")
            print("PASS: Phase 10 read/sleep/yield/exit + keyboard coexistence")
        finally:
            if process.poll() is None:
                process.terminate()

            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

            monitor.unlink(missing_ok=True)


if __name__ == "__main__":
    test_phase10()
    print("PASS: all Phase 10 syscall tests.")
