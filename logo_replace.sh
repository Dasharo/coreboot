#!/bin/bash
# A script to replace the boot logo for coreboot with tianocore payload

if [ $# -ne 1 ]; then 
	echo "usage: $0 [path to image]"
	exit 0
fi

if [ ! -f ".config" ]; then
	echo "File .config does not exist. Create a configuration file first."
	exit 1
fi

path=custom_bootsplash.bmp
cp $1 $path

if [[ $(grep "CONFIG_TIANOCORE_BOOTSPLASH_IMAGE" .config; echo $?) == 1 ]]; then
	echo "CONFIG_TIANOCORE_BOOTSPLASH_IMAGE=y" >> .config
else
	sed -i "/CONFIG_TIANOCORE_BOOTSPLASH_IMAGE/c\CONFIG_TIANOCORE_BOOTSPLASH_IMAGE=y" .config
fi

if [[ $(grep "CONFIG_TIANOCORE_BOOTSPLASH_FILE" .config; echo $?) == 1 ]]; then
	echo "CONFIG_TIANOCORE_BOOTSPLASH_FILE=\"$path\"" >> .config
else
	sed -i "/CONFIG_TIANOCORE_BOOTSPLASH_FILE/c\CONFIG_TIANOCORE_BOOTSPLASH_FILE=\"$path\"" .config
fi

echo "Boot logo configuration updated. Rebuild coreboot to apply changes."

