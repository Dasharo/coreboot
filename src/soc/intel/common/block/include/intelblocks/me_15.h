/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _SOC_INTEL_COMMON_ME_SPEC_15_H_
#define _SOC_INTEL_COMMON_ME_SPEC_15_H_

/* HAP bit */
#if CONFIG(SOC_INTEL_TIGERLAKE_PCH_H)
#define HAP_OFFSET			0x196
#elif CONFIG(SOC_INTEL_TIGERLAKE)
#define HAP_OFFSET			0x17E
#elif CONFIG(SOC_INTEL_ELKHARTLAKE)
#define HAP_OFFSET			0x186
#endif

/* ME Host Firmware Status register 1 */
union me_hfsts1 {
	uint32_t data;
	struct {
		uint32_t working_state		: 4;
		uint32_t mfg_mode		: 1;
		uint32_t fpt_bad		: 1;
		uint32_t operation_state	: 3;
		uint32_t fw_init_complete	: 1;
		uint32_t ft_bup_ld_flr		: 1;
		uint32_t update_in_progress	: 1;
		uint32_t error_code		: 4;
		uint32_t operation_mode		: 4;
		uint32_t reserved_0		: 4;
		uint32_t boot_options_present	: 1;
		uint32_t invoke_enhance_dbg_mode: 1;
		uint32_t reserved_1		: 5;
		uint32_t d0i3_support_valid	: 1;
	} __packed fields;
};

/* Host Firmware Status Register 2 */
union me_hfsts2 {
	uint32_t data;
	struct {
		uint32_t reserved_0		: 4;
		uint32_t cpu_replaced		: 1;
		uint32_t reserved_1		: 3;
		uint32_t cpu_replaced_valid	: 1;
		uint32_t low_power_state	: 1;
		uint32_t reserved_2		: 22;
	} __packed fields;
};

/* ME Host Firmware Status Register 3 */
union me_hfsts3 {
	uint32_t data;
	struct {
		uint32_t reserved_0		: 4;
		uint32_t fw_sku			: 3;
		uint32_t reserved_1		: 25;
	} __packed fields;
};

/* Host Firmware Status Register 4 */
union me_hfsts4 {
	uint32_t data;
	struct {
		uint32_t reserved_0;
	} __packed fields;
};

/* Host Firmware Status Register 5 */
union me_hfsts5 {
	uint32_t data;
	struct {
		uint32_t reserved_0;
	} __packed fields;
};

/* Host Firmware Status Register 6 */
union me_hfsts6 {
	uint32_t data;
	struct {
		uint32_t reserved_0		: 1;
		uint32_t cpu_debug_disable	: 1;
		uint32_t reserved_1		: 28;
		uint32_t fpf_soc_lock		: 1;
		uint32_t txt_support		: 1;
	} __packed fields;
};
#endif /* _SOC_INTEL_COMMON_ME_SPEC_15_H_ */
