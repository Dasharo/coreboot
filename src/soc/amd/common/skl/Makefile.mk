# SPDX-License-Identifier: GPL-2.0-only

ramstage-$(CONFIG_LAUNCH_DRTM_PAYLOAD) += skinit.c

SKL_SOURCE := $(src)/../3rdparty/secure-kernel-loader
SKL_TARGET := $(SKL_SOURCE)/skl.bin

SKL := $(obj)/skl.bin

$(SKL): $(obj)/build.h
	printf "    MAKE       $(subst $(obj)/,,$(@))\n"
	$(MAKE) -C $(SKL_SOURCE) $(SKL_MAKEARGS) CC=$(HOSTCC) DEBUG=y
	mv $(SKL_TARGET) $@

.PHONY: $(SKL)

SKL_CBFS := $(CONFIG_CBFS_PREFIX)/drtm_payload
$(SKL_CBFS)-file := $(SKL)
$(SKL_CBFS)-type := raw
$(SKL_CBFS)-compression := $(CBFS_PAYLOAD_COMPRESS_FLAG)
$(SKL_CBFS)-options := $(ADDITIONAL_PAYLOAD_OPTIONS)
cbfs-files-y += $(SKL_CBFS)

check-ramstage-overlap-files += $(SKL_CBFS)
