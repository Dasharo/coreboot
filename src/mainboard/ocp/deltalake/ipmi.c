/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <console/console.h>
#include <drivers/ipmi/ipmi_kcs.h>
#include <drivers/ipmi/ipmi_ops.h>
#include <drivers/vpd/vpd.h>
#include <string.h>

#include "ipmi.h"
#include "vpd.h"

enum cb_err ipmi_set_ppin(struct ppin_req *req)
{
	int ret;
	struct ipmi_rsp rsp;

	ret = ipmi_kcs_message(CONFIG_BMC_KCS_BASE, IPMI_NETFN_OEM, 0x0, IPMI_OEM_SET_PPIN,
			(const unsigned char *) req, sizeof(*req),
			(unsigned char *) &rsp, sizeof(rsp));

	if (ret < sizeof(struct ipmi_rsp) || rsp.completion_code) {
		printk(BIOS_ERR, "IPMI: %s command failed (ret=%d resp=0x%x)\n",
			__func__, ret, rsp.completion_code);
		return CB_ERR;
	}

	return CB_SUCCESS;
}

enum cb_err ipmi_get_pcie_config(uint8_t *pcie_config)
{
	int ret;
	struct ipmi_config_rsp {
		struct ipmi_rsp resp;
		uint8_t config;
	} __packed;
	struct ipmi_config_rsp rsp;

	ret = ipmi_kcs_message(CONFIG_BMC_KCS_BASE, IPMI_NETFN_OEM, 0x0,
			IPMI_OEM_GET_PCIE_CONFIG, NULL, 0, (unsigned char *) &rsp,
			sizeof(rsp));

	if (ret < sizeof(struct ipmi_rsp) || rsp.resp.completion_code) {
		printk(BIOS_ERR, "IPMI: %s command failed (ret=%d resp=0x%x)\n",
			__func__, ret, rsp.resp.completion_code);
		return CB_ERR;
	}
	*pcie_config = rsp.config;

	return CB_SUCCESS;
}

enum cb_err ipmi_get_slot_id(uint8_t *slot_id)
{
	int ret;
	struct ipmi_config_rsp {
		struct ipmi_rsp resp;
		uint8_t board_sku_id;
		uint8_t board_rev_id;
		uint8_t slot_id;
		uint8_t slot_config_id;
	} __packed;
	struct ipmi_config_rsp rsp;

	ret = ipmi_kcs_message(CONFIG_BMC_KCS_BASE, IPMI_NETFN_OEM, 0x0, IPMI_OEM_GET_BOARD_ID,
			NULL, 0, (unsigned char *) &rsp, sizeof(rsp));

	if (ret < sizeof(struct ipmi_rsp) || rsp.resp.completion_code) {
		printk(BIOS_ERR, "IPMI: %s command failed (ret=%d resp=0x%x)\n",
			__func__, ret, rsp.resp.completion_code);
		return CB_ERR;
	}
	*slot_id = rsp.slot_id;

	return CB_SUCCESS;
}

enum cb_err ipmi_set_post_start(const int port)
{
	int ret;
	struct ipmi_rsp rsp;

	ret = ipmi_kcs_message(port, IPMI_NETFN_OEM, 0x0,
				 IPMI_BMC_SET_POST_START, NULL, 0, (u8 *) &rsp,
				 sizeof(rsp));

	if (ret < sizeof(struct ipmi_rsp) || rsp.completion_code) {
		printk(BIOS_ERR, "IPMI: %s command failed (ret=%d rsp=0x%x)\n",
		       __func__, ret, rsp.completion_code);
		return CB_ERR;
	}
	if (ret != sizeof(rsp)) {
		printk(BIOS_ERR, "IPMI: %s response truncated\n", __func__);
		return CB_ERR;
	}

	printk(BIOS_DEBUG, "IPMI BMC POST is started\n");
	return CB_SUCCESS;
}

void init_frb2_wdt(void)
{
	char val[VPD_LEN];
	uint8_t enable, action;
	uint16_t countdown;

	if (vpd_get_bool(FRB2_TIMER, VPD_RW_THEN_RO, &enable)) {
		printk(BIOS_DEBUG, "Got VPD %s value: %d\n", FRB2_TIMER, enable);
	} else {
		printk(BIOS_INFO, "Not able to get VPD %s, default set to %d\n", FRB2_TIMER,
			FRB2_TIMER_DEFAULT);
		enable = FRB2_TIMER_DEFAULT;
	}

	if (enable) {
		if (vpd_gets(FRB2_COUNTDOWN, val, VPD_LEN, VPD_RW_THEN_RO)) {
			countdown = (uint16_t)atol(val);
			printk(BIOS_DEBUG, "FRB2 timer countdown set to: %d ms\n",
				countdown * 100);
		} else {
			printk(BIOS_DEBUG, "FRB2 timer use default value: %d ms\n",
				FRB2_COUNTDOWN_DEFAULT * 100);
			countdown = FRB2_COUNTDOWN_DEFAULT;
		}

		if (vpd_gets(FRB2_ACTION, val, VPD_LEN, VPD_RW_THEN_RO)) {
			action = (uint8_t)atol(val);
			printk(BIOS_DEBUG, "FRB2 timer action set to: %d\n", action);
		} else {
			printk(BIOS_DEBUG, "FRB2 timer action use default value: %d\n",
				FRB2_ACTION_DEFAULT);
			action = FRB2_ACTION_DEFAULT;
		}
		ipmi_init_and_start_bmc_wdt(CONFIG_BMC_KCS_BASE, countdown, action);
	} else {
		printk(BIOS_DEBUG, "Disable FRB2 timer\n");
		ipmi_stop_bmc_wdt(CONFIG_BMC_KCS_BASE);
	}
}

enum cb_err ipmi_set_cmos_clear(void)
{
	int ret;

	struct ipmi_oem_rsp {
		struct ipmi_rsp resp;
		struct boot_order data;
	} __packed;

	struct ipmi_oem_rsp rsp;
	struct boot_order req;

	/* IPMI OEM get bios boot order command to check if the valid bit and
	   the CMOS clear bit are both set from the response BootMode byte. */

	ret = ipmi_kcs_message(CONFIG_BMC_KCS_BASE, IPMI_NETFN_OEM, 0x0,
			IPMI_OEM_GET_BIOS_BOOT_ORDER,
			NULL, 0,
			(unsigned char *) &rsp, sizeof(rsp));

	if (ret < sizeof(struct ipmi_rsp) || rsp.resp.completion_code) {
		printk(BIOS_ERR, "IPMI: %s command failed (read ret=%d resp=0x%x)\n",
			__func__, ret, rsp.resp.completion_code);
		return CB_ERR;
	}

	if (!IS_CMOS_AND_VALID_BIT(rsp.data.boot_mode)) {
		req = rsp.data;
		SET_CMOS_AND_VALID_BIT(req.boot_mode);
		ret = ipmi_kcs_message(CONFIG_BMC_KCS_BASE, IPMI_NETFN_OEM, 0x0,
				IPMI_OEM_SET_BIOS_BOOT_ORDER,
				(const unsigned char *) &req, sizeof(req),
				(unsigned char *) &rsp, sizeof(rsp));

		if (ret < sizeof(struct ipmi_rsp) || rsp.resp.completion_code) {
			printk(BIOS_ERR, "IPMI: %s command failed (sent ret=%d resp=0x%x)\n",
				__func__, ret, rsp.resp.completion_code);
			return CB_ERR;
		}

		printk(BIOS_INFO, "IPMI CMOS clear requested because CMOS data is invalid.\n");

		return CB_SUCCESS;
	}

	return CB_SUCCESS;
}
