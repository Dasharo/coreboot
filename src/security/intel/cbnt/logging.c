/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <cpu/x86/msr.h>
#include <cpu/intel/msr.h>
#include <device/mmio.h>
#include <stdint.h>
#include <security/intel/txt/txt.h>

#include "cbnt.h"

#define LOG(...) printk(BIOS_INFO, "CBnT: " __VA_ARGS__)

union sacm_info {
	struct {
		uint64_t nem_enabled : 1;
		uint64_t tpm_type : 2;
		uint64_t tpm_success : 1;
		uint64_t facb : 1;
		uint64_t measured_boot : 1;
		uint64_t verified_boot : 1;
		uint64_t revoked : 1;
		uint64_t : 24;
		uint64_t btg_cap : 1;
		uint64_t : 1;
		uint64_t txt_server_cap : 1;
		uint64_t no_rst_secrets_prot : 1;
		uint64_t : 28;
	};
	msr_t msr;
	uint64_t raw;
};

_Static_assert(sizeof(union sacm_info) == sizeof(uint64_t), "Wrong size of sacm_info");

static const char *const tpm_type[] = {
	"No TPM",
	"TPM 1.2",
	"TPM 2.0",
	"PTT",
};

union cbnt_bootstatus {
	struct {
		uint64_t : 30;
		uint64_t txt_startup_success : 1;
		uint64_t btg_startup_success : 1;
		uint64_t block_boot_en : 1;
		uint64_t pfr_startup_success : 1;
		uint64_t : 13;
		uint64_t mem_pwr_down : 1;
		uint64_t btg_failed : 1;
		uint64_t : 10;
		uint64_t bios_trusted : 1;
		uint64_t txt_dis_pol : 1;
		uint64_t btg_startup_err : 1;
		uint64_t txt_err : 1;
		uint64_t sacm_success : 1;
	};
	uint64_t raw;
};
_Static_assert(sizeof(union cbnt_bootstatus) == sizeof(uint64_t),
	       "Wrong size of cbnt_bootstatus");

union cbnt_errorcode {
	struct {
		uint32_t type : 15;
		uint32_t : 15;
		uint32_t external : 1;
		uint32_t valid : 1;
	} microcode;
	struct {
		uint32_t ac_type : 4;
		uint32_t class : 6;
		uint32_t major : 5;
		uint32_t minor_invalid : 1;
		uint32_t minor : 9;
		uint32_t : 5;
		uint32_t external : 1;
		uint32_t valid : 1;
	} sinit;
	uint32_t raw;
};

_Static_assert(sizeof(union cbnt_errorcode) == sizeof(uint32_t),
	       "Wrong size of cbnt_errorcode");

union cbnt_biosacm_errorcode {
	struct {
		uint32_t ac_type : 4;
		uint32_t class : 6;
		uint32_t major : 5;
		uint32_t minor_invalid : 1;
		uint32_t minor : 12;
		uint32_t : 2;
		uint32_t external : 1;
		uint32_t valid : 1;
	} txt;
	struct {
		uint32_t ac_type : 4;
		uint32_t class : 6;
		uint32_t error : 5;
		uint32_t acm_started : 1;
		uint32_t km_id : 4;
		uint32_t bp : 5;
		uint32_t : 6;
		uint32_t valid : 1;
	} btg;
	uint32_t raw;
};
_Static_assert(sizeof(union cbnt_biosacm_errorcode) == sizeof(uint32_t),
	       "Wrong size of cbnt_biosacm_errorcode");

union cbnt_biosacm_policy {
	struct {
		uint64_t km_id : 4;
		uint64_t bp : 9;
		uint64_t tpm_type : 2;
		uint64_t tpm_success : 1;
		uint64_t : 1;
		uint64_t pfr : 1;
		uint64_t bckup_act : 2;
		uint64_t txt_profile : 5;
		uint64_t scrub_policy : 2;
		uint64_t : 2;
		uint64_t dma_protection : 1;
		uint64_t : 2;
		uint64_t scrtm_status : 3;
		uint64_t cpu_co_signing : 1;
		uint64_t tpm_startup_locality : 1;
		uint64_t : 27;
	} status;
	uint64_t raw;
};
_Static_assert(sizeof(union cbnt_biosacm_policy) == sizeof(uint64_t),
	       "Wrong size of cbnt_biosacm_policy");

