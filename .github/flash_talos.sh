#!/bin/bash
TALOS="${1:-root@192.168.20.9}"
LOG_FILE="${2:-/dev/null}"

COREBOOT_SOURCE=build/coreboot.rom.signed.ecc
COREBOOT_BMC_LOCATION=/tmp/coreboot.rom.signed.ecc
COREBOOT_PARTITION=HBI

BOOTBLOCK_SOURCE=build/bootblock.signed.ecc
BOOTBLOCK_BMC_LOCATION=/tmp/bootblock.signed.ecc
BOOTBLOCK_PARTITION=HBB

echo "Power off Talos II"
ssh $TALOS obmcutil -w poweroff >> $LOG_FILE 2>&1

echo "Copy coreboot image"
scp $COREBOOT_SOURCE $TALOS:$COREBOOT_BMC_LOCATION >> $LOG_FILE 2>&1
scp $BOOTBLOCK_SOURCE $TALOS:$BOOTBLOCK_BMC_LOCATION >> $LOG_FILE 2>&1

echo "Flash coreboot image"
ssh $TALOS pflash -f -e -P $COREBOOT_PARTITION -p $COREBOOT_BMC_LOCATION >> $LOG_FILE 2>&1
ssh $TALOS pflash -f -e -P $BOOTBLOCK_PARTITION -p $BOOTBLOCK_BMC_LOCATION >> $LOG_FILE 2>&1

echo "Power on Talos II"
ssh $TALOS obmcutil -w poweron > $LOG_FILE
