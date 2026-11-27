# qemu q35 mainboard

## Running coreboot in qemu
Emulators like qemu don't need a firmware to do hardware init.
The hardware starts in the configured state already.

The coreboot port allows to test non mainboard specific code.
As you can easily attach a debugger, it's a good target for
experimental code.

## coreboot x86_64 support
coreboot historically runs in 32-bit protected mode, even though the
processor supports x86_64 instructions (long mode).

The qemu-q35 mainboard has been ported to x86_64 and will serve as
reference platform to enable additional platforms.

To enable the support set the Kconfig option ``CONFIG_USE_EXP_X86_64_SUPPORT=y``.

## Installing qemu

On debian you can install qemu by running:
```bash
$ sudo apt-get install qemu
```

On redhat you can install qemu by running:
```bash
$ sudo dnf install qemu
```

## Running coreboot
### To run the i386 version of coreboot (default)
Running on qemu-system-i386 will require a 32 bit operating system.

```bash
qemu-system-i386 -bios build/coreboot.rom -serial stdio -M q35
```

### To run the experimental x86_64 version of coreboot
Running on `qemu-system-x86_64` allows to run a 32 bit or 64 bit operating system
and firmware.

```bash
qemu-system-x86_64 -bios build/coreboot.rom -serial stdio -M q35
```

## ACPI S3 resume (QEMU ACPI PM, not ICH9)

QEMU's ACPI PM (`hw/acpi/core.c`) does **not** implement ICH9 sleep types:

| Sleep | QEMU `SLP_TYP` | ICH9 `SLP_TYP` | QEMU action |
| ----- | ------------- | -------------- | ----------- |
| S3    | **1**         | 5              | `qemu_system_suspend_request()` |
| S5    | **0**         | 7              | `qemu_system_shutdown_request()` |

Intel `sleepstates.asl` (`_S3=5`, `_S5=7`) must not be included on this board: Linux would write typ=5, which QEMU ignores (default branch; not S3).

Wake path (QEMU 8.2+ `pc_machine_wakeup()`):

1. Guest writes `PM1_CNT` with `SLP_TYP=1 | SLP_EN`, or QMP `system_suspend`.
2. `system_wakeup` runs `RESET_TYPE_WAKEUP` (reset vector, RAM preserved).
3. `acpi_notify_wakeup()` sets `PM1_STS.WAK_STS` (and `PWRBTN_STS` for `QEMU_WAKEUP_REASON_OTHER`).
4. `PM1_CNT` is typically 0 after wakeup reset, so `SLP_TYP` is no longer 1.
5. coreboot `acpi_get_sleep_type()` treats `WAK_STS` (and leftover QEMU typ=1) as `ACPI_S3`.
6. romstage recovers CBMEM and `romstage_handoff_init(true)`; ramstage jumps to the FACS 32-bit `firmware_waking_vector` if the OS stored one.

`ICH9-LPC.disable_s3` only changes fw_cfg `etc/system-states`. It does not change PM1_CNT behavior. This DSDT always advertises `_S3={1,1,0,0}`.

### Test matrix

| # | Case | Expected |
| - | ---- | -------- |
| 1 | Cold boot (`qemu-system-x86_64 -M q35 -bios coreboot.rom`) | Serial: `Q35 S3: ... s3resume=0`. No FACS jump. |
| 2 | Firmware-only wake: HMP `o 0x605 0x24` (`SLP_TYP=1\|SLP_EN`) then `system_wakeup`. QMP `system_suspend` without a guest OS often does not enter S3. | Second pass: `Q35 S3: ... s3resume=1` and `S3 Resume`. If FACS vector is 0, continue to payload (not a failure). |
| 3 | Guest S3 (Linux `systemctl suspend`) with waking vector | Detect S3, jump to OS vector, userspace resumes. |
| 4 | Guest S5 (`systemctl poweroff`) | QEMU exits (typ=0). Next VM start is cold (`s3resume=0`). |
| 5 | `ICH9-LPC.disable_s3=on` | Guest may still enter S3 via PM1; this port still detects `WAK_STS`. |
| 6 | CBMEM lost on wake | `S3 resume: CBMEM recovery failed, cold boot` then payload. |
| 7 | TSEG stage cache too small (`SMM_RESERVED_SIZE=0`) | Detect `s3resume=1`, then `Can't find 57a9e002 metadata in imd` / `postcar cache invalid` / `board_reset`. Next pass is a cold boot (`s3resume=0`) because the reset clears `WAK_STS`. |
| 8 | TSEG 8MiB + `SMM_RESERVED_SIZE=0x200000` (defaults with `CPU_QEMU_X86_TSEG_SMM`) | Cold boot stashes postcar/ramstage. Wake prints `S3 Resume` and continues; no `postcar cache invalid`. |
| 9 | QEMU ACPI 1.0 RSDP (`XSDT=0`) + FADT `x_firmware_ctl=0` | Resume must walk **RSDT** and `firmware_ctrl`, then print `FADT found` / `FACS found`. XSDT-only lookup prints `No FADT found` and never jumps. |
| 10 | Payload programs `FACS.firmware_waking_vector` at `0x1000` then S3 | Serial: `OS waking vector is 0x00001000` then stub `Q35VEC` on COM1. Stub must be below qemu CAR (`DCACHE_RAM_BASE=0x10000`); bootblock only zeros allocated CAR, not 64KiB–640KiB unused DRAM. |

