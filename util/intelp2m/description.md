Intel Pad to Macro (intelp2m) converter
=======================================

This utility allows to convert the configuration DW0/1 registers value
from [inteltool](../inteltool/description.md) dump to coreboot macros.

```bash
(shell)$ make
(shell)$ ./intelp2m -h
(shell)$ ./intelp2m -file /path/to/inteltool.log
```

### Platforms

It is possible to use templates for parsing files of excellent inteltool.log.
To specify such a pattern, use the option -t <template number>. For example,
using template type # 1, you can parse gpio.h from an already added board in
the coreboot project.

```bash
(shell)$ ./intelp2m -h
	-t
	template type number
		0 - inteltool.log (default)
		1 - gpio.h
		2 - your template
(shell)$ ./intelp2m -t 1 -file coreboot/src/mainboard/youboard/gpio.h
```
You can also add add a template to 'parser/template.go' for your file type with
the configuration of the pads.

platform type is set using the -p option (Sunrise by default):

```bash
	-p string
	set up a platform
		snr - Sunrise PCH with Skylake/Kaby Lake CPU
		lbg - Lewisburg PCH with Xeon SP CPU
		apl - Apollo Lake SoC
		cnl - CannonLake-LP or Whiskeylake/Coffelake/Cometlake-U SoC
	(default "snr")

(shell)$ ./intelp2m -p <platform> -file path/to/inteltool.log
```

### Packages

![][pckgs]

[pckgs]: config/gopackages.png

### Bit fields in macros

Use the -fld=cb option to only generate a sequence of bit fields in a new macro:

```bash
(shell)$ ./intelp2m -fld cb -p apl -file ../apollo-inteltool.log
```

```c
_PAD_CFG_STRUCT(GPIO_37, PAD_FUNC(NF1) | PAD_TRIG(OFF) | PAD_TRIG(OFF), PAD_PULL(DN_20K)), /* LPSS_UART0_TXD */
```

### Raw DW0, DW1 register value

To generate the gpio.c with raw PAD_CFG_DW0 and PAD_CFG_DW1 register values you need
to use the -fld=raw option:

```bash
  (shell)$ ./intelp2m -fld raw -file /path/to/inteltool.log
```

```c
_PAD_CFG_STRUCT(GPP_A10, 0x44000500, 0x00000000),	/* CLKOUT_LPC1 */
```

```bash
  (shell)$ ./intelp2m -iiii -fld raw -file /path/to/inteltool.log
```

```c
/* GPP_A10 - CLKOUT_LPC1 DW0: 0x44000500, DW1: 0x00000000 */
/* PAD_CFG_NF(GPP_A10, NONE, DEEP, NF1), */
/* DW0 : 0x04000100 - IGNORED */
_PAD_CFG_STRUCT(GPP_A10, 0x44000500, 0x00000000),
```

### FSP-style macro

The utility allows to generate macros that include fsp/edk2-palforms style bitfields:

```bash
(shell)$ ./intelp2m -fld fsp -p lbg -file ../crb-inteltool.log
```

```c
{ GPIO_SKL_H_GPP_A12, { GpioPadModeGpio, GpioHostOwnAcpi, GpioDirInInvOut, GpioOutLow, GpioIntSci | GpioIntLvlEdgDis, GpioResetNormal, GpioTermNone,  GpioPadConfigLock },	/* GPIO */
```

```bash
(shell)$ ./intelp2m -iiii -fld fsp -p lbg -file ../crb-inteltool.log
```

```c
/* GPP_A12 - GPIO DW0: 0x80880102, DW1: 0x00000000 */
/* PAD_CFG_GPI_SCI(GPP_A12, NONE, PLTRST, LEVEL, INVERT), */
{ GPIO_SKL_H_GPP_A12, { GpioPadModeGpio, GpioHostOwnAcpi, GpioDirInInvOut, GpioOutLow, GpioIntSci | GpioIntLvlEdgDis, GpioResetNormal, GpioTermNone,  GpioPadConfigLock },
```

### Macro Check

After generating the macro, the utility checks all used
fields of the configuration registers. If some field has been
ignored, the utility generates field macros. To not check
macros, use the -n option:

