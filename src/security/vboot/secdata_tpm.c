/* SPDX-License-Identifier: BSD-3-Clause */

/*
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#include <security/vboot/antirollback.h>
#include <security/vboot/tpm_common.h>
#include <security/tpm/tspi.h>
#include <security/tpm/tss.h>
#include <security/tpm/tss/tcg-1.2/tss_structures.h>
#include <security/tpm/tss/tcg-2.0/tss_structures.h>
#include <vb2_api.h>
#include <console/console.h>

#define VBDEBUG(format, args...) \
	printk(BIOS_INFO, "%s():%d: " format, __func__, __LINE__, ## args)

#define RETURN_ON_FAILURE(tpm_cmd) do {				\
		tpm_result_t rc_;					\
		if ((rc_ = (tpm_cmd)) != TPM_SUCCESS) {		\
			VBDEBUG("Antirollback: %08x returned by " #tpm_cmd \
				 "\n", (tpm_result_t)rc_);			\
			return rc_;					\
		}							\
	} while (0)

static tpm_result_t safe_write(uint32_t index, const void *data, uint32_t length);

tpm_result_t antirollback_read_space_kernel(struct vb2_context *ctx)
{
	if (tlcl_get_family() == TPM_1) {
		/*
		 * Before reading the kernel space, verify its permissions. If
		 * the kernel space has the wrong permission, we give up. This
		 * will need to be fixed by the recovery kernel. We will have
		 * to worry about this because at any time (even with PP turned
		 * off) the TPM owner can remove and redefine a PP-protected
		 * space (but not write to it).
		 */
		uint32_t perms;

		RETURN_ON_FAILURE(tlcl1_get_permissions(KERNEL_NV_INDEX, &perms));
		if (perms != TPM_NV_PER_PPWRITE) {
			printk(BIOS_ERR,
			       "TPM: invalid secdata_kernel permissions\n");
			return TPM_CB_CORRUPTED_STATE;
		}
	}

	uint8_t size = VB2_SECDATA_KERNEL_SIZE;
	tpm_result_t rc;

	/* Start with the version 1.0 size used by all modern Cr50/Ti50 boards. */
	rc = tlcl_read(KERNEL_NV_INDEX, ctx->secdata_kernel, size);
	if (rc == TPM_CB_RANGE) {
		/* Fallback to version 0.2(minimum) size and re-read. */
		VBDEBUG("Antirollback: NV read out of range, trying min size\n");
		size = VB2_SECDATA_KERNEL_MIN_SIZE;
		rc = tlcl_read(KERNEL_NV_INDEX, ctx->secdata_kernel, size);
	}
	RETURN_ON_FAILURE(rc);

	if (vb2api_secdata_kernel_check(ctx, &size) == VB2_ERROR_SECDATA_KERNEL_INCOMPLETE)
		/* Re-read. vboot will run the check and handle errors. */
		RETURN_ON_FAILURE(tlcl_read(KERNEL_NV_INDEX, ctx->secdata_kernel, size));

	return TPM_SUCCESS;
}

#if CONFIG(TPM2)

static tpm_result_t read_space_mrc_hash(uint32_t index, uint8_t *data)
{
	RETURN_ON_FAILURE(tlcl_read(index, data,
				    HASH_NV_SIZE));
	return TPM_SUCCESS;
}

/*
 * This is used to initialize the TPM space for recovery hash after defining
 * it. Since there is no data available to calculate hash at the point where TPM
 * space is defined, initialize it to all 0s.
 */
static const uint8_t mrc_hash_data[HASH_NV_SIZE] = { };

/*
 * Different sets of NVRAM space attributes apply to the "ro" spaces,
 * i.e. those which should not be possible to delete or modify once
 * the RO exits, and the rest of the NVRAM spaces.
 */
static const TPMA_NV ro_space_attributes = {
	.TPMA_NV_PPWRITE = 1,
	.TPMA_NV_AUTHREAD = 1,
	.TPMA_NV_PPREAD = 1,
	.TPMA_NV_PLATFORMCREATE = 1,
	.TPMA_NV_WRITE_STCLEAR = 1,
	.TPMA_NV_POLICY_DELETE = 1,
};

