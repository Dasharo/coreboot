# SPDX-License-Identifier: GPL-2.0-only

romstage-y += dimmSpd.c

ramstage-y += iommu.c
ramstage-y += northbridge.c

bootblock-y += state_machine.c
romstage-y += state_machine.c
ramstage-y += state_machine.c
