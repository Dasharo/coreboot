/*
 * Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include "lib/fmap.h"
#include "lib/lib_vpd.h"
#include "lib/lib_smbios.h"
#include "lib/vpd.h"
#include "lib/vpd_tables.h"

/* The buffer length. Right now the VPD partition size on flash is 128KB. */
#define BUF_LEN (128 * 1024)

/* The comment shown in the begin of --sh output */
#define SH_COMMENT                                                      \
  "#\n"                                                                 \
  "# Prepend 'vpd -O' before this text to always reset VPD content.\n"  \
  "# Append more -s at end to set additional key/values.\n"             \
  "# Or an empty line followed by other commands.\n"                    \
  "#\n"

/* Forward reference(s) */
static uint8_t *readFileContent(const char* filename, uint32_t *filesize);

/* Linked list to track temporary files
 */
struct TempfileNode {
  char *filename;
  struct TempfileNode *next;
} *tempfile_list = NULL;


enum FileFlag {
  HAS_SPD = (1 << 0),
  HAS_VPD_2_0 = (1 << 1),
  HAS_VPD_1_2 = (1 << 2),
  /* TODO(yjlou): refine those variables in main() to here:
   *              overwrite_it, modified. */
} file_flag = 0;


/* 2 containers:
 *   file:      stores decoded pairs from file.
 *   argument:  stores parsed pairs from command arguments.
 */
struct PairContainer file;
struct PairContainer set_argument;
struct PairContainer del_argument;

int export_type = VPD_EXPORT_KEY_VALUE;

/* The current padding length value.
 * Default: VPD_AS_LONG_AS
 */
int pad_value_len = VPD_AS_LONG_AS;

/* The output buffer */
uint8_t buf[BUF_LEN];
int buf_len = 0;
int max_buf_len = sizeof(buf);

/* The EPS base address used to fill the EPS table entry.
 * If the VPD partition can be found in fmap, this points to the starting
 * offset of VPD partition. If not found, this is used to be the base address
 * to increase SPD and VPD 2.0 offset fields.
 *
 * User can overwrite this by -E argument.
 */
#define UNKNOWN_EPS_BASE ((uint32_t)-1)
uint32_t eps_base = UNKNOWN_EPS_BASE;
int eps_base_force_specified = 0;  /* a bool value to indicate if -E argument
                                    * is given. */

/* the fmap name of VPD. */
char fmap_vpd_area_name[FMAP_STRLEN] = "RO_VPD";

/* If found_vpd, replace the VPD partition when saveFile().
 * If not found, always create new file when saveFlie(). */
int found_vpd = 0;

/* The VPD partition offset and size in buf[]. The whole partition includes:
 *
 *   SMBIOS EPS
 *   SMBIOS tables[]
 *   SPD
 *   VPD 2.0 data
 *
 */
uint32_t vpd_offset = 0, vpd_size;  /* The whole partition */
/* Below offset are related to vpd_offset and assume positive.
 * Those are used in saveFile() to write back data. */
uint32_t eps_offset = 0;  /* EPS's starting address. Tables[] is following. */
uint32_t spd_offset = GOOGLE_SPD_OFFSET;  /* SPD address .*/
off_t vpd_2_0_offset = GOOGLE_VPD_2_0_OFFSET;  /* VPD 2.0 data address. */

/* This points to the SPD data if it is availiable when loadFile().
 * The memory is allocated in loadFile(), will be used in saveFile(),
 * and freed at end of main(). */
uint8_t *spd_data = NULL;
int32_t spd_len = 256;  /* max value for DDR3 */

/*  Erases all files created by myMkTemp
 */
void cleanTempFiles() {
  struct TempfileNode *node;

  while (tempfile_list) {
    node = tempfile_list;
    tempfile_list = node->next;
    if (unlink(node->filename) < 0) {
      fprintf(stderr, "warning: failed removing temporary file: %s\n",
              node->filename);
    }
    free(node->filename);
    free(node);
  }
}


/*  Given the offset of blob block (related to the first byte of EPS) and
 *  the size of blob, the is function generates an SMBIOS ESP.
 */