static const char *decode_err_type(uint8_t type)
{
	switch (type) {
	case 0:
		return "BIOS ACM Error";
	case 1:
		return "SINIT ACM Error";
	case 3:
		return "Boot Guard Error";
	default:
		return "Reserved";
	}
}

static const char* backup_actions[] = {
	"Memory Power Down",
	"BtG Unbreakable Shutdown",
	"PFR Recovery",
	"Reserved",
};

static const char* scrub_policies[] = {
	"Default - trust valid BIOS to scrub memory",
	"BIOS action to scrub memory.",
	"ACM action to scrub memory",
	"Reserved",
};

static const char *decode_scrtm_status(uint8_t status)
{
	switch (status) {
	case 0:
		return "None";
	case 1:
		return "Boot Guard established";
	case 2:
		return "TXT established";
	case 4:
		return "PFR established";
	default:
		return "Reserved";
	}
}

void intel_cbnt_log_registers(void)
{
	const union sacm_info acm_info = { .msr = rdmsr(MSR_BOOT_GUARD_SACM_INFO) };
	LOG("SACM INFO MSR (0x13A) raw: 0x%016llx\n", acm_info.raw);
	LOG("  NEM status:              %u\n", acm_info.nem_enabled);
	LOG("  TPM type:                %s\n", tpm_type[acm_info.tpm_type]);
	LOG("  TPM success:             %u\n", acm_info.tpm_success);
	LOG("  FACB:                    %u\n", acm_info.facb);
	LOG("  measured boot:           %u\n", acm_info.measured_boot);
	LOG("  verified boot:           %u\n", acm_info.verified_boot);
	LOG("  revoked:                 %u\n", acm_info.revoked);
	LOG("  BtG capable:             %u\n", acm_info.btg_cap);
	LOG("  Server TXT capable:      %u\n", acm_info.txt_server_cap);
	LOG("  RST Secrets Protection:  %s\n", acm_info.no_rst_secrets_prot? "no" : "yes");

	const union cbnt_bootstatus btsts = {
		.raw = read64p(CBNT_BOOTSTATUS),
	};
	LOG("BOOTSTATUS (0xA0) raw: 0x%016llx\n", btsts.raw);
	LOG("  TXT startup success:     %u\n", btsts.bios_trusted);
	LOG("  BtG startup success:     %u\n", btsts.bios_trusted);
	LOG("  Block boot:              %s\n", btsts.block_boot_en ? "enabled" : "disabled");
	LOG("  PFR startup success:     %u\n", btsts.bios_trusted);
	LOG("  Memory power down:       %sexecuted\n", btsts.mem_pwr_down ? "" : "not ");
	LOG("  BtG failed:              %s\n", btsts.btg_failed ? "yes" : "no");
	LOG("  Bios trusted:            %u\n", btsts.bios_trusted);
	LOG("  TXT disabled by policy:  %u\n", btsts.txt_dis_pol);
	LOG("  BtG startup error:       %u\n", btsts.btg_startup_err);
	LOG("  TXT ucode or ACM error:  %u\n", btsts.txt_err);
	LOG("  S-ACM success:           %s\n", btsts.sacm_success ? "yes" : "no");

	const union cbnt_errorcode err = {
		.raw = read32p(CBNT_ERRORCODE),
	};
	LOG("ERRORCODE (0x30) raw: 0x%08x\n", err.raw);
	/* It looks like the hardware does not set the txt error bit properly */
	const bool txt_err_valid = btsts.txt_err || true;
	if (txt_err_valid && !btsts.txt_dis_pol) {
		if (err.microcode.valid && !err.microcode.external) {
			LOG("ERRORCODE is ucode error\n");
			LOG("  type:                    %s\n",
			      intel_txt_processor_error_type(err.microcode.type));
		} else if (err.sinit.valid && err.sinit.external) {
			LOG("ERRORCODE is SINIT error\n");
			const char *type = decode_err_type(err.sinit.ac_type);
			LOG("  AC Module Type:          %s\n", type);
			LOG("  class:                   0x%x\n", err.sinit.class);
			LOG("  major:                   0x%x\n", err.sinit.major);
			if (!err.sinit.minor_invalid)
				LOG("  minor:                   0x%x\n", err.sinit.minor);
		}
	} else if (txt_err_valid && btsts.txt_dis_pol) {
		LOG("TXT disabled in Policy\n");
	}

	const union cbnt_biosacm_errorcode biosacm_err = {
		.raw = read32p(CBNT_BIOSACM_ERRORCODE),
	};
	LOG("BIOSACM_ERRORCODE (0x328) raw: 0x%08x\n", biosacm_err.raw);
	if (txt_err_valid && biosacm_err.txt.valid) {
		LOG("BIOSACM_ERRORCODE: TXT ucode or ACM error\n");
		const char *type = decode_err_type(biosacm_err.txt.ac_type);
		LOG("  AC Module Type:          %s\n", type);
		LOG("  class:                   0x%x\n", biosacm_err.txt.class);
		LOG("  major:                   0x%x\n", biosacm_err.txt.major);
		if (!biosacm_err.txt.minor_invalid)
			LOG("  minor:                   0x%x\n", biosacm_err.txt.minor);
		LOG("  External:                0x%x\n", biosacm_err.txt.external);
	}

	if (btsts.btg_startup_err && biosacm_err.btg.valid) {
		LOG("BIOSACM_ERRORCODE: Bootguard error\n");
		const char *type = decode_err_type(biosacm_err.btg.ac_type);
		LOG("  AC Module Type:          %s\n", type);
		LOG("  class:                   0x%x\n", biosacm_err.btg.class);
		LOG("  error:                   0x%x\n", biosacm_err.btg.error);
		LOG("  ACM started:             %u\n", biosacm_err.btg.acm_started);
		LOG("  KMID:                    0x%x\n", biosacm_err.btg.km_id);
		LOG("  BootPolicies:            0x%x\n", biosacm_err.btg.bp);
	}

	const union cbnt_biosacm_policy biosacm_sts = {
		.raw = read64p(CBNT_BIOSACM_POLICY_STS),
	};

	LOG("CBNT_BIOSACM_POLICY_STS (0x378) raw: 0x%016llx\n", biosacm_sts.raw);
	LOG("  KMID:                    0x%x\n", biosacm_sts.status.km_id);
	LOG("  Boot policies :          0x%x\n", biosacm_sts.status.bp);
	LOG("  TPM type:                %s\n", tpm_type[biosacm_sts.status.tpm_type]);
	LOG("  TPM success:             %s\n", biosacm_sts.status.tpm_success ? "yes" : "no");
	LOG("  PFR supported:           %s\n", biosacm_sts.status.pfr ? "yes" : "no");
	LOG("  Backup action:           %s\n", backup_actions[biosacm_sts.status.bckup_act]);

	if (biosacm_sts.status.txt_profile == 1)
		LOG("  TXT Profile:             "
		    "B - BIOS is trusted to create alias-free memory topology\n");
	else if (biosacm_sts.status.txt_profile == 2)
		LOG("  TXT Profile:             "
		    "D - BIOS runs ACHECK to verify alias-free topology\n");
	else
		LOG("  TXT Profile:             Unknown\n");

	LOG("  Memory scrubbing policy: %s\n", scrub_policies[biosacm_sts.status.scrub_policy]);
	LOG("  IBB DMA protection:      %s\n", biosacm_sts.status.dma_protection ?
					       "enabled" : "disabled");
	LOG("  S-CRTM status:           %s\n",
	    decode_scrtm_status(biosacm_sts.status.scrtm_status));
	LOG("  CPU Co-signing:          %s\n", biosacm_sts.status.cpu_co_signing ?
					       "enabled" : "disabled");
	LOG("  TPM Startup locality:    %d\n",
	    biosacm_sts.status.tpm_startup_locality ? 3 : 0);


}