static const TPMA_NV rw_space_attributes = {
	.TPMA_NV_PPWRITE = 1,
	.TPMA_NV_AUTHREAD = 1,
	.TPMA_NV_PPREAD = 1,
	.TPMA_NV_PLATFORMCREATE = 1,
	.TPMA_NV_WRITE_STCLEAR = 1,
};

static const TPMA_NV rw_auth_space_attributes = {
	.TPMA_NV_AUTHWRITE = 1,
	.TPMA_NV_AUTHREAD = 1,
	.TPMA_NV_NO_DA = 1,
	.TPMA_NV_PPREAD = 1,
	.TPMA_NV_PPWRITE = 1,
	.TPMA_NV_PLATFORMCREATE = 1,
	.TPMA_NV_WRITE_STCLEAR = 1,
	.TPMA_NV_POLICY_DELETE = 1,
};

static const TPMA_NV fwmp_attr = {
	.TPMA_NV_PLATFORMCREATE = 1,
	.TPMA_NV_OWNERWRITE = 1,
	.TPMA_NV_AUTHREAD = 1,
	.TPMA_NV_PPREAD = 1,
	.TPMA_NV_PPWRITE = 1,
};

/* Attributes for spaces that enable zero-touch enrollment (ZTE) */
static const TPMA_NV zte_attr = {
	.TPMA_NV_PLATFORMCREATE = 1,
	.TPMA_NV_WRITEDEFINE = 1,
	.TPMA_NV_AUTHWRITE = 1,
	.TPMA_NV_AUTHREAD = 1,
	.TPMA_NV_PPWRITE = 1,
	.TPMA_NV_PPREAD = 1,
	.TPMA_NV_NO_DA = 1,
	.TPMA_NV_POLICY_DELETE = 1,
};

static const TPMA_NV zte_rma_bytes_attr = {
	.TPMA_NV_PLATFORMCREATE = 1,
	.TPMA_NV_BITS = 1,
	.TPMA_NV_AUTHWRITE = 1,
	.TPMA_NV_AUTHREAD = 1,
	.TPMA_NV_PPWRITE = 1,
	.TPMA_NV_PPREAD = 1,
	.TPMA_NV_NO_DA = 1,
	.TPMA_NV_POLICY_DELETE = 1,
};

static const TPMA_NV rw_orderly_counter_attributes = {
	.TPMA_NV_COUNTER = 1,
	.TPMA_NV_ORDERLY = 1,
	.TPMA_NV_AUTHREAD = 1,
	.TPMA_NV_AUTHWRITE = 1,
	.TPMA_NV_PLATFORMCREATE = 1,
	.TPMA_NV_WRITE_STCLEAR = 1,
	.TPMA_NV_PPREAD = 1,
	.TPMA_NV_PPWRITE = 1,
	.TPMA_NV_NO_DA = 1,
};

/*
 * This policy digest was obtained using TPM2_PolicyOR on 3 digests
 * corresponding to a sequence of
 *   -) TPM2_PolicyCommandCode(TPM_CC_NV_UndefineSpaceSpecial),
 *   -) TPM2_PolicyPCR(PCR0, <extended_value>).
 * where <extended value> is
 *   1) all zeros = initial, unextended state:
 *      - Value to extend to initial PCR0:
 *        <none>
 *      - Resulting PCR0:
 *        0000000000000000000000000000000000000000000000000000000000000000
 *      - Policy digest for PolicyCommandCode + PolicyPCR:
 *        4B44FC4192DB5AD7167E0135708FD374890A06BFB56317DF01F24F2226542A3F
 *   2) result of extending (SHA1(0x00|0x01|0x00) | 00s to SHA256 size)
 *      - Value to extend to initial PCR0:
 *        62571891215b4efc1ceab744ce59dd0b66ea6f73000000000000000000000000
 *      - Resulting PCR0:
 *        9F9EA866D3F34FE3A3112AE9CB1FBABC6FFE8CD261D42493BC6842A9E4F93B3D
 *      - Policy digest for PolicyCommandCode + PolicyPCR:
 *        CB5C8014E27A5F7586AAE42DB4F9776A977BCBC952CA61E33609DA2B2C329418
 *   3) result of extending (SHA1(0x01|0x01|0x00) | 00s to SHA256 size)
 *      - Value to extend to initial PCR0:
 *        47ec8d98366433dc002e7721c9e37d5067547937000000000000000000000000
 *      - Resulting PCR0:
 *        2A7580E5DA289546F4D2E0509CC6DE155EA131818954D36D49E027FD42B8C8F8
 *      - Policy digest for PolicyCommandCode + PolicyPCR:
 *        E6EF4F0296AC3EF0F53906480985B1BE8058E0E517E5F74A5B8A415EFE339D87
 * Values #2 and #3 correspond to two forms of recovery mode as extended by
 * vb2api_get_pcr_digest().
 * As a result, the digest allows deleting the space with UndefineSpaceSpecial
 * at early RO stages (before extending PCR0) or from recovery mode.
 */
