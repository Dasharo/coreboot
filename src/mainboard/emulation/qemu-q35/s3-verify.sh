#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
#
# Firmware-only qemu-q35 S3 detect smoke test (no guest OS required).
#
# QEMU q35 S3 wake is pc_machine_wakeup() -> RESET_TYPE_WAKEUP (CPUs restart
# at the reset vector, RAM preserved) then acpi_notify_wakeup() sets
# PM1_STS.WAK_STS. This script:
#   1. Boots coreboot once (cold) and captures serial
#   2. Issues QMP system_suspend + system_wakeup
#   3. Confirms the second firmware pass prints s3resume=1
#
# Usage:
#   s3-verify.sh [coreboot.rom]
#   COREBOOT_ROM=build/coreboot.rom s3-verify.sh
#
# Requires: qemu-system-x86_64, python3, gcc -m32 (for waking-vector payload)
set -euo pipefail

ROM="${1:-${COREBOOT_ROM:-build/coreboot.rom}}"
QEMU="${QEMU:-qemu-system-x86_64}"
TIMEOUT_SEC="${TIMEOUT_SEC:-90}"
WORKDIR="${WORKDIR:-$(mktemp -d /tmp/q35-s3-XXXXXX)}"
QMP="${WORKDIR}/qmp.sock"
SERIAL="${WORKDIR}/serial.log"
PIDFILE="${WORKDIR}/qemu.pid"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${HERE}/../../../.." && pwd)"
CBFSTOOL="${CBFSTOOL:-${ROOT}/build/cbfstool}"

cleanup() {
	if [[ -f "${PIDFILE}" ]]; then
		local pid
		pid="$(cat "${PIDFILE}" 2>/dev/null || true)"
		if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
			kill "${pid}" 2>/dev/null || true
			wait "${pid}" 2>/dev/null || true
		fi
	fi
}
trap cleanup EXIT

if [[ ! -f "${ROM}" ]]; then
	echo "error: ROM not found: ${ROM}" >&2
	echo "Build a qemu-q35 image first, e.g.:" >&2
	echo "  cp configs/config.emulation_qemu_x86_q35_s3_smoke .config" >&2
	echo "  make olddefconfig && make -j\$(nproc)" >&2
	exit 2
fi
if ! command -v "${QEMU}" >/dev/null; then
	echo "error: ${QEMU} not found" >&2
	exit 2
fi
if ! command -v python3 >/dev/null; then
	echo "error: python3 not found" >&2
	exit 2
fi

qmp_cmd() {
	python3 - "${QMP}" "$@" <<'PY'
import json, socket, sys, time

sock_path = sys.argv[1]
cmd = json.loads(sys.argv[2])
deadline = time.time() + 10
last_err = None
while time.time() < deadline:
    try:
        s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        s.settimeout(15)
        s.connect(sock_path)
        break
    except OSError as exc:
        last_err = exc
        time.sleep(0.1)
else:
    raise SystemExit(f"qmp connect failed: {last_err}")

def recv_obj(sock):
    buf = b""
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            raise RuntimeError("qmp eof")
        buf += chunk
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            if not line.strip():
                continue
            return json.loads(line.decode())

greeting = recv_obj(s)
if "QMP" not in greeting:
    raise SystemExit(f"unexpected qmp greeting: {greeting}")
s.sendall(b'{"execute":"qmp_capabilities"}\n')
recv_obj(s)
s.sendall((json.dumps(cmd) + "\n").encode())
print(json.dumps(recv_obj(s)))
s.close()
PY
}

serial_plain() {
	# Strip ANSI and CR so displayed lines and grep stay stable.
	# Do not pipe a huge buffer into grep -q under `set -o pipefail`:
	# grep -q exits early and echo gets SIGPIPE, which looks like a miss.
	if [[ ! -f "${SERIAL}" ]]; then
		return 0
	fi
	tr -d '\r' < "${SERIAL}" | sed 's/\x1b\[[0-9;]*m//g' || true
}

serial_has() {
	[[ -f "${SERIAL}" ]] && grep -aE "$1" "${SERIAL}" >/dev/null
}