```bash
(shell)$ ./intelp2m -n -file /path/to/inteltool.log
```

In this case, some fields of the configuration registers
DW0 will be ignored.

```c
PAD_CFG_NF_IOSSTATE_IOSTERM(GPIO_38, UP_20K, DEEP, NF1, HIZCRx1, DISPUPD),	/* LPSS_UART0_RXD */
PAD_CFG_NF_IOSSTATE_IOSTERM(GPIO_39, UP_20K, DEEP, NF1, TxLASTRxE, DISPUPD),	/* LPSS_UART0_TXD */
```

### Information level

The utility can generate additional information about the bit
fields of the DW0 and DW1 configuration registers:

```c
/* GPIO_39 - LPSS_UART0_TXD (DW0: 0x44000400, DW1: 0x00003100) */ --> (2)
/* PAD_CFG_NF_IOSSTATE_IOSTERM(GPIO_39, UP_20K, DEEP, NF1, TxLASTRxE, DISPUPD), */ --> (3)
/* DW0 : PAD_TRIG(OFF) - IGNORED */ --> (4)
_PAD_CFG_STRUCT(GPIO_39, PAD_FUNC(NF1) | PAD_RESET(DEEP) | PAD_TRIG(OFF), PAD_PULL(UP_20K) | PAD_IOSTERM(DISPUPD)),
```

Using the options -i, -ii, -iii, -iiii you can set the info level
from (1) to (4):

```bash
(shell)$./intelp2m -i -file /path/to/inteltool.log
(shell)$./intelp2m -ii -file /path/to/inteltool.log
(shell)$./intelp2m -iii -file /path/to/inteltool.log
(shell)$./intelp2m -iiii -file /path/to/inteltool.log
```
(1) : print /* GPIO_39 - LPSS_UART0_TXD */

(2) : print initial raw values of configuration registers from
inteltool dump
DW0: 0x44000400, DW1: 0x00003100

(3) : print the target macro that will generate if you use the
-n option
PAD_CFG_NF_IOSSTATE_IOSTERM(GPIO_39, UP_20K, DEEP, NF1, TxLASTRxE, DISPUPD),

(4) : print decoded fields from (3) as macros
DW0 : PAD_TRIG(OFF) - IGNORED

### Ignoring Fields

Utilities can generate the _PAD_CFG_STRUCT macro and exclude fields
from it that are not in the corresponding PAD_CFG_*() macro:

```bash
(shell)$ ./intelp2m -iiii -fld cb -ign -file /path/to/inteltool.log
```

```c
/* GPIO_39 - LPSS_UART0_TXD DW0: 0x44000400, DW1: 0x00003100 */
/* PAD_CFG_NF_IOSSTATE_IOSTERM(GPIO_39, UP_20K, DEEP, NF1, TxLASTRxE, DISPUPD), */
/* DW0 : PAD_TRIG(OFF) - IGNORED */
_PAD_CFG_STRUCT(GPIO_39, PAD_FUNC(NF1) | PAD_RESET(DEEP), PAD_PULL(UP_20K) | PAD_IOSTERM(DISPUPD)),
```

If you generate macros without checking, you can see bit fields that
were ignored:

```bash
(shell)$ ./intelp2m -iiii -n -file /path/to/inteltool.log
```

```c
/* GPIO_39 - LPSS_UART0_TXD DW0: 0x44000400, DW1: 0x00003100 */
PAD_CFG_NF_IOSSTATE_IOSTERM(GPIO_39, UP_20K, DEEP, NF1, TxLASTRxE, DISPUPD),
/* DW0 : PAD_TRIG(OFF) - IGNORED */
```

```bash
(shell)$ ./intelp2m -n -file /path/to/inteltool.log
```

```c
/* GPIO_39 - LPSS_UART0_TXD */
PAD_CFG_NF_IOSSTATE_IOSTERM(GPIO_39, UP_20K, DEEP, NF1, TxLASTRxE, DISPUPD),
```
### Supports Chipsets

  Sunrise PCH, Lewisburg PCH, Apollo Lake SoC, CannonLake-LP SoCs
