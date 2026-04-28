/* SPDX-License-Identifier: GPL-2.0-only */

#include <types.h>
#include <cpu/x86/smm.h>
#include <cbmem.h>
#include <console/console.h>
#include <drivers/efi/efivars.h>
#include <security/tpm/tss.h>
#include <smmstore.h>

#include "tpm_ppi.h"

static const EFI_GUID tcg2_pp_guid = {
	0xaeb9c5c1, 0x94f1, 0x4d02, { 0xbf, 0xd9, 0x46, 0x2, 0xdb, 0x2d, 0x3c, 0x54 }
};

#define PHYSICAL_PRESENCE_FLAGS_VARIABLE  "PhysicalPresenceFlags"

#define PHYSICAL_PRESENCE_VARIABLE  "PhysicalPresence"

typedef struct {
	uint8_t  pp_request;
	uint8_t  last_pp_request;
	uint32_t pp_response;
} EFI_PHYSICAL_PRESENCE;

static const EFI_GUID tcg_pp_guid = {
	0xf6499b1, 0xe9ad, 0x493d, { 0xb9, 0xc2, 0x2f, 0x90, 0x81, 0x5c, 0x6c, 0xbc }
};

#define TPM2_PP_VARIABLE  "Tcg2PhysicalPresence"

typedef struct {
	uint8_t  pp_request;
	uint32_t pp_request_parameter;
	uint8_t  last_pp_request;
	uint32_t pp_response;
} EFI_TCG2_PHYSICAL_PRESENCE;

#define TPM2_PP_FLAGS_VARIABLE  "Tcg2PhysicalPresenceFlags"

#define REQUESTED_ACTIVE_PCR_BANKS_VARIABLE_NAME  "RequestedActivePcrBanks"

