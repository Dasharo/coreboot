/* SPDX-License-Identifier: GPL-2.0-only */

#include <cpu/amd/msr.h>
#include <cpu/amd/mtrr.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/cache.h>
#include <cpu/x86/post_code.h>

.code32
.global chipset_teardown_car

chipset_teardown_car:
	pop %esp

	post_code(0x30)

	/* Disable cache. */
	movl	%cr0, %eax
	orl	$CR0_CacheDisable, %eax
	movl	%eax, %cr0

	post_code(0x31)

	/* Disable MTRR. */
	movl	$MTRR_DEF_TYPE_MSR, %ecx
	rdmsr
	andl	$(~MTRR_DEF_TYPE_EN), %eax
	wrmsr

	xorl	%eax, %eax
	xorl	%edx, %edx
	movl	$MTRR_FIX_64K_00000, %ecx
	wrmsr

	/* disable fixed mtrr from now on,
	 * it will be enabled by ramstage again
	 */
	movl	$SYSCFG_MSR, %ecx
	rdmsr
	andl	$(~SYSCFG_MSR_MtrrFixDramEn), %eax
	wrmsr

	/* INVDWBINVD = 1 */
	movl	$HWCR_MSR, %ecx
	rdmsr
	orl	$(1 << 4), %eax
	wrmsr

	/* Get CPU family */
	movl	$0x80000001, %eax
	cpuid
	movl	%eax, %ecx
	andl	$0xf00, %ecx
	shr	$8, %ecx
	andl	$0xf00000, %eax
	shr	$16, %eax
	orl	%ecx, %eax

	cmp	$0x6f, %eax
	jb	fam10h_teardown_done
	/* Family 15h or later */

	/* DisSS = 0 */
	movl	$LS_CFG_MSR, %ecx
	rdmsr
	andl	$(~(1 << 28)), %eax
	wrmsr

	/* DisSpecTlbRld = 0 */
	movl	$IC_CFG_MSR, %ecx
	rdmsr
	andl	$(~(1 << 9)), %eax
	wrmsr

	/* Erratum 714: SpecNbReqDis = 0 */
	movl	$BU_CFG2_MSR, %ecx
	rdmsr
	andl	$(~(1 << 8)), %eax
	wrmsr

	movl	$DC_CFG_MSR, %ecx
	rdmsr
	andl	$(~(1 << 4)), %eax	/* DisSpecTlbRld = 0 */
	andl	$(~(1 << 13)), %eax	/* DisHwPf = 0 */
	wrmsr

	post_code(0x32)

fam10h_teardown_done:
	/* Return to caller. */
	jmp	*%esp
