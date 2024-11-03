#ifndef ROKE_COMMON_CACHE_H
#define ROKE_COMMON_CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

//#define CACHE_INITIAL_CAPACITY 256  // Initial size for hash table; can be resized as needed
#define ROKE_INODE_CACHE_LOAD_FACTOR 0.75            // Load factor to trigger resizing

typedef struct {
    int64_t *table;          // Array for storing inode numbers
    size_t capacity;         // Current capacity of the table
    size_t size;             // Number of entries currently in the cache
} roke_inode_cache_t;

int roke_inode_cache_init(roke_inode_cache_t *cache, size_t initial_capacity);
void roke_inode_cache_free(roke_inode_cache_t *cache);
int roke_inode_cache_resize(roke_inode_cache_t *cache);
int roke_inode_cache_insert(roke_inode_cache_t *cache, int64_t inode);
bool roke_inode_cache_contains(roke_inode_cache_t *cache, int64_t inode);

#define ROKE_INODE_CACHE_ERROR -1
#define ROKE_INODE_CACHE_SUCCESS 0
#define ROKE_INODE_CACHE_EXISTS 1

#endif