On default `qemu-system-x86_64 -M q35`, coreboot loads QEMU's fw_cfg ACPI tables first and does **not** install the CBFS DSDT. Linux then sees QEMU's DSDT, which already uses `SLP_TYP 1` for S3 when `ICH9-LPC.disable_s3` is off. The CBFS `_S3={1,1,0,0}` still matters if ACPI build is off (`-machine acpi=off`) so coreboot's own DSDT is used. Firmware wake detection always uses `WAK_STS`, not the DSDT. The waking-vector jump uses the low-memory RSDP copy (often QEMU's ACPI 1.0 RSDP).

Stock Dasharo `configs/config.emulation_qemu_x86_q35_uefi` uses `CONFIG_DEFAULT_CONSOLE_LOGLEVEL_0`. S3 detect is printed at `BIOS_EMERG` so it still appears. For PM1 dumps on cold boot, use loglevel 6+ or `configs/config.emulation_qemu_x86_q35_s3_smoke`.

The board selects `DASHARO_PREFER_S3_SLEEP` so the EDK2 payload is built with S3 as the default sleep (otherwise `PAYLOAD_EDK2` defaults `DASHARO_PREFER_S3_SLEEP` off / S0ix).

`HAVE_ACPI_RESUME` plus `SMM_TSEG` selects `TSEG_STAGE_CACHE`. qemu-q35 must set `SMM_RESERVED_SIZE` (2MiB) and a larger TSEG (8MiB): `CBMEM_STAGE_CACHE` is unavailable while TSEG SMM is on, and a zero-sized TSEG subregion makes detect-then-reset look like a failed S3.

On QEMU, cache-as-RAM is ordinary DRAM at `DCACHE_RAM_BASE` (`0x10000`) for `DCACHE_RAM_SIZE` (up to VGA at `0xa0000`). Bootblock zeros only allocated CAR objects (`_car_region_start` through `_car_unallocated_start`), not the unused tail. Wiping 64KiB–640KiB on every reset would erase a guest `firmware_waking_vector` before ramstage jumped to it.

Firmware-only script (no Ubuntu disk):

```bash
cp configs/config.emulation_qemu_x86_q35_s3_smoke .config
make olddefconfig && make -j$(nproc)
src/mainboard/emulation/qemu-q35/s3-verify.sh build/coreboot.rom
```

Captured smoke output from QEMU 8.2.2: ``Documentation/mainboard/emulation/qemu-q35-s3-verify.txt``.

Linux check after a real guest suspend/resume:

```text
dmesg | grep -E 'ACPI:.*S3|PM: suspend|thaw'
```

Do not include Intel `src/southbridge/intel/common/acpi/sleepstates.asl` on qemu-q35.

## Finding bugs
To test coreboot's x86 code it's recommended to run on a x86 host and enable KVM.
It will not only run faster, but is closer to real hardware. If you see the
following message:

    KVM internal error. Suberror: 1
    emulation failure

something went wrong. The same bug will likely cause a FAULT on real hardware,
too.

To enable KVM run:

```bash
qemu-system-x86_64 -bios build/coreboot.rom -serial stdio -M q35 -accel kvm -cpu host
```

