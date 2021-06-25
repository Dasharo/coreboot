/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_18.h>

void istep_18_11(void)
{
    printk(BIOS_EMERG, "starting istep 18.11\n");
	report_istep(18,11);

    write_scom(PERV_TOD_S_PATH_CTRL_REG, PPC_BITMASK(27, 32));
    write_scom(PERV_TOD_PRI_PORT_0_CTRL_REG, PPC_BITMASK(38, 39));
    write_scom(PERV_TOD_PSS_MSS_CTRL_REG, PPC_BITMASK(1, 2) | PPC_BITMASK(9, 10));
    write_scom(PERV_TOD_SEC_PORT_1_CTRL_REG, 0);
    write_scom(PERV_TOD_PRI_PORT_1_CTRL_REG, 0);
    write_scom(
        PERV_TOD_M_PATH_CTRL_REG,
        PPC_BIT(1)
      | PPC_BIT(8)
      | PPC_BITMASK(14, 15));
    write_scom(PERV_TOD_SEC_PORT_0_CTRL_REG, PPC_BITMASK(38, 39));
    write_scom(
        PERV_TOD_I_PATH_CTRL_REG,
        PPC_BITMASK(8, 11)
      | PPC_BITMASK(14, 15)
      | PPC_BIT(29)
      | PPC_BITMASK(34, 36)
      | PPC_BITMASK(38, 39));
    write_scom(PERV_TOD_CHIP_CTRL_REG, PPC_BITMASK(10, 15));
}
