
#include "cache.h"

// Hash function to map int64_t inode to an index
static size_t hash(int64_t inode, size_t capacity) {
    return (size_t)(inode % (int64_t)capacity);
}


// Initialize the cache
int roke_inode_cache_init(roke_inode_cache_t *cache, size_t initial_capacity) {

    cache->table = malloc(initial_capacity * sizeof(int64_t));
    if (!cache->table) {
        return ROKE_INODE_CACHE_ERROR;
    }
    memset(cache->table, 0, initial_capacity * sizeof(int64_t));
    cache->capacity = initial_capacity;
    cache->size = 0;
    return ROKE_INODE_CACHE_SUCCESS;
}

// Free the cache
void roke_inode_cache_free(roke_inode_cache_t *cache) {
    free(cache->table);
    cache->table = NULL;
}

// Resize the cache when load factor is exceeded
int roke_inode_cache_resize(roke_inode_cache_t *cache) {
    size_t new_capacity = cache->capacity * 2;
    int64_t *new_table = malloc(new_capacity * sizeof(int64_t));
    if (!new_table) {
        return ROKE_INODE_CACHE_ERROR;
    }

    memset(new_table, 0, new_capacity * sizeof(int64_t));

    // Rehash all existing values
    for (size_t i = 0; i < cache->capacity; i++) {
        if (cache->table[i] != 0) {  // 0 indicates an empty slot
            int64_t inode = cache->table[i];
            size_t idx = hash(inode, new_capacity);
            while (new_table[idx] != 0) {
                idx = (idx + 1) % new_capacity;  // Linear probing
            }
            new_table[idx] = inode;
        }
    }

    free(cache->table);
    cache->table = new_table;
    cache->capacity = new_capacity;

    return ROKE_INODE_CACHE_SUCCESS;
}

// Add an inode to the cache if it doesn't exist
int roke_inode_cache_insert(roke_inode_cache_t *cache, int64_t inode) {
    if (cache->size >= cache->capacity * ROKE_INODE_CACHE_LOAD_FACTOR) {
        roke_inode_cache_resize(cache);
    }

    size_t idx = hash(inode, cache->capacity);
    if (idx >= cache->capacity) {
        printf("idx: %ld greated than capacity\n", idx);
        return ROKE_INODE_CACHE_ERROR;
    }
    while (cache->table[idx] != 0) {
        if (cache->table[idx] == inode) {
            return ROKE_INODE_CACHE_EXISTS;  // Inode is already in the cache
        }
        idx = (idx + 1) % cache->capacity;  // Linear probing
    }
    if (idx >= cache->capacity) {
        printf("idx: %ld greated than capacity\n", idx);
        return ROKE_INODE_CACHE_ERROR;
    }
    cache->table[idx] = inode;
    cache->size++;
    return ROKE_INODE_CACHE_SUCCESS;
}

// Check if an inode is already in the cache
bool roke_inode_cache_contains(roke_inode_cache_t *cache, int64_t inode) {
    size_t idx = hash(inode, cache->capacity);
    while (cache->table[idx] != 0) {
        if (cache->table[idx] == inode) {
            return true;  // Inode found
        }
        idx = (idx + 1) % cache->capacity;  // Linear probing
    }
    return false;  // Inode not found
}

// Example usage
/*
int main() {
    roke_inode_cache_t cache;
    roke_inode_cache_init(&cache, CACHE_INITIAL_CAPACITY);
    int rv;

    int64_t inodes[] = {12345, 67890, 12345, 54321};  // Sample inode numbers
    for (size_t i = 0; i < sizeof(inodes) / sizeof(inodes[0]); ++i) {
        rv = roke_inode_cache_insert(&cache, inodes[i]);
        if (rv == ROKE_INODE_CACHE_EXISTS) {
            printf("Inode %ld already exists in cache.\n", inodes[i]);
        } else if (rv == ROKE_INODE_CACHE_SUCCESS) {
            printf("Inode %ld added to cache.\n", inodes[i]);
        }
    }

    roke_inode_cache_free(&cache);
    return 0;
}
*/