static const uint8_t pcr0_allowed_policy[] = {
	0x44, 0x44, 0x79, 0x00, 0xCB, 0xB8, 0x3F, 0x5B, 0x15, 0x76, 0x56,
	0x50, 0xEF, 0x96, 0x98, 0x0A, 0x2B, 0x96, 0x6E, 0xA9, 0x09, 0x04,
	0x4A, 0x01, 0xB8, 0x5F, 0xA5, 0x4A, 0x96, 0xFC, 0x59, 0x84};

static const uint8_t unsatisfiable_policy[VB2_SHA256_DIGEST_SIZE] =
	"hmwhat if RBR beat merc in 2021";

static uint32_t define_space(const char *name, uint32_t index, uint32_t length,
			     const TPMA_NV nv_attributes,
			     const uint8_t *nv_policy, size_t nv_policy_size)
{
	tpm_result_t rc;

	rc = tlcl2_define_space(index, length, nv_attributes, nv_policy, nv_policy_size);
	if (rc == TPM_CB_NV_DEFINED) {
		/*
		 * Continue with writing: it may be defined, but not written
		 * to. In that case a subsequent tlcl_read() would still return
		 * TPM_BADINDEX on TPM 2.0. The cases when some non-firmware
		 * space is defined while the firmware space is not there
		 * should be rare (interrupted initialization), so no big harm
		 * in writing once again even if it was written already.
		 */
		VBDEBUG("%s: %s space already exists\n", __func__, name);
		rc = TPM_SUCCESS;
	}

	return rc;
}

static tpm_result_t setup_space(const char *name, uint32_t index, const void *data,
				uint32_t length, const TPMA_NV nv_attributes,
				const uint8_t *nv_policy, size_t nv_policy_size)
{
	tpm_result_t rc;

	rc = define_space(name, index, length, nv_attributes, nv_policy,
			  nv_policy_size);
	if (rc != TPM_SUCCESS)
		return rc;

	return safe_write(index, data, length);
}

static tpm_result_t setup_firmware_space(struct vb2_context *ctx)
{
	uint32_t firmware_space_size = vb2api_secdata_firmware_create(ctx);

	return setup_space("firmware", FIRMWARE_NV_INDEX,
			   ctx->secdata_firmware, firmware_space_size,
			   ro_space_attributes, pcr0_allowed_policy,
			   sizeof(pcr0_allowed_policy));
}

static tpm_result_t setup_fwmp_space(struct vb2_context *ctx)
{
	uint32_t fwmp_space_size = vb2api_secdata_fwmp_create(ctx);

	return setup_space("FWMP", FWMP_NV_INDEX, ctx->secdata_fwmp, fwmp_space_size,
			   fwmp_attr, NULL, 0);
}

static tpm_result_t setup_kernel_space(struct vb2_context *ctx)
{
	uint32_t kernel_space_size = vb2api_secdata_kernel_create(ctx);

	return setup_space("kernel", KERNEL_NV_INDEX, ctx->secdata_kernel,
			    kernel_space_size, rw_space_attributes, NULL, 0);
}

static tpm_result_t set_mrc_hash_space(uint32_t index, const uint8_t *data)
{
	if (index == MRC_REC_HASH_NV_INDEX) {
		return setup_space("RO MRC Hash", index, data, HASH_NV_SIZE,
				   ro_space_attributes, pcr0_allowed_policy,
				 sizeof(pcr0_allowed_policy));
	} else {
		return setup_space("RW MRC Hash", index, data, HASH_NV_SIZE,
				   rw_space_attributes, NULL, 0);
	}
}

/**
 * Set up the Zero-Touch Enrollment(ZTE) related spaces.
 *
 * These spaces are not used by firmware, but we do need to initialize them.
 */
