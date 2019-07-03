/*
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/lib_vpd.h"


#ifndef MIN
#define MIN(a, b) ((a < b) ? a : b)
#endif


/***********************************************************************
 * Container helpers
 ***********************************************************************/
void initContainer(struct PairContainer *container) {
  container->first = NULL;
}


/*
 * Returns the pointer to the 'key' entry.
 * Returns NULL if the key is not found in 'container'.
 *
 * If 'prev_next' is not NULL, findString() stores the address of the "next"
 * member of the StringPair prior to the returned StringPair (the special case
 * is &container->first if first StringPair matches 'key'). This is for linked
 * list manipulation. If 'prev_next' is NULL, findString() would just ignore
 * it.
 */
struct StringPair *findString(struct PairContainer *container,
                              const uint8_t *key,
                              struct StringPair ***prev_next) {
  struct StringPair *current;

  if (prev_next) {
    *prev_next = &container->first;
  }

  for (current = container->first; current; current = current->next) {
    if (!strcmp((char*)key, (char*)current->key)) {
      return current;
    }
    if (prev_next) {
      *prev_next = &current->next;
    }
  }
  return NULL;
}

/* Just a helper function for setString() */
static void fillStringPair(struct StringPair *pair,
                           const uint8_t *key,
                           const uint8_t *value,
                           const int pad_len) {
  pair->key = malloc(strlen((char*)key) + 1);
  assert(pair->key);
  strcpy((char*)pair->key, (char*)key);
  pair->value = malloc(strlen((char*)value) + 1);
  strcpy((char*)pair->value, (char*)value);
  pair->pad_len = pad_len;
}

/* If key is already existed in container, its value will be replaced.
 * If not existed, creates new entry in container.
 */
void setString(struct PairContainer *container,
               const uint8_t *key,
               const uint8_t *value,
               const int pad_len) {
  struct StringPair *found;

  found = findString(container, key, NULL);
  if (found) {
    free(found->key);
    free(found->value);
    fillStringPair(found, key, value, pad_len);
  } else {
    struct StringPair *new_pair = malloc(sizeof(struct StringPair));
    assert(new_pair);
    memset(new_pair, 0, sizeof(struct StringPair));

    fillStringPair(new_pair, key, value, pad_len);

    /* append this pair to the end of list. to keep the order */
    if ((found = container->first)) {
      while (found->next) found = found->next;
      found->next = new_pair;
    } else {
      container->first = new_pair;
    }
    new_pair->next = NULL;
  }
}


/*
 * Remove a key.
 * Returns VPD_OK if deleted successfully. Otherwise, VPD_FAIL.
 */
vpd_err_t deleteKey(struct PairContainer *container,
                    const uint8_t *key) {
  struct StringPair *found, **prev_next;

  found = findString(container, key, &prev_next);
  if (found) {
    free(found->key);
    free(found->value);

    /* remove the 'found' from the linked list. */
    assert(prev_next);
    *prev_next = found->next;
    free(found);

    return VPD_OK;
  } else {
    return VPD_FAIL;
  }
}


/*
 * Returns number of pairs in container.
 */
int lenOfContainer(const struct PairContainer *container) {
  int count;
  struct StringPair *current;

  for (count = 0, current = container->first;
       current;
       count++, current = current->next);

  return count;
}


/* Iterate src container and setString() in dst.
 * so that if key is duplicate, the one in dst is overwritten.
 */
void mergeContainer(struct PairContainer *dst,
                    const struct PairContainer *src) {
  struct StringPair *current;

  for (current = src->first; current; current = current->next) {
    setString(dst, current->key, current->value, current->pad_len);
  }
}


int subtractContainer(struct PairContainer *dst,
                       const struct PairContainer *src) {
  struct StringPair *current;
  int count = 0;

  for (current = src->first; current; current = current->next) {
    if (VPD_OK == deleteKey(dst, current->key))
      count++;
  }

  return count;
}