vpd_err_t buildEpsAndTables(
    const int size_blob,
    const int max_buf_len,
    unsigned char *buf,
    int *generated) {
  struct vpd_entry *eps;
  unsigned char *table = NULL;          /* the structure table */
  int table_len = 0;
  int num_structures = 0;
  vpd_err_t retval = VPD_OK;

  assert(buf);
  assert(generated);
  assert(eps_base != UNKNOWN_EPS_BASE);

  buf += *generated;

  /* Generate type 241 - SPD data */
  table_len = vpd_append_type241(0, &table, table_len,
                                 GOOGLE_SPD_UUID,
                                 eps_base + GOOGLE_SPD_OFFSET,
                                 spd_len,  /* Max length for DDR3 */
                                 GOOGLE_SPD_VENDOR,
                                 GOOGLE_SPD_DESCRIPTION,
                                 GOOGLE_SPD_VARIANT);
  if (table_len < 0) {
    retval = VPD_FAIL;
    goto error_1;
  }
  num_structures++;

  /*
   * TODO(hungte) Once most systems have been updated to support VPD_INFO
   * record, we can remove the +sizeof(google_vpd_info) hack.
   */

  /* Generate type 241 - VPD 2.0 */
  table_len = vpd_append_type241(1, &table, table_len,
                                 GOOGLE_VPD_2_0_UUID,
                                 (eps_base + GOOGLE_VPD_2_0_OFFSET +
                                  sizeof(struct google_vpd_info)),
                                 size_blob,
                                 GOOGLE_VPD_2_0_VENDOR,
                                 GOOGLE_VPD_2_0_DESCRIPTION,
                                 GOOGLE_VPD_2_0_VARIANT);
  if (table_len < 0) {
    retval = VPD_FAIL;
    goto error_1;
  }
  num_structures++;

  /* Generate type 127 */
  table_len = vpd_append_type127(2, &table, table_len);
  if (table_len < 0) {
    retval = VPD_FAIL;
    goto error_1;
  }
  num_structures++;

  /* Generate EPS */
  eps = vpd_create_eps(table_len, num_structures, eps_base);
  if ((*generated + eps->entry_length) > max_buf_len) {
    retval = VPD_FAIL;
    goto error_2;
  }

  /* Copy EPS back to buf */
  memcpy(buf, eps, eps->entry_length);
  buf += eps->entry_length;
  *generated += eps->entry_length;

  /* Copy tables back to buf */
  if ((*generated + table_len) > max_buf_len) {
    retval = VPD_FAIL;
    goto error_2;
  }
  memcpy(buf, table, table_len);
  buf += table_len;
  *generated += table_len;

error_2:
  free(eps);
error_1:
  free(table);

  return retval;
}

static int isbase64(uint8_t c)
{
  return isalnum(c) || (c == '+') || (c == '/') || (c == '=');
}

static uint8_t *read_string_from_file(const char *file_name)
{

  uint32_t i, j, file_size;
  uint8_t *file_buffer;

  file_buffer = readFileContent(file_name, &file_size);

  if (!file_buffer)
    return NULL; /* The error has been reported already. */

  /*
   * Is the contents a proper base64 blob? Verify it and drop EOL characters
   * along the way, this will help when displaying the contents.
   */
  for (i = 0, j = 0; i < file_size; i++) {
    uint8_t c = file_buffer[i];

    if ((c == 0xa) || (c == 0xd))
      continue;  /* Skip EOL characters */

    if (!isbase64(c)) {
      fprintf(stderr, "[ERROR] file %s is not in base64 format (%c at %d)\n",
              file_name, c, i);
      free(file_buffer);
      return NULL;
    }
    file_buffer[j++] = c;
  }
  file_buffer[j] = '\0';

  return (uint8_t *)file_buffer;
}


/*
 * Check if given key name is compliant to recommended format.
 */
static vpd_err_t checkKeyName(const uint8_t *name) {
  unsigned char c;
  while ((c = *name++)) {
    if (!(isalnum(c) || c == '_' || c == '.')) {
      fprintf(stderr, "[ERROR] VPD key name does not allow char [%c].\n", c);
      return VPD_ERR_PARAM;
    }
  }
  return VPD_OK;
}


/*
 * Given a key=value string, this function parses it and adds to arugument
 * pair container. The 'value' can be stored in a base64 format file, in this
 * case the value field is the file name.
 */
vpd_err_t parseString(const uint8_t *string, int read_from_file) {
  uint8_t *key;
  uint8_t *value;
  uint8_t *file_contents = NULL;
  vpd_err_t retval = VPD_OK;

  key = (uint8_t*)strdup((char*)string);
  if (!key || key[0] == '\0' || key[0] == '=') {
    if (key) free(key);
    return VPD_ERR_SYNTAX;
  }

  /*
   * Goes through the key string, and stops at the first '='.
   * If '=' is not found, the whole string is the key and
   * the value points to the end of string ('\0').
   */
  for (value = key; *value && *value != '='; value++);
  if (*value == '=') {
    *(value++) = '\0';

    if (read_from_file) {
      /* 'value' in fact is a file name */
      file_contents = read_string_from_file((const char *)value);
      if (!file_contents) {
        free(key);
        return VPD_ERR_SYNTAX;
      }
      value = file_contents;
    }
  }

  retval = checkKeyName(key);
  if (retval == VPD_OK)
    setString(&set_argument, key, value, pad_value_len);

  free(key);
  if (file_contents)
    free(file_contents);

  return retval;
}


