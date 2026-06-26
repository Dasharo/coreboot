/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef PROMONTORY21_CHIP_H
#define PROMONTORY21_CHIP_H

#include <stdbool.h>
#include <stdint.h>
#include <device/pci_def.h>

#define PROM21_MAX_PCIE_LANES		12
#define PROM21_MAX_PCIE_CLKREQ		6
#define PROM21_MAX_SATA_PORTS		4
#define PROM21_XHCI_MAX_USB3_PORTS	6
#define PROM21_XHCI_MAX_USB2_PORTS	12
#define PROM21_MAX_GPIOS		24

#define PROM21_XHCI_DEVFN		PCI_DEVFN(0xc, 0)
#define PROM21_SATA_DEVFN		PCI_DEVFN(0xd, 0)

struct prom21_sata_phy {
	bool    override;
	uint8_t gen1_swing;
	uint8_t gen2_swing;
	uint8_t gen3_swing;
	uint8_t gen1_emp_level;
	uint8_t gen2_emp_level;
	uint8_t gen3_emp_level;
};

struct prom21_usb3_phy {
	bool    override;
	uint8_t gen1_swing;
	uint8_t gen1_emp_level_en;
	uint8_t gen1_emp_level;
	uint8_t gen1_preshoot_en;
	uint8_t gen1_preshoot;
	uint8_t gen2_swing;
	uint8_t gen2_cp0_emp_level_en;
	uint8_t gen2_cp0_emp_level;
	uint8_t gen2_cp0_preshoot_en;
	uint8_t gen2_cp0_preshoot;
	uint8_t gen2_cp13_emp_level_en;
	uint8_t gen2_cp13_emp_level;
	uint8_t gen2_cp13_preshoot_en;
	uint8_t gen2_cp13_preshoot;
	uint8_t gen2_cp14_emp_level_en;
	uint8_t gen2_cp14_emp_level;
	uint8_t gen2_cp14_preshoot_en;
	uint8_t gen2_cp14_preshoot;
	uint8_t gen2_cp15_emp_level_en;
	uint8_t gen2_cp15_emp_level;
	uint8_t gen2_cp15_preshoot_en;
	uint8_t gen2_cp15_preshoot;
	uint8_t gen2_cp16_emp_level_en;
	uint8_t gen2_cp16_emp_level;
	uint8_t gen2_cp16_preshoot_en;
	uint8_t gen2_cp16_preshoot;
};

struct prom21_usb2_phy {
	bool    override;
	uint8_t slew_rate;
	uint8_t driving_current;
	uint8_t termination;
};

enum prom21_xhci_port_gen {
	XhciPortGenDefault = 0,
	XhciPortGen1 = 1,
	XhciPortGen2 = 2
};

enum prom21_xhci_gen {
	XhciGenDefault = 0,
	XhciGen2x2 = 1,
	XhciGen2x1 = 2
};

enum prom21_boolean {
	HwDefault = 0,
	Disable = 1,
	Enable = 2
};

enum prom21_sata_mode {
	SataAhci = 0,
	SataRAID = 1
};

enum prom21_clkreq_pin_select {
	ClkreqPort0,
	ClkreqPort1,
	ClkreqPort2,
	ClkreqPort3,
	ClkreqPort4,
	ClkreqPort5,
	ClkreqPort6,
	ClkreqPort7,
	ClkreqPort8,
	ClkreqPort9,
	ClkreqPort10,
	ClkreqPort11,
	ClkreqUnused = 0xe
};

enum prom21_clkreq_mode {
	ClkreqMode,
	ClkAlwaysOn,
	ClkAlwaysOff,
	GpioMode
};

struct prom21_sata_config {
	enum prom21_sata_mode sata_mode;
	enum prom21_boolean aggresive_link_pm_cap;
	enum prom21_boolean psc_cap;
	enum prom21_boolean ssc_cap;
	enum prom21_boolean hot_plug;
	enum prom21_boolean cccs_cap;
	enum prom21_boolean msi_cap;
	uint8_t port_enable[PROM21_MAX_SATA_PORTS];
	enum prom21_boolean aggressive_dev_slp[PROM21_MAX_SATA_PORTS];
};

struct prom21_usb_config {
	enum prom21_xhci_gen usb3_gen;
	enum prom21_xhci_port_gen port_gen[PROM21_XHCI_MAX_USB3_PORTS];
	enum prom21_boolean hw_lpm;
	enum prom21_boolean dbc;
};

struct prom21_pcie_config {
	enum prom21_boolean report_small_ltr;
	bool gen1_swing_enable;
	uint8_t pcie_gen1_swing[PROM21_MAX_PCIE_LANES];
	uint8_t eq_preset;
	bool gpio_perst_enable;
	enum prom21_boolean msi;
	enum prom21_boolean msix;
	enum prom21_clkreq_pin_select clkreq_pin_select[PROM21_MAX_PCIE_CLKREQ];
	enum prom21_clkreq_mode clkreq_mode[PROM21_MAX_PCIE_CLKREQ];
	enum prom21_boolean lane_reversal_en[PROM21_MAX_PCIE_LANES / 2];
	uint8_t port_target_speed[PROM21_MAX_PCIE_LANES];
};

union prom21_gpio_common_setting {
	struct {
		uint16_t debounce_timer:3;
		uint16_t debounce_timeout_th:3;
		uint16_t int_output_en:1;
		uint16_t int_act_level:1;
		uint16_t int_mode:1;
		uint16_t reserved:7;
	} common;
	uint16_t raw;
};

union prom21_gpio_setting {
	struct {
		uint16_t out_en:1;
		uint16_t out:1;
		uint16_t int_enable:1;
		uint16_t int_level_trig_type:1;
		uint16_t int_type:2;
		uint16_t int_mask:1;
		uint16_t reserved:9;
	} gpio;
	uint16_t raw;
};

struct prom21_gpio_item {
	uint16_t pin;
	union prom21_gpio_setting setting;
} ;

struct prom21_gpio_init_table {
	union prom21_gpio_common_setting gpiocommon;
	struct prom21_gpio_item          gpio_list[PROM21_MAX_GPIOS * 2 + 1];
};

struct drivers_amd_promontory21_config {
	enum prom21_boolean si_prog_enable;
	struct prom21_usb3_phy usb3_phy[PROM21_XHCI_MAX_USB3_PORTS];
	struct prom21_usb2_phy usb2_phy[PROM21_XHCI_MAX_USB2_PORTS];
	struct prom21_sata_phy sata_phy[PROM21_MAX_SATA_PORTS];

	struct prom21_usb_config usb;
	struct prom21_sata_config sata;
	struct prom21_pcie_config pcie;
	struct prom21_gpio_init_table gpio;
};

#endif /* OPENSIL_PHOENIX_POC_MPIO_CHIP_H */