static tpm_result_t setup_zte_spaces(void)
{
	tpm_result_t rc;
	uint64_t rma_bytes_counter_default = 0;
	uint8_t rma_sn_bits_default[16];
	uint8_t board_id_default[12];

	/* Initialize defaults:  Board ID and RMA+SN Bits must be initialized
	 to all 0xFFs. */
	memset(rma_sn_bits_default, 0xFF, ARRAY_SIZE(rma_sn_bits_default));
	memset(board_id_default, 0xFF, ARRAY_SIZE(board_id_default));

	/* Set up RMA + SN Bits */
	rc = setup_space("RMA + SN Bits", ZTE_RMA_SN_BITS_INDEX,
			 rma_sn_bits_default, sizeof(rma_sn_bits_default),
			 zte_attr,
			 unsatisfiable_policy, sizeof(unsatisfiable_policy));
	if (rc != TPM_SUCCESS) {
		VBDEBUG("%s: Failed to set up RMA + SN Bits space with error %#x\n", __func__, rc);
		return rc;
	}

	rc = setup_space("Board ID", ZTE_BOARD_ID_NV_INDEX,
			 board_id_default, sizeof(board_id_default),
			 zte_attr,
			 unsatisfiable_policy, sizeof(unsatisfiable_policy));
	if (rc != TPM_SUCCESS) {
		VBDEBUG("%s: Failed to set up Board ID space with error %#x\n", __func__, rc);
		return rc;
	}

	/* Set up RMA Bytes counter */
	rc = define_space("RMA Bytes Counter", ZTE_RMA_BYTES_COUNTER_INDEX,
			  sizeof(rma_bytes_counter_default),
			  zte_rma_bytes_attr,
			  unsatisfiable_policy, sizeof(unsatisfiable_policy));
	if (rc != TPM_SUCCESS) {
		VBDEBUG("%s: Failed to define RMA Bytes space with error %#x\n", __func__, rc);
		return rc;
	}

	/*
	 * Since the RMA counter has the BITS attribute, we need to call
	 * TPM2_NV_SetBits() in order to initialize it.
	 */
	rc = tlcl2_set_bits(ZTE_RMA_BYTES_COUNTER_INDEX, rma_bytes_counter_default);
	if (rc != TPM_SUCCESS) {
		VBDEBUG("%s: Failed to init RMA Bytes counter space wit error %#x\n",
			__func__, rc);
		return rc;
	}

	return rc;
}

/*
 * Set up enterprise rollback space.
 *
 * This space is not used by firmware but needs to survive owner clear. Thus, it
 * needs to be created here.
 */
static tpm_result_t enterprise_rollback_create_space(void)
{
	uint8_t rollback_space_default[32] = {0};

	return setup_space("Enterprise Rollback Space",
			   ENT_ROLLBACK_SPACE_INDEX, rollback_space_default,
			   sizeof(rollback_space_default), rw_auth_space_attributes,
			   unsatisfiable_policy, sizeof(unsatisfiable_policy));
}

static tpm_result_t setup_widevine_counter_spaces(void)
{
	uint32_t index;
	tpm_result_t rc;

	for (index = 0; index < NUM_WIDEVINE_COUNTERS; index++) {
		rc = define_space(WIDEVINE_COUNTER_NAME,
				WIDEVINE_COUNTER_NV_INDEX(index),
				WIDEVINE_COUNTER_SIZE,
				rw_orderly_counter_attributes,
				NULL,
				0);
		if (rc != TPM_SUCCESS)
			return rc;
	}
	return rc;
}