/* Given an address, compare if it is SMBIOS signature ("_SM_"). */
int isEps(const void* ptr) {
  return !memcmp(VPD_ENTRY_MAGIC, ptr, sizeof(VPD_ENTRY_MAGIC) - 1);
  /* TODO(yjlou): need more EPS sanity checks here. */
}


/* There are two possible file content appearng here:
 *   1. a full and complete BIOS file
 *   2. a full but only VPD partition area is valid. (no fmap)
 *   3. a full BIOS, but VPD partition is blank.
 *
 * The first case is easy. Just lookup the fmap and find out the VPD partition.
 * The second is harder. We try to search the SMBIOS signature (since others
 * are blank). For the third, we just return and leave caller to read full
 * content, including fmap info.
 *
 * If found, vpd_offset and vpd_size are updated.
 */
vpd_err_t findVpdPartition(const uint8_t* read_buf, const uint32_t file_size,
                     uint32_t* vpd_offset, uint32_t* vpd_size) {
  off_t sig_offset;
  struct fmap *fmap;
  int i;

  assert(read_buf);
  assert(vpd_offset);
  assert(vpd_size);

  /* scan the file and find out the VPD partition. */
  sig_offset = fmapFind(read_buf, file_size);
  if (-1 != sig_offset) {
    /* FMAP signature is found, try to search the partition name in table. */
    fmap = (struct fmap *)&read_buf[sig_offset];
    for(i = 0; i < fmap->nareas; i++) {
      fmapNormalizeAreaName(fmap->areas[i].name);
    }

    if (FMAP_OK == fmapGetArea(fmap_vpd_area_name, fmap,
                               vpd_offset, vpd_size)) {
      found_vpd = 1;  /* Mark found here then saveFile() knows where to
                       * write back (vpd_offset, vpd_size). */
      return VPD_OK;
    } else {
      fprintf(stderr, "[ERROR] The VPD partition [%s] is not found.\n",
              fmap_vpd_area_name);
      return VPD_ERR_NOT_FOUND;
    }
  }

  /* The signature must be aligned to 16-byte. */
  for (i = 0; i < file_size; i += 16) {
    if (isEps(&read_buf[i])) {
      found_vpd = 1;  /* Mark found here then saveFile() knows where to
                       * write back (vpd_offset, vpd_size). */
      *vpd_offset = i;
      /* FIXME: We don't know the VPD partition size in this case.
       *        However, return 4K should be safe enough now.
       *        In the long term, this code block will be obscured. */
      *vpd_size = 4096;
      return VPD_OK;
    }
  }
  return VPD_ERR_NOT_FOUND;
}


/* Load file content into memory.
 * Returns: NULL if file opens error or read error.
 *          Others, pointer to the memory. The filesize is also returned.
 *
 * Note: it's caller's responsbility to free the memory.
 */
static uint8_t *readFileContent(const char* filename, uint32_t *filesize) {
  FILE *fp;
  uint8_t *read_buf;

  assert(filename);
  assert(filesize);

  if (!(fp = fopen(filename, "r"))) {
    return NULL;
  }
  /* get file size */
  fseek(fp, 0, SEEK_END);
  *filesize = ftell(fp);

  /* read file content */
  fseek(fp, 0, SEEK_SET);
  read_buf = malloc(*filesize + 1); /* Might need room for a \0. */
  assert(read_buf);
  if (*filesize != fread(read_buf, 1, *filesize, fp)) {
    fprintf(stderr, "[ERROR] Reading file [%s] failed.\n", filename);
    return NULL;
  }
  fclose(fp);

  return read_buf;
}

/* Below 2 functions are the helper functions for extract data from VPD 1.x
 * binary-encoded structure.
 * Note that the returning pointer is a static buffer. Thus the later call will
 * destroy the former call's result.
 */
static uint8_t* extractString(const uint8_t* value, const int max_len) {
  static uint8_t buf[128];
  int copy_len;

  /* not longer than the buffer size */
  copy_len = (max_len > sizeof(buf) - 1) ? sizeof(buf) - 1 : max_len;
  memcpy(buf, value, copy_len);
  buf[copy_len] = '\0';

  return buf;
}

