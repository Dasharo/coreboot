
# SPDX-License-Identifier: GPL-2.0-only

bootblock-y += bootblock.c

romstage-y += romstage.c

ramstage-y += mainboard.c

bootblock-y += die.c
romstage-y += die.c
ramstage-y += die.c
smm-y += die.c

SPD_SOURCES = MT53E512M32D4NQ-053 # 2GBx4
SPD_SOURCES += MT53E1G32D2NP-046 # 4GBx4
