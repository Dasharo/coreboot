ifeq ($(CONFIG_EC_CLEVO_IT5570),y)

ramstage-y += chip.c
ramstage-y += ec.c

smm-y += ec.c

endif
