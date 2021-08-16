/* SPDX-License-Identifier: Apache-2.0 */

/* Copied out of HostBoot mostly as is (with some renaming and removal of unused
 * functions).  Upstream URL:
 * https://git.raptorcs.com/git/talos-hostboot/tree/src/import/chips/p9/utils/imageProcs/p9_scan_compression.C */

/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/import/chips/p9/utils/imageProcs/p9_scan_compression.C $  */
/*                                                                        */
/* OpenPOWER HostBoot Project                                             */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2016,2017                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */
// Note: This file was originally named p8_scan_compression.c; See CVS archive
// for revision history of p8_scan_compression.c.

#include "rs4.h"

/// RS4 Compression Format (version 2)
/// ==================================
///
/// Scan strings are compressed using a simple run-length encoding called
/// RS4. The string to be decompressed and scanned is the difference between
/// the current state of the ring and the desired final state of the ring.
///
/// Both the data to be compressed and the final compressed data are treated
/// as strings of 4-bit nibbles. In the scan data structure the compressed
/// strings are padded with 0x0 nibbles to the next even multiple of 4.
/// The compressed string consists of control nibbles and data
/// nibbles. The string format includes a special control/data sequence that
/// marks the end of the string and the final bits of scan data.
///
/// Special control/data sequences have been been added for RS4v2 to
/// store pairs of care mask nibble and data nibble. This enhancement
/// is needed to allow the scanning of significant zeros.
/// The RS4v1 format assumed that all zeros have no meaning other than
/// the positioning of 1 bits.
///
/// Runs of 0x0 nibbles as determined by the care mask (rotates) are encoded
/// using a simple variable-length integer encoding known as a "stop code".
/// This code treats each nibble in a variable-length integer encoding as an
/// octal digit (the low-order 3 bits) plus a stop bit (the high-order bit).
/// The examples below illustrate the encoding.
///
///     1xxx            - Rotate 0bxxx       nibbles (0 - 7)
///     0xxx 1yyy       - Rotate 0bxxxyyy    nibbles (8 - 63)
///     0xxx 0yyy 1zzz  - Rotate 0bxxxyyyzzz nibbles (64 - 511)
///     etc.
///
/// A 0-length rotate (code 0b1000) is needed to resynchronize the state
/// machine in the event of long scans (see below), or a string that begins
/// with a non-0x0 nibble.
///
/// Runs of non-0x0 nibbles (scans) are inserted verbatim into the compressed
/// string after a control nibble indicating the number of nibbles of
/// uncompressed data.  If a run is longer than 14 nibbles, the compression
/// algorithm must insert a 0-length rotate and a new scan-length control
/// before continuing with the non-0 data nibbles.
///
///     xxxx - Scan 0bxxxx nibbles which follow, 0bxxxx != 0 and 0bxxxx != 15
///
/// The special case of a 0b0000 code where a scan count is expected marks the
/// end of the string.  The end of string marker is always followed by a
/// nibble that contains the terminal bit count in the range 0-3.  If the
/// length of the original binary string was not an even multiple of 4, then a
/// final nibble contains the final scan data left justified.
///
///     0000 00nn [ttt0] - Terminate 0bnn bits, data 0bttt0 if 0bnn != 0
///
/// The special case of a 0b1111 code where a scan count is expected announces
/// a pair of care mask nibble and data nibble containing significant zeros.
/// Only a single pair can be stored this way, and longer sequences of such
/// pairs require resynchronization using zero rotates and special scan count
/// 0b1111 to be inserted.
///
/// Termination with care mask and data is accomplished by a special
/// terminal data count:
///
///     0000 10nn [ccc0] [ttt0] - Terminate
///                               0bnn bits care mask and 0bnn bits data,
///                               care mask 0bccc0 and data 0bttt0 if 0bnn != 0
///
/// BNF Grammar
/// ===========
///
/// Following is a BNF grammar for the strings accepted by the RS4
/// decompression and scan algorithm. At a high level, the state machine
/// recognizes a series of 1 or more sequences of a rotate (R) followed by a
/// scan (S) or end-of-string marker (E), followed by the terminal count (T)
/// and optional terminal data (D).
///
///     (R S)* (R E) T D?
///
/// \code
///
/// <rs4_string>             ::= <rotate> <terminate> |
///                              <rotate> <scan> <rs4_string>
///
/// <rotate>                 ::= <octal_stop> |
///                              <octal_go> <rotate>
///
/// <octal_go>               ::= '0x0' | ... | '0x7'
///
/// <octal_stop>             ::= '0x8' | ... | '0xf'
///
/// <scan>                   ::= <scan_count(N)> <data(N)> |
///                              <scan_count(15)> <care_data>
///
/// <scan_count(N)>          ::= * 0bnnnn, for N = 0bnnnn, N != 0  &  N != 15 *
///
/// <scan_count(15)>         ::= '0xf'
///
/// <data(N)>                ::= * N nibbles of uncompressed data, 0 < N < 15 *
///
/// <terminate>              ::=
///              '0x0' <terminal_count(0)> |
///              '0x0' <terminal_count(T, T > 0)> <terminal_data(T)> |
///              '0x0' <terminal_care_count(T, T > 0)> <terminal_care_data(T)>
///
/// <terminal_count(T)>      ::= * 0b00nn, for T = 0bnn *
///
/// <terminal_care_count(T)> ::= * 0b10nn, for T = 0bnn  &  T != 0 *
///
/// <terminal_data(1)>       ::= '0x0' | '0x8'
///
/// <terminal_data(2)>       ::= '0x0' | '0x4' | '0x8' | '0xc'
///
/// <terminal_data(3)>       ::= '0x0' | '0x2' | '0x4' | ... | '0xe'
///
/// <terminal_care_data(1)>  ::= * 0b1000 0b0000 *
///
/// <terminal_care_data(2)>  ::= * 0bij00 0bwx00, for
///                                i >= w  &  j >= x  &
///                                ij > wx *
///
/// <terminal_care_data(3)>  ::= * 0bijk0 0bwxy0, for
///                                i >= w  &  j >= x  &  k >= y  &
///                                ijk > wxy *
///
/// <care_data>              ::= * 0bijkl 0bwxyz, for
///                                i >= w  &  j >= x  &  k >= y  &  l >= z  &
///                                ijkl > wxyz *
///
/// \endcode

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>

