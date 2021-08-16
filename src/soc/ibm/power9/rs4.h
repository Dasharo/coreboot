/* SPDX-License-Identifier: Apache-2.0 */

#ifndef __SOC_IBM_POWER9_RS4_H
#define __SOC_IBM_POWER9_RS4_H

#include <stdint.h>

#define RS4_MAGIC (uint16_t)0x5253 // "RS"

/// Normal return code
#define SCAN_COMPRESSION_OK (uint8_t)0

/// The (de)compression algorithm could not allocate enough memory for the
/// (de)compression.
#define SCAN_COMPRESSION_NO_MEMORY (uint8_t)1

/// Magic number mismatch on scan decompression
#define SCAN_DECOMPRESSION_MAGIC_ERROR (uint8_t)2

/// Decompression size error
///
/// Decompression produced a string of a size different than indicated in the
/// header, indicating either a bug or data corruption.  Note that the entire
/// application should be considered corrupted if this error occurs since it
/// may not be discovered until after the decompression buffer is
/// overrun. This error may also be returned by rs4_redundant() in the event
/// of inconsistencies in the compressed string.
#define SCAN_DECOMPRESSION_SIZE_ERROR (uint8_t)3

/// A buffer would overflow
///
/// Either the caller-supplied memory buffer to rs4_decompress() was too
/// small to contain the decompressed string, or a caller-supplied buffer to
/// rs4_compress() was not large enough to hold the worst-case compressed
/// string.
#define SCAN_COMPRESSION_BUFFER_OVERFLOW (uint8_t)4

/// Inconsistent input data
///
/// 1 in data is masked by 0 in care mask
#define SCAN_COMPRESSION_INPUT_ERROR 5

/// Invalid transition in state machine
#define SCAN_COMPRESSION_STATE_ERROR 6

/// wrong compression version
#define SCAN_COMPRESSION_VERSION_ERROR 7

/* Header of an RS4 compressed ring */
struct ring_hdr {
	uint16_t magic;		// Always "RS"
	uint8_t  version;
	uint8_t  type;
	uint16_t size;		// Header + data size in BE
	uint16_t ring_id;
	uint32_t scan_addr;
	uint8_t	 data[];
} __attribute__((packed));

int rs4_compress(struct ring_hdr *io_rs4, const uint32_t i_size,
		 const uint8_t *i_data_str, const uint8_t *i_care_str,
		 const uint32_t i_length, const uint32_t i_scanAddr,
		 const uint16_t ring_id);

int rs4_decompress(uint8_t *o_data_str, uint8_t *o_care_str, uint32_t i_size,
		   uint32_t *o_length, const struct ring_hdr *i_rs4);

int rs4_redundant(const struct ring_hdr *i_data, int *o_redundant);

#endif // __SOC_IBM_POWER9_RS4_H