static tpm_result_t _factory_initialize_tpm2(struct vb2_context *ctx)
{
	RETURN_ON_FAILURE(tlcl_force_clear());

	/*
	 * Of all NVRAM spaces defined by this function the firmware space
	 * must be defined last, because its existence is considered an
	 * indication that TPM factory initialization was successfully
	 * completed.
	 */
	RETURN_ON_FAILURE(setup_kernel_space(ctx));

	/*
	 * Define and set rec hash space, if available.  No need to
	 * create the RW hash space because we will definitely boot
	 * once in normal mode before shipping, meaning that the space
	 * will get created with correct permissions while still in
	 * our hands.
	 */
	if (CONFIG(VBOOT_HAS_REC_HASH_SPACE))
		RETURN_ON_FAILURE(set_mrc_hash_space(MRC_REC_HASH_NV_INDEX, mrc_hash_data));

	/* Define and write firmware management parameters space. */
	RETURN_ON_FAILURE(setup_fwmp_space(ctx));

	/*
	 * Define and write zero-touch enrollment (ZTE) spaces.  For ChromeOS devices with
	 * Google TPM, these are set up elsewhere via TPM vendor commands.
	 */
	if (CONFIG(CHROMEOS) && !(CONFIG(TPM_GOOGLE)))
		RETURN_ON_FAILURE(setup_zte_spaces());

	/*
	 * On TPM 2.0, create a space that survives TPM clear. This allows to
	 * securely lock data during enterprise rollback by binding to this
	 * space's value.
	 */
	if (CONFIG(CHROMEOS))
		RETURN_ON_FAILURE(enterprise_rollback_create_space());

	/* Define widevine counter space. No need to increment/write to the secure counters
	   and are expected to be incremented during the first use. */
	if (CONFIG(VBOOT_DEFINE_WIDEVINE_COUNTERS))
		RETURN_ON_FAILURE(setup_widevine_counter_spaces());

	RETURN_ON_FAILURE(setup_firmware_space(ctx));

	return TPM_SUCCESS;
}

tpm_result_t antirollback_read_space_mrc_hash(uint32_t index, uint8_t *data, uint32_t size)
{
	if (size != HASH_NV_SIZE) {
		VBDEBUG("TPM: Incorrect buffer size for hash idx %#x. "
			"(Expected=%#x Actual=%#x).\n", index, HASH_NV_SIZE,
			size);
		return TPM_CB_READ_FAILURE;
	}
	return read_space_mrc_hash(index, data);
}

tpm_result_t antirollback_write_space_mrc_hash(uint32_t index, const uint8_t *data, uint32_t size)
{
	uint8_t spc_data[HASH_NV_SIZE];
	tpm_result_t rc;

	if (size != HASH_NV_SIZE) {
		VBDEBUG("TPM: Incorrect buffer size for hash idx %#x. "
			"(Expected=%#x Actual=%#x).\n", index, HASH_NV_SIZE,
			size);
		return TPM_CB_WRITE_FAILURE;
	}

	rc = read_space_mrc_hash(index, spc_data);
	if (rc == TPM_BADINDEX) {
		/*
		 * If space is not defined already for hash, define
		 * new space.
		 */
		VBDEBUG("TPM: Initializing hash space.\n");
		return set_mrc_hash_space(index, data);
	}

	if (rc != TPM_SUCCESS)
		return rc;

	return safe_write(index, data, size);
}

tpm_result_t antirollback_lock_space_mrc_hash(uint32_t index)
{
	return tlcl2_lock_nv_write(index);
}

static tpm_result_t read_space_vbios_hash(uint8_t *data)
{
	RETURN_ON_FAILURE(tlcl_read(VBIOS_CACHE_NV_INDEX, data, HASH_NV_SIZE));
	return TPM_SUCCESS;
}

tpm_result_t antirollback_read_space_vbios_hash(uint8_t *data, uint32_t size)
{
	if (size != HASH_NV_SIZE) {
		VBDEBUG("TPM: Incorrect buffer size for hash idx %#x. "
			"(Expected=%#x Actual=%#x).\n", VBIOS_CACHE_NV_INDEX, HASH_NV_SIZE,
			size);
		return TPM_CB_READ_FAILURE;
	}
	return read_space_vbios_hash(data);
}

tpm_result_t antirollback_write_space_vbios_hash(const uint8_t *data, uint32_t size)
{
	uint8_t spc_data[HASH_NV_SIZE];
	tpm_result_t rc;

	if (size != HASH_NV_SIZE) {
		VBDEBUG("TPM: Incorrect buffer size for hash idx %#x. "
			"(Expected=%#x Actual=%#x).\n", VBIOS_CACHE_NV_INDEX, HASH_NV_SIZE,
			size);
		return TPM_CB_WRITE_FAILURE;
	}

	rc = read_space_vbios_hash(spc_data);
	if (rc == TPM_BADINDEX) {
		/*
		 * If space is not defined already for hash, define
		 * new space.
		 */
		VBDEBUG("TPM: Initializing hash space.\n");
		return setup_space("VBIOS Cache Hash", VBIOS_CACHE_NV_INDEX, data, HASH_NV_SIZE,
				   rw_space_attributes, NULL, 0);
	}

	if (rc != TPM_SUCCESS)
		return rc;

	return safe_write(VBIOS_CACHE_NV_INDEX, data, size);
}

