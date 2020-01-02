
#ifndef ROKE_COMMON_PATHUTIL_H
#define ROKE_COMMON_PATHUTIL_H

/**
 *
 * @file roke/common/pathutil.h
 * @brief Filesystem path helper functions
 */

#include "roke/common/compat.h"

#define ROKE_PATH_MAX 4096
#define ROKE_NAME_MAX 1024
#define ROKE_RECURSION_DEPTH 1024

#ifdef _WIN32
#define SEP '\\'
#define DSEP "\\"
#else
#define SEP '/'
#define DSEP "/"
#endif

ROKE_INTERNAL_API size_t _abspath_impl(const uint8_t* src, size_t srclen,
                  uint8_t* dst, size_t dstlen,
                  const uint8_t* root, size_t rootlen);
ROKE_INTERNAL_API size_t _abspath(const uint8_t* src, size_t srclen,
                                  uint8_t* dst, size_t dstlen);
ROKE_INTERNAL_API size_t _joinpath(const uint8_t** parts, size_t partslen,
                                   uint8_t* dst, size_t dstlen);

ROKE_INTERNAL_API uint64_t creation_time(uint8_t *path);
ROKE_INTERNAL_API int makedirs(const uint8_t *dir);
ROKE_INTERNAL_API FILE* fopen_safe(const uint8_t * path, const char * mode);
#endif