static void handle_tpm1_ppi_request(struct cb_tpm_ppi_payload_handshake *ppib)
{
	EFI_PHYSICAL_PRESENCE pp_data;
	uint8_t pp_flags;
	struct region_device rdev;
	enum cb_err ret;
	uint32_t size;

	switch (ppib->ppip) {
	case TPM_PPI_RETURN_REQUEST_RESPONSE_TO_OS:
		if (smmstore_lookup_region(&rdev)) {
			ppib->pprp = 0;
			ppib->lppr = 0;
			ppib->fret = PPI5_RET_GENERAL_FAILURE;
			return;
		}

		size = sizeof(pp_data);
		ret = efi_fv_get_option(&rdev, &tcg_pp_guid,
					PHYSICAL_PRESENCE_VARIABLE,
					&pp_data, &size);
		if (ret != CB_SUCCESS) {
			printk(BIOS_ERR, "PPI: failed to read PP variable\n");
			ppib->pprp = 0;
			ppib->lppr = 0;
			ppib->fret = PPI5_RET_GENERAL_FAILURE;
			return;
		}

		ppib->pprp = pp_data.pp_response;
		ppib->lppr = pp_data.last_pp_request;
		ppib->fret = PPI5_RET_SUCCESS;
		break;
	case TPM_PPI_SUBMIT_REQUEST_TO_BIOS:
	case TPM_PPI_SUBMIT_REQUEST_TO_BIOS_2:
		if (ppib->pprq == 13 || ppib->pprq >= VENDOR_SPECIFIC_OFFSET) {
			ppib->fret = PPI7_RET_NOT_IMPLEMENTED;
			return;
		}
		if (smmstore_lookup_region(&rdev)) {
			ppib->fret = PPI7_RET_GENERAL_FAILURE;
			return;
		}

		size = sizeof(pp_data);
		ret = efi_fv_get_option(&rdev, &tcg_pp_guid,
					PHYSICAL_PRESENCE_VARIABLE,
					&pp_data, &size);
		if (ret != CB_SUCCESS) {
			printk(BIOS_ERR, "PPI: failed to read PP variable\n");
			ppib->fret = PPI7_RET_GENERAL_FAILURE;
			return;
		}

		if (pp_data.pp_request != ppib->pprq) {
			pp_data.pp_request = ppib->pprq;
			size = sizeof(pp_data);
			ret = efi_fv_set_option(&rdev, &tcg_pp_guid,
						PHYSICAL_PRESENCE_VARIABLE,
						&pp_data, size);
			if (ret != CB_SUCCESS) {
				printk(BIOS_WARNING, "PPI: failed to save request: %x\n",
				       ppib->pprq);
				ppib->fret = PPI7_RET_GENERAL_FAILURE;
				return;
			}
		}
		ppib->fret = PPI7_RET_SUCCESS;
		break;
	case TPM_PPI_GET_USER_CONFIRMATION_STATUS_FOR_REQUEST:
		if (ppib->pprq == 13) {
			ppib->fret = PPI8_RET_NOT_IMPLEMENTED;
			return;
		}

		if (smmstore_lookup_region(&rdev)) {
			ppib->fret = PPI8_RET_BLOCKED_FOR_OS_BY_FW;
			return;
		}

		size = sizeof(pp_flags);
		ret = efi_fv_get_option(&rdev, &tcg_pp_guid,
					PHYSICAL_PRESENCE_FLAGS_VARIABLE,
					&pp_flags, &size);
		if (ret != CB_SUCCESS) {
			printk(BIOS_ERR, "PPI: failed to read PP flags variable\n");
			ppib->fret = PPI8_RET_BLOCKED_FOR_OS_BY_FW;
			return;
		}

		ppib->fret = PPI8_RET_ALLOWED_WITH_PP;

		switch(ppib->ucrq) {
		case TPM_ENABLE:
		case TPM_DISABLE:
		case TPM_ACTIVATE:
		case TPM_DEACTIVATE:
		case TPM_ENABLE_ACTIVATE:
		case TPM_DEACTIVATE_DISABLE:
		case TPM_SETOWNERINSTALL_TRUE:
		case TPM_SETOWNERINSTALL_FALSE:
		case TPM_ENABLE_ACTIVATE_SETOWNERINSTALL_TRUE:
		case TPM_SETOWNERINSTALL_FALSE_DEACTIVATE_DISABLE:
			if (pp_flags & TCG_FLAG_NO_PPI_PROVISION)
				ppib->fret = PPI8_RET_ALLOWED;
			break;
		case TPM_CLEAR:
		case TPM_ENABLE_ACTIVE_CLEAR:
			if (pp_flags & TCG_FLAG_NO_PPI_CLEAR)
				ppib->fret = PPI8_RET_ALLOWED;
			break;
		case TPM_DEFERRED_PP_UNOWNERED_FIELD_UPGRADE:
			if (pp_flags & TCG_FLAG_NO_PPI_MAINTENANCE)
				ppib->fret = PPI8_RET_ALLOWED;

			break;
		case TPM_ENABLE_ACTIVE_CLEAR_ENABLE_ACTIVE:
		case TPM_CLEAR_ENABLE_ACTIVATE:
			if ((pp_flags & TCG_FLAG_NO_PPI_CLEAR) &&
			    (pp_flags & TCG_FLAG_NO_PPI_PROVISION))
				ppib->fret = PPI8_RET_ALLOWED;

			break;
		case TPM_NOOP:
		case TPM_SET_NOPPIPROVISION_FALSE:
		case TPM_SET_NOPPICLEAR_FALSE:
		case TPM_SET_NOPPIMAINTAINANCE_FALSE:
			ppib->fret = PPI8_RET_ALLOWED;
			break;
		default:
			break;
		}
		break;
	default:
		/* Should not land here */
		printk(BIOS_WARNING, "PPI: unexpected request: %x\n", ppib->ppip);
		ppib->fret = PPI8_RET_BLOCKED_FOR_OS_BY_FW;
		break;
	}
}

