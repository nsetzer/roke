
#ifndef ROKE_COMMON_STACK_H
#define ROKE_COMMON_STACK_H

/**
 *
 * @file roke/common/stack.h
 * @brief Depth-first directory traversal
 */

#include "roke/common/strutil.h"

typedef struct rstack_data {
    uint32_t index;
    uint32_t depth;
    uint8_t* path;
} rstack_data_t;

typedef struct rstack {
    uint32_t size;
    uint32_t capacity;
    rstack_data_t* data;
} rstack_t;

ROKE_INTERNAL_API void rstack_init(rstack_t *stack);
ROKE_INTERNAL_API void rstack_free(rstack_t *stack);
ROKE_INTERNAL_API int rstack_empty(rstack_t *stack);
ROKE_INTERNAL_API int rstack_push(rstack_t *stack, uint32_t index, uint32_t depth, const uint8_t* path);
ROKE_INTERNAL_API void rstack_pop(rstack_t *stack);
ROKE_INTERNAL_API rstack_data_t* rstack_head(rstack_t *stack);

#endif
