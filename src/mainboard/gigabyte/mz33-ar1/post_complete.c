/* SPDX-License-Identifier: GPL-2.0-only */

#include <amdblocks/gpio.h>
#include <boot/coreboot_tables.h>
#include <bootstate.h>
#include <console/console.h>
#include <drivers/ipmi/gigabyte_smbios_oem.h>
#include <drivers/ipmi/ipmi_if.h>
#include <gpio.h>

/*
 * FCH GPIO 115 tells the BMC that POST is complete ("BIOS ready"), active
 * low. The BMC reads it through the CPLD. The vendor BIOS keeps it high
 * during POST and drives it low at ReadyToBoot, followed by the Gigabyte OEM
 * IPMI command "BIOS ready", which starts the BMC's MCTP and I3C device
 * discovery.
 *
 * The bootblock configures the GPIO as an output driven high. With the edk2
 * Gigabyte BMC driver, the payload completes POST at ReadyToBoot, so it finds
 * the GPIO by name in the coreboot table and signals both. The port is the
 * address of the GPIO control register and the value the bits in it the
 * payload clears (active low) or sets (active high) to assert the GPIO:
 * GPIO_OUTPUT_VALUE. Without that driver coreboot signals it before booting
 * the payload.
 */
#define MZ33_GPIO_POST_COMPLETE		GPIO_115
#define MZ33_GPIO_POST_COMPLETE_NAME	"POST complete"

void fill_lb_gpios(struct lb_gpios *gpios)
{
	const struct lb_gpio mz33_gpios[] = {
		{
			.port = gpio_control_address(MZ33_GPIO_POST_COMPLETE),
			.polarity = ACTIVE_LOW,
			.value = GPIO_OUTPUT_VALUE,
			.name = MZ33_GPIO_POST_COMPLETE_NAME,
		},
	};

	lb_add_gpios(gpios, mz33_gpios, ARRAY_SIZE(mz33_gpios));
}

static void signal_post_complete(void *unused)
{
	const struct gigabyte_set_bios_ready_req req = {
		.hdr = {
			.iana = GIGABYTE_IANA,
			.subcmd = GIGABYTE_SUBCMD_SET_BIOS_READY,
		},
		.data = 0,
	};
	struct ipmi_rsp rsp;
	int ret;

	if (CONFIG(EDK2_GIGABYTE_BMC_SYSINFO))
		return;

	gpio_set(MZ33_GPIO_POST_COMPLETE, 0);

	ret = ipmi_message(CONFIG_BMC_KCS_BASE, IPMI_NETFN_GIGABYTE_OEM, 0,
			   IPMI_CMD_GIGABYTE_OEM_SET, (const unsigned char *)&req, sizeof(req),
			   (unsigned char *)&rsp, sizeof(rsp));
	if (ret < (int)sizeof(rsp) || rsp.completion_code)
		printk(BIOS_ERR, "IPMI: Gigabyte BIOS ready failed (ret=%d resp=0x%x)\n",
		       ret, rsp.completion_code);
	else
		printk(BIOS_DEBUG, "IPMI: Gigabyte BIOS ready sent\n");
}

BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_BOOT, BS_ON_ENTRY, signal_post_complete, NULL);