static uint8_t* extractHex(const uint8_t* value, const int len) {
  char tmp[4];  /* for a hex string */
  static uint8_t buf[128];
  int in, out;  /* in points to value[], while out points to buf[]. */

  for (in = 0, out = 0;; ++in) {
    if (out + 3 > sizeof(buf) - 1) {
      goto end_of_func;  /* no more buffer */
    }
    if (in >= len) {  /* no more input */
      if (out) --out;  /* remove the tailing colon */
      goto end_of_func;
    }
    sprintf(tmp, "%02x:", value[in]);
    memcpy(&buf[out], tmp, strlen(tmp));
    out += strlen(tmp);
  }

end_of_func:
  buf[out] = '\0';

  return buf;
}


vpd_err_t loadFile(const char *filename, struct PairContainer *container,
             int overwrite_it) {
  uint32_t file_size;
  uint8_t *read_buf;
  uint8_t *vpd_buf;
  struct vpd_entry *eps;
  uint32_t related_eps_base;
  struct vpd_header *header;
  struct vpd_table_binary_blob_pointer *data;
  uint8_t spd_uuid[16], vpd_2_0_uuid[16], vpd_1_2_uuid[16];
  int expected_handle;
  int table_len;
  uint32_t index;
  vpd_err_t retval = VPD_OK;

  if (!(read_buf = readFileContent(filename, &file_size))) {
    fprintf(stderr, "[WARN] Cannot LoadFile('%s'), that's fine.\n", filename);
    return VPD_OK;
  }

  if (0 == findVpdPartition(read_buf, file_size, &vpd_offset, &vpd_size)) {
    if (!eps_base_force_specified) {
      eps_base = vpd_offset;
    }
  } else {
        goto teardown;
  }

  /* Update the following variables:
   *   eps_base: integer, the VPD EPS address in ROM.
   *   vpd_offset: integer, the VPD partition offset in file (read_buf[]).
   *   vpd_buf: uint8_t*, points to the VPD partition.
   *   eps: vpd_entry*, points to the EPS structure.
   *   eps_offset: integer, the offset of EPS related to vpd_buf[].
   */
  vpd_buf = &read_buf[vpd_offset];
  /* eps and eps_offset will be set slightly later. */

  if (eps_base == UNKNOWN_EPS_BASE) {
    fprintf(stderr, "[ERROR] Cannot determine eps_base. Cannot go on.\n"
                    "        You may use -E to specify the value.\n");
    retval = VPD_ERR_INVALID;
    goto teardown;
  }

  /* In overwrite mode, we don't care the content inside. Stop parsing. */
  if (overwrite_it) {
    retval = VPD_OK;
    goto teardown;
  }

  if (vpd_size < sizeof(struct vpd_entry)) {
    fprintf(stderr, "[ERROR] vpd_size:%d is too small to be compared.\n",
            vpd_size);
    retval = VPD_ERR_INVALID;
    goto teardown;
  }
  /* try to search the EPS if it is not aligned to the begin of partition. */
  for (index = 0; index < vpd_size; index += 16) {
    if (isEps(&vpd_buf[index])) {
      eps = (struct vpd_entry *)&vpd_buf[index];
      eps_offset = index;
      break;
    }
  }
  /* jump if the VPD partition is not recognized. */
  if (index >= vpd_size) {
    /* But OKAY if the VPD partition starts with FF, which might be un-used. */
    if (!memcmp("\xff\xff\xff\xff", vpd_buf, sizeof(VPD_ENTRY_MAGIC) - 1)) {
      fprintf(stderr, "[WARN] VPD partition not formatted. It's fine.\n");
      retval = VPD_OK;
      goto teardown;
    } else {
      fprintf(stderr, "SMBIOS signature is not matched.\n");
      fprintf(stderr, "You may use -O to overwrite the data.\n");
      retval = VPD_ERR_INVALID;
      goto teardown;
    }
  }

  /* adjust the eps_base for data->offset field below. */
  if (!eps_base_force_specified) {
    related_eps_base = eps->table_address - sizeof(*eps);
  } else {
    related_eps_base = eps_base;
  }

  /* EPS is done above. Parse structure tables below. */
  /* Get the first type 241 blob, at the tail of EPS. */
  header = (struct vpd_header*)(((uint8_t*)eps) + eps->entry_length);
  data = (struct vpd_table_binary_blob_pointer *)
         ((uint8_t *)header + sizeof(*header));


  /* prepare data structure to compare */
  uuid_parse(GOOGLE_SPD_UUID, spd_uuid);
  uuid_parse(GOOGLE_VPD_2_0_UUID, vpd_2_0_uuid);
  uuid_parse(GOOGLE_VPD_1_2_UUID, vpd_1_2_uuid);

  /* Iterate all tables */
  for (expected_handle = 0;
       header->type != VPD_TYPE_END;
       ++expected_handle) {
    /* make sure we haven't have too much handle already. */
    if (expected_handle > 65535) {
      fprintf(stderr, "[ERROR] too many handles. Terminate parsing.\n");
      retval = VPD_ERR_INVALID;
      goto teardown;
    }

    /* check type */
    if (header->type != VPD_TYPE_BINARY_BLOB_POINTER) {
      fprintf(stderr, "[ERROR] We now only support Binary Blob Pointer (241). "
                      "But the %dth handle is type %d. Terminate parsing.\n",
                      header->handle, header->type);
      retval = VPD_ERR_INVALID;
      goto teardown;
    }

    /* make sure handle is increasing as expected */
    if (header->handle != expected_handle) {
      fprintf(stderr, "[ERROR] The handle value must be %d, but is %d.\n"
                      "        Use -O option to re-format.\n",
                      expected_handle, header->handle);
      retval = VPD_ERR_INVALID;
      goto teardown;
    }

    /* point to the table 241 data part */
    index = data->offset - related_eps_base;
    if (index >= file_size) {
      fprintf(stderr, "[ERROR] the table offset looks suspicious. "
                      "index=0x%x, data->offset=0x%x, related_eps_base=0x%x\n",
                      index, data->offset, related_eps_base);
      retval = VPD_ERR_INVALID;
      goto teardown;
    }

    /*
     * The main switch case
     */
    if (!memcmp(data->uuid, spd_uuid, sizeof(data->uuid))) {
      /* SPD */
      spd_offset = index;
      spd_len = data->size;
      if (vpd_offset + spd_offset + spd_len >= file_size) {
        fprintf(stderr, "[ERROR] SPD offset in BBP is not correct.\n"
                        "        vpd=0x%x spd=0x%x len=0x%x file_size=0x%x\n"
                        "        If this file is VPD partition only, try to\n"
                        "        use -E to adjust offset values.\n",
                        (uint32_t)vpd_offset, (uint32_t)spd_offset,
                        spd_len, file_size);
        retval = VPD_ERR_INVALID;
        goto teardown;
      }

      if (!(spd_data = malloc(spd_len))) {
        fprintf(stderr, "spd_data: malloc(%d bytes) failed.\n", spd_len);
        retval = VPD_ERR_SYSTEM;
        goto teardown;
      }
      memcpy(spd_data, &read_buf[vpd_offset + spd_offset], spd_len);
      file_flag |= HAS_SPD;

    } else if (!memcmp(data->uuid, vpd_2_0_uuid, sizeof(data->uuid))) {
      /* VPD 2.0 */
      /* iterate all pairs */
      for (;
           vpd_buf[index] != VPD_TYPE_TERMINATOR &&
           vpd_buf[index] != VPD_TYPE_IMPLICIT_TERMINATOR;) {
        retval = decodeToContainer(container, vpd_size, vpd_buf, &index);
        if (VPD_OK != retval)
        {
          fprintf(stderr, "decodeToContainer() error.\n");
          goto teardown;
        }
      }
      file_flag |= HAS_VPD_2_0;

    } else if (!memcmp(data->uuid, vpd_1_2_uuid, sizeof(data->uuid))) {
      /* VPD 1_2: please refer to "Google VPD Type 241 Format v1.2" */
      struct V12 {
        uint8_t prod_sn[0x20];
        uint8_t sku[0x10];
        uint8_t uuid[0x10];
        uint8_t mb_sn[0x10];
        uint8_t imei[0x10];
        uint8_t ssd_sn[0x10];
        uint8_t mem_sn[0x10];
        uint8_t wlan_mac[0x06];
      } *v12 = (void*)&vpd_buf[index];
      setString(container, (uint8_t*)"Product_SN",
                extractString(v12->prod_sn, sizeof(v12->prod_sn)),
                VPD_AS_LONG_AS);
      setString(container, (uint8_t*)"SKU",
                extractString(v12->sku, sizeof(v12->sku)), VPD_AS_LONG_AS);
      setString(container, (uint8_t*)"UUID",
                extractHex(v12->uuid, sizeof(v12->uuid)),
                VPD_AS_LONG_AS);
      setString(container, (uint8_t*)"MotherBoard_SN",
                extractString(v12->mb_sn, sizeof(v12->mb_sn)),
                VPD_AS_LONG_AS);
      setString(container, (uint8_t*)"IMEI",
                extractString(v12->imei, sizeof(v12->imei)),
                VPD_AS_LONG_AS);
      setString(container, (uint8_t*)"SSD_SN",
                extractString(v12->ssd_sn, sizeof(v12->ssd_sn)),
                VPD_AS_LONG_AS);
      setString(container, (uint8_t*)"Memory_SN",
                extractString(v12->mem_sn, sizeof(v12->mem_sn)),
                VPD_AS_LONG_AS);
      setString(container, (uint8_t*)"WLAN_MAC",
                extractHex(v12->wlan_mac, sizeof(v12->wlan_mac)),
                VPD_AS_LONG_AS);
      file_flag |= HAS_VPD_1_2;

    } else {
      /* un-supported UUID */
      char outstr[37];  /* 36-char + 1 null terminator */

      uuid_unparse(data->uuid, outstr);
      fprintf(stderr, "[ERROR] un-supported UUID: %s\n", outstr);
      retval = VPD_ERR_INVALID;
      goto teardown;
    }

    /* move to next table */
    if ((table_len = vpd_type241_size(header)) < 0) {
      fprintf(stderr, "[ERROR] Cannot get type 241 structure table length.\n");
      retval = VPD_ERR_INVALID;
      goto teardown;
    }

    header = (struct vpd_header*)((uint8_t*)header + table_len);
    data = (struct vpd_table_binary_blob_pointer *)
           ((uint8_t *)header + sizeof(*header));
  }

teardown:
  free(read_buf);

  return retval;
}


