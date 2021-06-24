#include <console/console.h>
#include <cpu/power/istep_18.h>
#include <timer.h>

static void p9_tod_clear_error_reg(void)
{
    write_scom(PERV_TOD_ERROR_REG, 0xFFFFFFFFFFFFFFFF);
}

static void init_tod_node(void)
{
    write_scom(PERV_TOD_TX_TTYPE_2_REG, PPC_BIT(PERV_TOD_TX_TTYPE_2_REG_TRIGGER));
    write_scom(PERV_TOD_LOAD_TOD_MOD_REG, PPC_BIT(PERV_TOD_LOAD_TOD_MOD_REG_FSM_TRIGGER));
    write_scom(PERV_TOD_TX_TTYPE_5_REG, PPC_BIT(PERV_TOD_TX_TTYPE_5_REG_TRIGGER));
    write_scom(PERV_TOD_LOAD_TOD_REG, PERV_TOD_LOAD_REG_LOAD_VALUE);
    write_scom(PERV_TOD_START_TOD_REG, PPC_BIT(PERV_TOD_START_TOD_REG_FSM_TRIGGER));
    write_scom(PERV_TOD_TX_TTYPE_4_REG, PPC_BIT(PERV_TOD_TX_TTYPE_4_REG_TRIGGER));

    wait_us(1000, read_scom(PERV_TOD_FSM_REG) & PPC_BIT(PERV_TOD_FSM_REG_IS_RUNNING));

    write_scom(PERV_TOD_ERROR_REG,
        PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_2)
      | PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_4)
      | PPC_BIT(PERV_TOD_ERROR_REG_RX_TTYPE_5));
    write_scom(PERV_TOD_ERROR_MASK_REG,
        PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_0)
      | PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_1)
      | PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_2)
      | PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_3)
      | PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_4)
      | PPC_BIT(PERV_TOD_ERROR_MASK_REG_RX_TTYPE_5));
}

void istep_18_12(void)
{
    printk(BIOS_EMERG, "starting istep 18.12\n");
	report_istep(18,12);
    p9_tod_clear_error_reg();
    init_tod_node();
}