static void handle_tpm2_ppi_request(struct cb_tpm_ppi_payload_handshake *ppib)
{
	EFI_TCG2_PHYSICAL_PRESENCE pp_data;
	uint32_t pp_flags;
	struct region_device rdev;
	enum cb_err ret;
	uint32_t size;

	switch (ppib->ppip) {
	case TPM_PPI_RETURN_REQUEST_RESPONSE_TO_OS:
		if (smmstore_lookup_region(&rdev)) {
			ppib->pprp = 0;
			ppib->lppr = 0;
			ppib->fret = PPI5_RET_GENERAL_FAILURE;
			return;
		}

		size = sizeof(pp_data);
		ret = efi_fv_get_option(&rdev, &tcg2_pp_guid,
					TPM2_PP_VARIABLE,
					&pp_data, &size);
		if (ret != CB_SUCCESS) {
			printk(BIOS_ERR, "PPI: failed to read PP variable\n");
			ppib->pprp = 0;
			ppib->lppr = 0;
			ppib->fret = PPI5_RET_GENERAL_FAILURE;
			return;
		}

		ppib->pprp = pp_data.pp_response;
		ppib->lppr = pp_data.last_pp_request;
		ppib->fret = PPI5_RET_SUCCESS;
		return;
	case TPM_PPI_SUBMIT_REQUEST_TO_BIOS:
	case TPM_PPI_SUBMIT_REQUEST_TO_BIOS_2:
		if ((ppib->pprq >= VENDOR_SPECIFIC_OFFSET) ||
		    (ppib->pprq > TPM2_DISABLE_ENDORSMENT_ENABLE_STORAGE_HISTORY &&
		     ppib->pprq < TPM2_ENABLE_BLOCK_SID)) {
			ppib->fret = PPI7_RET_NOT_IMPLEMENTED;
			goto tpm2_pp_exit;
		}

		if (smmstore_lookup_region(&rdev)) {
			ppib->pprq = 0;
			ppib->pprm = 0;
			ppib->fret = PPI7_RET_GENERAL_FAILURE;
			return;
		}

		size = sizeof(pp_data);
		ret = efi_fv_get_option(&rdev, &tcg2_pp_guid,
					TPM2_PP_VARIABLE,
					&pp_data, &size);
		if (ret != CB_SUCCESS) {
			printk(BIOS_ERR, "PPI: failed to read PP variable\n");
			ppib->fret = PPI7_RET_GENERAL_FAILURE;
			goto tpm2_pp_exit;
		}

		if ((pp_data.pp_request != ppib->pprq) ||
		    (pp_data.pp_request_parameter != ppib->pprm)) {
			pp_data.pp_request = ppib->pprq;
			pp_data.pp_request_parameter = ppib->pprm;
			size = sizeof(pp_data);
			ret = efi_fv_set_option(&rdev, &tcg2_pp_guid,
						TPM2_PP_VARIABLE,
						&pp_data, size);
			if (ret != CB_SUCCESS) {
				printk(BIOS_ERR, "PPI: failed to save request: %x\n",
				       ppib->pprq);
				ppib->fret = PPI7_RET_GENERAL_FAILURE;
				goto tpm2_pp_exit;
			}
		}

		ppib->fret = PPI7_RET_SUCCESS;
		break;
	case TPM_PPI_GET_USER_CONFIRMATION_STATUS_FOR_REQUEST:
		if (smmstore_lookup_region(&rdev)) {
			ppib->fret = PPI8_RET_BLOCKED_FOR_OS_BY_FW;
			return;
		}

		size = sizeof(pp_flags);
		ret = efi_fv_get_option(&rdev, &tcg2_pp_guid,
					TPM2_PP_FLAGS_VARIABLE,
					&pp_flags, &size);
		if (ret != CB_SUCCESS) {
			printk(BIOS_ERR, "PPI: failed to read PP flags variable\n");
			ppib->fret = PPI8_RET_BLOCKED_FOR_OS_BY_FW;
			return;
		}

		ppib->fret = PPI8_RET_ALLOWED_WITH_PP;

		switch (ppib->ucrq) {
		case TPM2_CLEAR:
		case TPM2_CLEAR_ENABLE_ACTIVE:
		case TPM2_ENABLE_CLEAR:
		case TPM2_ENABLE_CLEAR2:
			if (!(pp_flags & TCG2_FLAG_PP_REQUIRED_FOR_CLEAR))
				ppib->fret = PPI8_RET_ALLOWED;
			break;
		case TPM2_NOOP:
		case TPM2_LOG_ALL_DIGEST:
		case TPM2_SET_PP_REQUIRED_FOR_CLEAR_TRUE:
		case TPM2_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_TRUE:
		case TPM2_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_TRUE:
			ppib->fret = PPI8_RET_ALLOWED;
			break;

		case TPM2_SET_PP_REQUIRED_FOR_CLEAR_FALSE:
		case TPM2_SET_PP_REQUIRED_FOR_ENABLE_BLOCK_SID_FALSE:
		case TPM2_SET_PP_REQUIRED_FOR_DISABLE_BLOCK_SID_FALSE:
			break;

		case TPM2_SET_PCR_BANKS:
			if (!(pp_flags & TCG2_FLAG_PP_REQUIRED_FOR_CHANGE_PCRS))
				ppib->fret = PPI8_RET_ALLOWED;
			break;
		case TPM2_CHANGE_EPS:
			if (!(pp_flags & TCG2_FLAG_PP_REQUIRED_FOR_CHANGE_EPS))
				ppib->fret = PPI8_RET_ALLOWED;
			break;
		case TPM2_ENABLE_BLOCK_SID:
			if (!(pp_flags & TCG2_FLAG_PP_REQUIRED_FOR_ENABLE_BLOCK_SID))
				ppib->fret = PPI8_RET_ALLOWED;
			break;
		case TPM2_DISABLE_BLOCK_SID:
			if (!(pp_flags & TCG2_FLAG_PP_REQUIRED_FOR_DISABLE_BLOCK_SID))
				ppib->fret = PPI8_RET_ALLOWED;
			break;
		default:
			/*
			 * TCG2 PP1.3 spec defined operations that are
			 * reserved or un-implemented.
			 */
			if (ppib->ucrq < VENDOR_SPECIFIC_OFFSET) {
				ppib->fret = PPI8_RET_NOT_IMPLEMENTED;
				return;
			}
			break;
		}
		break;
	default:
		/* Should not land here */
		ppib->fret = PPI8_RET_BLOCKED_FOR_OS_BY_FW;
		printk(BIOS_WARNING, "PPI: unexpected request: %x\n", ppib->ppip);
		break;
	}

	return;

tpm2_pp_exit:
	if (ppib->fret != PPI7_RET_SUCCESS) {
		memset(&pp_data, 0, sizeof(pp_data));
		if (!smmstore_lookup_region(&rdev)) {
			size = sizeof(pp_data);
			efi_fv_set_option(&rdev, &tcg2_pp_guid,
						TPM2_PP_VARIABLE,
						&pp_data, size);
		}
		ppib->pprq = 0;
		ppib->pprm = 0;
	}
}

