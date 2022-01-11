/* SPDX-License-Identifier: GPL-2.0-only */

#include <stdint.h>
#include <acpi/acpi.h>
#include <arch/cpu.h>
#include <cpu/amd/common/common.h>
#include <device/pci_ops.h>
#include <string.h>
#include <cbmem.h>
#include <console/console.h>
#include <northbridge/amd/amdfam10/debug.h>
#include <northbridge/amd/amdfam10/raminit.h>
#include <northbridge/amd/amdfam10/amdfam10.h>
#include <timestamp.h>

void __weak activate_spd_rom(const struct mem_controller *ctrl)
{
}

void fam15h_switch_dct(u32 dev, u8 dct)
{
	u32 dword;

	dword = pci_read_config32(dev, 0x10c);
	dword &= ~0x1;
	dword |= (dct & 0x1);
	pci_write_config32(dev, 0x10c, dword);
}

static inline void fam15h_switch_nb_pstate_config_reg(u32 dev, u8 nb_pstate)
{
	u32 dword;

	dword = pci_read_config32(dev, 0x10c);
	dword &= ~(0x3 << 4);
	dword |= (nb_pstate & 0x3) << 4;
	pci_write_config32(dev, 0x10c, dword);
}

u32 Get_NB32_DCT(u32 dev, u8 dct, u32 reg)
{
	if (is_fam15h()) {
		/* Obtain address of function 0x1 */
		u32 dev_map = (dev & (~(0x7 << 12))) | (0x1 << 12);
		fam15h_switch_dct(dev_map, dct);
		return pci_read_config32(dev, reg);
	} else {
		return pci_read_config32(dev, (0x100 * dct) + reg);
	}
}

void Set_NB32_DCT(u32 dev, u8 dct, u32 reg, u32 val)
{
	if (is_fam15h()) {
		/* Obtain address of function 0x1 */
		u32 dev_map = (dev & (~(0x7 << 12))) | (0x1 << 12);
		fam15h_switch_dct(dev_map, dct);
		pci_write_config32(dev, reg, val);
	} else {
		pci_write_config32(dev, (0x100 * dct) + reg, val);
	}
}

u32 Get_NB32_DCT_NBPstate(u32 dev, u8 dct, u8 nb_pstate, u32 reg)
{
	if (is_fam15h()) {
		/* Obtain address of function 0x1 */
		u32 dev_map = (dev & (~(0x7 << 12))) | (0x1 << 12);
		fam15h_switch_dct(dev_map, dct);
		fam15h_switch_nb_pstate_config_reg(dev_map, nb_pstate);
		return pci_read_config32(dev, reg);
	} else {
		return pci_read_config32(dev, (0x100 * dct) + reg);
	}
}

void Set_NB32_DCT_NBPstate(u32 dev, u8 dct, u8 nb_pstate, u32 reg, u32 val)
{
	if (is_fam15h()) {
		/* Obtain address of function 0x1 */
		u32 dev_map = (dev & (~(0x7 << 12))) | (0x1 << 12);
		fam15h_switch_dct(dev_map, dct);
		fam15h_switch_nb_pstate_config_reg(dev_map, nb_pstate);
		pci_write_config32(dev, reg, val);
	} else {
		pci_write_config32(dev, (0x100 * dct) + reg, val);
	}
}

u32 Get_NB32_index_wait_DCT(u32 dev, u8 dct, u32 index_reg, u32 index)
{
	if (is_fam15h()) {
		/* Obtain address of function 0x1 */
		u32 dev_map = (dev & (~(0x7 << 12))) | (0x1 << 12);
		fam15h_switch_dct(dev_map, dct);
		return Get_NB32_index_wait(dev, index_reg, index);
	} else {
		return Get_NB32_index_wait(dev, (0x100 * dct) + index_reg, index);
	}
}

