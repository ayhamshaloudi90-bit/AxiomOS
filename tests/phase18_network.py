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
        self.send_header("Connection", "close")
        self.end_headers()
        self.close_connection = True
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
