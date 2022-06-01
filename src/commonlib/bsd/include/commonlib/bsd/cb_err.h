/* SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-or-later */

#ifndef _COMMONLIB_BSD_CB_ERR_H_
#define _COMMONLIB_BSD_CB_ERR_H_

#include <stdint.h>

/**
 * coreboot error codes
 *
 * Common error definitions that can be used for any function. All error values
 * should be negative -- when useful, positive values can also be used to denote
 * success. Allocate a new group or errors every 100 values.
 */
enum cb_err {
	CB_SUCCESS = 0,		/**< Call completed successfully */
	CB_ERR = -1,		/**< Generic error code */
	CB_ERR_ARG = -2,	/**< Invalid argument */

	/* NVRAM/CMOS errors */
	CB_CMOS_OTABLE_DISABLED = -100,		/**< Option table disabled */
	CB_CMOS_LAYOUT_NOT_FOUND = -101,	/**< Layout file not found */
	CB_CMOS_OPTION_NOT_FOUND = -102,	/**< Option string not found */
	CB_CMOS_ACCESS_ERROR = -103,		/**< CMOS access error */
	CB_CMOS_CHECKSUM_INVALID = -104,	/**< CMOS checksum is invalid */

	/* Keyboard test failures */
	CB_KBD_CONTROLLER_FAILURE = -200,
	CB_KBD_INTERFACE_FAILURE = -201,

	/* I2C controller failures */
	CB_I2C_NO_DEVICE	= -300,	/**< Device is not responding */
	CB_I2C_BUSY		= -301,	/**< Device tells it's busy */
	CB_I2C_PROTOCOL_ERROR	= -302,	/**< Data lost or spurious slave
					     device response, try again? */
	CB_I2C_TIMEOUT		= -303, /**< Transmission timed out */

	/* CBFS errors */
	CB_CBFS_IO		= -400, /**< Underlying I/O error */
	CB_CBFS_NOT_FOUND	= -401, /**< File not found in directory */
	CB_CBFS_HASH_MISMATCH	= -402, /**< Master hash validation failed */
	CB_CBFS_CACHE_FULL	= -403, /**< Metadata cache overflowed */

	/* EFI errors */
	CB_EFI_FVH_INVALID		= -500, /**< UEFI FVH is corrupted */
	CB_EFI_CHECKSUM_INVALID		= -501, /**< UEFI FVH checksum is invalid */
	CB_EFI_VS_NOT_FORMATTED_INVALID	= -502, /**< UEFI variable store not formatted */
	CB_EFI_VS_CORRUPTED_INVALID	= -503, /**< UEFI variable store is corrupted */
	CB_EFI_ACCESS_ERROR		= -504, /**< UEFI variable store access error */
	CB_EFI_STORE_FULL		= -505, /**< UEFI variable store is full */
	CB_EFI_OPTION_NOT_FOUND		= -506, /**< UEFI variable not found */
	CB_EFI_BUFFER_TOO_SMALL		= -507, /**< UEFI Buffer is too small. */
};

/* Don't typedef the enum directly, so the size is unambiguous for serialization. */
typedef int32_t cb_err_t;

#endif	/* _COMMONLIB_BSD_CB_ERR_H_ */