# Match only bytes written after offset (so cold-boot strings cannot satisfy
# post-wake waits). grep -q on a pipe is unsafe with set -o pipefail.
serial_has_from() {
	local off="$1"
	local pat="$2"
	[[ -f "${SERIAL}" ]] || return 1
	python3 - "${SERIAL}" "${off}" "${pat}" <<'PY'
import re, sys

path, off, pat = sys.argv[1], int(sys.argv[2]), sys.argv[3]
data = open(path, "rb").read()[off:]
sys.exit(0 if re.search(pat.encode(), data) else 1)
PY
}

wait_serial() {
	local pattern="$1"
	local seconds="$2"
	local deadline=$((SECONDS + seconds))
	while (( SECONDS < deadline )); do
		if serial_has "${pattern}"; then
			return 0
		fi
		sleep 0.2
	done
	return 1
}

wait_serial_from() {
	local off="$1"
	local pattern="$2"
	local seconds="$3"
	local deadline=$((SECONDS + seconds))
	while (( SECONDS < deadline )); do
		if serial_has_from "${off}" "${pattern}"; then
			return 0
		fi
		sleep 0.2
	done
	return 1
}

echo "ROM=${ROM}"
echo "WORKDIR=${WORKDIR}"

TEST_ROM="${WORKDIR}/coreboot.rom"
cp "${ROM}" "${TEST_ROM}"
PAYLOAD_ELF="${WORKDIR}/s3-wvec.elf"
HAVE_WVEC=0
if command -v gcc >/dev/null && [[ -x "${CBFSTOOL}" ]] && [[ -f "${HERE}/s3-wvec-payload.c" ]]; then
	if gcc -m32 -nostdlib -fno-pic -fno-stack-protector -static \
		-Wl,-T,"${HERE}/s3-wvec.ld" -Wl,--build-id=none -s \
		-o "${PAYLOAD_ELF}" "${HERE}/s3-wvec-payload.c"; then
		"${CBFSTOOL}" "${TEST_ROM}" add-payload -f "${PAYLOAD_ELF}" -n fallback/payload
		HAVE_WVEC=1
		echo "Added S3 waking-vector payload to test ROM"
	else
		echo "warning: gcc -m32 payload build failed; skipping vector inject" >&2
	fi
else
	echo "warning: gcc/cbfstool missing; skipping waking-vector payload" >&2
fi

"${QEMU}" \
	-M q35 \
	-smp 1 \
	-m 1G \
	-bios "${TEST_ROM}" \
	-display none \
	-serial "file:${SERIAL}" \
	-qmp "unix:${QMP},server,nowait" \
	-pidfile "${PIDFILE}" \
	-daemonize

if ! wait_serial "Q35 S3:.*s3resume=" "${TIMEOUT_SEC}"; then
	echo "error: timed out waiting for cold-boot Q35 S3 line" >&2
	echo "----- serial -----" >&2
	serial_plain >&2 || true
	exit 1
fi

# Finish ramstage / payload so PMBASE stays programmed and FACS can be filled.
if [[ "${HAVE_WVEC}" -eq 1 ]]; then
	if ! wait_serial "S3-PAYLOAD: programmed" "${TIMEOUT_SEC}"; then
		echo "error: waking-vector payload did not program FACS" >&2
		serial_plain >&2 || true
		exit 1
	fi
	echo "PAYLOAD: $(serial_plain | grep -E 'S3-PAYLOAD:' | tail -n 1 || true)"
else
	wait_serial "Payload not loaded|Jumping to|Boot failed" "${TIMEOUT_SEC}" || true
fi

cold="$(serial_plain | grep -E "Q35 S3:.*s3resume=" | tail -n 1 || true)"
echo "COLD: ${cold}"
if serial_has "s3resume=1"; then
	echo "error: cold boot was detected as S3 resume" >&2
	exit 1
fi

echo "STATUS before S3: $(qmp_cmd '{"execute":"query-status"}')"
# QEMU ACPI PM: byte write to PM1_CNT high byte (I/O 0x605) with
# SLP_TYP=1 | SLP_EN (0x24) is the guest S3 entry path (hw/acpi/core.c).
echo "HMP out PM1_CNT: $(qmp_cmd '{"execute":"human-monitor-command","arguments":{"command-line":"o 0x605 0x24"}}')"
sleep 1
echo "STATUS after S3 write: $(qmp_cmd '{"execute":"query-status"}')"
wake_off=0
if [[ -f "${SERIAL}" ]]; then
	wake_off="$(wc -c < "${SERIAL}")"
