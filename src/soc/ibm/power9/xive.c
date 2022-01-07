/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/mmio.h>
#include <assert.h>
#include <cpu/power/scom.h>
#include <string.h>

#include "homer.h"

#define CODE_SIZE(x) ((x ## _end) - (x))

extern uint8_t sys_reset_int[];
extern uint8_t sys_reset_int_end[];
extern uint8_t ext_int[];
extern uint8_t ext_int_end[];
extern uint8_t hyp_virt_int[];
extern uint8_t hyp_virt_int_end[];

#define IVPE_BAR	0x0006020000000000
#define FSP_BAR		0x0006030100000000
#define PSI_HB_BAR	0x0006030203000000
#define PSI_HB_ESB_BAR	0x00060302031C0000
#define XIVE_IC_BAR	0x0006030203100000

/*
 * XIVE is not officially documented anywhere. There are bits and pieces that
 * can be put together in [KVM] and [QEMU], but those are mostly about using
 * XIVE for virtualization, not bare metal. Code below was ported from Hostboot
 * and probably this is the best that we can hope for without documentation.
 *
 * [KVM]  https://www.kernel.org/doc/html/latest/virt/kvm/devices/xive.html
 * [QEMU] https://qemu.readthedocs.io/en/latest/specs/ppc-xive.html
 */
void configure_xive(int core)
{
	uint64_t tmp;

	/* Install handlers */
	memcpy((void *)0x100, sys_reset_int, CODE_SIZE(sys_reset_int));
	memcpy((void *)0x500, ext_int, CODE_SIZE(ext_int));
	memcpy((void *)0xEA0, hyp_virt_int, CODE_SIZE(hyp_virt_int));

	/* IVPE BAR + enable bit */
	write_scom(0x05013012, IVPE_BAR | PPC_BIT(0));

	/* FSP BAR */
	write_scom(0x0501290B, FSP_BAR);

	/* PSI HB BAR + enable bit */
	write_scom(0x0501290A, PSI_HB_BAR | PPC_BIT(63));

	/* Disable VPC Pull error */
	scom_and(0x05013179, ~PPC_BIT(30));

	/* PSI HB ESB BAR + enable bit */
	write_scom(0x05012916, PSI_HB_ESB_BAR | PPC_BIT(63));

	/* XIVE IC BAR + enable bit */
	write_scom(0x05013010, XIVE_IC_BAR | PPC_BIT(0));

	/* Set HB mode on P3PC register */
	scom_or(0x05013110, PPC_BIT(33));

	/* Disable PSI interrupts */
	write_scom(0x05012913, PPC_BIT(3));

	void *esb_bar = (void *)PSI_HB_ESB_BAR;
	/* Mask all interrupt sources */
	for (int i = 0; i < 14; i++) {
		tmp = read64(esb_bar + i*0x1000 + 0xD00);
		eieio();
		tmp = read64(esb_bar + i*0x1000 + 0x800);
		assert(tmp == 1);
	}

	/* Route interrupts to CEC (whatever that is) instead of FSP */
	void *hb_bar = (void *)PSI_HB_BAR;
	write64(hb_bar + 0x20, read64(hb_bar + 0x20) | PPC_BIT(3));

	/* Enable PSIHB interrupts */
	write64(hb_bar + 0x58, read64(hb_bar + 0x58) | PPC_BIT(0));

	/* Route interrupts to first thread of active core */
	int offset = (core < 16) ? 0x48 : 0x68;
	void *xive_ic_bar = (void *)XIVE_IC_BAR;
	write64(xive_ic_bar + 0x400 + offset, PPC_BIT(4 * (core % 16)));
	eieio();

	/* Configure LSI mode for HB CEC interrupts */
	void *ivpe_bar = (void *)IVPE_BAR;
	write8(ivpe_bar + 0x38, 0x81);
	eieio();

	/* Route LSI to master processor */
	write64(hb_bar + 0x68, 0x0006030203102001);
	write64(hb_bar + 0x58, 0);

	/* Enable LSI interrupts */
	tmp = read64(xive_ic_bar + 0x3000 + 0xC00);

	/* Unmask PSU interrupts */
	tmp = read64(esb_bar + 0xD*0x1000 + 0xC00);
	eieio();
	tmp = read64(esb_bar + 0xD*0x1000 + 0x800);
	assert(tmp == 0);
}
