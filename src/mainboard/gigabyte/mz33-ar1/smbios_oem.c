/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Gigabyte OEM SMBIOS records read by the MZ33-AR1 BMC (AMI MegaRAC firmware,
 * libgbtmisc.so SmbiosInterface*), as produced by the vendor BIOS
 * (GbtUpdateSMBIOS.efi and SendIpmiSystemInfo.efi):
 *
 *  - Type 0x80: Gigabyte SMBIOS version, read by the BMC as a big-endian
 *    UINT16 (0x0100).
 *  - Type 0x84: CPU extras, paired by index with the type 4 records
 *    ("hexGbtChars" / "hexGbtStatus" in the BMC CPU inventory).
 *  - Type 0x85: DIMM extras ("iMaxSize" / "iTotalSlot" in the BMC DIMM
 *    inventory).
 *  - Type 0x83: one record per NIC port, onboard or add-in card ("strMAC",
 *    PCI location, onboard flag in the BMC network inventory).
 *
 * Disk records (type 0x81) need the UEFI storage stack and are added by the
 * edk2 payload (DasharoPayloadPkg/GigabyteBmcSysInfoDxe), which also builds
 * these records itself if coreboot did not.
 */

#include <arch/cpuid.h>
#include <commonlib/endian.h>
#include <console/console.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_def.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <device/pciexp.h>
#include <drivers/ipmi/ipmi_ops.h>
#include <rtc.h>
#include <smbios.h>
#include <static.h>
#include <stddef.h>
#include <string.h>
#include <types.h>

#include "smbios_oem.h"

#define GBT_SMBIOS_TYPE_VERSION		0x80
#define GBT_SMBIOS_TYPE_NETWORK		0x83
#define GBT_SMBIOS_TYPE_CPU		0x84
#define GBT_SMBIOS_TYPE_DIMM		0x85

/* Vendor BIOS value, shown by the BMC as 0x0100 */
#define GBT_SMBIOS_VERSION		0x0001

/*
 * The vendor BIOS always writes two CPU records, both from the BSP's CPUID,
 * on this single socket board as well.
 */
#define GBT_CPU_RECORDS			2

#define GBT_CPU_CHAR_VMX		(1 << 0)
#define GBT_CPU_CHAR_SMT		(1 << 1)
#define GBT_CPU_CHAR_NX			(1 << 2)
#define GBT_CPU_CHAR_TURBO		(1 << 3)

#define PCI_VID_CHELSIO			0x1425

#define GBT_ONBOARD_NONE		0xffff
#define GBT_HANDLE_NONE			0xffff

/*
 * Functions of multi-personality Chelsio adapters that are not NICs, skipped
 * like in the vendor BIOS: device ID and a bit mask of PCI functions.
 */
static const struct {
	u16 device;
	u8 functions;
} chelsio_skip[] = {
	{ 0x540e, 0x10 },
	{ 0x6001, 0x0c }, { 0x6401, 0x10 },
	{ 0x6002, 0x0c }, { 0x6402, 0x10 },
	{ 0x6007, 0x0c }, { 0x6407, 0x10 },
	{ 0x6008, 0x0c }, { 0x6408, 0x10 },
	{ 0x600d, 0x0c }, { 0x640d, 0x10 },
};

struct gbt_smbios_type80 {
	struct smbios_header header;
	u16 version;
	u8 eos[2];
} __packed;

struct gbt_smbios_type84 {
	struct smbios_header header;
	u32 characteristics;	/* GBT_CPU_CHAR_* supported */
	u32 status;		/* Always 0 in the vendor BIOS */
	u8 eos[2];
} __packed;

struct gbt_smbios_type85 {
	struct smbios_header header;
	u32 max_size_gib;
	u32 total_slots;
	u8 eos[2];
} __packed;

struct gbt_smbios_type83 {
	struct smbios_header header;
	u16 device_id;
	u16 vendor_id;
	u8 unknown0;		/* Always 1 */
	u8 mac[6];
	u8 reserved0[18];
	u32 unknown1;		/* Always 0xee000000 */
	u16 slot_handle;	/* Type 9 slot or 0xffff */
	u16 port;		/* PCI function number */
	u8 reserved1[40];
	u8 speed;		/* Always 0 */
	u16 onboard_index;	/* 0xffff: add-in card */
	u16 segment;
	u8 bus;
	u8 devfn;
	u16 subsystem_vendor_id;
	u16 subsystem_id;
	u8 eos[2];
} __packed;

