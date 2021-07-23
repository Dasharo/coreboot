/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/power/istep_18.h>

#define MDMT_TOD_GRID_CYCLE_STAGING_DELAY (6)
#define FREQ_X_MHZ (0x708)
#define TOD_GRID_PS (400)
#define PU_PB_ELINK_RT_DELAY_CTL_REG (0x05013419)
#define PU_PB_ELINK_DLY_0123_REG (0x0501340E)
// need to actually check the topology
#define MDMT (1)

static uint32_t calculate_topology_dealy(void)
{
  // in simple topology with one proc it is enough to assign 0
  // with multiple processors this will get more complicated
  // see calculate_node_link_delay() in Hosboot

  if(MDMT)
  {
    uint32_t node_link_delay = 0;
    uint32_t longest_topology_delay = 0;

    return
      - longest_topology_delay
      + node_link_delay
      + MDMT_TOD_GRID_CYCLE_STAGING_DELAY;
  }

  write_scom(PU_PB_ELINK_RT_DELAY_CTL_REG, PPC_BITMASK(2, 3));
  uint64_t l_bus_mode_reg = read_scom(PU_PB_ELINK_DLY_0123_REG);

  uint32_t bus_delay =
    (l_bus_mode_reg & PPC_BITMASK(36, 47) >> 16)
  + (l_bus_mode_reg & PPC_BITMASK(52, 63));

  return (uint32_t)(1 + (
    (double)(bus_delay * 8 * 1000000)
    / (double)(4 * FREQ_X_MHZ * TOD_GRID_PS)));
}

static int64_t calculate_m_path(uint64_t value)
{
  uint64_t value |=
    (read_scom(PERV_ROOT_CTRL8_SCOM) & P9A_PERV_ROOT_CTRL8_TP_PLL_CLKIN_SEL9_DC)
    ? PPC_BIT(PERV_TOD_M_PATH_CTRL_REG_STEP_CREATE_DUAL_EDGE_DISABLE)
    : 0;
  if(MDMT)
  {
    value &= ~PPC_BIT(0);
    value &= ~PPC_BIT(2);
    value &= ~PPC_BITMASK(5, 7);
    value |= PPC_BIT(8);
    value &= ~PPC_BITMASK(9, 11);
    value |= PPC_BIT(13);
    value &= ~PPC_BITMASK(14, 15);
    value |= PPC_BIT(1);
    value &= ~PPC_BITMASK(24, 25);
  }
  printk(BIOS_EMERG, "calculated %lx\n", value);
  return value;
}

void istep_18_11(void)
{
    printk(BIOS_EMERG, "starting istep 18.11\n");
    report_istep(18,11);

    write_scom(PERV_TOD_S_PATH_CTRL_REG, PPC_BITMASK(27, 32));
    write_scom(PERV_TOD_PRI_PORT_0_CTRL_REG, calculate_topology_dealy());
    write_scom(PERV_TOD_PSS_MSS_CTRL_REG, PPC_BITMASK(1, 2) | PPC_BITMASK(9, 10)); // this value only for drawer master, else <1> should be cleared
    write_scom(PERV_TOD_SEC_PORT_1_CTRL_REG, 0); // just 0
    write_scom(PERV_TOD_PRI_PORT_1_CTRL_REG, 0); // just 0
    write_scom(
        PERV_TOD_M_PATH_CTRL_REG, calculate_m_path(read_scom(PERV_TOD_M_PATH_CTRL_REG));
      //   PPC_BIT(1)
      // | PPC_BIT(8)
      // | PPC_BITMASK(14, 15));
    write_scom(PERV_TOD_SEC_PORT_0_CTRL_REG, PPC_BITMASK(38, 39));
    write_scom(
        PERV_TOD_I_PATH_CTRL_REG,
        PPC_BITMASK(8, 11)
      | PPC_BITMASK(14, 15)
      | PPC_BIT(29)
      | PPC_BITMASK(34, 36)
      | PPC_BITMASK(38, 39));

    write_scom(PERV_TOD_CHIP_CTRL_REG, PPC_BITMASK(10, 15));
    printk(BIOS_EMERG, "ending istep 18.11\n");
}
