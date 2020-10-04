/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef SOC_MEDIATEK_MT8192_INFRACFG_H
#define SOC_MEDIATEK_MT8192_INFRACFG_H

#include <soc/addressmap.h>
#include <types.h>

struct mt8192_infracfg_regs {
	u32 reserved1[20];
	u32 infra_globalcon_dcmctl;	/* 0x0050 */
	u32 reserved2[7];
	u32 infra_bus_dcm_ctrl;	/* 0x0070 */
	u32 peri_bus_dcm_ctrl;
	u32 mem_dcm_ctrl;
	u32 dfs_mem_dcm_ctrl;
	u32 module_sw_cg_0_set;
	u32 module_sw_cg_0_clr;
	u32 module_sw_cg_1_set;
	u32 module_sw_cg_1_clr;
	u32 module_sw_cg_0_sta;
	u32 module_sw_cg_1_sta;
	u32 module_clk_sel;
	u32 mem_cg_ctrl;
	u32 p2p_rx_clk_on;
	u32 module_sw_cg_2_set;
	u32 module_sw_cg_2_clr;
	u32 module_sw_cg_2_sta;
	u32 reserved3[1];
	u32 dramc_wbr;	/* 0x00b4 */
	u32 reserved4[2];
	u32 module_sw_cg_3_set;	/* 0x00c0 */
	u32 module_sw_cg_3_clr;
	u32 module_sw_cg_3_sta;
	u32 reserved5[5];
	u32 module_sw_cg_4_set;	/* 0x00e0 */
	u32 module_sw_cg_4_clr;
	u32 module_sw_cg_4_sta;
	u32 reserved6[5];
	u32 i2c_dbtool_misc;	/* 0x0100 */
	u32 md_sleep_ctrl_mask;
	u32 pmicw_clock_ctrl;
	u32 reserved7[5];
	u32 infra_globalcon_rst0_set;	/* 0x0120 */
	u32 infra_globalcon_rst0_clr;
	u32 infra_globalcon_rst0_sta;
	u32 reserved8[1];
	u32 infra_globalcon_rst1_set;	/* 0x0130 */
	u32 infra_globalcon_rst1_clr;
	u32 infra_globalcon_rst1_sta;
	u32 reserved9[1];
	u32 infra_globalcon_rst2_set;	/* 0x0140 */
	u32 infra_globalcon_rst2_clr;
	u32 infra_globalcon_rst2_sta;
	u32 reserved10[1];
	u32 infra_globalcon_rst3_set;	/* 0x0150 */
	u32 infra_globalcon_rst3_clr;
	u32 infra_globalcon_rst3_sta;
	u32 reserved11[41];
	u32 infra_topaxi_si0_ctl;	/* 0x0200 */
	u32 infra_topaxi_si1_ctl;
	u32 infra_topaxi_mdbus_ctl;
	u32 infra_mci_si0_ctl;
	u32 infra_mci_si1_ctl;
	u32 infra_mci_si2_ctl;
	u32 infra_mci_async_ctl;
	u32 infra_mci_cg_mfg_sec_sta;
	u32 infra_topaxi_protecten;
	u32 infra_topaxi_protecten_sta0;
	u32 infra_topaxi_protecten_sta1;
	u32 reserved12[1];
	u32 infra_apb_async_sta;	/* 0x0230 */
	u32 infra_topaxi_si2_ctl;
	u32 infra_topaxi_fmem_mdhw_ctrl;
	u32 infra_conn_gals_ctl;
	u32 infra_mci_trans_con_read;
	u32 infra_mci_trans_con_write;
	u32 infra_mci_id_remap_con;
	u32 infra_mci_emi_trans_con;
	u32 infra_topaxi_protecten_1;
	u32 infra_topaxi_protecten_sta0_1;
	u32 infra_topaxi_protecten_sta1_1;
	u32 reserved13[1];
	u32 infra_topaxi_aslice_ctrl;	/* 0x0260 */
	u32 infra_topaxi_aslice_ctrl_1;
	u32 infra_topaxi_aslice_ctrl_2;
	u32 infra_topaxi_aslice_ctrl_3;
	u32 infra_topaxi_mi_ctrl;
	u32 infra_topaxi_cbip_aslice_ctrl;
	u32 infra_topaxi_cbip_slice_ctrl;
	u32 infra_top_master_sideband;
	u32 infra_ssusb_dev;
	u32 reserved14[1];
	u32 infra_topaxi_emi_gmc_l2c_ctrl;	/* 0x0288 */
	u32 infra_topaxi_cbip_slice_ctrl_1;
	u32 infra_mfg_slave_gals_ctrl;
	u32 infra_mfg_master_m0_gals_ctrl;
	u32 infra_mfg_master_m1_gals_ctrl;
	u32 infra_top_master_sideband_1;
	u32 infra_topaxi_protecten_set;
	u32 infra_topaxi_protecten_clr;
	u32 infra_topaxi_protecten_1_set;
	u32 infra_topaxi_protecten_1_clr;
	u32 infra_topaxi_cbip_slice_ctrl_2;
	u32 reserved15[1];
	u32 infra_topaxi_aslice_ctrl_4;	/* 0x02b8 */
	u32 reserved16[1];
	u32 infra_topaxi_protecten_mcu;	/* 0x02c0 */
	u32 infra_topaxi_protecten_mcu_set;
	u32 infra_topaxi_protecten_mcu_clr;
	u32 reserved17[1];
	u32 infra_topaxi_protecten_mm;	/* 0x02d0 */
	u32 infra_topaxi_protecten_mm_set;
	u32 infra_topaxi_protecten_mm_clr;
	u32 reserved18[1];
	u32 infra_topaxi_protecten_mcu_sta0;	/* 0x02e0 */
	u32 infra_topaxi_protecten_mcu_sta1;
	u32 infra_topaxi_protecten_mm_sta0;
	u32 infra_topaxi_protecten_mm_sta1;
	u32 reserved19[1];
	u32 infra_apu_master_m0_gals_ctl;	/* 0x02f4 */
	u32 infra_apu_master_m1_gals_ctl;
	u32 infra_topaxi_bus_dbg_con_ao;
	u32 md1_bank0_map0;
	u32 md1_bank0_map1;
	u32 md1_bank0_map2;
	u32 md1_bank0_map3;
	u32 md1_bank1_map0;
	u32 md1_bank1_map1;
	u32 md1_bank1_map2;
	u32 md1_bank1_map3;
	u32 md1_bank4_map0;
	u32 md1_bank4_map1;
	u32 md1_bank4_map2;
	u32 md1_bank4_map3;
	u32 md2_bank0_map0;
	u32 md2_bank0_map1;
	u32 md2_bank0_map2;
	u32 md2_bank0_map3;
	u32 reserved20[4];
	u32 md2_bank4_map0;	/* 0x0350 */
	u32 md2_bank4_map1;
	u32 md2_bank4_map2;
	u32 md2_bank4_map3;
	u32 c2k_config;
	u32 c2k_status;
	u32 c2k_spm_ctrl;
	u32 reserved21[1];
	u32 ap2md_dummy;	/* 0x0370 */
	u32 reserved22[3];
	u32 conn_map0;	/* 0x0380 */
	u32 cldma_map0;
	u32 conn_map1;
	u32 conn_bus_con;
	u32 mcusys_dfd_map;
	u32 conn_map2;
	u32 conn_map3;
	u32 conn_map4;
	u32 module_clk_sel_set;
	u32 module_clk_sel_clr;
	u32 pmicw_clock_ctrl_set;
	u32 pmicw_clock_ctrl_clr;
	u32 dramc_wbr_set;
	u32 dramc_wbr_clr;
	u32 topaxi_si0_ctl_set;
	u32 topaxi_si0_ctl_clr;
	u32 topaxi_si1_ctl_set;
	u32 topaxi_si1_ctl_clr;
	u32 reserved23[14];
	u32 peri_cci_sideband_con;	/* 0x0400 */
	u32 mfg_cci_sideband_con;
	u32 reserved24[2];
	u32 infra_pwm_cksw_ctrl;	/* 0x0410 */
	u32 reserved25[59];
	u32 infra_ao_dbg_con0;	/* 0x0500 */
	u32 infra_ao_dbg_con1;
	u32 infra_ao_dbg_con2;
	u32 infra_ao_dbg_con3;
	u32 md_dbg_ck_con;
	u32 infra_ao_dbg_sta;
	u32 reserved26[58];
	u32 mfg_misc_con;	/* 0x0600 */
	u32 reserved27[63];
	u32 infra_rsvd0;	/* 0x0700 */
	u32 infra_rsvd1;
	u32 infra_rsvd2;
	u32 infra_rsvd3;
	u32 infra_topaxi_protecten_2;
	u32 infra_topaxi_protecten_set_2;
	u32 infra_topaxi_protecten_clr_2;
	u32 reserved28[1];
	u32 infra_topaxi_protecten_sta0_2;	/* 0x0720 */
	u32 infra_topaxi_protecten_sta1_2;
	u32 reserved29[2];
	u32 infra_globalcon_rst4_set;	/* 0x0730 */
	u32 infra_globalcon_rst4_clr;
	u32 infra_globalcon_rst4_sta;
	u32 infra_ao_sec_rst_con4;
	u32 reserved30[16];
	u32 mcu2emi_m0_parity;	/* 0x0780 */
	u32 mcu2emi_m0_parity_dbg_aw_1;
	u32 mcu2emi_m0_parity_dbg_aw_2;
	u32 mcu2emi_m0_parity_dbg_ar_1;
	u32 mcu2emi_m0_parity_dbg_ar_2;
	u32 mcu2emi_m1_parity;
	u32 mcu2emi_m1_parity_dbg_aw_1;
	u32 mcu2emi_m1_parity_dbg_aw_2;
	u32 mcu2emi_m1_parity_dbg_ar_1;
	u32 mcu2emi_m1_parity_dbg_ar_2;
	u32 mcu2ifr_reg_parity;
	u32 mcu2ifr_reg_parity_dbg_aw_1;
	u32 mcu2ifr_reg_parity_dbg_aw_2;
	u32 mcu2ifr_reg_parity_dbg_ar_1;
	u32 mcu2ifr_reg_parity_dbg_ar_2;
	u32 ifr_l3c2mcu_parity;
	u32 ifr_l3c2mcu_parity_dbg_r_1;
	u32 reserved31[47];
	u32 md1_sbc_key0;	/* 0x0880 */
	u32 md1_sbc_key1;
	u32 md1_sbc_key2;
	u32 md1_sbc_key3;
	u32 md1_sbc_key4;
	u32 md1_sbc_key5;
	u32 md1_sbc_key6;
	u32 md1_sbc_key7;
	u32 md1_sbc_key_lock;
	u32 reserved32[1];
	u32 md1_misc_lock;	/* 0x08a8 */
	u32 md1_misc;
	u32 c2k_sbc_key0;
	u32 c2k_sbc_key1;
	u32 c2k_sbc_key2;
	u32 c2k_sbc_key3;
	u32 c2k_sbc_key4;
	u32 c2k_sbc_key5;
	u32 c2k_sbc_key6;
	u32 c2k_sbc_key7;
	u32 c2k_sbc_key_lock;
	u32 reserved33[11];
	u32 infra_bonding;	/* 0x0900 */
	u32 reserved34[63];
	u32 infra_ao_scpsys_apb_async_sta;	/* 0x0a00 */
	u32 infra_ao_md32_tx_apb_async_sta;
	u32 infra_ao_md32_rx_apb_async_sta;
	u32 infra_ao_cksys_apb_async_sta;
	u32 infra_ao_pmic_wrap_tx_apb_async_sta;
	u32 infra_mcu2apu_asl0_ctl;
	u32 infra_mcu2reg_asl0_ctl;
	u32 infra_mcu_decoder_infra_ctl;
	u32 infra_mcu_decoder_sta0;
	u32 infra_mcu_decoder_sta1;
	u32 infra_idle_async_bit_en_0;
	u32 infra_apu_slave_gals_ctrl;
	u32 infra_aximem_idle_bit_en_0;
	u32 infra_mcu_path_sync_ctl;
	u32 infra_conn2ap_int_mask;
	u32 infra_mcu_pwr_ctl_mask;
	u32 infra_md_rsv;
	u32 reserved35[7];
	u32 infra_mem_26m_cksel;	/* 0x0a60 */
	u32 reserved36[39];
	u32 pll_ulposc_con0;	/* 0x0b00 */
	u32 pll_ulposc_con1;
	u32 reserved37[2];
	u32 pll_auxadc_con0;	/* 0x0b10 */
	u32 scp_infra_irq_set;
	u32 scp_infra_irq_clr;
	u32 scp_infra_ctrl;
	u32 reserved38[24];
	u32 infra_topaxi_protecten_vdnr;	/* 0x0b80 */
	u32 infra_topaxi_protecten_vdnr_set;
	u32 infra_topaxi_protecten_vdnr_clr;
	u32 infra_topaxi_protecten_vdnr_sta0;
	u32 infra_topaxi_protecten_vdnr_sta1;
	u32 reserved39[3];
	u32 infra_topaxi_protecten_vdnr_1;	/* 0x0ba0 */
	u32 infra_topaxi_protecten_vdnr_set_1;
	u32 infra_topaxi_protecten_vdnr_clr_1;
	u32 infra_topaxi_protecten_vdnr_sta0_1;
	u32 infra_topaxi_protecten_vdnr_sta1_1;
	u32 reserved40[19];
	u32 cldma_ctrl;	/* 0x0c00 */
	u32 reserved41[63];
	u32 infrabus_dbg0;	/* 0x0d00 */
	u32 infrabus_dbg1;
	u32 infrabus_dbg2;
	u32 infrabus_dbg3;
	u32 infrabus_dbg4;
	u32 infrabus_dbg5;
	u32 infrabus_dbg6;
	u32 infrabus_dbg7;
	u32 infrabus_dbg8;
	u32 infrabus_dbg9;
	u32 infrabus_dbg10;
	u32 infrabus_dbg11;
	u32 infrabus_dbg12;
	u32 infrabus_dbg13;
	u32 infrabus_dbg14;
	u32 infrabus_dbg15;
	u32 infrabus_dbg16;
	u32 infrabus_dbg17;
	u32 infrabus_dbg18;
	u32 infrabus_dbg19;
	u32 infrabus_dbg20;
	u32 infrabus_dbg21;
	u32 infrabus_dbg22;
	u32 infrabus_dbg23;
	u32 infrabus_dbg24;
	u32 infrabus_dbg25;
	u32 infrabus_dbg26;
	u32 infrabus_dbg27;
	u32 infrabus_dbg28;
	u32 infrabus_dbg29;
	u32 infrabus_dbg30;
	u32 infrabus_dbg31;
	u32 infrabus_dbg32;
	u32 infrabus_dbg33;
	u32 infrabus_dbg34;
	u32 infrabus_dbg35;
	u32 infrabus_dbg36;
	u32 infrabus_dbg37;
	u32 infrabus_dbg38;
	u32 infrabus_dbg39;
	u32 infrabus_dbg40;
	u32 infrabus_dbg41;
	u32 infrabus_dbg42;
	u32 infrabus_dbg43;
	u32 infrabus_dbg44;
	u32 infrabus_dbg45;
	u32 reserved42[4];
	u32 infra_topaxi_protecten_mm_2;	/* 0x0dc8 */
	u32 infra_topaxi_protecten_mm_set_2;
	u32 infra_topaxi_protecten_mm_clr_2;
	u32 infra_topaxi_protecten_mm_sta0_2;
	u32 infra_topaxi_protecten_mm_sta1_2;
	u32 reserved43[5];
	u32 infrabus_dbg_mask2;	/* 0x0df0 */
	u32 reserved44[19];
	u32 infra_ao_sec_mm0;	/* 0x0e40 */
	u32 infra_ao_sec_mm1;
	u32 infra_ao_sec_mm2;
	u32 infra_ao_sec_mm3;
	u32 infra_ao_sec_mm4;
	u32 infra_ao_sec_mm5;
	u32 infra_ao_sec_mm6;
	u32 infra_ao_sec_mm7;
	u32 infra_ao_sec_mm8;
	u32 infra_ao_sec_mm9;
	u32 infra_ao_sec_mm10;
	u32 infra_ao_sec_mm11;
	u32 infra_ao_sec_mm12;
	u32 infra_ao_sec_mm13;
	u32 infra_ao_sec_mm14;
	u32 infra_ao_sec_mm15;
	u32 infra_ao_sec_mm16;
	u32 reserved45[5];
	u32 infra_ao_mm_hang_free;	/* 0x0e98 */
	u32 infra_ao_module_hang_free;
	u32 reserved46[24];
	u32 infra_misc;	/* 0x0f00 */
	u32 infra_acp;
	u32 misc_config;
	u32 infra_misc2;
	u32 mdsys_misc_con;
	u32 reserved47[27];
	u32 infra_ao_sec_con;	/* 0x0f80 */
	u32 infra_ao_sec_cg_con0;
	u32 infra_ao_sec_cg_con1;
	u32 infra_ao_sec_rst_con0;
	u32 infra_ao_sec_rst_con1;
	u32 infra_ao_sec_rst_con2;
	u32 reserved48[1];
	u32 infra_ao_sec_cg_con2;	/* 0x0f9c */
	u32 infra_ao_sec_rst_con3;
	u32 infra_ao_sec_cg_con3;
	u32 reserved49[2];
	u32 infra_ao_sec_hyp;	/* 0x0fb0 */
	u32 infra_ao_sec_mfg_hyp;
};

