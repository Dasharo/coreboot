/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cpu.h>
#include <stdint.h>
#include <console/console.h>

#include <drivers/amd/amdmct/wrappers/mcti.h>
#include "mct_d_gcc.h"

u8 fam15_dimm_dic(struct DCTStatStruc *p_dct_stat, u8 dct, u8 dimm, u8 rank, u8 package_type)
{
	u8 dic;

	/* Calculate DIC based on recommendations in MR1_dct[1:0] */
	if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
		/* TODO
		* LRDIMM unimplemented
		*/
		dic = 0x0;
	} else {
		dic = 0x1;
	}

	return dic;
}

u8 fam15_rttwr(struct DCTStatStruc *p_dct_stat, u8 dct, u8 dimm, u8 rank, u8 package_type)
{
	u8 term = 0;
	u8 number_of_dimms = p_dct_stat->ma_dimms[dct];
	u8 frequency_index;
	u8 rank_count = p_dct_stat->dimm_ranks[(dimm * 2) + dct];

	u8 rank_count_dimm0;
	u8 rank_count_dimm1;
	u8 rank_count_dimm2;

	if (is_fam15h())
		frequency_index = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x1f;
	else
		frequency_index = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x7;

	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	if (is_fam15h()) {
		if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* TODO
			 * LRDIMM unimplemented
			 */
		} else if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			if (package_type == PT_GR) {
				/* Socket G34: Fam15h BKDG v3.14 Table 57 */
				if (max_dimms_installable == 1) {
					if ((frequency_index == 0x4) || (frequency_index == 0x6)
						|| (frequency_index == 0xa) || (frequency_index == 0xe)) {
						/* DDR3-667 - DDR3-1333 */
						if (rank_count < 3)
							term = 0x0;
						else
							term = 0x2;
					} else {
						/* DDR3-1600 */
						term = 0x0;
					}
				} else if (max_dimms_installable == 2) {
					rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((frequency_index == 0x4) || (frequency_index == 0x6)) {
						/* DDR3-667 - DDR3-800 */
						if ((number_of_dimms == 1) && ((rank_count_dimm0 < 4)
							&& (rank_count_dimm1 < 4)))
							term = 0x0;
						else
							term = 0x2;
					} else if (frequency_index == 0xa) {
						/* DDR3-1066 */
						if (number_of_dimms == 1) {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4))
								term = 0x0;
							else
								term = 0x2;
						} else {
							term = 0x1;
						}
					} else if (frequency_index == 0xe) {
						/* DDR3-1333 */
						term = 0x2;
					} else {
						/* DDR3-1600 */
						if (number_of_dimms == 1)
							term = 0x0;
						else
							term = 0x1;
					}
				} else if (max_dimms_installable == 3) {
					rank_count_dimm2 = p_dct_stat->dimm_ranks[(2 * 2) + dct];

					if ((frequency_index == 0xa) || (frequency_index == 0xe)) {
						/* DDR3-1066 - DDR3-1333 */
						if (rank_count_dimm2 < 4)
							term = 0x1;
						else
							term = 0x2;
					} else if (frequency_index == 0x12) {
						/* DDR3-1600 */
						term = 0x1;
					} else {
						term = 0x2;
					}
				}
			} else if (package_type == PT_C3) {
				/* Socket C32: Fam15h BKDG v3.14 Table 60 */
				if (max_dimms_installable == 1) {
					if ((frequency_index == 0x4) || (frequency_index == 0x6)
						|| (frequency_index == 0xa) || (frequency_index == 0xe)) {
						/* DDR3-667 - DDR3-1333 */
						if (rank_count < 3)
							term = 0x0;
						else
							term = 0x2;
					} else {
						/* DDR3-1600 */
						term = 0x0;
					}
				} else if (max_dimms_installable == 2) {
					rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((frequency_index == 0x4) || (frequency_index == 0x6)) {
						/* DDR3-667 - DDR3-800 */
						if ((number_of_dimms == 1) && ((rank_count_dimm0 < 4)
							&& (rank_count_dimm1 < 4)))
							term = 0x0;
						else
							term = 0x2;
					} else if (frequency_index == 0xa) {
						/* DDR3-1066 */
						if (number_of_dimms == 1) {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4))
								term = 0x0;
							else
								term = 0x2;
						} else {
							term = 0x1;
						}
					} else if (frequency_index == 0xe) {
						/* DDR3-1333 */
						term = 0x2;
					} else {
						/* DDR3-1600 */
						if (number_of_dimms == 1)
							term = 0x0;
						else
							term = 0x1;
					}
				} else if (max_dimms_installable == 3) {
					rank_count_dimm2 = p_dct_stat->dimm_ranks[(2 * 2) + dct];

					if ((frequency_index == 0xa) || (frequency_index == 0xe)) {
						/* DDR3-1066 - DDR3-1333 */
						if (rank_count_dimm2 < 4)
							term = 0x1;
						else
							term = 0x2;
					} else if (frequency_index == 0x12) {
						/* DDR3-1600 */
						term = 0x1;
					} else {
						term = 0x2;
					}
				}
			} else {
				/* TODO
				 * Other sockets unimplemented
				 */
			}
		} else {
			/* UDIMM */
			if (package_type == PT_GR) {
				/* Socket G34: Fam15h BKDG v3.14 Table 56 */
				if (max_dimms_installable == 1) {
					term = 0x0;
				} else if (max_dimms_installable == 2) {
					if ((number_of_dimms == 2) && (frequency_index == 0x12)) {
						term = 0x1;
					} else if (number_of_dimms == 1) {
						term = 0x0;
					} else {
						term = 0x2;
					}
				} else if (max_dimms_installable == 3) {
					if (number_of_dimms == 1) {
						if (frequency_index <= 0xa) {
							term = 0x2;
						} else {
							if (rank_count < 3) {
								term = 0x1;
							} else {
								term = 0x2;
							}
						}
					} else if (number_of_dimms == 2) {
						term = 0x2;
					}
				}
			} else if (package_type == PT_C3) {
				/* Socket C32: Fam15h BKDG v3.14 Table 59 */
				if (max_dimms_installable == 1) {
					term = 0x0;
				} else if (max_dimms_installable == 2) {
					if ((number_of_dimms == 2) && (frequency_index == 0x12)) {
						term = 0x1;
					} else if (number_of_dimms == 1) {
						term = 0x0;
					} else {
						term = 0x2;
					}
				} else if (max_dimms_installable == 3) {
					if (number_of_dimms == 1) {
						if (frequency_index <= 0xa) {
							term = 0x2;
						} else {
							if (rank_count < 3) {
								term = 0x1;
							} else {
								term = 0x2;
							}
						}
					} else if (number_of_dimms == 2) {
						term = 0x2;
					}
				}
			} else if (package_type == PT_FM2) {
				/* Socket FM2: Fam15h Model10 BKDG 3.12 Table 32 */
				if (max_dimms_installable == 1) {
					term = 0x0;
				} else if (max_dimms_installable == 2) {
					if ((number_of_dimms == 2) && (frequency_index >= 0x12)) {
						term = 0x1;
					} else if (number_of_dimms == 1) {
						term = 0x0;
					} else {
						term = 0x2;
					}
				}
			} else {
				/* TODO
				* Other sockets unimplemented
				*/
			}
		}
	}

	printk(BIOS_INFO, "DIMM %d RttWr: %01x\n", dimm, term);

	return term;
}

