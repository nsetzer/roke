

#ifndef ROKE_COMMON_REGEX_H
#define ROKE_COMMON_REGEX_H

/**
 *
 * @file roke/common/regex.h
 * @brief wrapper around regular expression engines
 */

#include "roke/common/compat.h"

typedef struct rregex {
    void * ptr;
    char* error;
} rregex_t;

ROKE_INTERNAL_API int regex_compile(rregex_t* prex,
    const uint8_t* pattern, size_t patlen);
ROKE_INTERNAL_API int regex_match(rregex_t* prex,
    const uint8_t* str, size_t strlen);
ROKE_INTERNAL_API int regex_free(rregex_t* prex);

#endif
