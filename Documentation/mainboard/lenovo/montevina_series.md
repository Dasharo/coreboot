# Lenovo X200 / T400 / T500 / X301 common

These models are sold with either 8 MiB or 4 MiB flash chip. You can identify
the chip in your machine through flashrom:
```console
# flashrom -p internal
```

Note that this does not allow you to determine whether the chip is in a SOIC-8
or a SOIC-16 package.

## Installing without ME firmware

```eval_rst
.. Note::
   **ThinkPad R500** has slightly different flash layout (it doesn't have
   ``gbe`` region), so the process would be a little different for that model.
```

On Montevina machines it's possible to disable ME and remove its firmware from
SPI flash by modifying the flash descriptor. This also makes it possible to use
the flash region the ME used for `bios` region, allowing for much larger
payloads.

First of all create a backup of your ROM with an external programmer:
```console
# flashrom -p YOUR_PROGRAMMER -r backup.rom
```

Then, split the IFD regions into separate files with ifdtool. You will need
`flashregion_3_gbe.bin` later.
```console
$ ifdtool -x backup.rom
```

Now you need to patch the flash descriptor. You can either [modify the one from
your backup with **ifdtool**](#modifying-flash-descriptor-using-ifdtool), or
[generate a completely new one with **bincfg**](#creating-a-new-flash-descriptor-using-bincfg).

#### Modifying flash descriptor using ifdtool

Pick the layout according to your chip size from the table below and save it to
the `new_layout.txt` file:

```eval_rst
+---------------------------+---------------------------+---------------------------+
| 4 MB chip                 | 8 MB chip                 | 16 MB chip                |
+===========================+===========================+===========================+
| .. code-block:: none      | .. code-block:: none      | .. code-block:: none      |
|                           |                           |                           |
|    00000000:00000fff fd   |    00000000:00000fff fd   |    00000000:00000fff fd   |
|    00001000:00002fff gbe  |    00001000:00002fff gbe  |    00001000:00002fff gbe  |
|    00003000:003fffff bios |    00003000:007fffff bios |    00003000:01ffffff bios |
|    00fff000:00000fff pd   |    00fff000:00000fff pd   |    00fff000:00000fff pd   |
|    00fff000:00000fff me   |    00fff000:00000fff me   |    00fff000:00000fff me   |
+---------------------------+---------------------------+---------------------------+
```

The last two lines define `pd` and `me` regions of negative size. This way
ifdtool will mark those as unused.

Update regions in the flash descrpitor (it was extracted previously with
`ifdtool -x`):
```console
$ ifdtool -n new_layout.txt flashregion_0_flashdescriptor.bin
```

Set `MeDisable` bit in ICH0 and MCH0 straps:
```console
$ ifdtool -M 1 flashregion_0_flashdescriptor.bin.new
```

Delete previous descriptors and rename the final one:
```console
$ rm flashregion_0_flashdescriptor.bin
$ rm flashregion_0_flashdescriptor.bin.new
$ mv flashregion_0_flashdescriptor.bin.new.new flashregion_0_flashdescriptor.bin
```

Continue to the [Configuring coreboot](#configuring-coreboot) section.

#### Creating a new flash descriptor using bincfg

There is a tool to generate a modified flash descriptor called **bincfg**. Go to
`util/bincfg` and build it:
```console
$ cd util/bincfg
$ make
```

If your flash is not 8 MB, you need to change values of `flcomp_density1` and
`flreg1_limit` in the `ifd-x200.set` file according to following table:

```eval_rst
+-----------------+-------+-------+--------+
|                 | 4 MB  | 8 MB  | 16 MB  |
+=================+=======+=======+========+
| flcomp_density1 | 0x3   | 0x4   | 0x5    |
+-----------------+-------+-------+--------+
| flreg1_limit    | 0x3ff | 0x7ff | 0x1fff |
+-----------------+-------+-------+--------+
```

Then create the flash descriptor:
```console
$ ./bincfg ifd-x200.spec ifd-x200.set ifd.bin
```

#### Configuring coreboot

Now configure coreboot. You need to select correct chip size and specify paths
to flash descriptor and gbe dump.

```
Mainboard --->
    ROM chip size (8192 KB (8 MB)) # According to your chip
    (0x7fd000) Size of CBFS filesystem in ROM # or 0x3fd000 for 4 MB chip / 0x1ffd000 for 16 MB chip

Chipset --->
    [*] Add Intel descriptor.bin file
    # Note: if you used bincfg, specify path to generated util/bincfg/ifd.bin
    (/path/to/flashregion_0_flashdescriptor.bin) Path and filename of the descriptor.bin file

    [*] Add gigabit ethernet configuration
    (/path/to/flashregion_3_gbe.bin) Path to gigabit ethernet configuration
```

Then build coreboot and flash whole `build/coreboot.rom` to the chip.

## Installing with ME firmware

To install coreboot and keep ME working, you don't need to do anything special
with the flash descriptor. Just flash only `bios` externally and don't touch any
other regions:
```console
# flashrom -p YOUR_PROGRAMMER -w coreboot.rom --ifd -i bios
```

## Flash layout

The flash layouts of the OEM firmware are as follows:

```eval_rst
+---------------------------------+---------------------------------+
| 4 MB chip                       | 8 MB chip                       |
+=================================+=================================+
| .. code-block:: none            | .. code-block:: none            |
|                                 |                                 |
|    00000000:00000fff fd         |    00000000:00000fff fd         |
|    00001000:001f5fff me         |    00001000:005f5fff me         |
|    001f6000:001f7fff gbe        |    005f6000:005f7fff gbe        |
|    001f8000:001fffff pd         |    005f8000:005fffff pd         |
|    00200000:003fffff bios       |    00600000:007fffff bios       |
|    00290000:002affff ec         |    00690000:006affff ec         |
|    003e0000:003fffff bootblock  |    007e0000:007fffff bootblock  |
+---------------------------------+---------------------------------+
```

On each boot of vendor BIOS `ec` area in flash is checked for having firmware
there, and if there is one, it proceedes to update firmware on H8S/2116 (when
both external power and main battery are attached). Once update is performed,
first 64 KB of `ec` area is erased. Visit
[thinkpad-ec repository](https://github.com/hamishcoleman/thinkpad-ec) to learn
more about how to extract EC firmware from vendor updates.