_Static_assert(sizeof(struct gbt_smbios_type80) - 2 == 6, "Wrong type 0x80 size");
_Static_assert(sizeof(struct gbt_smbios_type84) - 2 == 12, "Wrong type 0x84 size");
_Static_assert(sizeof(struct gbt_smbios_type85) - 2 == 12, "Wrong type 0x85 size");
_Static_assert(sizeof(struct gbt_smbios_type83) - 2 == 0x5c, "Wrong type 0x83 size");
_Static_assert(offsetof(struct gbt_smbios_type83, slot_handle) == 0x25, "Wrong type 0x83 layout");
_Static_assert(offsetof(struct gbt_smbios_type83, onboard_index) == 0x52, "Wrong type 0x83 layout");

static int write_record(unsigned long *current, int *handle, void *record, size_t size)
{
	struct smbios_header *header = record;
	const int len = smbios_full_table_len(header, (u8 *)record + size - 2);

	*current += len;
	*handle += 1;
	return len;
}

static int write_type80(unsigned long *current, int *handle)
{
	struct gbt_smbios_type80 *t = smbios_carve_table(*current, GBT_SMBIOS_TYPE_VERSION,
							 sizeof(*t), *handle);

	t->version = GBT_SMBIOS_VERSION;
	return write_record(current, handle, t, sizeof(*t));
}

/* Same CPUID checks as the vendor BIOS (they target Intel; VMX and Turbo read 0 on AMD) */
static u32 cpu_characteristics(void)
{
	u32 chars = 0;

	if (cpuid_ecx(1) & (1 << 5))
		chars |= GBT_CPU_CHAR_VMX;
	if ((cpuid_ext(0xb, 0).ebx & 0xff) > 1)
		chars |= GBT_CPU_CHAR_SMT;
	if (cpuid_edx(0x80000001) & (1 << 20))
		chars |= GBT_CPU_CHAR_NX;
	if (cpuid_eax(6) & (1 << 1))
		chars |= GBT_CPU_CHAR_TURBO;

	return chars;
}

static int write_type84(unsigned long *current, int *handle)
{
	const u32 chars = cpu_characteristics();
	int len = 0;

	for (int i = 0; i < GBT_CPU_RECORDS; i++) {
		struct gbt_smbios_type84 *t = smbios_carve_table(*current, GBT_SMBIOS_TYPE_CPU,
								 sizeof(*t), *handle);
		t->characteristics = chars;
		t->status = 0;
		len += write_record(current, handle, t, sizeof(*t));
	}

	return len;
}

static int write_type85(unsigned long *current, int *handle)
{
	struct gbt_smbios_type85 *t = smbios_carve_table(*current, GBT_SMBIOS_TYPE_DIMM,
							 sizeof(*t), *handle);

	t->max_size_gib = MZ33_MAX_MEMORY_GIB;
	t->total_slots = CONFIG_DIMM_MAX;
	return write_record(current, handle, t, sizeof(*t));
}

/*
 * NICs put their MAC address in the PCIe Device Serial Number as an EUI-64
 * built from the EUI-48 with ff:fe inserted in the middle, e.g. DSN
 * 10-ff-e0-ff-fe-ab-c8-b1 for MAC 10:ff:e0:ab:c8:b1. All functions of a
 * multi-port NIC report the same DSN, holding the MAC of the first port; the
 * other ports follow in function number order (vendor BIOS on the onboard
 * BCM57416: function 1 is 10:ff:e0:ab:c8:b2).
 */
