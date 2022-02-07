/* SPDX-License-Identifier: GPL-2.0-only */

#include <drivers/amd/amdmct/wrappers/mcti.h>

/* Low swap bit vs bank size encoding (physical, not logical address bit)
 * ;To calculate the number by hand, add the number of Bank address bits
 * ;(2 or 3) to the number of column address bits, plus 3 (the logical
 * ;page size), and subtract 8.
 */
static const u8 tab_int_d[] = { 6,7,7,8,8,8,8,8,9,9,8,9 };

void interleave_banks_d(struct MCTStatStruc *p_mct_stat,
			struct DCTStatStruc *p_dct_stat, u8 dct)
{
	u8 chip_sel, en_chip_sels;
	u32 addr_lo_mask, addr_hi_mask;
	u32 addr_lo_mask_n, addr_hi_mask_n, mem_size = 0;
	u8 do_intlv, _cs_int_cap;
	u32 bit_delta, bank_encd = 0;

	u32 dev;
	u32 reg;
	u32 reg_off;
	u32 val;
	u32 val_lo, val_hi;

	do_intlv = mct_get_nv_bits(NV_BANK_INTLV);
	_cs_int_cap = 0;
	en_chip_sels = 0;

	dev = p_dct_stat->dev_dct;
	reg_off = 0x100 * dct;

	chip_sel = 0;		/* Find out if current configuration is capable */
	while (do_intlv && (chip_sel < MAX_CS_SUPPORTED)) {
		reg = 0x40+(chip_sel << 2) + reg_off;	/* Dram CS Base 0 */
		val = get_nb32(dev, reg);
		if (val & (1 << CS_ENABLE)) {
			en_chip_sels++;
			reg = 0x60+((chip_sel >> 1) << 2)+reg_off; /*Dram CS Mask 0 */
			val = get_nb32(dev, reg);
			val >>= 19;
			val &= 0x3ff;
			val++;
			if (en_chip_sels == 1)
				mem_size = val;
			else
				/*If mask sizes not same then skip */
				if (val != mem_size)
					break;
			reg = 0x80 + reg_off;		/*Dram Bank Addressing */
			val = get_nb32(dev, reg);
			val >>= (chip_sel >> 1) << 2;
			val &= 0x0f;
			if (en_chip_sels == 1)
				bank_encd = val;
			else
				/*If number of Rows/Columns not equal, skip */
				if (val != bank_encd)
					break;
		}
		chip_sel++;
	}
	if (chip_sel == MAX_CS_SUPPORTED) {
		if ((en_chip_sels == 2) || (en_chip_sels == 4) || (en_chip_sels == 8))
			_cs_int_cap = 1;
	}

	if (do_intlv) {
		if (!_cs_int_cap) {
			p_dct_stat->err_status |= 1 << SB_BK_INT_DIS;
			do_intlv = 0;
		}
	}

	if (do_intlv) {
		val = tab_int_d[bank_encd];
		if (p_dct_stat->status & (1 << SB_128_BIT_MODE))
			val++;

		addr_lo_mask = (en_chip_sels - 1)  << val;
		addr_lo_mask_n = ~addr_lo_mask;

		val = bsf(mem_size) + 19;
		addr_hi_mask = (en_chip_sels -1) << val;
		addr_hi_mask_n = ~addr_hi_mask;

		bit_delta = bsf(addr_hi_mask) - bsf(addr_lo_mask);

		for (chip_sel = 0; chip_sel < MAX_CS_SUPPORTED; chip_sel++) {
			reg = 0x40+(chip_sel << 2) + reg_off;	/*Dram CS Base 0 */
			val = get_nb32(dev, reg);
			if (val & 3) {
				val_lo = val & addr_lo_mask;
				val_hi = val & addr_hi_mask;
				val &= addr_lo_mask_n;
				val &= addr_hi_mask_n;
				val_lo <<= bit_delta;
				val_hi >>= bit_delta;
				val |= val_lo;
				val |= val_hi;
				set_nb32(dev, reg, val);

				if (chip_sel & 1)
					continue;

				/*Dram CS Mask 0 */
				reg = 0x60 + ((chip_sel >> 1) << 2) + reg_off;
				val = get_nb32(dev, reg);
				val_lo = val & addr_lo_mask;
				val_hi = val & addr_hi_mask;
				val &= addr_lo_mask_n;
				val &= addr_hi_mask_n;
				val_lo <<= bit_delta;
				val_hi >>= bit_delta;
				val |= val_lo;
				val |= val_hi;
				set_nb32(dev, reg, val);
			}
		}
		print_t("interleave_banks_d: Banks Interleaved ");
	}	/* do_intlv */

	print_tx("interleave_banks_d: status ", p_dct_stat->status);
	print_tx("interleave_banks_d: err_status ", p_dct_stat->err_status);
	print_tx("interleave_banks_d: err_code ", p_dct_stat->err_code);
	print_t("interleave_banks_d: Done\n");
}