#define MY_DBG(...) while(false)

// Diagnostic aids for debugging
#ifdef DEBUG_P9_SCAN_COMPRESSION

#include <stdio.h>


#define BUG(rc)                                     \
    ({                                              \
        fprintf(stderr,"%s:%d : Trapped rc = %d\n", \
                __FILE__, __LINE__, (rc));          \
        (rc);                                       \
    })

#define BUGX(rc, ...)                           \
    ({                                          \
        BUG(rc);                                \
        fprintf(stderr, ##__VA_ARGS__);         \
        (rc);                                   \
    })

#else // DEBUG_P9_SCAN_COMPRESSION

#define BUG(rc) (rc)
#define BUGX(rc, ...) (rc)

#endif  // DEBUG_P9_SCAN_COMPRESSION

#define RS4_MAGIC (uint16_t)0x5253 // "RS"

/// Scan data types
#define RS4_SCAN_DATA_TYPE_CMSK     1
#define RS4_SCAN_DATA_TYPE_NON_CMSK 0

#define MAX_RING_BUF_SIZE_TOOL 200000

#define RS4_VERSION 3

typedef uint16_t RingId_t;

typedef struct ring_hdr CompressedScanData;

// Return a big-endian-indexed nibble from a byte string

static int
rs4_get_nibble(const uint8_t* i_string, const uint32_t i_i)
{
    uint8_t byte;
    int nibble;

    byte = i_string[i_i / 2];

    if (i_i % 2)
    {
        nibble = byte & 0xf;
    }
    else
    {
        nibble = byte >> 4;
    }

    return nibble;
}


// Set a big-endian-indexed nibble in a byte string

static int
rs4_set_nibble(uint8_t* io_string, const uint32_t i_i, const int i_nibble)
{
    uint8_t* byte;

    byte = &(io_string[i_i / 2]);

    if (i_i % 2)
    {
        *byte = (*byte & 0xf0) | i_nibble;
    }
    else
    {
        *byte = (*byte & 0x0f) | (i_nibble << 4);
    }

    return i_nibble;
}


// Encode an unsigned integer into a 4-bit octal stop code directly into a
// nibble stream at io_string<i_i>, returning the number of nibbles in the
// resulting code.

static int
rs4_stop_encode(const uint32_t i_count, uint8_t* io_string, const uint32_t i_i)
{
    uint32_t count;
    int digits, offset;

    // Determine the number of octal digits.  There is always at least 1.

    count = i_count >> 3;
    digits = 1;

    while (count)
    {
        count >>= 3;
        digits++;
    }

    // First insert the stop (low-order) digit

    offset = digits - 1;
    rs4_set_nibble(io_string, i_i + offset, (i_count & 0x7) | 0x8);

    // Now insert the high-order digits

    count = i_count >> 3;
    offset--;

    while (count)
    {
        rs4_set_nibble(io_string, i_i + offset, count & 0x7);
        offset--;
        count >>= 3;
    }

    return digits;
}


// Decode an unsigned integer from a 4-bit octal stop code appearing in a byte
// string at i_string<i_i>, returning the number of nibbles decoded.

static int
stop_decode(uint32_t* o_count, const uint8_t* i_string, const uint32_t i_i)
{
    int digits, nibble;
    uint32_t i, count;

    digits = 0;
    count = 0;
    i = i_i;

    do
    {
        nibble = rs4_get_nibble(i_string, i);
        count = (count * 8) + (nibble & 0x7);
        i++;
        digits++;
    }
    while ((nibble & 0x8) == 0);

    *o_count = count;
    return digits;
}


// RS4 compression algorithm notes:
//
// RS4 compression processes i_data_str/i_care_str as a strings of nibbles.
// Final special-case code handles the 0-3 remaining terminal bits.
//
// There is a special case for 0x0 nibbles embedded in a string of non-0x0
// nibbles. It is more efficient to encode a single 0x0 nibble as part of a
// longer string of non 0x0 nibbles.  However it is break-even (actually a
// slight statistical advantage) to break a scan sequence for 2 0x0 nibbles.
//
// If a run of 14 scan nibbles is found the scan is terminated and we return
// to the rotate state.  Runs of more than 14 scans will always include a
// 0-length rotate between the scan sequences.
//
// The ability to store a 15th consecutive scan nibble was given up for an
// enhancement of the compression algorithm:
// The scan count 15 has a special meaning and is reserved for handling
// single nibbles that come with a care mask, that is, an extra nibble that
// determines the significance of scan bits, including both 1 and 0 bits.
//
// Returns a scan compression return code.

static int
__rs4_compress(uint8_t* o_rs4_str,
               uint32_t* o_nibbles,
               const uint8_t* i_data_str,
               const uint8_t* i_care_str,
               const uint32_t i_length)
{
    int state;                  /* 0 : Rotate, 1 : Scan */
    uint32_t n;                 /* Number of whole nibbles in i_data */
    uint32_t r;                 /* Number of reminaing bits in i_data */
    uint32_t i;                 /* Nibble index in i_data_str/i_care_str */
    uint32_t j;                 /* Nibble index in o_rs4_str */
    uint32_t k;                 /* Location to place <scan_count(N)> */
    uint32_t count;             /* Counts rotate/scan nibbles */
    int care_nibble;
    int data_nibble;

    n = i_length / 4;
    r = i_length % 4;
    i = 0;
    j = 0;
    k = 0;                      /* Makes GCC happy */
    care_nibble = 0;
    data_nibble = 0;
    count = 0;
    state = 0;

    // Process the bulk of the string.  Note that state changes do not
    // increment 'i' - the nibble at i_data<i> is always scanned again.

    while (i < n)
    {
        care_nibble = rs4_get_nibble(i_care_str, i);
        data_nibble = rs4_get_nibble(i_data_str, i);

        if (~care_nibble & data_nibble)
        {
            return BUGX(SCAN_COMPRESSION_INPUT_ERROR,
                        "Conflicting data and mask bits in nibble %d\n", i);
        }

        if (state == 0)
            //----------------//
            // Rotate section //
            //----------------//
        {
            if (care_nibble == 0)
            {
                count++;
                i++;
            }
            else
            {
                j += rs4_stop_encode(count, o_rs4_str, j);
                count = 0;
                k = j;
                j++;

                if ((care_nibble ^ data_nibble) == 0)
                {
                    // Only one-data in nibble.
                    state = 1;
                }
                else
                {
                    // There is zero-data in nibble.
                    state = 2;
                }
            }
        }
        else if (state == 1)
            //------------------//
            // One-data section //
            //------------------//
        {
            if (care_nibble == 0)
            {
                if (((i + 1) < n) && (rs4_get_nibble(i_care_str, i + 1) == 0))
                {
                    // Set the <scan_count(N)> in nibble k since no more data in
                    //   current AND next nibble (or next nibble might be last).
                    rs4_set_nibble(o_rs4_str, k, count);
                    count = 0;
                    state = 0;
                }
                else
                {
                    // Whether next nibble is last nibble or contains data, lets include the
                    //   current empty nibble in the scan_data(N) count because its
                    //   more efficient than inserting rotate go+stop nibbles.
                    rs4_set_nibble(o_rs4_str, j, 0);
                    count++;
                    i++;
                    j++;
                }
            }
            else if ((care_nibble ^ data_nibble) == 0)
            {
                // Only one-data in nibble. Continue pilling on one-data nibbles.
                rs4_set_nibble(o_rs4_str, j, data_nibble);
                count++;
                i++;
                j++;
            }
            else
            {
                // There is zero-data in nibble.
                // First set the <scan_count(N)> in nibble k to end current
                //   sequence of one-data nibbles.
                rs4_set_nibble(o_rs4_str, k, count);
                count = 0;
                state = 0;
            }

            if ((state == 1) && (count == 14))
            {
                rs4_set_nibble(o_rs4_str, k, 14);
                count = 0;
                state = 0;
            }
        }
        else // state==2
            //-------------------//
            // Zero-data section //
            //-------------------//
        {
            rs4_set_nibble(o_rs4_str, k, 15);
            rs4_set_nibble(o_rs4_str, j, care_nibble);
            j++;
            rs4_set_nibble(o_rs4_str, j, data_nibble);
            i++;
            j++;
            count = 0;
            state = 0;
        }
    } // End of while (i<n)

    // Finish the current state and insert the terminate code (scan 0).  If we
    // finish on a scan we must insert a null rotate first.

    if (state == 0)
    {
        j += rs4_stop_encode(count, o_rs4_str, j);
    }
    else if (state == 1)
    {
        rs4_set_nibble(o_rs4_str, k, count);
        j += rs4_stop_encode(0, o_rs4_str, j);
    }
    else
    {
        return BUGX(SCAN_COMPRESSION_STATE_ERROR,
                    "Termination can not immediately follow masked data\n");
    }

    // Indicate termination start
    rs4_set_nibble(o_rs4_str, j, 0);
    j++;

    // Insert the remainder count nibble, and if r>0, the remainder data
    // nibble. Note that here we indicate the number of bits (0<=r<4).
    if (r == 0)
    {
        rs4_set_nibble(o_rs4_str, j, r);
        j++;
    }
    else
    {
        care_nibble = rs4_get_nibble(i_care_str, n) & ((0xf >> (4 - r)) << (4 - r)); // Make excess bits zero
        data_nibble = rs4_get_nibble(i_data_str, n) & ((0xf >> (4 - r)) << (4 - r)); // Make excess bits zero

        if (~care_nibble & data_nibble)
        {
            return BUGX(SCAN_COMPRESSION_INPUT_ERROR,
                        "Conflicting data and mask bits in nibble %d\n", i);
        }

        if ((care_nibble ^ data_nibble) == 0)
        {
            // Only one-data in rem nibble.
            rs4_set_nibble(o_rs4_str, j, r);
            j++;
            rs4_set_nibble(o_rs4_str, j, data_nibble);
            j++;
        }
        else
        {
            // Zero-data in rem nibble.
            rs4_set_nibble(o_rs4_str, j, r + 8);
            j++;
            rs4_set_nibble(o_rs4_str, j, care_nibble);
            j++;
            rs4_set_nibble(o_rs4_str, j, data_nibble);
            j++;
        }
    }

    *o_nibbles = j;

    return SCAN_COMPRESSION_OK;
}


// The worst-case compression for RS4 v2 occurs if all data nibbles
// contain significant zeros as specified by corresponding care nibbles,
// and if the raw ring length is a whole multiple of four.
//
// In general, each data and care nibble pair, which are one nibble
// in terms of input string length, are compressed into 4 nibbles:
//
// 1. a special data count nibble that indicates special case with care mask
// 2. a care mask nibble
// 3. a data nibble
// 4. a rotate nibble
//
// Then, if the raw ring length is a whole multiple of four (worst case),
// the last raw nibble also requires those RS4 four nibbles, and it is
// followed by 2 additional nibbles that terminate the compressed data.
// So a total of six nibbles to account for the last input nibble:
//
// 5. a '0x0' terminate nibble
// 6. a terminal count(0) nibble
//
// If on the other hand the last input nibble is partial, then that requires
// only four output nibbles because the terminate tag and data are combined
// in the encoding of <terminate>:
//
// 1. a '0x0' terminate nibbel
// 2. a terminal count nibble for masked data
// 3. a care mask nibble
// 4. a data nibble
//
// Besides there is always a rotate nibble at the begin of the compressed
// data:
//
// 0. rotate

static inline uint32_t
rs4_max_compressed_nibbles(const uint32_t i_length)
{
    uint32_t nibbles_raw, nibbles_rs4;

    nibbles_raw = (i_length + 3) / 4;    // bits rounded up to full nibbles
    nibbles_rs4 = 1                      // initial rotate nibble
                  + nibbles_raw * 4      // worst case whole nibble encoding
                  + 1                    // terminate nibble
                  + 1;                   // zero terminal count nibble

    return nibbles_rs4;
}

static inline uint32_t
rs4_max_compressed_bytes(uint32_t nibbles)
{
    uint32_t bytes;

    bytes  = ((nibbles + 1) / 2);        // nibbles rounded up to full bytes
    bytes += sizeof(CompressedScanData); // plus rs4 header
    bytes  = ((bytes + 3) / 4) * 4;      // rounded up to multiple of 4 bytes

    return bytes;
}


// We always require the worst-case amount of memory including the header and
// any rounding required to guarantee that the data size is a multiple of 4
// bytes. The final image size is also rounded up to a multiple of 4 bytes.
//
// Returns a scan compression return code.

int
rs4_compress(CompressedScanData* io_rs4,
             const uint32_t i_size,
             const uint8_t* i_data_str,
             const uint8_t* i_care_str,
             const uint32_t i_length,
             const uint32_t i_scanAddr,
             const RingId_t i_ringId)
{
    int rc;
    uint32_t nibbles = rs4_max_compressed_nibbles(i_length);
    uint32_t bytes   = rs4_max_compressed_bytes(nibbles);
    uint8_t* rs4_str = (uint8_t*)io_rs4 + sizeof(CompressedScanData);

    if (bytes > i_size)
    {
        return BUG(SCAN_COMPRESSION_BUFFER_OVERFLOW);
    }

    memset(io_rs4, 0, i_size);

    rc = __rs4_compress(rs4_str, &nibbles, i_data_str, i_care_str, i_length);

    if (rc == SCAN_COMPRESSION_OK)
    {
        bytes = rs4_max_compressed_bytes(nibbles);

        io_rs4->magic     = htobe16(RS4_MAGIC);
        io_rs4->version   = RS4_VERSION;
        // For now this assumes non-CMSK scan data.
        // For CMSK support, we would need to:
        // - either add a CMSK function parameter and set type here,
        // - or rely on caller to set type later.
        io_rs4->type      = RS4_SCAN_DATA_TYPE_NON_CMSK;
        io_rs4->size      = htobe16(bytes);
        io_rs4->ring_id   = htobe16(i_ringId);
        io_rs4->scan_addr = htobe32(i_scanAddr);
    }

    return rc;
}


// Decompress an RS4-encoded string into a output string whose length must be
// exactly i_length bits.
//
// Returns a scan compression return code.

static int
__rs4_decompress(uint8_t* o_data_str,
                 uint8_t* o_care_str,
                 uint32_t i_size,
                 uint32_t* o_length,
                 const uint8_t* i_rs4_str)
{
    int state;                  /* 0 : Rotate, 1 : Scan */
    uint32_t i;                 /* Nibble index in i_rs4_str */
    uint32_t j;                 /* Nibble index in o_data_str/o_care_str */
    uint32_t k;                 /* Loop index */
    uint32_t bits;              /* Number of output bits decoded so far */
    uint32_t count;             /* Count of rotate nibbles */
    uint32_t nibbles;           /* Rotate encoding or scan nibbles to process */
    int r;                      /* Remainder bits */
    int masked;                 /* if a care mask is available */

    i = 0;
    j = 0;
    bits = 0;
    state = 0;

    // Decompress the bulk of the string
    do
    {
        if (state == 0)
        {
            nibbles = stop_decode(&count, i_rs4_str, i);
            i += nibbles;

            bits += 4 * count;

            if (bits > i_size * 8)
            {
                return BUG(SCAN_COMPRESSION_BUFFER_OVERFLOW);
            }

            // keep 'count' zero care and data nibbles
            // as initialised by memset in calling function
            j += count;

            state = 1;
        }
        else
        {
            nibbles = rs4_get_nibble(i_rs4_str, i);
            i++;

            if (nibbles == 0)
            {
                break;
            }

            masked = (nibbles == 15 ? 1 : 0);
            nibbles = (masked ? 1 : nibbles);
            bits += 4 * nibbles;

            if (bits > i_size * 8)
            {
                return BUG(SCAN_COMPRESSION_BUFFER_OVERFLOW);
            }

            for (k = 0; k < nibbles; k++)
            {
                rs4_set_nibble(o_care_str, j, rs4_get_nibble(i_rs4_str, i));
                i = (masked ? i + 1 : i);
                rs4_set_nibble(o_data_str, j, rs4_get_nibble(i_rs4_str, i));
                i++;
                j++;
            }

            state = 0;
        }
    }
    while (1);

    // Now handle string termination

    nibbles = rs4_get_nibble(i_rs4_str, i);
    i++;

    masked = nibbles & 0x8;
    r = nibbles & 0x3;
    bits += r;

    if (bits > i_size * 8)
    {
        return BUG(SCAN_COMPRESSION_BUFFER_OVERFLOW);
    }

    if (r != 0)
    {
        rs4_set_nibble(o_care_str, j, rs4_get_nibble(i_rs4_str, i));
        i = (masked ? i + 1 : i);
        rs4_set_nibble(o_data_str, j, rs4_get_nibble(i_rs4_str, i));
    }

    *o_length = bits;
    return SCAN_COMPRESSION_OK;
}

int
rs4_decompress(uint8_t* o_data_str,
               uint8_t* o_care_str,
               uint32_t i_size,
               uint32_t* o_length,
               const CompressedScanData* i_rs4)
{
    uint8_t* rs4_str = (uint8_t*)i_rs4 + sizeof(CompressedScanData);

    if (be16toh(i_rs4->magic) != RS4_MAGIC)
    {
        return BUG(SCAN_DECOMPRESSION_MAGIC_ERROR);
    }

    if (i_rs4->version != RS4_VERSION)
    {
        return BUG(SCAN_COMPRESSION_VERSION_ERROR);
    }

    memset(o_data_str, 0, i_size);
    memset(o_care_str, 0, i_size);

    return __rs4_decompress(o_data_str, o_care_str, i_size,
                            o_length, rs4_str);
}

int
rs4_redundant(const CompressedScanData* i_data, int* o_redundant)
{
    uint8_t* data;
    uint32_t length, pos;

    *o_redundant = 0;

    if (htobe16(i_data->magic) != RS4_MAGIC)
    {
        return BUG(SCAN_DECOMPRESSION_MAGIC_ERROR);
    }

    data = (uint8_t*)i_data + sizeof(CompressedScanData);

    // A compressed scan string is redundant if the initial rotate is
    // followed by the end-of-string marker, and any remaining mod-4 bits
    // are also 0.

    pos = stop_decode(&length, data, 0);
    length *= 4;

    if (rs4_get_nibble(data, pos) == 0)
    {
        if (rs4_get_nibble(data, pos + 1) == 0)
        {
            *o_redundant = 1;
        }
        else
        {
            length += rs4_get_nibble(data, pos + 1);

            if (rs4_get_nibble(data, pos + 2) == 0)
            {
                *o_redundant = 1;
            }
        }
    }

    return SCAN_COMPRESSION_OK;
}