static bool nic_mac_from_dsn(struct device *dev, u8 mac[6])
{
	unsigned int cap;
	u32 low, high, nic;

	cap = pciexp_find_extended_cap(dev, PCI_EXT_CAP_ID_DSN, 0);
	if (!cap)
		return false;

	low = pci_read_config32(dev, cap + 4);
	high = pci_read_config32(dev, cap + 8);
	if ((high & 0xff) != 0xff || (low >> 24) != 0xfe)
		return false;

	nic = (low & 0xffffff) + PCI_FUNC(dev->path.pci.devfn);
	mac[0] = high >> 24;
	mac[1] = high >> 16;
	mac[2] = high >> 8;
	mac[3] = nic >> 16;
	mac[4] = nic >> 8;
	mac[5] = nic;
	return true;
}

static bool is_skipped_function(const struct device *dev)
{
	if (dev->vendor != PCI_VID_CHELSIO)
		return false;

	for (size_t i = 0; i < ARRAY_SIZE(chelsio_skip); i++) {
		if (chelsio_skip[i].device == dev->device &&
		    chelsio_skip[i].functions & (1 << PCI_FUNC(dev->path.pci.devfn)))
			return true;
	}

	return false;
}

/*
 * The onboard dual-port BCM57416 sits behind gpp_bridge_5_0_b (the vendor
 * BIOS uses a list of onboard root ports the same way). Every NIC function
 * below it is onboard and gets the instance of the "Onboard LAN" SMBIOS
 * type 41 record from the devicetree; anything else is an add-in card.
 */
static u16 dev_onboard_index(const struct device *dev)
{
	const struct device *bridge = DEV_PTR(gpp_bridge_5_0_b);
	const struct device *br;

	for (br = dev->upstream ? dev->upstream->dev : NULL; br && br != bridge;
	     br = br->upstream ? br->upstream->dev : NULL) {
		if (br->path.type != DEVICE_PATH_PCI)
			return GBT_ONBOARD_NONE;
	}

	if (!br)
		return GBT_ONBOARD_NONE;

	for (const struct device *child = bridge->downstream ? bridge->downstream->children : NULL;
	     child; child = child->sibling) {
		if (child->smbios_instance_id_valid)
			return child->smbios_instance_id;
	}

	return 0;
}

static int write_type83(unsigned long *current, int *handle)
{
	int len = 0;

	for (struct device *dev = all_devices; dev; dev = dev->next) {
		struct gbt_smbios_type83 *t;
		u8 mac[6];

		if (!is_enabled_pci(dev) || (dev->class >> 8) != PCI_CLASS_NETWORK_ETHERNET ||
		    is_skipped_function(dev))
			continue;

		if (!nic_mac_from_dsn(dev, mac)) {
			printk(BIOS_INFO, "%s: no MAC address in the PCIe DSN, skipping\n",
			       dev_path(dev));
			continue;
		}

		t = smbios_carve_table(*current, GBT_SMBIOS_TYPE_NETWORK, sizeof(*t), *handle);
		t->device_id = dev->device;
		t->vendor_id = dev->vendor;
		t->unknown0 = 1;
		memcpy(t->mac, mac, sizeof(t->mac));
		t->unknown1 = 0xee000000;
		t->slot_handle = GBT_HANDLE_NONE;
		t->port = PCI_FUNC(dev->path.pci.devfn);
		t->speed = 0;
		t->segment = dev->upstream->segment_group;
		t->bus = dev->upstream->secondary;
		t->devfn = dev->path.pci.devfn;
		t->subsystem_vendor_id = pci_read_config16(dev, PCI_SUBSYSTEM_VENDOR_ID);
		t->subsystem_id = pci_read_config16(dev, PCI_SUBSYSTEM_ID);
		t->onboard_index = dev_onboard_index(dev);

		printk(BIOS_INFO, "%s: MAC %02x:%02x:%02x:%02x:%02x:%02x, %s\n", dev_path(dev),
		       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
		       t->onboard_index == GBT_ONBOARD_NONE ? "add-in" : "onboard");

		len += write_record(current, handle, t, sizeof(*t));
	}

	return len;
}

