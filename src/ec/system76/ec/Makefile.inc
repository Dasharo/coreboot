ifeq ($(CONFIG_EC_SYSTEM76_EC),y)

all-y += system76_ec.c
all-y += buttons.c
smm-$(CONFIG_DEBUG_SMI) += system76_ec.c

endif
