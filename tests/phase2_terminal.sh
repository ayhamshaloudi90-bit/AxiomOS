#!/usr/bin/env bash

set -euo pipefail


ISO_PATH="${1:-build/AxiomOS.iso}"

SERIAL_LOG="build/phase2-serial.log"

QEMU_LOG="build/phase2-qemu.log"


if [[ ! -f "${ISO_PATH}" ]]; then
    echo "ERROR: ISO does not exist: ${ISO_PATH}"

    exit 1
fi


rm -f "${SERIAL_LOG}"
rm -f "${QEMU_LOG}"


echo "Booting AxiomOS for Phase 2 terminal test..."


set +e


timeout 6s \
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


if [[ "${QEMU_STATUS}" -ne 0 &&
      "${QEMU_STATUS}" -ne 124 ]]; then

    echo "FAIL: QEMU exited unexpectedly."

    cat "${QEMU_LOG}" || true

    exit 1
fi


EXPECTED_LINES=(
    "AxiomOS kernel booted successfully."
    "AxiomOS Phase 2 terminal online."
    "Decimal test: 123456789"
    "Hex test: 0xDEADBEEF"
    "64-bit test: 18446744073709551615"
    "Color output: OK"
    "Phase 2 terminal test complete."
)


for expected in "${EXPECTED_LINES[@]}"; do

    if ! grep -Fq \
        "${expected}" \
        "${SERIAL_LOG}"; then

        echo
        echo "FAIL: Missing output:"
        echo "  ${expected}"

        echo
        echo "Serial log:"
        echo "----------------------------------------"

        cat "${SERIAL_LOG}" || true

        echo "----------------------------------------"

        exit 1
    fi

done


echo
echo "PASS: Phase 2 terminal output verified."
echo

grep -F \
    "AxiomOS Phase 2 terminal online." \
    "${SERIAL_LOG}"

grep -F \
    "Decimal test:" \
    "${SERIAL_LOG}"

grep -F \
    "Hex test:" \
    "${SERIAL_LOG}"

grep -F \
    "64-bit test:" \
    "${SERIAL_LOG}"

grep -F \
    "Phase 2 terminal test complete." \
    "${SERIAL_LOG}"