vpd_err_t encodeContainer(const struct PairContainer *container,
                          const int max_buf_len,
                          uint8_t *buf,
                          int *generated) {
  struct StringPair *current;

  for (current = container->first; current; current = current->next) {
    if (VPD_OK != encodeVpdString(current->key,
                                  current->value,
                                  current->pad_len,
                                  max_buf_len,
                                  buf,
                                  generated)) {
      return VPD_FAIL;
    }
  }
  return VPD_OK;
}

static int callbackDecodeToContainer(const uint8_t *key,
                                           uint32_t key_len,
                                           const uint8_t *value,
                                           uint32_t value_len,
                                           void *arg) {
  struct PairContainer *container = (struct PairContainer*)arg;
  uint8_t *key_string = (uint8_t*)malloc(key_len + 1),
          *value_string = (uint8_t*)malloc(value_len + 1);
  assert(key_string && value_string);
  memcpy(key_string, key, key_len);
  memcpy(value_string, value, value_len);
  key_string[key_len] = '\0';
  value_string[value_len] = '\0';
  setString(container, key_string, value_string, value_len);
  return VPD_DECODE_OK;
}

vpd_err_t decodeToContainer(struct PairContainer *container,
                            const uint32_t max_len,
                            const uint8_t *input_buf,
                            uint32_t *consumed) {
  return decodeVpdString(max_len, input_buf, consumed,
                         callbackDecodeToContainer, (void*)container);
}

vpd_err_t setContainerFilter(struct PairContainer *container,
                             const uint8_t *filter) {
  struct StringPair *str;

  for (str = container->first; str; str = str->next) {
    if (filter) {
      /*
       * TODO(yjlou):
       * Now, we treat the inputing filter string as plain string.
       * Will support regular expression syntax in future if needed.
       */
      if (strcmp((char*)str->key, (char*)filter)) {
        str->filter_out = 1;
      }
    } else {
      str->filter_out = 0;
    }
  }
  return VPD_OK;
}


/*
 * A helper function to append a sequence of bytes to the given buffer.  If
 * the buffer size is not enough, this function will return VPD_ERR_OVERFLOW;
 * otherwise it will return VPD_OK.
 */
static vpd_err_t _appendToBuf(const void *buf_to_append,
                              int len,
                              const int max_buf_len,
                              uint8_t *buf,
                              int *generated) {
  if (*generated + len > max_buf_len) return VPD_ERR_OVERFLOW;
  memcpy(&buf[*generated], buf_to_append, len);
  *generated += len;
  return VPD_OK;
}


/*
 * A helper function to resolve the number of bytes to be exported for the
 * value field of an instance of StringPair.
 */
static int _getStringPairValueLen(const struct StringPair *str) {
  int len = strlen((const char*)(str->value));
  return VPD_AS_LONG_AS == str->pad_len ? len : MIN(str->pad_len, len);
}


/* A helper function to export an instance of StringPair to the given buffer. */
static vpd_err_t _exportStringPairKeyValue(const struct StringPair *str,
                                           const int max_buf_len,
                                           uint8_t *buf,
                                           int *generated) {
  const void *strs[5] = {"\"", str->key, "\"=\"", str->value, "\"\n"};
  const int lens[5] = {
      1, strlen((const char*)str->key), 3, _getStringPairValueLen(str), 2};

  int retval;
  int i;

  for (i = 0; i < sizeof(lens) / sizeof(int); ++i) {
    retval = _appendToBuf(strs[i], lens[i], max_buf_len, buf, generated);
    if (VPD_OK != retval) {
      break;
    }
  }

  return retval;
}


/*
 * A helper function to escape the special character in a string and then append
 * the result into the buffer.
 */
static vpd_err_t _appendToBufWithShellEscape(const char *str_to_export,
                                             const int max_buf_len,
                                             uint8_t *buf,
                                             int *generated) {
  int len = strlen(str_to_export);
  int i;
  int retval;
  for (i = 0; i < len; ++i) {
    if ('\'' == str_to_export[i]) {
      retval = _appendToBuf("'\"'\"'", 5, max_buf_len, buf, generated);
    } else {
      retval = _appendToBuf(str_to_export + i, 1,
                            max_buf_len, buf, generated);
    }
    if (VPD_OK != retval) return retval;
  }
  return VPD_OK;
}


