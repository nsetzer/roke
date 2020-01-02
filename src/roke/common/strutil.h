
#ifndef ROKE_COMMON_STRUTIL_H
#define ROKE_COMMON_STRUTIL_H

/**
 *
 * @file roke/common/strutil.h
 * @brief String helper functions
 */

#include "roke/common/compat.h"

ROKE_INTERNAL_API size_t tolowercase(uint8_t* dst, size_t dstlen, const uint8_t* str);
ROKE_INTERNAL_API uint8_t* strdup_safe(const uint8_t *str);
ROKE_INTERNAL_API size_t strcpy_safe(uint8_t *dst, size_t dst_len, const uint8_t *str);
ROKE_INTERNAL_API int has_suffix(const uint8_t *str, size_t lenstr, const uint8_t *suffix, size_t lensuffix);
ROKE_INTERNAL_API int strglob(const uint8_t *ptn, const uint8_t *str);
ROKE_INTERNAL_API int strescape(uint8_t* str, size_t len);

#endif
