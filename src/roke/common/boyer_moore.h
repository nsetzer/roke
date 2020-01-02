

#ifndef ROKE_COMMON_BOYER_MOORE_H
#define ROKE_COMMON_BOYER_MOORE_H

/**
 *
 * @file roke/common/boyer_moore.h
 * @brief fast string comparison implementation
 */

#include "roke/common/compat.h"
#define ALPHABET_LEN 256

/**
 * @brief holds compiled Boyer-Moore settings for fast string matching
 */
typedef struct bmopt {
    int delta1[ALPHABET_LEN];
    int *delta2;
    size_t patlen;
    uint8_t* pat;
} bmopt_t;

ROKE_INTERNAL_API int boyer_moore_init(bmopt_t* bmopt,
	const uint8_t *pat, size_t patlen);
ROKE_INTERNAL_API const uint8_t* boyer_moore_match(bmopt_t* bmopt,
	const uint8_t *string, size_t stringlen);
ROKE_INTERNAL_API void boyer_moore_free(bmopt_t* bmopt);

#endif