fi
echo "WAKEUP: $(qmp_cmd '{"execute":"system_wakeup"}')"

if ! wait_serial_from "${wake_off}" "s3resume=1" "${TIMEOUT_SEC}"; then
	echo "error: timed out waiting for S3 resume detect" >&2
	echo "----- serial -----" >&2
	serial_plain >&2 || true
	exit 1
fi

resume="$(serial_plain | grep -E "Q35 S3:.*s3resume=1" | tail -n 1 || true)"
echo "WAKE: ${resume}"

# Detection alone is not enough: zero TSEG stage cache prints s3resume=1 then
# postcar_cache_invalid() -> board_reset(), which clears WAK_STS.
# Search only post-wake bytes so cold-boot "Payload not loaded" cannot match.
if ! wait_serial_from "${wake_off}" "S3 Resume" "${TIMEOUT_SEC}"; then
	echo "error: missing romstage_handoff S3 Resume after s3resume=1" >&2
	serial_plain >&2
	exit 1
fi
wait_serial_from "${wake_off}" "Jumping to image|postcar cache invalid|board_reset" "${TIMEOUT_SEC}" || true

if serial_has_from "${wake_off}" "postcar cache invalid"; then
	echo "error: S3 detected but postcar stage cache was empty/invalid" >&2
	serial_plain >&2
	exit 1
fi
if serial_has_from "${wake_off}" "Can't find 57a9e002 metadata"; then
	echo "error: S3 detected but postcar was not in the TSEG stage cache" >&2
	serial_plain >&2
	exit 1
fi
if serial_has_from "${wake_off}" "board_reset"; then
	echo "error: S3 resume path reset the board (not a successful resume)" >&2
	serial_plain >&2
	exit 1
fi
if ! serial_has_from "${wake_off}" "Jumping to image"; then
	echo "error: postcar did not jump to cached ramstage on S3 resume" >&2
	serial_plain >&2
	exit 1
fi
wait_serial_from "${wake_off}" "Trying to find the wakeup vector|No FADT found" "${TIMEOUT_SEC}" || true
if serial_has_from "${wake_off}" "No FADT found"; then
	echo "error: S3 resume could not find FADT (need RSDT walk for QEMU ACPI 1.0)" >&2
	serial_plain >&2
	exit 1
fi
if [[ "${HAVE_WVEC}" -eq 1 ]]; then
	if ! wait_serial_from "${wake_off}" "OS waking vector is 0x0*1000" "${TIMEOUT_SEC}"; then
		echo "error: FACS lookup did not return 0x1000" >&2
		serial_plain >&2
		exit 1
	fi
	if ! wait_serial_from "${wake_off}" "Q35VEC" "${TIMEOUT_SEC}"; then
		echo "error: waking-vector stub did not print Q35VEC (jump may have failed)" >&2
		echo "HMP registers: $(qmp_cmd '{"execute":"human-monitor-command","arguments":{"command-line":"info registers"}}')" >&2
		echo "HMP stub: $(qmp_cmd '{"execute":"human-monitor-command","arguments":{"command-line":"xp /32xb 0x1000"}}')" >&2
		serial_plain >&2
		exit 1
	fi
	echo "PASS: QEMU q35 S3 detect, TSEG cache resume, and FACS waking-vector jump"
else
	if ! wait_serial_from "${wake_off}" "FADT found|No FADT found" "${TIMEOUT_SEC}"; then
		true
	fi
	if serial_has_from "${wake_off}" "No FADT found"; then
		echo "error: S3 resume could not find FADT (need RSDT walk for QEMU ACPI 1.0)" >&2
		serial_plain >&2
		exit 1
	fi
	echo "PASS: QEMU q35 firmware distinguished cold boot vs S3 wake and resumed without reset"
fi

echo "SERIAL=${SERIAL}"
# Keep logs for the caller; do not delete WORKDIR on success.
trap - EXIT
if [[ -f "${PIDFILE}" ]]; then
	pid="$(cat "${PIDFILE}")"
	kill "${pid}" 2>/dev/null || true
	wait "${pid}" 2>/dev/null || true
fi
echo "Full serial log:"
serial_plain
