/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_18.h>
#include <timer.h>

static void init_tod_node(void)
{

    write_scom(PERV_TOD_TX_TTYPE_2_REG, PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));
    write_scom(
        PERV_TOD_LOAD_TOD_MOD_REG,
        PPC_BIT(PERV_TOD_LOAD_TOD_MOD_REG_FSM_TRIGGER));
    write_scom(PERV_TOD_TX_TTYPE_5_REG, PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));
    write_scom(
        PERV_TOD_LOAD_TOD_REG,
        (0x3FF << LOAD_TOD_VALUE_OFFSET) | (0xF << WOF_OFFSET));
    write_scom(
        PERV_TOD_START_TOD_REG,
        PPC_BIT(PERV_TOD_START_TOD_REG_FSM_TRIGGER));
    write_scom(PERV_TOD_TX_TTYPE_4_REG, PPC_BIT(PERV_TOD_TX_TTYPE_REG_TRIGGER));

    // wait until TOD is running
    wait_us(
        1000,
        read_scom(PERV_TOD_FSM_REG) & PPC_BIT(PERV_TOD_FSM_REG_IS_RUNNING));
    if(!(read_scom(PERV_TOD_FSM_REG) & PPC_BIT(PERV_TOD_FSM_REG_IS_RUNNING)))
    {
        printk(BIOS_EMERG, "Error: TOD is not running!\n");
    }
    write_scom(PERV_TOD_ERROR_REG,
        PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 2)
      | PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 4)
      | PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_0 + 5));

    write_scom(PERV_TOD_ERROR_MASK_REG,
        PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0)
      | PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 + 1)
      | PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 + 2)
      | PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 + 3)
      | PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 + 4)
      | PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0 + 5));
}

void istep_18_12(void)
{
    printk(BIOS_EMERG, "starting istep 18.12\n");
    report_istep(18,12);
    init_tod_node();
    printk(BIOS_EMERG, "ending istep 18.12\n");
}