vpd_err_t saveFile(const struct PairContainer *container,
                   const char *filename) {
  FILE *fp;
  unsigned char eps[1024];
  int eps_len = 0;
  vpd_err_t retval = VPD_OK;
  uint32_t file_seek;
  struct google_vpd_info *info = (struct google_vpd_info *)buf;

  memset(eps, 0xff, sizeof(eps));
  buf_len = sizeof(*info);

  /* prepare info */
  memset(info, 0, sizeof(*info));
  memcpy(info->header.magic, VPD_INFO_MAGIC, sizeof(info->header.magic));

  /* encode into buffer */
  retval = encodeContainer(&file, max_buf_len, buf, &buf_len);
  if (VPD_OK != retval) {
    fprintf(stderr, "encodeContainer() error.\n");
    goto teardown;
  }
  retval = encodeVpdTerminator(max_buf_len, buf, &buf_len);
  if (VPD_OK != retval) {
    fprintf(stderr, "Out of space for terminator.\n");
    goto teardown;
  }
  info->size = buf_len - sizeof(*info);

  retval = buildEpsAndTables(buf_len, sizeof(eps), eps, &eps_len);
  if (VPD_OK != retval) {
    fprintf(stderr, "Cannot build EPS.\n");
    goto teardown;
  }
  assert(eps_len <= GOOGLE_SPD_OFFSET);

  /* Write data in the following order:
   *   1. EPS
   *   2. SPD
   *   3. VPD 2.0
  */
  if (found_vpd) {
    /* We found VPD partition in -f file, which means file is existed.
     * Instead of truncating the whole file, open to write partial. */
    if (!(fp = fopen(filename, "r+"))) {
      fprintf(stderr, "File [%s] cannot be opened for write.\n", filename);
      retval = VPD_ERR_SYSTEM;
      goto teardown;
    }
  } else {
    /* VPD is not found, which means the file is pure VPD data.
     * Always creates the new file and overwrites the original content. */
    if (!(fp = fopen(filename, "w+"))) {
      fprintf(stderr, "File [%s] cannot be opened for write.\n", filename);
      retval = VPD_ERR_SYSTEM;
      goto teardown;
    }
  }

  file_seek = vpd_offset;


  /* write EPS */
  fseek(fp, file_seek + eps_offset, SEEK_SET);
  if (fwrite(eps, eps_len, 1, fp) != 1) {
    fprintf(stderr, "fwrite(EPS) error (%s)\n", strerror(errno));
    retval = VPD_ERR_SYSTEM;
    goto teardown;
  }

  /* write SPD */
  if (spd_data) {
    fseek(fp, file_seek + spd_offset, SEEK_SET);
    if (fwrite(spd_data, spd_len, 1, fp) != 1) {
      fprintf(stderr, "fwrite(SPD) error (%s)\n", strerror(errno));
      retval = VPD_ERR_SYSTEM;
      goto teardown;
    }
  }

  /* write VPD 2.0 */
  fseek(fp, file_seek + vpd_2_0_offset, SEEK_SET);
  if (fwrite(buf, buf_len, 1, fp) != 1) {
    fprintf(stderr, "fwrite(VPD 2.0) error (%s)\n", strerror(errno));
    retval = VPD_ERR_SYSTEM;
    goto teardown;
  }
  fclose(fp);

teardown:
  return retval;
}