/*
 * SMBIOS type 1 UUID. The vendor BIOS (DxeIpmiInitialize.efi) takes it from
 * the BMC with IPMI Get System GUID. The BMC answers with the UUID of the
 * SMBIOS tables sent to it on the previous boot, or with its own GUID
 * (/conf/Sys_GUID.dat) if it has none, so the UUID stays the same across
 * boots. IPMI sends every GUID field least significant byte first, SMBIOS
 * only the time fields.
 *
 * Firmware that reported no UUID leaves an all-zero one in the BMC. Generate
 * a new UUID then, in the same form as the one the vendor set on the board:
 * RFC 4122 version 1 with the midnight of the current day as timestamp, clock
 * sequence 0 and the MAC address of the onboard NIC port 0 (the device behind
 * gpp_bridge_5_0_b, function 0) as node, e.g.
 * f69d0000-f592-11f0-8000-10ffe0abc8b1 for 2026-01-20 and 10:ff:e0:ab:c8:b1.
 * The BMC returns it from the next boot on. Without an answer from the BMC
 * the UUID is left unset, a new one on every boot would be worse.
 */

/* 100 ns intervals from 1582-10-15, the UUID epoch, to 1970-01-01 */
#define UUID_TIME_OFFSET	0x01b21dd213814000ULL
#define UUID_VERSION_1		0x1000
#define UUID_VARIANT_RFC4122	0x80

static bool uuid_is_valid(const u8 uuid[16])
{
	bool zeros = true, ones = true;

	for (size_t i = 0; i < 16; i++) {
		zeros &= uuid[i] == 0x00;
		ones &= uuid[i] == 0xff;
	}

	return !zeros && !ones;
}

static void ipmi_guid_to_smbios_uuid(const u8 guid[16], u8 uuid[16])
{
	memcpy(&uuid[0], &guid[12], 4);	/* time_low */
	memcpy(&uuid[4], &guid[10], 2);	/* time_mid */
	memcpy(&uuid[6], &guid[8], 2);	/* time_hi_and_version */
	for (size_t i = 0; i < 8; i++)	/* clock_seq and node */
		uuid[8 + i] = guid[7 - i];
}

/* MAC of port 0 of the onboard BCM57416, the device behind gpp_bridge_5_0_b */
static bool onboard_nic_mac(u8 mac[6])
{
	const struct device *bridge = DEV_PTR(gpp_bridge_5_0_b);

	if (!bridge->enabled || !bridge->downstream)
		return false;

	for (struct device *dev = bridge->downstream->children; dev; dev = dev->sibling) {
		if (!is_enabled_pci(dev) || (dev->class >> 8) != PCI_CLASS_NETWORK_ETHERNET ||
		    dev->path.pci.devfn != PCI_DEVFN(0, 0))
			continue;

		return nic_mac_from_dsn(dev, mac);
	}

	return false;
}

static bool generate_uuid(u8 uuid[16])
{
	struct rtc_time today;
	u8 mac[6];
	u64 time;

	if (!onboard_nic_mac(mac) || rtc_get(&today) || rtc_invalid(&today))
		return false;

	today.hour = 0;
	today.min = 0;
	today.sec = 0;
	time = (u64)rtc_mktime(&today) * 10000000 + UUID_TIME_OFFSET;

	write_le32(&uuid[0], time);
	write_le16(&uuid[4], time >> 32);
	write_le16(&uuid[6], ((time >> 48) & 0x0fff) | UUID_VERSION_1);
	uuid[8] = UUID_VARIANT_RFC4122;
	uuid[9] = 0;
	memcpy(&uuid[10], mac, sizeof(mac));

	return true;
}

void smbios_system_set_uuid(u8 *uuid)
{
	u8 guid[16];

	if (ipmi_get_system_guid(CONFIG_BMC_KCS_BASE, guid) != CB_SUCCESS)
		return;

	if (uuid_is_valid(guid))
		ipmi_guid_to_smbios_uuid(guid, uuid);
	else if (generate_uuid(uuid))
		printk(BIOS_INFO, "BMC has no system GUID, generated a new UUID\n");
	else
		printk(BIOS_ERR, "BMC has no system GUID, could not generate a UUID\n");
}

int mainboard_smbios_oem_data(struct device *dev, int *handle, unsigned long *current)
{
	int len = 0;

	len += write_type80(current, handle);
	len += write_type84(current, handle);
	len += write_type85(current, handle);
	len += write_type83(current, handle);

	return len;
}
