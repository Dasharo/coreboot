ifeq ($(CONFIG_DRIVERS_AMD_MCT),y)

subdirs-y += wrappers

ifeq ($(CONFIG_DIMM_DDR3),y)
subdirs-y += mct/mct_ddr3
else
subdirs-y += mct/mct_ddr2
endif

endif
