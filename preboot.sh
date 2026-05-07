#!/bbin/gosh
# preboot.sh — installed as /bin/uinit; u-root init exec's it before
# we hand off to /bbin/boot to kexec Ubuntu.
#
# 1. Probe /dev/ttyUSB[0-3] for the SP (faux-ipcc ensure-ready, fast timeout).
#    ensure-ready does Status -> AckSpStart-if-restarted -> Identity.
# 2. If found, sync the APOB. apob-sync reads the SP-side active slot via
#    IPCC and compares against the in-DRAM APOB. On full match it locks the
#    SP state machine via a no-write ApobCommit; on size/signature/byte
#    mismatch (or NoValidApob on first boot) it falls back to a full
#    apob-flash cycle. Address/size come from CONFIG_PSP_APOB_DRAM_ADDRESS /
#    CONFIG_PSP_APOB_DRAM_SIZE in the coreboot config; update both values
#    here if the coreboot SoC config changes.
# 3. Always exec /bbin/boot at the end so a probe/sync failure does not
#    block boot.

APOB_ADDR=0x7010000
APOB_MAX=0xd0000
PROBE_TIMEOUT=2000
USB_WAIT_SECS=10

# uinit can fire before USB enumeration finishes. Poll for the first
# /dev/ttyUSB[0-3] to appear, with a hard timeout so we still kexec on
# systems that have no USB serial.
i=0
while [ "$i" -lt "$USB_WAIT_SECS" ]; do
    if [ -e /dev/ttyUSB0 ] || [ -e /dev/ttyUSB1 ] \
       || [ -e /dev/ttyUSB2 ] || [ -e /dev/ttyUSB3 ]; then
        break
    fi
    sleep 1
    i=$((i + 1))
done
if [ "$i" -ge "$USB_WAIT_SECS" ]; then
    echo "preboot: no /dev/ttyUSB[0-3] after ${USB_WAIT_SECS}s, skipping APOB flash"
    exec /bbin/boot
fi

PORT=
/bin/faux-ipcc --timeout-ms $PROBE_TIMEOUT --port /dev/ttyUSB0 ensure-ready && PORT=/dev/ttyUSB0
[ -z "$PORT" ] && /bin/faux-ipcc --timeout-ms $PROBE_TIMEOUT --port /dev/ttyUSB1 ensure-ready && PORT=/dev/ttyUSB1
[ -z "$PORT" ] && /bin/faux-ipcc --timeout-ms $PROBE_TIMEOUT --port /dev/ttyUSB2 ensure-ready && PORT=/dev/ttyUSB2
[ -z "$PORT" ] && /bin/faux-ipcc --timeout-ms $PROBE_TIMEOUT --port /dev/ttyUSB3 ensure-ready && PORT=/dev/ttyUSB3

if [ -n "$PORT" ]; then
    echo "preboot: SP at $PORT, syncing APOB from $APOB_ADDR (max $APOB_MAX)"
    /bin/faux-ipcc --port "$PORT" apob-sync --from-mem $APOB_ADDR --max-size $APOB_MAX \
        || echo "preboot: APOB sync failed (continuing)"
else
    echo "preboot: SP not found on /dev/ttyUSB[0-3], skipping APOB sync"
fi

exec /bbin/boot