u8 fam15_rttnom(struct DCTStatStruc *p_dct_stat, u8 dct, u8 dimm, u8 rank, u8 package_type)
{
	u8 term = 0;
	u8 number_of_dimms = p_dct_stat->ma_dimms[dct];
	u8 frequency_index;

	u8 rank_count_dimm0;
	u8 rank_count_dimm1;

	if (is_fam15h())
		frequency_index = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x1f;
	else
		frequency_index = get_nb32_dct(p_dct_stat->dev_dct, dct, 0x94) & 0x7;

	u8 max_dimms_installable = mct_get_nv_bits(NV_MAX_DIMMS_PER_CH);

	if (is_fam15h()) {
		if (p_dct_stat->status & (1 << SB_LOAD_REDUCED)) {
			/* TODO
			 * LRDIMM unimplemented
			 */
		} else if (p_dct_stat->status & (1 << SB_REGISTERED)) {
			/* RDIMM */
			if (package_type == PT_GR) {
				/* Socket G34: Fam15h BKDG v3.14 Table 57 */
				if (max_dimms_installable == 1) {
					rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];

					if ((frequency_index == 0x4) || (frequency_index == 0x6)) {
						/* DDR3-667 - DDR3-800 */
						if (rank_count_dimm0 < 4) {
							term = 0x2;
						} else {
							if (!rank)
								term = 0x2;
							else
								term = 0x0;
						}
					} else if (frequency_index == 0xa) {
						/* DDR3-1066 */
						term = 0x1;
					} else if (frequency_index == 0xe) {
						/* DDR3-1333 */
						if (rank_count_dimm0 < 4) {
							term = 0x1;
						} else {
							if (!rank)
								term = 0x3;
							else
								term = 0x0;
						}
					} else {
						term = 0x3;
					}
				} else if (max_dimms_installable == 2) {
					rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((frequency_index == 0x4) || (frequency_index == 0x6)) {
						/* DDR3-667 - DDR3-800 */
						if (number_of_dimms == 1) {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4))
								term = 0x2;
							else if (rank)
								term = 0x0;
							else
								term = 0x2;
						} else {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4)) {
								term = 0x3;
							} else {
								if (rank_count_dimm0 == 4) {
									if (rank_count_dimm1 == 1)
										term = 0x5;
									else
										term = 0x1;
								} else if (rank_count_dimm1 == 4) {
									if (rank_count_dimm0 == 1)
										term = 0x5;
									else
										term = 0x1;
								}
								if (rank)
									term = 0x0;
							}
						}
					} else if (frequency_index == 0xa) {
						/* DDR3-1066 */
						if (number_of_dimms == 1) {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4))
								term = 0x1;
							else if (rank)
								term = 0x0;
							else
								term = 0x1;
						} else {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4)) {
								term = 0x3;
							} else {
								if (rank_count_dimm0 == 4) {
									if (rank_count_dimm1 == 1)
										term = 0x5;
									else
										term = 0x1;
								} else if (rank_count_dimm1 == 4) {
									if (rank_count_dimm0 == 1)
										term = 0x5;
									else
										term = 0x1;
								}
								if (rank)
									term = 0x0;
							}
						}
					} else if (frequency_index == 0xe) {
						/* DDR3-1333 */
						if (number_of_dimms == 1) {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4))
								term = 0x1;
							else if (rank)
								term = 0x0;
							else
								term = 0x3;
						} else {
							term = 0x5;
						}
					} else {
						/* DDR3-1600 */
						if (number_of_dimms == 1)
							term = 0x3;
						else
							term = 0x4;
					}
				} else if (max_dimms_installable == 3) {
					/* TODO
					 * 3 DIMM/channel support unimplemented
					 */
				}
			} else if (package_type == PT_C3) {
				/* Socket C32: Fam15h BKDG v3.14 Table 60 */
				if (max_dimms_installable == 1) {
					rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];

					if ((frequency_index == 0x4) || (frequency_index == 0x6)) {
						/* DDR3-667 - DDR3-800 */
						if (rank_count_dimm0 < 4) {
							term = 0x2;
						} else {
							if (!rank)
								term = 0x2;
							else
								term = 0x0;
						}
					} else if (frequency_index == 0xa) {
						/* DDR3-1066 */
						term = 0x1;
					} else if (frequency_index == 0xe) {
						/* DDR3-1333 */
						if (rank_count_dimm0 < 4) {
							term = 0x1;
						} else {
							if (!rank)
								term = 0x3;
							else
								term = 0x0;
						}
					} else {
						term = 0x3;
					}
				} else if (max_dimms_installable == 2) {
					rank_count_dimm0 = p_dct_stat->dimm_ranks[(0 * 2) + dct];
					rank_count_dimm1 = p_dct_stat->dimm_ranks[(1 * 2) + dct];

					if ((frequency_index == 0x4) || (frequency_index == 0x6)) {
						/* DDR3-667 - DDR3-800 */
						if (number_of_dimms == 1) {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4))
								term = 0x2;
							else if (rank)
								term = 0x0;
							else
								term = 0x2;
						} else {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4)) {
								term = 0x3;
							} else {
								if (rank_count_dimm0 == 4) {
									if (rank_count_dimm1 == 1)
										term = 0x5;
									else
										term = 0x1;
								} else if (rank_count_dimm1 == 4) {
									if (rank_count_dimm0 == 1)
										term = 0x5;
									else
										term = 0x1;
								}
								if (rank)
									term = 0x0;
							}
						}
					} else if (frequency_index == 0xa) {
						/* DDR3-1066 */
						if (number_of_dimms == 1) {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4))
								term = 0x1;
							else if (rank)
								term = 0x0;
							else
								term = 0x1;
						} else {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4)) {
								term = 0x3;
							} else {
								if (rank_count_dimm0 == 4) {
									if (rank_count_dimm1 == 1)
										term = 0x5;
									else
										term = 0x1;
								} else if (rank_count_dimm1 == 4) {
									if (rank_count_dimm0 == 1)
										term = 0x5;
									else
										term = 0x1;
								}
								if (rank)
									term = 0x0;
							}
						}
					} else if (frequency_index == 0xe) {
						/* DDR3-1333 */
						if (number_of_dimms == 1) {
							if ((rank_count_dimm0 < 4) && (rank_count_dimm1 < 4))
								term = 0x1;
							else if (rank)
								term = 0x0;
							else
								term = 0x3;
						} else {
							term = 0x5;
						}
					} else {
						/* DDR3-1600 */
						if (number_of_dimms == 1)
							term = 0x3;
						else
							term = 0x4;
					}
				} else if (max_dimms_installable == 3) {
					/* TODO
					 * 3 DIMM/channel support unimplemented
					 */
				}
			} else {
				/* TODO
				 * Other sockets unimplemented
				 */
			}
		} else {
			/* UDIMM */
			if (package_type == PT_GR) {
				/* Socket G34: Fam15h BKDG v3.14 Table 56 */
				if (max_dimms_installable == 1) {
					if ((frequency_index == 0x4) || (frequency_index == 0x6))
						term = 0x2;
					else if ((frequency_index == 0xa) || (frequency_index == 0xe))
						term = 0x1;
					else
						term = 0x3;
				}
				if (max_dimms_installable == 2) {
					if (number_of_dimms == 1) {
						if (frequency_index <= 0x6) {
							term = 0x2;
						} else if (frequency_index <= 0xe) {
							term = 0x1;
						} else {
							term = 0x3;
						}
					} else {
						if (frequency_index <= 0xa) {
							term = 0x3;
						} else if (frequency_index <= 0xe) {
							term = 0x5;
						} else {
							term = 0x4;
						}
					}
				} else if (max_dimms_installable == 3) {
					if (number_of_dimms == 1) {
						term = 0x0;
					} else if (number_of_dimms == 2) {
						if (frequency_index <= 0xa) {
							if (rank == 1) {
								term = 0x0;
							} else {
								term = 0x3;
							}
						} else if (frequency_index <= 0xe) {
							if (rank == 1) {
								term = 0x0;
							} else {
								term = 0x5;
							}
						}
					}
				}
			} else if (package_type == PT_C3) {
				/* Socket C32: Fam15h BKDG v3.14 Table 62 */
				if (max_dimms_installable == 1) {
					if ((frequency_index == 0x4) || (frequency_index == 0x6))
						term = 0x2;
					else if ((frequency_index == 0xa) || (frequency_index == 0xe))
						term = 0x1;
					else
						term = 0x3;
				}
				if (max_dimms_installable == 2) {
					if (number_of_dimms == 1) {
						if (frequency_index <= 0x6) {
							term = 0x2;
						} else if (frequency_index <= 0xe) {
							term = 0x1;
						} else {
							term = 0x3;
						}
					} else {
						if (frequency_index <= 0xa) {
							term = 0x3;
						} else if (frequency_index <= 0xe) {
							term = 0x5;
						} else {
							term = 0x4;
						}
					}
				} else if (max_dimms_installable == 3) {
					if (number_of_dimms == 1) {
						term = 0x0;
					} else if (number_of_dimms == 2) {
						if (frequency_index <= 0xa) {
							if (rank == 1) {
								term = 0x0;
							} else {
								term = 0x3;
							}
						} else if (frequency_index <= 0xe) {
							if (rank == 1) {
								term = 0x0;
							} else {
								term = 0x5;
							}
						}
					}
				}
			} else if (package_type == PT_FM2) {
				/* Socket FM2: Fam15h Model10 BKDG 3.12 Table 32 */
				if (max_dimms_installable == 1) {
					if ((frequency_index == 0x4)
							|| (frequency_index == 0x6)
							|| (frequency_index == 0xa))
						term = 0x4;
					else if (frequency_index == 0xe)
						term = 0x3;
					else if (frequency_index >= 0x12)
						term = 0x2;
				}
				if (max_dimms_installable == 2) {
					if (number_of_dimms == 1) {
						if (frequency_index <= 0xa) {
							term = 0x4;
						} else if (frequency_index <= 0xe) {
							term = 0x3;
						} else {
							term = 0x2;
						}
					} else {
						if (frequency_index <= 0xa) {
							term = 0x2;
						} else if (frequency_index <= 0xe) {
							term = 0x1;
						} else {
							term = 0x0;
						}
					}
				}
			} else {
				/* TODO
				 * Other sockets unimplemented
				 */
			}
		}
	}

	printk(BIOS_INFO, "DIMM %d RttNom: %01x\n", dimm, term);
	return term;
}

