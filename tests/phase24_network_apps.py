#!/usr/bin/env python3
"""Validate Phase-24 standalone Ring-3 networking applications and HTTP server."""

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
            tail = "\n".join(text.splitlines()[-200:])
            raise AssertionError(f"phase24 kernel panicked\n--- serial tail ---\n{tail}")
        require(process.poll() is None, f"QEMU exited before {marker!r}")
        if time.monotonic() >= deadline:
            tail = "\n".join(text.splitlines()[-200:])
            raise AssertionError(
                f"phase24 timed out waiting for {marker!r}; see {serial}\n"
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
            tail = "\n".join(text.splitlines()[-180:])
            raise AssertionError(
                f"phase24 timed out waiting for prompt #{count}\n"
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
    require(character in mapping, f"phase24: no HMP key mapping for {character!r}")
    return mapping[character]


def send_command(monitor_path, command):
    keys = [key_for_character(character) for character in command]
    keys.append("ret")
    send_hmp_keys(monitor_path, keys)


def free_tcp_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


class Phase24Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"

    def do_GET(self):
        body = b"AXIOMOS_PHASE24_HTTP_CLIENT_OK\n"
        if self.path != "/phase24":
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


def fetch_guest_http_server(host_port):
    deadline = time.monotonic() + 20
    last_error = None
    while time.monotonic() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", host_port), timeout=2) as s:
                s.sendall(b"GET / HTTP/1.0\r\nHost: axiom.local\r\nConnection: close\r\n\r\n")
                chunks = []
                while True:
                    data = s.recv(4096)
                    if not data:
                        break
                    chunks.append(data)
                return b"".join(chunks)
        except OSError as exc:
            last_error = exc
            time.sleep(0.1)
    raise AssertionError(f"phase24 could not connect to guest HTTP server: {last_error}")


def test_phase24():
    directory = ROOT / "build"
    directory.mkdir(exist_ok=True)

    with (directory / "phase24-build.log").open("w") as output:
        subprocess.run(
            ["make", "MODE=normal", "all"],
            cwd=ROOT,
            stdout=output,
            stderr=subprocess.STDOUT,
            check=True,
        )

    outbound_server = HTTPServer(("127.0.0.1", 0), Phase24Handler)
    outbound_port = outbound_server.server_address[1]
    outbound_thread = threading.Thread(target=outbound_server.serve_forever, daemon=True)
    outbound_thread.start()
    inbound_port = free_tcp_port()

    serial = directory / "phase24-serial.log"
    qemu_log = directory / "phase24-qemu.log"
    monitor = directory / "phase24-monitor.sock"
    serial.write_text("")
    monitor.unlink(missing_ok=True)

    netdev = (
        "user,id=net0,ipv6=off,"
        f"guestfwd=tcp:10.0.2.100:80-tcp:127.0.0.1:{outbound_port},"
        f"hostfwd=tcp:127.0.0.1:{inbound_port}-10.0.2.15:8080"
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
                time.monotonic() + 260,
            )
            for marker in [
                "AxiomOS Phase 24 networking applications online.",
                "Phase 24 userspace tools: ifconfig + ping + dnslookup + httpget + httpd",
                "Phase 24 TCP passive-open HTTP server: READY",
                "Phase 24 networking applications initialization complete.",
            ]:
                require(marker in text, f"phase24: missing boot marker {marker!r}")

            prompt = text.count("axiom> ")

            send_command(monitor, "ifconfig")
            text = wait_for_text(process, serial, "Phase 24 ifconfig application: OK", time.monotonic() + 30)
            require("inet 10.0.2.15" in text, "phase24: ifconfig did not report guest address")
            wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 30)
            prompt += 1

            # With no explicit target, the shell launches the Phase-24 /bin/ping ELF.
            send_command(monitor, "ping")
            wait_for_text(process, serial, "Phase 24 ping application: OK", time.monotonic() + 45)
            wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 45)
            prompt += 1

            send_command(monitor, "dnslookup")
            text = wait_for_text(process, serial, "Phase 24 dnslookup application: OK", time.monotonic() + 60)
            require(re.search(r"example\.com -> \d+\.\d+\.\d+\.\d+", text) is not None,
                    "phase24: dnslookup did not print an A record")
            wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 60)
            prompt += 1

            # With no host, the shell launches the standalone Phase-24 HTTP client.
            send_command(monitor, "httpget")
            text = wait_for_text(process, serial, "Phase 24 httpget application: OK", time.monotonic() + 60)
            require("AXIOMOS_PHASE24_HTTP_CLIENT_OK" in text,
                    "phase24: standalone HTTP client did not receive deterministic body")
            wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 60)
            prompt += 1

            send_command(monitor, "httpd")
            wait_for_text(
                process,
                serial,
                "AxiomOS Phase 24 httpd listening on port 8080 for one request",
                time.monotonic() + 30,
            )
            response = fetch_guest_http_server(inbound_port)
            require(b"HTTP/1.0 200 OK" in response,
                    "phase24: guest HTTP server did not return HTTP 200")
            require(b"AXIOMOS_PHASE24_HTTP_SERVER_OK" in response,
                    "phase24: guest HTTP server body missing")
            wait_for_text(process, serial, "Phase 24 HTTP server application: OK", time.monotonic() + 30)
            wait_for_prompt_count(process, serial, prompt + 1, time.monotonic() + 30)

            final = serial_text(serial)
            require("KERNEL PANIC" not in final, "phase24: kernel panicked")
            require(" application: OK" in final, "phase24: application markers missing")

            print("PASS: Phase 24 /bin/ifconfig userspace application")
            print("PASS: Phase 24 /bin/ping userspace ICMP application")
            print("PASS: Phase 24 /bin/dnslookup userspace DNS application")
            print("PASS: Phase 24 /bin/httpget userspace HTTP client")
            print("PASS: Phase 24 passive-open TCP + /bin/httpd HTTP server")
        finally:
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
            monitor.unlink(missing_ok=True)
            outbound_server.shutdown()
            outbound_server.server_close()
            outbound_thread.join(timeout=2)


if __name__ == "__main__":
    test_phase24()
    print("PASS: all Phase 24 networking application tests.")
