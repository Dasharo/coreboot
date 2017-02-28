/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <console/console.h>
#include <arch/smp/mpspec.h>
#include <device/pci.h>
#include <arch/io.h>
#include <arch/ioapic.h>
#include <string.h>
#include <stdint.h>
#include <cpu/amd/amdfam15.h>
#include <arch/cpu.h>
#include <cpu/x86/lapic.h>
//#include <southbridge/amd/agesa/hudson/hudson.h> /* pm_ioread() */
#include <southbridge/amd/pi/hudson/hudson.h> /* pm_ioread() */
#include <southbridge/amd/pi/hudson/amd_pci_int_defs.h> // IRQ routing info


u8 picr_data[FCH_INT_TABLE_SIZE] = {
	0x03,0x03,0x05,0x07,0x0B,0x0A,0x1F,0x1F,  /* 00 - 07 : INTA - INTF and 2 reserved dont map 4*/
	0xFA,0xF1,0x00,0x00,0x1F,0x1F,0x1F,0x1F,  /* 08 - 0F */
	0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,  /* 10 - 17 */
	0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 18 - 1F */
	0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x00,0x00,  /* 20 - 27 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 28 - 2F */
	0x05,0x1F,0x05,0x1F,0x04,0x1F,0x1F,0x1F,  /* 30 - 37 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 38 - 3F */
	0x1F,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,  /* 40 - 47 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 48 - 4F */
//	0x03,0x04,0x05,0x07,0x00,0x00,0x00,0x00,  /* 50 - 57 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 50 - 57 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 58 - 5F */
	0x00,0x00,0x1F 							   /* 60 - 62 */
};
u8 intr_data[FCH_INT_TABLE_SIZE] = {
	0x10,0x10,0x12,0x13,0x14,0x15,0x1F,0x1F,  /* 00 - 07 : INTA - INTF and 2 reserved dont map 4*/
	0x00,0x00,0x00,0x00,0x1F,0x1F,0x1F,0x1F,  /* 08 - 0F */
	0x09,0x1F,0x1F,0x1F,0x1F,0x1f,0x1F,0x10,  /* 10 - 17 */
	0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 18 - 1F */
	0x05,0x1F,0x1F,0x1F,0x1F,0x1F,0x00,0x00,  /* 20 - 27 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 28 - 2F */
	0x12,0x1f,0x12,0x1F,0x12,0x1F,0x1F,0x00,  /* 30 - 37 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 38 - 3F */
	0x1f,0x13,0x00,0x00,0x00,0x00,0x00,0x00,  /* 40 - 47 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 48 - 4F */
//	0x10,0x11,0x12,0x13,0x00,0x00,0x00,0x00,  /* 50 - 57 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 50 - 57 */
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 58 - 5F */
	0x00,0x00,0x1F 							   /* 60 - 62 */
};

static void smp_add_mpc_entry(struct mp_config_table *mc, unsigned length)
{
	mc->mpc_length += length;
	mc->mpc_entry_count++;
}

static void my_smp_write_bus(struct mp_config_table *mc,
			     unsigned char id, const char *bustype)
{
	struct mpc_config_bus *mpc;
	mpc = smp_next_mpc_entry(mc);
	memset(mpc, '\0', sizeof(*mpc));
	mpc->mpc_type = MP_BUS;
	mpc->mpc_busid = id;
	memcpy(mpc->mpc_bustype, bustype, sizeof(mpc->mpc_bustype));
	smp_add_mpc_entry(mc, sizeof(*mpc));
}