static void mct_dct_access_done(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 dev = p_dct_stat->dev_dct;
	u32 val;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	do {
		val = get_nb32_dct(dev, dct, 0x98);
	} while (!(val & (1 << DCT_ACCESS_DONE)));

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

static u32 swap_addr_bits(struct DCTStatStruc *p_dct_stat, u32 mr_register_setting, u8 mrs_chip_sel, u8 dct)
{
	u16 word;
	u32 ret;

	if (!(p_dct_stat->status & (1 << SB_REGISTERED))) {
		word = p_dct_stat->mirr_pres_u_num_reg_r;
		if (dct == 0) {
			word &= 0x55;
			word <<= 1;
		} else
			word &= 0xAA;

		if (word & (1 << mrs_chip_sel)) {
			/* A3<->A4,A5<->A6,A7<->A8,BA0<->BA1 */
			ret = 0;
			if (mr_register_setting & (1 << 3)) ret |= 1 << 4;
			if (mr_register_setting & (1 << 4)) ret |= 1 << 3;
			if (mr_register_setting & (1 << 5)) ret |= 1 << 6;
			if (mr_register_setting & (1 << 6)) ret |= 1 << 5;
			if (mr_register_setting & (1 << 7)) ret |= 1 << 8;
			if (mr_register_setting & (1 << 8)) ret |= 1 << 7;
			if (is_fam15h()) {
				if (mr_register_setting & (1 << 18)) ret |= 1 << 19;
				if (mr_register_setting & (1 << 19)) ret |= 1 << 18;
				mr_register_setting &= ~0x000c01f8;
			} else {
				if (mr_register_setting & (1 << 16)) ret |= 1 << 17;
				if (mr_register_setting & (1 << 17)) ret |= 1 << 16;
				mr_register_setting &= ~0x000301f8;
			}
			mr_register_setting |= ret;
		}
	}
	return mr_register_setting;
}

static void mct_send_mrs_cmd(struct DCTStatStruc *p_dct_stat, u8 dct, u32 emrs)
{
	u32 dev = p_dct_stat->dev_dct;
	u32 val;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	val = get_nb32_dct(dev, dct, 0x7c);
	val &= ~0x00ffffff;
	val |= emrs;
	val |= 1 << SEND_MRS_CMD;
	set_nb32_dct(dev, dct, 0x7c, val);

	do {
		val = get_nb32_dct(dev, dct, 0x7c);
	} while (val & (1 << SEND_MRS_CMD));

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

u32 mct_mr2(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_chip_sel)
{
	u32 dev = p_dct_stat->dev_dct;
	u32 dword, ret;

	/* The formula for chip select number is: CS = dimm*2+rank */
	u8 dimm = mrs_chip_sel / 2;
	u8 rank = mrs_chip_sel % 2;

	if (is_fam15h()) {
		u8 package_type = mct_get_nv_bits(NV_PACK_TYPE);

		/* FIXME: These parameters should be configurable
		 * For now, err on the side of caution and enable automatic 2x refresh
		 * when the DDR temperature rises above the internal limits
		 */
		u8 force_2x_self_refresh = 0;	/* ASR */
		u8 auto_2x_self_refresh = 1;	/* SRT */

		ret = 0x80000;
		ret |= (mrs_chip_sel << 21);

		/* Set self refresh parameters */
		ret |= (force_2x_self_refresh << 6);
		ret |= (auto_2x_self_refresh << 7);

		/* Obtain Tcwl, adjust, and set CWL with the adjusted value */
		dword = get_nb32_dct(dev, dct, 0x20c) & 0x1f;
		dword -= p_dct_stat->tcwl_delay[dct];
		ret |= ((dword - 5) << 3);

		/* Obtain and set RttWr */
		ret |= (fam15_rttwr(p_dct_stat, dct, dimm, rank, package_type) << 9);
	} else {
		ret = 0x20000;
		ret |= (mrs_chip_sel << 20);

		/* program MrsAddress[5:3]=CAS write latency (CWL):
		 * based on F2x[1,0]84[Tcwl] */
		dword = get_nb32_dct(dev, dct, 0x84);
		dword = mct_adjust_spd_timings(p_mct_stat, p_dct_stat, dword);

		ret |= ((dword >> 20) & 7) << 3;

		/* program MrsAddress[6]=auto self refresh method (ASR):
		 * based on F2x[1,0]84[ASR]
		 * program MrsAddress[7]=self refresh temperature range (SRT):
		 * based on F2x[1,0]84[ASR and SRT]
		 */
		ret |= ((dword >> 18) & 3) << 6;

		/* program MrsAddress[10:9]=dynamic termination during writes (RTT_WR)
		 * based on F2x[1,0]84[DramTermDyn]
		 */
		ret |= ((dword >> 10) & 3) << 9;
	}

	printk(BIOS_SPEW, "Going to send DCT %d DIMM %d rank %d MR2 control word %08x\n", dct, dimm, rank, ret);

	return ret;
}

static u32 mct_mr3(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_chip_sel)
{
	u32 dev = p_dct_stat->dev_dct;
	u32 dword, ret;

	/* The formula for chip select number is: CS = dimm*2+rank */
	u8 dimm = mrs_chip_sel / 2;
	u8 rank = mrs_chip_sel % 2;

	if (is_fam15h()) {
		ret = 0xc0000;
		ret |= (mrs_chip_sel << 21);

		/* Program MPR and MPRLoc to 0 */
		// ret |= 0x0;		/* MPR */
		// ret |= (0x0 << 2);	/* MPRLoc */
	} else {
		ret = 0x30000;
		ret |= (mrs_chip_sel << 20);

		/* program MrsAddress[1:0]=multi purpose register address location
		 * (MPR Location):based on F2x[1,0]84[MprLoc]
		 * program MrsAddress[2]=multi purpose register
		 * (MPR):based on F2x[1,0]84[MprEn]
		 */
		dword = get_nb32_dct(dev, dct, 0x84);
		ret |= (dword >> 24) & 7;
	}

	printk(BIOS_SPEW, "Going to send DCT %d DIMM %d rank %d MR3 control word %08x\n", dct, dimm, rank, ret);

	return ret;
}

u32 mct_mr1(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_chip_sel)
{
	u32 dev = p_dct_stat->dev_dct;
	u32 dword, ret;

	/* The formula for chip select number is: CS = dimm*2+rank */
	u8 dimm = mrs_chip_sel / 2;
	u8 rank = mrs_chip_sel % 2;

	if (is_fam15h()) {
		u8 package_type = mct_get_nv_bits(NV_PACK_TYPE);

		/* Set defaults */
		u8 qoff = 0;	/* Enable output buffers */
		u8 wrlvl = 0;	/* Disable write levelling */
		u8 tqds = 0;
		u8 rttnom = 0;
		u8 dic = 0;
		u8 additive_latency = 0;
		u8 dll_enable = 0;

		ret = 0x40000;
		ret |= (mrs_chip_sel << 21);

		/* Determine if TQDS should be set */
		if ((p_dct_stat->dimm_x8_present & (1 << dimm))
			&& (((dimm & 0x1) ? (p_dct_stat->dimm_x4_present & 0x55) : (p_dct_stat->dimm_x4_present & 0xaa)) != 0x0)
			&& (p_dct_stat->status & (1 << SB_LOAD_REDUCED)))
			tqds = 1;

		/* Obtain RttNom */
		rttnom = fam15_rttnom(p_dct_stat, dct, dimm, rank, package_type);

		/* Obtain DIC */
		dic = fam15_dimm_dic(p_dct_stat, dct, dimm, rank, package_type);

		/* Load data into MRS word */
		ret |= (qoff & 0x1) << 12;
		ret |= (tqds & 0x1) << 11;
		ret |= ((rttnom & 0x4) >> 2) << 9;
		ret |= ((rttnom & 0x2) >> 1) << 6;
		ret |= ((rttnom & 0x1) >> 0) << 2;
		ret |= (wrlvl & 0x1) << 7;
		ret |= ((dic & 0x2) >> 1) << 5;
		ret |= ((dic & 0x1) >> 0) << 1;
		ret |= (additive_latency & 0x3) << 3;
		ret |= (dll_enable & 0x1);
	} else {
		ret = 0x10000;
		ret |= (mrs_chip_sel << 20);

		/* program MrsAddress[5,1]=output driver impedance control (DIC):
		 * based on F2x[1,0]84[DrvImpCtrl]
		 */
		dword = get_nb32_dct(dev, dct, 0x84);
		if (dword & (1 << 3))
			ret |= 1 << 5;
		if (dword & (1 << 2))
			ret |= 1 << 1;

		/* program MrsAddress[9,6,2]=nominal termination resistance of ODT (RTT):
		 * based on F2x[1,0]84[DramTerm]
		 */
		if (!(p_dct_stat->status & (1 << SB_REGISTERED))) {
			if (dword & (1 << 9))
				ret |= 1 << 9;
			if (dword & (1 << 8))
				ret |= 1 << 6;
			if (dword & (1 << 7))
				ret |= 1 << 2;
		} else {
			ret |= mct_mr_1Odt_r_dimm(p_mct_stat, p_dct_stat, dct, mrs_chip_sel);
		}

		/* program MrsAddress[11]=TDQS: based on F2x[1,0]94[RDQS_EN] */
		if (get_nb32_dct(dev, dct, 0x94) & (1 << RDQS_EN)) {
			u8 bit;
			/* Set TDQS = 1b for x8 DIMM, TDQS = 0b for x4 DIMM, when mixed x8 & x4 */
			bit = (ret >> 21) << 1;
			if ((dct & 1) != 0)
				bit ++;
			if (p_dct_stat->dimm_x8_present & (1 << bit))
				ret |= 1 << 11;
		}

		/* program MrsAddress[12]=QOFF: based on F2x[1,0]84[QOFF] */
		if (dword & (1 << 13))
			ret |= 1 << 12;
	}

	printk(BIOS_SPEW, "Going to send DCT %d DIMM %d rank %d MR1 control word %08x\n", dct, dimm, rank, ret);

	return ret;
}

static u32 mct_mr0(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct, u32 mrs_chip_sel)
{
	u32 dev = p_dct_stat->dev_dct;
	u32 dword, ret, dword2;

	/* The formula for chip select number is: CS = dimm*2+rank */
	u8 dimm = mrs_chip_sel / 2;
	u8 rank = mrs_chip_sel % 2;

	if (is_fam15h()) {
		ret = 0x00000;
		ret |= (mrs_chip_sel << 21);

		/* Set defaults */
		u8 ppd = 0;
		u8 wr_ap = 0;
		u8 dll_reset = 1;
		u8 test_mode = 0;
		u8 cas_latency = 0;
		u8 read_burst_type = 1;
		u8 burst_length = 0;

		/* Obtain PCHG_PD_MODE_SEL */
		dword = get_nb32_dct(dev, dct, 0x84);
		ppd = (dword >> 23) & 0x1;

		/* Obtain Twr */
		dword = get_nb32_dct(dev, dct, 0x22c) & 0x1f;

		/* Calculate wr_ap (Fam15h BKDG v3.14 Table 82) */
		if (dword == 0x10)
			wr_ap = 0x0;
		else if (dword == 0x5)
			wr_ap = 0x1;
		else if (dword == 0x6)
			wr_ap = 0x2;
		else if (dword == 0x7)
			wr_ap = 0x3;
		else if (dword == 0x8)
			wr_ap = 0x4;
		else if (dword == 0xa)
			wr_ap = 0x5;
		else if (dword == 0xc)
			wr_ap = 0x6;
		else if (dword == 0xe)
			wr_ap = 0x7;

		/* Obtain Tcl */
		dword = get_nb32_dct(dev, dct, 0x200) & 0x1f;

		/* Calculate cas_latency (Fam15h BKDG v3.14 Table 83) */
		if (dword == 0x5)
			cas_latency = 0x2;
		else if (dword == 0x6)
			cas_latency = 0x4;
		else if (dword == 0x7)
			cas_latency = 0x6;
		else if (dword == 0x8)
			cas_latency = 0x8;
		else if (dword == 0x9)
			cas_latency = 0xa;
		else if (dword == 0xa)
			cas_latency = 0xc;
		else if (dword == 0xb)
			cas_latency = 0xe;
		else if (dword == 0xc)
			cas_latency = 0x1;
		else if (dword == 0xd)
			cas_latency = 0x3;
		else if (dword == 0xe)
			cas_latency = 0x5;
		else if (dword == 0xf)
			cas_latency = 0x7;
		else if (dword == 0x10)
			cas_latency = 0x9;

		/* Obtain BurstCtrl */
		burst_length = get_nb32_dct(dev, dct, 0x84) & 0x3;

		/* Load data into MRS word */
		ret |= (ppd & 0x1) << 12;
		ret |= (wr_ap & 0x7) << 9;
		ret |= (dll_reset & 0x1) << 8;
		ret |= (test_mode & 0x1) << 7;
		ret |= ((cas_latency & 0xe) >> 1) << 4;
		ret |= ((cas_latency & 0x1) >> 0) << 2;
		ret |= (read_burst_type & 0x1) << 3;
		ret |= (burst_length & 0x3);
	} else {
		ret = 0x00000;
		ret |= (mrs_chip_sel << 20);

		/* program MrsAddress[1:0]=burst length and control method
		(BL):based on F2x[1,0]84[BurstCtrl] */
		dword = get_nb32_dct(dev, dct, 0x84);
		ret |= dword & 3;

		/* program MrsAddress[3]=1 (BT):interleaved */
		ret |= 1 << 3;

		/* program MrsAddress[6:4,2]=read CAS latency
		(CL):based on F2x[1,0]88[Tcl] */
		dword2 = get_nb32_dct(dev, dct, 0x88);
		ret |= (dword2 & 0x7) << 4;		/* F2x88[2:0] to MrsAddress[6:4] */
		ret |= ((dword2 & 0x8) >> 3) << 2;	/* F2x88[3] to MrsAddress[2] */

		/* program MrsAddress[12]=0 (PPD):slow exit */
		if (dword & (1 << 23))
			ret |= 1 << 12;

		/* program MrsAddress[11:9]=write recovery for auto-precharge
		(WR):based on F2x[1,0]84[Twr] */
		ret |= ((dword >> 4) & 7) << 9;

		/* program MrsAddress[8]=1 (DLL):DLL reset
		just issue DLL reset at first time */
		ret |= 1 << 8;
	}

	printk(BIOS_SPEW, "Going to send DCT %d DIMM %d rank %d MR0 control word %08x\n", dct, dimm, rank, ret);

	return ret;
}

static void mct_send_zq_cmd(struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u32 dev = p_dct_stat->dev_dct;
	u32 dword;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	/*1.Program MrsAddress[10]=1
	  2.Set SEND_ZQ_CMD = 1
	 */
	dword = get_nb32_dct(dev, dct, 0x7C);
	dword &= ~0xFFFFFF;
	dword |= 1 << 10;
	dword |= 1 << SEND_ZQ_CMD;
	set_nb32_dct(dev, dct, 0x7C, dword);

	/* Wait for SEND_ZQ_CMD = 0 */
	do {
		dword = get_nb32_dct(dev, dct, 0x7C);
	} while (dword & (1 << SEND_ZQ_CMD));

	/* 4.Wait 512 MEMCLKs */
	mct_wait(300);

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}

void mct_dram_init_sw_d(struct MCTStatStruc *p_mct_stat,
				struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 mrs_chip_sel;
	u32 dword;
	u32 dev = p_dct_stat->dev_dct;

	printk(BIOS_DEBUG, "%s: Start\n", __func__);

	if (p_dct_stat->dimm_auto_speed == mhz_to_memclk_config(mct_get_nv_bits(NV_MIN_MEMCLK))) {
		/* 3.Program F2x[1,0]7C[EN_DRAM_INIT]=1 */
		dword = get_nb32_dct(dev, dct, 0x7c);
		dword |= 1 << EN_DRAM_INIT;
		set_nb32_dct(dev, dct, 0x7c, dword);
		mct_dct_access_done(p_dct_stat, dct);

		/* 4.wait 200us */
		mct_wait(40000);

		/* 5.Program F2x[1, 0]7C[DEASSERT_MEM_RST_X] = 1. */
		dword = get_nb32_dct(dev, dct, 0x7c);
		dword |= 1 << DEASSERT_MEM_RST_X;
		set_nb32_dct(dev, dct, 0x7c, dword);

		/* 6.wait 500us */
		mct_wait(200000);

		/* 7.Program F2x[1,0]7C[ASSERT_CKE]=1 */
		dword = get_nb32_dct(dev, dct, 0x7c);
		dword |= 1 << ASSERT_CKE;
		set_nb32_dct(dev, dct, 0x7c, dword);

		/* 8.wait 360ns */
		mct_wait(80);

		/* Set up address parity */
		if ((p_dct_stat->status & (1 << SB_REGISTERED))
			|| (p_dct_stat->status & (1 << SB_LOAD_REDUCED))) {
			if (is_fam15h()) {
				dword = get_nb32_dct(dev, dct, 0x90);
				dword |= 1 << PAR_EN;
				set_nb32_dct(dev, dct, 0x90, dword);
			}
		}

		/* The following steps are performed with registered DIMMs only and
		 * must be done for each chip select pair */
		if (p_dct_stat->status & (1 << SB_REGISTERED))
			mct_dram_control_reg_init_d(p_mct_stat, p_dct_stat, dct);

		/* The following steps are performed with load reduced DIMMs only and
		 * must be done for each DIMM */
		// if (p_dct_stat->status & (1 << SB_LOAD_REDUCED))
			/* TODO
			 * Implement LRDIMM configuration
			 */
	}

	p_dct_stat->cs_present = p_dct_stat->cs_present_dct[dct];
	if (p_dct_stat->ganged_mode & 1)
		p_dct_stat->cs_present = p_dct_stat->cs_present_dct[0];

	/* The following steps are performed once for unbuffered DIMMs and once for each
	 * chip select on registered DIMMs: */
	for (mrs_chip_sel = 0; mrs_chip_sel < 8; mrs_chip_sel++) {
		if (p_dct_stat->cs_present & (1 << mrs_chip_sel)) {
			u32 emrs;
			/* 13.Send emrs(2) */
			emrs = mct_mr2(p_mct_stat, p_dct_stat, dct, mrs_chip_sel);
			emrs = swap_addr_bits(p_dct_stat, emrs, mrs_chip_sel, dct);
			mct_send_mrs_cmd(p_dct_stat, dct, emrs);
			/* 14.Send emrs(3). Ordinarily at this time, MrsAddress[2:0]=000b */
			emrs = mct_mr3(p_mct_stat, p_dct_stat, dct, mrs_chip_sel);
			emrs = swap_addr_bits(p_dct_stat, emrs, mrs_chip_sel, dct);
			mct_send_mrs_cmd(p_dct_stat, dct, emrs);
			/* 15.Send emrs(1) */
			emrs = mct_mr1(p_mct_stat, p_dct_stat, dct, mrs_chip_sel);
			emrs = swap_addr_bits(p_dct_stat, emrs, mrs_chip_sel, dct);
			mct_send_mrs_cmd(p_dct_stat, dct, emrs);
			/* 16.Send MRS with MrsAddress[8]=1(reset the DLL) */
			emrs = mct_mr0(p_mct_stat, p_dct_stat, dct, mrs_chip_sel);
			emrs = swap_addr_bits(p_dct_stat, emrs, mrs_chip_sel, dct);
			mct_send_mrs_cmd(p_dct_stat, dct, emrs);

			if (p_dct_stat->dimm_auto_speed == mhz_to_memclk_config(mct_get_nv_bits(NV_MIN_MEMCLK)))
				if (!(p_dct_stat->status & (1 << SB_REGISTERED)))
					break; /* For UDIMM, only send MR commands once per channel */
		}
		if (p_dct_stat->logical_cpuid & (AMD_DR_Bx/* | AMD_RB_C0 */)) /* TODO: We dont support RB_C0 now. need to be added and tested. */
			if (!(p_dct_stat->status & (1 << SB_REGISTERED)))
				mrs_chip_sel ++;
	}

	if (p_dct_stat->dimm_auto_speed == mhz_to_memclk_config(mct_get_nv_bits(NV_MIN_MEMCLK))) {
		/* 17.Send two ZQCL commands */
		mct_send_zq_cmd(p_dct_stat, dct);
		mct_send_zq_cmd(p_dct_stat, dct);

		/* 18.Program F2x[1,0]7C[EN_DRAM_INIT]=0 */
		dword = get_nb32_dct(dev, dct, 0x7C);
		dword &= ~(1 << EN_DRAM_INIT);
		set_nb32_dct(dev, dct, 0x7C, dword);
		mct_dct_access_done(p_dct_stat, dct);
	}

	printk(BIOS_DEBUG, "%s: Done\n", __func__);
}
