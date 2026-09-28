#!/usr/bin/env bash

set -euo pipefail


ISO_PATH="${1:-build/AxiomOS.iso}"
SERIAL_LOG="${2:-build/phase1-serial.log}"
QEMU_LOG="${3:-build/phase1-qemu.log}"


if [[ ! -f "${ISO_PATH}" ]]; then
    echo "ERROR: ISO does not exist: ${ISO_PATH}"
    exit 1
fi


mkdir -p build

rm -f "${SERIAL_LOG}"
rm -f "${QEMU_LOG}"


echo "Booting AxiomOS in QEMU for Phase 1 test..."


set +e

timeout 5s \
    qemu-system-x86_64 \
        -machine q35 \
        -m 256M \
        -cdrom "${ISO_PATH}" \
        -boot d \
        -display none \
        -serial "file:${SERIAL_LOG}" \
        -monitor none \
        -no-reboot \
        -no-shutdown \
        >"${QEMU_LOG}" 2>&1

QEMU_STATUS=$?

set -e


# timeout returns 124 when it intentionally terminates QEMU.
# That is normal because AxiomOS intentionally halts forever.


if [[ "${QEMU_STATUS}" -ne 0 && "${QEMU_STATUS}" -ne 124 ]]; then
    echo "FAIL: QEMU exited unexpectedly with status ${QEMU_STATUS}."

    echo
    echo "QEMU log:"
    echo "----------------------------------------"
    cat "${QEMU_LOG}" || true
    echo "----------------------------------------"

    exit 1
fi


EXPECTED="AxiomOS kernel booted successfully."


if grep -Fq "${EXPECTED}" "${SERIAL_LOG}" 2>/dev/null; then
    echo "PASS: Phase 1 boot message detected."
    echo
    grep -F "${EXPECTED}" "${SERIAL_LOG}"
    exit 0
fi


echo "FAIL: Expected kernel output was not detected."

echo
echo "Serial log:"
echo "----------------------------------------"
cat "${SERIAL_LOG}" 2>/dev/null || true
echo "----------------------------------------"

echo
echo "QEMU log:"
echo "----------------------------------------"
cat "${QEMU_LOG}" 2>/dev/null || true
echo "----------------------------------------"

exit 1