static void *smp_write_config_table(void *v)
{
	struct mp_config_table *mc;
	int bus_isa;
	u8 byte;

	/*
	 * By the time this function gets called, the IOAPIC registers
	 * have been written so they can be read to get the correct
	 * APIC ID and Version
	 */
	u8 ioapic_id = (io_apic_read(VIO_APIC_VADDR, 0x00) >> 24);
	u8 ioapic_ver = (io_apic_read(VIO_APIC_VADDR, 0x01) & 0xFF);

	mc = (void *)(((char *)v) + SMP_FLOATING_TABLE_LEN);

	mptable_init(mc, LOCAL_APIC_ADDR);
	memcpy(mc->mpc_oem, "AMD     ", 8);

	smp_write_processors(mc);

	//mptable_write_buses(mc, NULL, &bus_isa);
	my_smp_write_bus(mc, 0, "PCI   ");
	my_smp_write_bus(mc, 1, "PCI   ");
	bus_isa = 0x02;
	my_smp_write_bus(mc, bus_isa, "ISA   ");

	/* I/O APICs:   APIC ID Version State   Address */
	smp_write_ioapic(mc, ioapic_id, ioapic_ver, VIO_APIC_VADDR);

	smp_write_ioapic(mc, ioapic_id+1, 0x21, (void *)0xFEC20000);
	/* PIC IRQ routine */
	for (byte = 0x0; byte < sizeof(picr_data); byte ++) {
		outb(byte, 0xC00);
		outb(picr_data[byte], 0xC01);
	}

	/* APIC IRQ routine */
	for (byte = 0x0; byte < sizeof(intr_data); byte ++) {
		outb(byte | 0x80, 0xC00);
		outb(intr_data[byte], 0xC01);
	}
	/* I/O Ints:    Type    Polarity    Trigger     Bus ID   IRQ    APIC ID PIN# */
#define IO_LOCAL_INT(type, intr, apicid, pin)				\
	smp_write_lintsrc(mc, (type), MP_IRQ_TRIGGER_EDGE | MP_IRQ_POLARITY_HIGH, bus_isa, (intr), (apicid), (pin));
	mptable_add_isa_interrupts(mc, bus_isa, ioapic_id, 0);

	/* PCI interrupts are level triggered, and are
	 * associated with a specific bus/device/function tuple.
	 */
#define PCI_INT(bus, dev, int_sign, pin)				\
        smp_write_intsrc(mc, mp_INT, MP_IRQ_TRIGGER_LEVEL|MP_IRQ_POLARITY_LOW, (bus), (((dev)<<2)|(int_sign)), ioapic_id, (pin))

	/* Internal VGA */
	PCI_INT(0x0, 0x01, 0x0, intr_data[PIRQ_B]);
	PCI_INT(0x0, 0x01, 0x1, intr_data[PIRQ_C]);

	/* SMBUS */
	PCI_INT(0x0, 0x14, 0x0, 0x10);

	/* HD Audio */
//	PCI_INT(0x0, 0x14, 0x0, intr_data[PIRQ_HDA]);

	/* SD card */
	PCI_INT(0x0, 0x14, 0x1, intr_data[PIRQ_SD]);

	/* USB */
	PCI_INT(0x0, 0x12, 0x0, intr_data[PIRQ_EHCI1]);
	PCI_INT(0x0, 0x13, 0x0, intr_data[PIRQ_EHCI2]);
	PCI_INT(0x0, 0x16, 0x0, intr_data[PIRQ_EHCI3]);

	/* sata */
	PCI_INT(0x0, 0x11, 0x0, intr_data[PIRQ_SATA]);

	/* on board NIC & Slot PCIE.  */


	/* PCIe Lan*/
//	PCI_INT(0x0, 0x06, 0x0, 0x13); // No integrated LAN

//	/* FCH PCIe PortA */
//	PCI_INT(0x0, 0x15, 0x0, 0x10);
//	/* FCH PCIe PortB */
//	PCI_INT(0x0, 0x15, 0x1, 0x11);
//	/* FCH PCIe PortC */
//	PCI_INT(0x0, 0x15, 0x2, 0x12);
//	/* FCH PCIe PortD */
//	PCI_INT(0x0, 0x15, 0x3, 0x13);

	/* GPP0 */
	PCI_INT(0x0, 0x2, 0x0, 0x10);	// Network 3
	/* GPP1 */
	PCI_INT(0x0, 0x2, 0x1, 0x11);	// Network 2
	/* GPP2 */
	PCI_INT(0x0, 0x2, 0x2, 0x12);	// Network 1
	/* GPP3 */
	PCI_INT(0x0, 0x2, 0x3, 0x13);	// mPCI
	/* GPP4 */
	PCI_INT(0x0, 0x2, 0x4, 0x14);	// mPCI



	/*Local Ints:   Type    Polarity    Trigger     Bus ID   IRQ    APIC ID PIN# */
	IO_LOCAL_INT(mp_ExtINT, 0, MP_APIC_ALL, 0x0);
	IO_LOCAL_INT(mp_NMI, 0, MP_APIC_ALL, 0x1);
	/* There is no extension information... */

	/* Compute the checksums */
	return mptable_finalize(mc);
}

unsigned long write_smp_table(unsigned long addr)
{
	void *v;
	v = smp_write_floating_table(addr, 0);
	return (unsigned long)smp_write_config_table(v);
}