#endif /* CONFIG(TPM2) */

#if CONFIG(TPM1)

/**
 * Similarly to safe_write(), this ensures we don't fail a DefineSpace because
 * we hit the TPM write limit. This is even less likely to happen than with
 * writes because we only define spaces once at initialization, but we'd
 * rather be paranoid about this.
 */
static tpm_result_t safe_define_space(uint32_t index, uint32_t perm, uint32_t size)
{
	tpm_result_t rc = tlcl1_define_space(index, perm, size);
	if (rc == TPM_MAXNVWRITES) {
		RETURN_ON_FAILURE(tpm_clear_and_reenable());
		return tlcl1_define_space(index, perm, size);
	} else {
		return rc;
	}
}

static tpm_result_t _factory_initialize_tpm1(struct vb2_context *ctx)
{
	TPM_PERMANENT_FLAGS pflags;
	tpm_result_t rc;

	vb2api_secdata_firmware_create(ctx);
	vb2api_secdata_kernel_create_v0(ctx);

	rc = tlcl1_get_permanent_flags(&pflags);
	if (rc != TPM_SUCCESS)
		return rc;

	/*
	 * TPM may come from the factory without physical presence finalized.
	 * Fix if necessary.
	 */
	VBDEBUG("TPM: physicalPresenceLifetimeLock=%d\n",
		 pflags.physicalPresenceLifetimeLock);
	if (!pflags.physicalPresenceLifetimeLock) {
		VBDEBUG("TPM: Finalizing physical presence\n");
		RETURN_ON_FAILURE(tlcl_finalize_physical_presence());
	}

	/*
	 * The TPM will not enforce the NV authorization restrictions until the
	 * execution of a TPM_NV_DefineSpace with the handle of
	 * TPM_NV_INDEX_LOCK.  Here we create that space if it doesn't already
	 * exist. */
	VBDEBUG("TPM: nvLocked=%d\n", pflags.nvLocked);
	if (!pflags.nvLocked) {
		VBDEBUG("TPM: Enabling NV locking\n");
		RETURN_ON_FAILURE(tlcl1_set_nv_locked());
	}

	/* Clear TPM owner, in case the TPM is already owned for some reason. */
	VBDEBUG("TPM: Clearing owner\n");
	RETURN_ON_FAILURE(tpm_clear_and_reenable());

	/* Define and write secdata_kernel space. */
	RETURN_ON_FAILURE(safe_define_space(KERNEL_NV_INDEX,
					    TPM_NV_PER_PPWRITE,
					    VB2_SECDATA_KERNEL_SIZE_V02));
	RETURN_ON_FAILURE(safe_write(KERNEL_NV_INDEX,
				     ctx->secdata_kernel,
				     VB2_SECDATA_KERNEL_SIZE_V02));

	/* Define and write secdata_firmware space. */
	RETURN_ON_FAILURE(safe_define_space(FIRMWARE_NV_INDEX,
					    TPM_NV_PER_GLOBALLOCK |
					    TPM_NV_PER_PPWRITE,
					    VB2_SECDATA_FIRMWARE_SIZE));
	RETURN_ON_FAILURE(safe_write(FIRMWARE_NV_INDEX,
				     ctx->secdata_firmware,
					VB2_SECDATA_FIRMWARE_SIZE));

	return TPM_SUCCESS;
}

#endif /* CONFIG(TPM1) */

static tpm_result_t safe_write(uint32_t index, const void *data, uint32_t length)
{
	tpm_result_t rc = tlcl_write(index, data, length);
	if (tlcl_get_family() == TPM_1 && rc == TPM_MAXNVWRITES) {
		/**
		 * Clear the TPM on write error due to hitting the 64-write
		 * limit.  This can only happen when the TPM is unowned, so it
		 * is OK to clear it (and we really have no choice).  This is
		 * not expected to happen frequently, but it could happen.
		 */
		RETURN_ON_FAILURE(tpm_clear_and_reenable());
		rc = tlcl_write(index, data, length);
	}
	return rc;
}