void Set_NB32_index_wait_DCT(u32 dev, u8 dct, u32 index_reg, u32 index, u32 data)
{
	if (is_fam15h()) {
		/* Obtain address of function 0x1 */
		u32 dev_map = (dev & (~(0x7 << 12))) | (0x1 << 12);
		fam15h_switch_dct(dev_map, dct);
		Set_NB32_index_wait(dev, index_reg, index, data);
	} else {
		Set_NB32_index_wait(dev, (0x100 * dct) + index_reg, index, data);
	}
}

static u16 voltage_index_to_mv(u8 index)
{
	if (index & 0x8)
		return 1150;
	if (index & 0x4)
		return 1250;
	else if (index & 0x2)
		return 1350;
	else
		return 1500;
}

u16 mct_MaxLoadFreq(u8 count, u8 highest_rank_count, u8 registered, u8 voltage, u16 freq)
{
	/* FIXME
	 * Mainboards need to be able to specify the maximum number of DIMMs installable per channel
	 * For now assume a maximum of 2 DIMMs per channel can be installed
	 */
	u8 MaxDimmsInstallable = 2;

	/* Return limited maximum RAM frequency */
	if (CONFIG(DIMM_DDR2)) {
		if (CONFIG(DIMM_REGISTERED) && registered) {
			/* K10 BKDG Rev. 3.62 Table 53 */
			if (count > 2) {
				/* Limit to DDR2-533 */
				if (freq > 266) {
					freq = 266;
					print_tf(__func__, ": More than 2 registered DIMMs on channel; limiting to DDR2-533\n");
				}
			}
		} else {
			/* K10 BKDG Rev. 3.62 Table 52 */
			if (count > 1) {
				/* Limit to DDR2-800 */
				if (freq > 400) {
					freq = 400;
					print_tf(__func__, ": More than 1 unbuffered DIMM on channel; limiting to DDR2-800\n");
				}
			}
		}
	} else if (CONFIG(DIMM_DDR3)) {
		if (voltage == 0) {
			printk(BIOS_DEBUG, "%s: WARNING: Mainboard DDR3 voltage unknown, assuming 1.5V!\n", __func__);
			voltage = 0x1;
		}

		if (is_fam15h()) {
			if (CONFIG_CPU_SOCKET_TYPE == 0x15) {
				/* Socket G34 */
				if (CONFIG(DIMM_REGISTERED) && registered) {
					/* Fam15h BKDG Rev. 3.14 Table 27 */
					if (voltage & 0x4) {
						/* 1.25V */
						if (count > 1) {
							if (highest_rank_count > 1) {
								/* Limit to DDR3-1066 */
								if (freq > 533) {
									freq = 533;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1066\n", __func__, voltage_index_to_mv(voltage));
								}
							} else {
								/* Limit to DDR3-1333 */
								if (freq > 666) {
									freq = 666;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
								}
							}
						} else {
							/* Limit to DDR3-1333 */
							if (freq > 666) {
								freq = 666;
								printk(BIOS_DEBUG, "%s: 1 registered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
							}
						}
					} else if (voltage & 0x2) {
						/* 1.35V */
						if (count > 1) {
							/* Limit to DDR3-1333 */
							if (freq > 666) {
								freq = 666;
								printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
							}
						} else {
							/* Limit to DDR3-1600 */
							if (freq > 800) {
								freq = 800;
								printk(BIOS_DEBUG, "%s: 1 registered DIMM on %dmV channel; limiting to DDR3-1600\n", __func__, voltage_index_to_mv(voltage));
							}
						}
					} else if (voltage & 0x1) {
						/* 1.50V */
						if (count > 1) {
							/* Limit to DDR3-1600 */
							if (freq > 800) {
								freq = 800;
								printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1600\n", __func__, voltage_index_to_mv(voltage));
							}
						} else {
							/* Limit to DDR3-1866 */
							if (freq > 933) {
								freq = 933;
								printk(BIOS_DEBUG, "%s: 1 registered DIMM on %dmV channel; limiting to DDR3-1866\n", __func__, voltage_index_to_mv(voltage));
							}
						}
					}
				} else {
					/* Fam15h BKDG Rev. 3.14 Table 26 */
					if (voltage & 0x4) {
						/* 1.25V */
						if (count > 1) {
							if (highest_rank_count > 1) {
								/* Limit to DDR3-1066 */
								if (freq > 533) {
									freq = 533;
									printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1066\n", __func__, voltage_index_to_mv(voltage));
								}
							} else {
								/* Limit to DDR3-1333 */
								if (freq > 666) {
									freq = 666;
									printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
								}
							}
						} else {
							/* Limit to DDR3-1333 */
							if (freq > 666) {
								freq = 666;
								printk(BIOS_DEBUG, "%s: 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
							}
						}
					} else if (voltage & 0x2) {
						/* 1.35V */
						if (MaxDimmsInstallable > 1) {
							/* Limit to DDR3-1333 */
							if (freq > 666) {
								freq = 666;
								printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
							}
						} else {
							/* Limit to DDR3-1600 */
							if (freq > 800) {
								freq = 800;
								printk(BIOS_DEBUG, "%s: 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1600\n", __func__, voltage_index_to_mv(voltage));
							}
						}
					} else if (voltage & 0x1) {
						if (MaxDimmsInstallable == 1) {
							if (count > 1) {
								/* Limit to DDR3-1600 */
								if (freq > 800) {
									freq = 800;
									printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1600\n", __func__, voltage_index_to_mv(voltage));
								}
							} else {
								/* Limit to DDR3-1866 */
								if (freq > 933) {
									freq = 933;
									printk(BIOS_DEBUG, "%s: 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1866\n", __func__, voltage_index_to_mv(voltage));
								}
							}
						} else {
							if (count > 1) {
								if (highest_rank_count > 1) {
									/* Limit to DDR3-1333 */
									if (freq > 666) {
										freq = 666;
										printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
									}
								} else {
									/* Limit to DDR3-1600 */
									if (freq > 800) {
										freq = 800;
										printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1600\n", __func__, voltage_index_to_mv(voltage));
									}
								}
							} else {
								/* Limit to DDR3-1600 */
								if (freq > 800) {
									freq = 800;
									printk(BIOS_DEBUG, "%s: 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1600\n", __func__, voltage_index_to_mv(voltage));
								}
							}
						}
					}
				}
			} else if (CONFIG_CPU_SOCKET_TYPE == 0x14) {
				/* Socket C32 */
				if (CONFIG(DIMM_REGISTERED) && registered) {
					/* Fam15h BKDG Rev. 3.14 Table 30 */
					if (voltage & 0x4) {
						/* 1.25V */
						if (count > 1) {
							if (highest_rank_count > 2) {
								/* Limit to DDR3-800 */
								if (freq > 400) {
									freq = 400;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-800\n", __func__, voltage_index_to_mv(voltage));
								}
							} else {
								/* Limit to DDR3-1333 */
								if (freq > 666) {
									freq = 666;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
								}
							}
						} else {
							/* Limit to DDR3-1333 */
							if (freq > 666) {
								freq = 666;
								printk(BIOS_DEBUG, "%s: 1 registered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
							}
						}
					} else if (voltage & 0x2) {
						/* 1.35V */
						if (count > 1) {
							if (highest_rank_count > 2) {
								/* Limit to DDR3-800 */
								if (freq > 400) {
									freq = 400;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-800\n", __func__, voltage_index_to_mv(voltage));
								}
							} else if (highest_rank_count > 1) {
								/* Limit to DDR3-1066 */
								if (freq > 533) {
									freq = 533;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1066\n", __func__, voltage_index_to_mv(voltage));
								}
							} else {
								/* Limit to DDR3-1333 */
								if (freq > 666) {
									freq = 666;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
								}
							}
						} else {
							/* Limit to DDR3-1600 */
							if (freq > 800) {
								freq = 800;
								printk(BIOS_DEBUG, "%s: 1 registered DIMM on %dmV channel; limiting to DDR3-1600\n", __func__, voltage_index_to_mv(voltage));
							}
						}
					} else if (voltage & 0x1) {
						/* 1.50V */
						if (count > 1) {
							if (highest_rank_count > 2) {
								/* Limit to DDR3-800 */
								if (freq > 400) {
									freq = 400;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-800\n", __func__, voltage_index_to_mv(voltage));
								}
							} else if (highest_rank_count > 1) {
								/* Limit to DDR3-1066 */
								if (freq > 533) {
									freq = 533;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1066\n", __func__, voltage_index_to_mv(voltage));
								}
							} else {
								/* Limit to DDR3-1333 */
								if (freq > 666) {
									freq = 666;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
								}
							}
						} else {
							if (highest_rank_count > 2) {
								/* Limit to DDR3-1333 */
								if (freq > 666) {
									freq = 666;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
								}
							} else {
								/* Limit to DDR3-1600 */
								if (freq > 800) {
									freq = 800;
									printk(BIOS_DEBUG, "%s: More than 1 registered DIMM on %dmV channel; limiting to DDR3-1600\n", __func__, voltage_index_to_mv(voltage));
								}
							}
						}
					}
				} else {
					/* Fam15h BKDG Rev. 3.14 Table 29 */
					if (voltage & 0x4) {
						/* 1.25V */
						if (count > 1) {
							/* Limit to DDR3-1066 */
							if (freq > 533) {
								freq = 533;
								printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1066\n", __func__, voltage_index_to_mv(voltage));
							}
						} else {
							/* Limit to DDR3-1333 */
							if (freq > 666) {
								freq = 666;
								printk(BIOS_DEBUG, "%s: 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
							}
						}
					} else if (voltage & 0x2) {
						if (count > 1) {
							if (highest_rank_count > 1) {
								/* Limit to DDR3-1066 */
								if (freq > 533) {
									freq = 533;
									printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1066\n", __func__, voltage_index_to_mv(voltage));
								}
							} else {
								/* Limit to DDR3-1333 */
								if (freq > 666) {
									freq = 666;
									printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
								}
							}
						} else {
							/* Limit to DDR3-1333 */
							if (freq > 666) {
								freq = 666;
								printk(BIOS_DEBUG, "%s: 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
							}
						}
					} else if (voltage & 0x1) {
						if (MaxDimmsInstallable == 1) {
							/* Limit to DDR3-1600 */
							if (freq > 800) {
								freq = 800;
								printk(BIOS_DEBUG, "%s: 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1600\n", __func__, voltage_index_to_mv(voltage));
							}
						} else {
							if (count > 1) {
								if (highest_rank_count > 1) {
									/* Limit to DDR3-1066 */
									if (freq > 533) {
										freq = 533;
										printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1066\n", __func__, voltage_index_to_mv(voltage));
									}
								} else {
									/* Limit to DDR3-1333 */
									if (freq > 666) {
										freq = 666;
										printk(BIOS_DEBUG, "%s: More than 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
									}
								}
							} else {
								/* Limit to DDR3-1600 */
								if (freq > 800) {
									freq = 800;
									printk(BIOS_DEBUG, "%s: 1 unbuffered DIMM on %dmV channel; limiting to DDR3-1600\n", __func__, voltage_index_to_mv(voltage));
								}
							}
						}
					}
				}
			} else {
				/* TODO
				 * Other socket support unimplemented
				 */
			}
		} else {
			if (CONFIG(DIMM_REGISTERED) && registered) {
				/* K10 BKDG Rev. 3.62 Table 34 */
				if (count > 2) {
					/* Limit to DDR3-800 */
					if (freq > 400) {
						freq = 400;
						printk(BIOS_DEBUG, "%s: More than 2 registered DIMMs on %dmV channel; limiting to DDR3-800\n", __func__, voltage_index_to_mv(voltage));
					}
				} else if (count == 2) {
					/* Limit to DDR3-1066 */
					if (freq > 533) {
						freq = 533;
						printk(BIOS_DEBUG, "%s: 2 registered DIMMs on %dmV channel; limiting to DDR3-1066\n", __func__, voltage_index_to_mv(voltage));
					}
				} else {
					/* Limit to DDR3-1333 */
					if (freq > 666) {
						freq = 666;
						printk(BIOS_DEBUG, "%s: 1 registered DIMM on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
					}
				}
			} else {
				/* K10 BKDG Rev. 3.62 Table 33 */
				/* Limit to DDR3-1333 */
				if (freq > 666) {
					freq = 666;
					printk(BIOS_DEBUG, "%s: unbuffered DIMMs on %dmV channel; limiting to DDR3-1333\n", __func__, voltage_index_to_mv(voltage));
				}
			}
		}
	}

	return freq;
}

int mctRead_SPD(u32 smaddr, u32 reg)
{
	return spd_read_byte(smaddr, reg);
}

void mctSMBhub_Init(u32 node)
{
	struct sys_info *sysinfo = get_sysinfo();
	struct mem_controller *ctrl = &(sysinfo->ctrl[node]);
	activate_spd_rom(ctrl);
}

void raminit_amdmct(struct sys_info *sysinfo)
{
	struct MCTStatStruc *pMCTstat = &(sysinfo->MCTstat);
	struct DCTStatStruc *pDCTstatA = sysinfo->DCTstatA;

	printk(BIOS_DEBUG, "raminit_amdmct begin:\n");
	timestamp_add_now(TS_BEFORE_INITRAM);

	mctAutoInitMCT_D(pMCTstat, pDCTstatA);

	timestamp_add_now(TS_AFTER_INITRAM);
	printk(BIOS_DEBUG, "raminit_amdmct end:\n");
}

void amdmct_cbmem_store_info(struct sys_info *sysinfo)
{
	if (!sysinfo)
		return;

	/* Save memory info structures for use in ramstage */
	size_t i;
	struct DCTStatStruc *pDCTstatA = NULL;

	if (!acpi_is_wakeup_s3()) {
		/* Allocate memory */
		struct amdmct_memory_info *mem_info;

		mem_info = (struct amdmct_memory_info *)cbmem_add(CBMEM_ID_AMDMCT_MEMINFO,
								  sizeof(*mem_info));
		if (!mem_info)
			return;

		printk(BIOS_DEBUG, "%s: Storing AMDMCT configuration in CBMEM\n", __func__);

		/* Initialize memory */
		memset(mem_info, 0,  sizeof(struct amdmct_memory_info));

		/* Copy data */
		memcpy(&mem_info->mct_stat, &sysinfo->MCTstat, sizeof(struct MCTStatStruc));
		for (i = 0; i < MAX_NODES_SUPPORTED; i++) {
			pDCTstatA = sysinfo->DCTstatA + i;
			memcpy(&mem_info->dct_stat[i], pDCTstatA, sizeof(struct DCTStatStruc));
		}
		mem_info->ecc_enabled = mctGet_NVbits(NV_ECC_CAP);
		mem_info->ecc_scrub_rate = mctGet_NVbits(NV_DramBKScrub);

		/* Zero out invalid/unused pointers */
		if (CONFIG(DIMM_DDR3)) {
			for (i = 0; i < MAX_NODES_SUPPORTED; i++) {
				mem_info->dct_stat[i].C_MCTPtr = NULL;
				mem_info->dct_stat[i].C_DCTPtr[0] = NULL;
				mem_info->dct_stat[i].C_DCTPtr[1] = NULL;
			}
		}
	}
}