/*
 * A helper function to export an instance of StringPair to the given buffer
 * as the arguments for the vpd commandline tool.
 */
static vpd_err_t _exportStringPairAsParameter(const struct StringPair *str,
                                              const int max_buf_len,
                                              uint8_t *buf,
                                              int *generated) {
  int retval;

  {
    char extra_params[32];
    snprintf(extra_params, sizeof(extra_params), "    -p %d -s ", str->pad_len);
    retval = _appendToBuf(extra_params, strlen(extra_params),
                          max_buf_len, buf, generated);
    if (VPD_OK != retval) return retval;
  }

  if (*generated + 1 > max_buf_len) return VPD_ERR_OVERFLOW;
  buf[(*generated)++] = '\'';

  retval = _appendToBufWithShellEscape(
      (const char*)str->key, max_buf_len, buf, generated);
  if (VPD_OK != retval) return retval;

  if (*generated + 1 > max_buf_len) return VPD_ERR_OVERFLOW;
  buf[(*generated)++] = '=';

  retval = _appendToBufWithShellEscape(
      (const char*)str->value, max_buf_len, buf, generated);
  if (VPD_OK != retval) return retval;

  retval = _appendToBuf("' \\\n", 4, max_buf_len, buf, generated);

  return retval;
}


/*
 * A helper function to export an instance of StringPair to the given buffer
 * in a null terminate format, i.e. "<key>=<value>\0"
 */
static vpd_err_t _exportStringPairNullTerminate(const struct StringPair *str,
                                                const int max_buf_len,
                                                uint8_t *buf,
                                                int *generated) {
  int retval;

  retval = _appendToBuf(str->key, strlen((const char*)str->key),
                        max_buf_len, buf, generated);
  if (VPD_OK != retval) return retval;

  if (*generated + 1 > max_buf_len) return VPD_ERR_OVERFLOW;
  buf[(*generated)++] = '=';

  retval = _appendToBuf(str->value, _getStringPairValueLen(str),
                        max_buf_len, buf, generated);
  if (VPD_OK != retval) return retval;

  if (*generated + 1 > max_buf_len) return VPD_ERR_OVERFLOW;
  buf[(*generated)++] = '\0';

  return VPD_OK;
}


/* Export the value field of the instance of StringPair. */
vpd_err_t exportStringValue(const struct StringPair *str,
                            const int max_buf_len,
                            uint8_t *buf,
                            int *generated) {
  assert(generated);

  return _appendToBuf(str->value, _getStringPairValueLen(str),
                      max_buf_len, buf, generated);
}


/* Export the container content with human-readable text. */
vpd_err_t exportContainer(const int export_type,
                          const struct PairContainer *container,
                          const int max_buf_len,
                          uint8_t *buf,
                          int *generated) {
  struct StringPair *str;
  int index;
  int retval;

  assert(generated);
  index = *generated;

  for (str = container->first; str; str = str->next) {
    if (str->filter_out)
      continue;

    if (VPD_EXPORT_KEY_VALUE == export_type) {
      retval = _exportStringPairKeyValue(str, max_buf_len, buf, &index);
    } else if (VPD_EXPORT_AS_PARAMETER == export_type) {
      retval = _exportStringPairAsParameter(str, max_buf_len, buf, &index);
    } else if (VPD_EXPORT_NULL_TERMINATE == export_type) {
      retval = _exportStringPairNullTerminate(str, max_buf_len, buf, &index);
    } else {
      /* this block shouldn't be reached */
      assert(0);
    }
    if (VPD_OK != retval) return retval;
  }

  *generated = index;

  return VPD_OK;
}

void destroyContainer(struct PairContainer *container) {
  struct StringPair *current;

  for (current = container->first; current;) {
    struct StringPair *next;

    if (current->key) free(current->key);
    if (current->value) free(current->value);
    next = current->next;
    free(current);
    current = next;
  }
}
