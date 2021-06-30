#!/bin/bash
TALOS_IP=192.168.20.9
TALOS_USER=root
LOG_FILE=/dev/null

COREBOOT_SOURCE=build/coreboot.rom.signed.ecc
COREBOOT_BMC_LOCATION=/tmp/coreboot.rom.signed.ecc
COREBOOT_PARTITION=HBI

BOOTBLOCK_SOURCE=build/bootblock.signed.ecc
BOOTBLOCK_BMC_LOCATION=/tmp/bootblock.signed.ecc
BOOTBLOCK_PARTITION=HBB

echo "Power off Talos II"
ssh $TALOS_USER@$TALOS_IP obmcutil -w chassisoff > $LOG_FILE

echo "Copy coreboot image"
scp $COREBOOT_SOURCE $TALOS_USER@$TALOS_IP:$COREBOOT_BMC_LOCATION > $LOG_FILE
scp $BOOTBLOCK_SOURCE $TALOS_USER@$TALOS_IP:$BOOTBLOCK_BMC_LOCATION > $LOG_FILE

echo "Flash coreboot image"
ssh $TALOS_USER@$TALOS_IP pflash -f -e -P $COREBOOT_PARTITION -p $COREBOOT_BMC_LOCATION > $LOG_FILE
ssh $TALOS_USER@$TALOS_IP pflash -f -e -P $BOOTBLOCK_PARTITION -p $BOOTBLOCK_BMC_LOCATION > $LOG_FILE

echo "Power on Talos II"
ssh $TALOS_USER@$TALOS_IP obmcutil -w poweron > $LOG_FILE
