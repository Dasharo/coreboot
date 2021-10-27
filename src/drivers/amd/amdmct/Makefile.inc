ifeq ($(CONFIG_DRIVERS_AMD_MCT),y)

subdirs-y += wrappers

ifeq ($(CONFIG_DIMM_DDR3),y)
subdirs-y += mct_ddr3
else
subdirs-y += mct
endif

endif