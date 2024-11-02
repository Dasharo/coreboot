/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef TIS_H_
#define TIS_H_

#include <security/tpm/tss_errors.h>
#include <types.h>

enum tis_access {
	TPM_ACCESS_VALID = (1 << 7),
	TPM_ACCESS_ACTIVE_LOCALITY = (1 << 5),
	TPM_ACCESS_REQUEST_PENDING = (1 << 2),
	TPM_ACCESS_REQUEST_USE = (1 << 1),
	TPM_ACCESS_ESTABLISHMENT = (1 << 0),
};

enum tis_status {
	TPM_STS_FAMILY_SHIFT = 26,
	TPM_STS_FAMILY_MASK = (0x3 << TPM_STS_FAMILY_SHIFT),
	TPM_STS_FAMILY_TPM_2_0 = (1 << TPM_STS_FAMILY_SHIFT),
	TPM_STS_FAMILY_TPM_1_2 = (0 << TPM_STS_FAMILY_SHIFT),
	TPM_STS_RESET_ESTABLISHMENT = (1 << 25),
	TPM_STS_COMMAND_CANCEL = (1 << 24),
	TPM_STS_BURST_COUNT_SHIFT = 8,
	TPM_STS_BURST_COUNT_MASK = (0xFFFF << TPM_STS_BURST_COUNT_SHIFT),
	TPM_STS_VALID = (1 << 7),
	TPM_STS_COMMAND_READY = (1 << 6),
	TPM_STS_GO = (1 << 5),
	TPM_STS_DATA_AVAIL = (1 << 4),
	TPM_STS_DATA_EXPECT = (1 << 3),
	TPM_STS_SELF_TEST_DONE = (1 << 2),
	TPM_STS_RESPONSE_RETRY = (1 << 1),
};

enum tpm_family {
	TPM_UNKNOWN = 0,
	TPM_1 = 1,
	TPM_2 = 2,
};

/*
 * tis_sendrecv()
 *
 * Send the requested data to the TPM and then try to get its response
 *
 * @sendbuf - buffer of the data to send
 * @send_size size of the data to send
 * @recvbuf - memory to save the response to
 * @recv_len - pointer to the size of the response buffer
 *
 * Returns TSS Return Code from TCG TPM Structures.  See tss_errors.h
 */
typedef tpm_result_t (*tis_sendrecv_fn)(const u8 *sendbuf, size_t send_size, u8 *recvbuf,
					size_t *recv_len);

/*
 * Probe for the TPM device and set it up for use within locality 0.
 *
 * @family - pointer which is set to TPM family of the device
 *
 * Returns pointer to send-receive function on success or NULL on failure.
 *
 * Do not call this explicitly, it's meant to be used exclusively by TSS
 * implementation (tlcl_lib_init() function to be specific).
 */
typedef tis_sendrecv_fn (*tis_probe_fn)(enum tpm_family *family);

/*
 * tis_vendor_write()
 *
 * Vendor-specific function to send the requested data to the TPM.
 *
 * @addr - address of the register to write to
 * @sendbuf - buffer of the data to send
 * @send_size - size of the data to send
 *
 * Returns CB_SUCCESS 0 on success, CB_ERR on failure.
 */
enum cb_err tis_vendor_write(unsigned int addr, const void *sendbuf, size_t send_size);

/*
 * tis_vendor_read()
 *
 * Vendor-specific function to read the requested data from the TPM.
 *
 * @addr - address of the register to read from
 * @recvbuf - buffer of the data to read
 * @recv_size - size of the output buffer
 *
 * Returns CB_SUCCESS on success or -1 on failure.
 */
enum cb_err tis_vendor_read(unsigned int addr, void *recvbuf, size_t recv_size);

static inline bool tpm_first_access_this_boot(void)
{
	return ENV_SEPARATE_VERSTAGE || ENV_BOOTBLOCK || !CONFIG(VBOOT);
}

#if (CONFIG(VBOOT_STARTS_IN_BOOTBLOCK) && ENV_BOOTBLOCK) ||            \
	(CONFIG(VBOOT_SEPARATE_VERSTAGE) && ENV_SEPARATE_VERSTAGE) ||  \
	(CONFIG(VBOOT_STARTS_IN_ROMSTAGE) && ENV_RAMINIT) ||           \
	(CONFIG(TPM_MEASURED_BOOT_INIT_BOOTBLOCK) && ENV_BOOTBLOCK) || \
	(CONFIG(TPM_MEASURED_BOOT) && (ENV_RAMINIT || ENV_POSTCAR || ENV_RAMSTAGE))
#define __tis_driver __attribute__((used, section(".rodata.tis_driver")))
#else
#define __tis_driver __always_unused
#endif

/** start of compile time generated tis driver array */
extern tis_probe_fn _tis_drivers[];
/** end of compile time generated tis driver array */
extern tis_probe_fn _etis_drivers[];

#endif /* TIS_H_ */
