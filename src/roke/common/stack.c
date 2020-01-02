

#include "roke/common/stack.h"

void rstack_init(rstack_t *stack)
{
    stack->size = 0;
    stack->capacity = 1024;
    stack->data = malloc(sizeof(rstack_data_t) * stack->capacity);
}

void rstack_free(rstack_t *stack)
{
    uint32_t i;
    if (stack!=NULL) {
        for (i=0; i<stack->size; i++) {
            free(stack->data[i].path);
        }
        free(stack->data);
    }
    stack->size = 0;
    stack->capacity = 0;
}

int rstack_empty(rstack_t *stack)
{
    return stack->size == 0;
}

int rstack_push(rstack_t *stack, uint32_t index, uint32_t depth, const uint8_t* path)
{

    if (stack->size == stack->capacity) {
        stack->capacity += 512;
        rstack_data_t* temp = realloc(stack->data,
                                      sizeof(rstack_data_t) * stack->capacity);
        if (!temp) {
            rstack_free(stack);
            return -1;
        }

        stack->data = temp;
    }

    rstack_data_t* item = &stack->data[stack->size++];

    item->index = index;
    item->depth = depth;
    item->path = strdup_safe(path);

    return 0;
}

void rstack_pop(rstack_t *stack)
{
    if (stack->size > 0) {
        rstack_data_t* item = &stack->data[--stack->size];
        free(item->path);
    }
}

rstack_data_t* rstack_head(rstack_t *stack)
{

    if (stack->size == 0) {
        return NULL;
    }

    return &stack->data[stack->size-1];
}