static uint32_t _factory_initialize_tpm(struct vb2_context *ctx)
{
#if CONFIG(TPM1)
	if (tlcl_get_family() == TPM_1)
		return _factory_initialize_tpm1(ctx);
#endif
#if CONFIG(TPM2)
	if (tlcl_get_family() == TPM_2)
		return _factory_initialize_tpm2(ctx);
#endif
	return TPM_CB_CORRUPTED_STATE;
}

uint32_t antirollback_lock_space_firmware(void)
{
	if (tlcl_get_family() == TPM_1)
		return tlcl1_set_global_lock();
	if (tlcl_get_family() == TPM_2)
		return tlcl2_lock_nv_write(FIRMWARE_NV_INDEX);
	return TPM_CB_CORRUPTED_STATE;
}

/**
 * Perform one-time initializations.
 *
 * Create the NVRAM spaces, and set their initial values as needed.  Sets the
 * nvLocked bit and ensures the physical presence command is enabled and
 * locked.
 */
static tpm_result_t factory_initialize_tpm(struct vb2_context *ctx)
{
	tpm_result_t rc;

	VBDEBUG("TPM: factory initialization\n");

	/*
	 * Do a full test.  This only happens the first time the device is
	 * turned on in the factory, so performance is not an issue.  This is
	 * almost certainly not necessary, but it gives us more confidence
	 * about some code paths below that are difficult to
	 * test---specifically the ones that set lifetime flags, and are only
	 * executed once per physical TPM.
	 */
	rc = tlcl_self_test_full();
	if (rc != TPM_SUCCESS)
		return rc;

	rc = _factory_initialize_tpm(ctx);
	if (rc != TPM_SUCCESS)
		return rc;

	/* _factory_initialize_tpm() writes initial secdata values to TPM
	   immediately, so let vboot know that it's up to date now. */
	ctx->flags &= ~(VB2_CONTEXT_SECDATA_FIRMWARE_CHANGED |
			VB2_CONTEXT_SECDATA_KERNEL_CHANGED);

	VBDEBUG("TPM: factory initialization successful\n");

	return TPM_SUCCESS;
}

tpm_result_t antirollback_read_space_firmware(struct vb2_context *ctx)
{
	tpm_result_t rc;

	rc = tlcl_read(FIRMWARE_NV_INDEX, ctx->secdata_firmware, VB2_SECDATA_FIRMWARE_SIZE);
	if (rc == TPM_BADINDEX) {
		/* This seems the first time we've run. Initialize the TPM. */
		VBDEBUG("TPM: Not initialized yet\n");
		RETURN_ON_FAILURE(factory_initialize_tpm(ctx));
	} else if (rc != TPM_SUCCESS) {
		printk(BIOS_ERR, "TPM: Failed to read firmware space: %#x\n", rc);
		return TPM_CB_CORRUPTED_STATE;
	}

	return rc;
}

tpm_result_t antirollback_write_space_firmware(struct vb2_context *ctx)
{
	if (CONFIG(TPM_GOOGLE_IMMEDIATELY_COMMIT_FW_SECDATA))
		tlcl_cr50_enable_nvcommits();
	return safe_write(FIRMWARE_NV_INDEX, ctx->secdata_firmware,
			  VB2_SECDATA_FIRMWARE_SIZE);
}

tpm_result_t antirollback_write_space_kernel(struct vb2_context *ctx)
{
	/* Learn the expected size. */
	uint8_t size = VB2_SECDATA_KERNEL_MIN_SIZE;
	vb2api_secdata_kernel_check(ctx, &size);

	/*
	 * Ensure that the TPM actually commits our changes to NVMEN in case
	 * there is a power loss or other unexpected event. The AP does not
	 * write to the TPM during normal boot flow; it only writes during
	 * recovery, software sync, or other special boot flows. When the AP
	 * wants to write, it is imporant to actually commit changes.
	 */
	if (CONFIG(TPM_GOOGLE_IMMEDIATELY_COMMIT_FW_SECDATA))
		tlcl_cr50_enable_nvcommits();

	return safe_write(KERNEL_NV_INDEX, ctx->secdata_kernel, size);
}

vb2_error_t vb2ex_tpm_clear_owner(struct vb2_context *ctx)
{
	printk(BIOS_INFO, "Clearing TPM owner\n");
	return tpm_clear_and_reenable() == TPM_SUCCESS ? VB2_SUCCESS : VB2_ERROR_EX_TPM_CLEAR_OWNER;
}