static void usage(const char *progname) {
  printf("Chrome OS VPD 2.0 utility --\n");
#ifdef VPD_VERSION
  printf("%s\n", VPD_VERSION);
#endif
  printf("\n");
  printf("Usage: %s [OPTION] ...\n", progname);
  printf("   OPTIONs include:\n");
  printf("      -h               This help page and version.\n");
  printf("      -f <filename>    The output file name.\n");
  printf("      -E <address>     EPS base address (default:0x240000).\n");
  printf("      -S <key=file>    To add/change a string value, reading its\n");
  printf("                       base64 contents from a file.\n");
  printf("      -s <key=value>   To add/change a string value.\n");
  printf("      -p <pad length>  Pad if length is shorter.\n");
  printf("      -i <partition>   Specify VPD partition name in fmap.\n");
  printf("      -l               List content in the file.\n");
  printf("      --sh             Dump content for shell script.\n");
  printf("      -0/--null-terminated\n");
  printf("                       Dump content in null terminate format.\n");
  printf("      -O               Overwrite and re-format VPD partition.\n");
  printf("      -g <key>         Print value string only.\n");
  printf("      -d <key>         Delete a key.\n");
  printf("\n");
  printf("   Notes:\n");
  printf("      You can specify multiple -s and -d. However, vpd always\n");
  printf("         applies -s first, then -d.\n");
  printf("      -g and -l must be mutually exclusive.\n");
  printf("\n");
}

