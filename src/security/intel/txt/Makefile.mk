## SPDX-License-Identifier: GPL-2.0-only

romstage-$(CONFIG_INTEL_TXT_LIB) += txtlib.c

ifeq ($(CONFIG_INTEL_TXT),y)

all-y += logging.c

romstage-y += romstage.c
romstage-y += getsec_sclean.S
romstage-y += getsec.c

romstage-y += common.c

ramstage-y += common.c
ramstage-y += getsec.c
ramstage-y += getsec_enteraccs.S
ramstage-y += ramstage.c

endif # INTEL_TXT