void tpm_ppi_process_request_smm(uint32_t ppi_address)
{
	static struct cb_tpm_ppi_payload_handshake *ppib = NULL;
	static bool initted = false;
	static enum tpm_family family = TPM_UNKNOWN;

	if (!initted) {
		/* First call will be made by coreboot to init the address. */
		ppib = (struct cb_tpm_ppi_payload_handshake *)(uintptr_t)ppi_address;
		family = tlcl_get_family();
		initted = true;
		return;
	}

	if (!ppib) {
		printk(BIOS_ERR, "PPI: Failed to find TPM PPI buffer\n");
		return;
	}

	printk(BIOS_DEBUG, "PPI: ACPI function: %u\n", ppib->ppip);
	printk(BIOS_DEBUG, "PPI: Pending OS request: 0x%x (0x%x)\n", ppib->pprq, ppib->pprm);
	printk(BIOS_DEBUG, "PPI: OS response: CMD 0x%x = 0x%x\n", ppib->lppr, ppib->pprp);
	printk(BIOS_DEBUG, "PPI: User confirmation request: %x\n", ppib->ucrq);

	switch (family) {
	case TPM_2:
		handle_tpm2_ppi_request(ppib);
		break;
	case TPM_1:
		handle_tpm1_ppi_request(ppib);
		break;
	case TPM_UNKNOWN:
	default:
		switch(ppib->ppip) {
		case TPM_PPI_SUBMIT_REQUEST_TO_BIOS:
			ppib->fret = PPI2_RET_GENERAL_FAILURE;
			break;
		case TPM_PPI_GET_PENDING_REQUEST_BY_OS:
			ppib->fret = PPI3_RET_GENERAL_FAILURE;
			break;
		case TPM_PPI_RETURN_REQUEST_RESPONSE_TO_OS:
			ppib->fret = PPI5_RET_GENERAL_FAILURE;
			break;
		case TPM_PPI_SUBMIT_REQUEST_TO_BIOS_2:
			ppib->fret = PPI7_RET_GENERAL_FAILURE;
			break;
		case TPM_PPI_GET_USER_CONFIRMATION_STATUS_FOR_REQUEST:
			ppib->fret = PPI8_RET_BLOCKED_FOR_OS_BY_FW;
			break;
		default:
			ppib->fret = 0;
			break;
		}
		ppib->pprq = 0;
		ppib->pprm = 0;
		printk(BIOS_WARNING, "PPI: %s: aborting, because no TPM detected\n", __func__);
		return;
	}

	printk(BIOS_DEBUG, "PPI response: ACPI function: %u\n", ppib->ppip);
	printk(BIOS_DEBUG, "PPI response: Pending OS request: 0x%x (0x%x)\n", ppib->pprq, ppib->pprm);
	printk(BIOS_DEBUG, "PPI response: OS response: CMD 0x%x = 0x%x\n", ppib->lppr, ppib->pprp);
	printk(BIOS_DEBUG, "PPI response: Return status: 0x%x\n", ppib->fret);
}