int main(int argc, char *argv[]) {
  int opt;
  int option_index = 0;
  vpd_err_t retval = VPD_OK;
  const char *optstring = "hf:E:s:S:p:i:lOg:d:0";
  static struct option long_options[] = {
    {"help", 0, 0, 'h'},
    {"file", 0, 0, 'f'},
    {"epsbase", 0, 0, 'E'},
    {"string", 0, 0, 's'},
    {"base64file", 0, 0, 'S'},
    {"pad", required_argument, 0, 'p'},
    {"partition", 0, 0, 'i'},
    {"list", 0, 0, 'l'},
    {"overwrite", 0, 0, 'O'},
    {"filter", 0, 0, 'g'},
    {"sh", 0, &export_type, VPD_EXPORT_AS_PARAMETER},
    {"null-terminated", 0, 0, '0'},
    {"delete", 0, 0, 'd'},
    {0, 0, 0, 0}
  };
  char *filename = NULL;
  const char *load_file = NULL;
  const char *save_file = NULL;
  uint8_t *key_to_export = NULL;
  int list_it = 0;
  int overwrite_it = 0;
  int modified = 0;
  int num_to_delete;
  int read_from_file = 0;

  initContainer(&file);
  initContainer(&set_argument);
  initContainer(&del_argument);

  while ((opt = getopt_long(argc, argv, optstring,
                            long_options, &option_index)) != EOF) {
    switch (opt) {
      case 'h':
        usage(argv[0]);
        goto teardown;
        break;

      case 'f':
        filename = strdup(optarg);
        break;

      case 'E':
        errno = 0;
        eps_base = strtoul(optarg, (char **) NULL, 0);
        eps_base_force_specified = 1;

        /* FIXME: this is not a stable way to detect error because
         *        implementation may (or may not) assign errno. */
        if (!eps_base && errno == EINVAL) {
          fprintf(stderr, "Not a number for EPS base address: %s\n", optarg);
          retval = VPD_ERR_SYNTAX;
          goto teardown;
        }
        break;

      case 'S':
        read_from_file = 1;
        /* Fall through into the next case */
      case 's':
        retval = parseString((uint8_t*)optarg, read_from_file);
        if (VPD_OK != retval) {
          fprintf(stderr, "The string [%s] cannot be parsed.\n\n", optarg);
          goto teardown;
        }
        read_from_file = 0;
        break;

      case 'p':
        errno = 0;
        pad_value_len = strtol(optarg, (char **) NULL, 0);

        /* FIXME: this is not a stable way to detect error because
         *        implementation may (or may not) assign errno. */
        if (!pad_value_len && errno == EINVAL) {
          fprintf(stderr, "Not a number for pad length: %s\n", optarg);
          retval = VPD_ERR_SYNTAX;
          goto teardown;
        }
        break;

      case 'i':
        snprintf(fmap_vpd_area_name, sizeof(fmap_vpd_area_name), "%s", optarg);
        break;

      case 'l':
        list_it = 1;
        break;

      case 'O':
        overwrite_it = 1;
        modified = 1;  /* This option forces to write empty data back even
                        * no new pair is given. */
        break;

      case 'g':
        key_to_export = (uint8_t*)strdup(optarg);
        break;

      case 'd':
        /* Add key into container for delete. Since value is non-sense,
         * keep it empty. */
        setString(&del_argument, (const uint8_t *)optarg,
                                 (const uint8_t *)"", 0);
        break;

      case '0':
        export_type = VPD_EXPORT_NULL_TERMINATE;
        break;

      case 0:
        break;

      default:
        fprintf(stderr, "Invalid option (%s), use --help for usage.\n", optarg);
        retval = VPD_ERR_SYNTAX;
        goto teardown;
        break;
    }
  }

  if (optind < argc) {
    fprintf(stderr, "[ERROR] unexpected argument: %s\n\n", argv[optind]);
    usage(argv[0]);
    retval = VPD_ERR_SYNTAX;
    goto teardown;
  }

  if (list_it && key_to_export) {
    fprintf(stderr, "[ERROR] -l and -g must be mutually exclusive.\n");
    retval = VPD_ERR_SYNTAX;
    goto teardown;
  }

  if (VPD_EXPORT_KEY_VALUE != export_type && !list_it) {
    fprintf(stderr,
            "[ERROR] --sh/--null-terminated can be set only if -l is set.\n");
    retval = VPD_ERR_SYNTAX;
    goto teardown;
  }

  /* to avoid malicious attack, we replace suspicious chars. */
  fmapNormalizeAreaName(fmap_vpd_area_name);

  /* if no filename is specified, call flashrom to read from flash. */
  if (!filename) {
    fprintf(stderr, "[ERROR] filename not provided.\n");
        goto teardown;
  } else {
    load_file = filename;
    save_file = filename;
  }

  retval = loadFile(load_file, &file, overwrite_it);
  if (VPD_OK != retval) {
    fprintf(stderr, "loadFile('%s') error.\n", load_file);
    goto teardown;
  }

  /* Do -s */
  if (lenOfContainer(&set_argument) > 0) {
    mergeContainer(&file, &set_argument);
    modified++;
  }

  /* Do -d */
  num_to_delete = lenOfContainer(&del_argument);
  if (subtractContainer(&file, &del_argument) !=
      num_to_delete) {
    fprintf(stderr, "[ERROR] At least one of the keys to delete"
        " does not exist. Command ignored.\n");
    retval = VPD_ERR_PARAM;
    goto teardown;
  } else if (num_to_delete > 0) {
    modified++;
  }

  /* Do -g */
  if (key_to_export) {
    struct StringPair* foundString = findString(&file, key_to_export, NULL);
    if (NULL == foundString) {
        fprintf(stderr, "findString(): Vpd data '%s' was not found.\n",
                key_to_export);
      retval = VPD_FAIL;
      goto teardown;
    } else {
      uint8_t dump_buf[BUF_LEN * 2];
      int dump_len = 0;

      retval = exportStringValue(foundString,
                                 sizeof(dump_buf), dump_buf, &dump_len);
      if (VPD_OK != retval) {
        fprintf(stderr, "exportStringValue(): Cannot export the value.\n");
        goto teardown;
      }

      fwrite(dump_buf, dump_len, 1, stdout);
    }
  }

  /* Do -l */
  if (list_it) {
    /* Reserve larger size because the exporting generates longer string than
     * the encoded data. */
    uint8_t list_buf[BUF_LEN * 5 + 64];
    int list_len = 0;

    retval = exportContainer(export_type, &file, sizeof(list_buf),
                             list_buf, &list_len);
    if (VPD_OK != retval) {
      fprintf(stderr, "exportContainer(): Cannot generate string.\n");
      goto teardown;
    }

    /* Export necessary program parameters */
    if (VPD_EXPORT_AS_PARAMETER == export_type) {
      printf("%s%s -i %s \\\n",
             SH_COMMENT, argv[0], fmap_vpd_area_name);

      if (filename)
        printf("    -f %s \\\n", filename);
    }

    fwrite(list_buf, list_len, 1, stdout);
  }

  if (modified) {
    if (file_flag & HAS_VPD_1_2) {
      fprintf(stderr, "[ERROR] Writing VPD 1.2 not supported yet.\n");
      retval = VPD_FAIL;
      goto teardown;
    }

    retval = saveFile(&file, save_file);
    if (VPD_OK != retval) {
      fprintf(stderr, "saveFile('%s') error: %d\n", filename, retval);
      goto teardown;
    }
  }

teardown:
  if (spd_data) free(spd_data);
  if (filename) free(filename);
  if (key_to_export) free(key_to_export);
  destroyContainer(&file);
  destroyContainer(&set_argument);
  destroyContainer(&del_argument);
  cleanTempFiles();

  return retval;
}