check_member(mt8192_infracfg_regs, infra_globalcon_dcmctl, 0x0050);
check_member(mt8192_infracfg_regs, infra_bus_dcm_ctrl, 0x0070);
check_member(mt8192_infracfg_regs, module_sw_cg_3_set, 0x00c0);
check_member(mt8192_infracfg_regs, module_sw_cg_4_set, 0x00e0);
check_member(mt8192_infracfg_regs, i2c_dbtool_misc, 0x0100);
check_member(mt8192_infracfg_regs, infra_globalcon_rst0_set, 0x0120);
check_member(mt8192_infracfg_regs, infra_topaxi_si0_ctl, 0x0200);
check_member(mt8192_infracfg_regs, md2_bank4_map0, 0x0350);
check_member(mt8192_infracfg_regs, conn_map0, 0x0380);
check_member(mt8192_infracfg_regs, peri_cci_sideband_con, 0x0400);
check_member(mt8192_infracfg_regs, infra_pwm_cksw_ctrl, 0x0410);
check_member(mt8192_infracfg_regs, infra_ao_dbg_con0, 0x0500);
check_member(mt8192_infracfg_regs, mfg_misc_con, 0x0600);
check_member(mt8192_infracfg_regs, infra_rsvd0, 0x0700);
check_member(mt8192_infracfg_regs, infra_globalcon_rst4_set, 0x0730);
check_member(mt8192_infracfg_regs, mcu2emi_m0_parity, 0x0780);
check_member(mt8192_infracfg_regs, md1_sbc_key0, 0x0880);
check_member(mt8192_infracfg_regs, infra_bonding, 0x0900);
check_member(mt8192_infracfg_regs, infra_ao_scpsys_apb_async_sta, 0x0a00);
check_member(mt8192_infracfg_regs, infra_mem_26m_cksel, 0x0a60);
check_member(mt8192_infracfg_regs, pll_ulposc_con0, 0x0b00);
check_member(mt8192_infracfg_regs, pll_auxadc_con0, 0x0b10);
check_member(mt8192_infracfg_regs, infra_topaxi_protecten_vdnr, 0x0b80);
check_member(mt8192_infracfg_regs, infra_topaxi_protecten_vdnr_1, 0x0ba0);
check_member(mt8192_infracfg_regs, cldma_ctrl, 0x0c00);
check_member(mt8192_infracfg_regs, infrabus_dbg0, 0x0d00);
check_member(mt8192_infracfg_regs, infra_topaxi_protecten_mm_2, 0x0dc8);
check_member(mt8192_infracfg_regs, infrabus_dbg_mask2, 0x0df0);
check_member(mt8192_infracfg_regs, infra_ao_sec_mm0, 0x0e40);
check_member(mt8192_infracfg_regs, infra_ao_mm_hang_free, 0x0e98);
check_member(mt8192_infracfg_regs, infra_misc, 0x0f00);
check_member(mt8192_infracfg_regs, infra_ao_sec_con, 0x0f80);
check_member(mt8192_infracfg_regs, infra_ao_sec_hyp, 0x0fb0);
check_member(mt8192_infracfg_regs, infra_ao_sec_mfg_hyp, 0x0fb4);

static struct mt8192_infracfg_regs *const mt8192_infracfg =
	(void *)INFRACFG_AO_BASE;

#endif	/* SOC_MEDIATEK_MT8192_INFRACFG_H */
