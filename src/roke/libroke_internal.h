
#ifndef ROKE_LIBROKE_INTERNAL_H
#define ROKE_LIBROKE_INTERNAL_H

/**
 *
 * @file roke/libroke_internal.h
 * @brief internal implementation details for libroke
 */


#include "roke/common/compat.h"
#include "roke/common/boyer_moore.h"
#include "roke/common/regex.h"
#include "roke/common/strutil.h"
#include "roke/common/pathutil.h"
#include "roke/common/stack.h"
#include "roke/common/argparse.h"
#include "roke/common/cache.h"
#include "roke/libroke.h"

#define ROKE_MATCH_MASK (ROKE_GLOB|ROKE_REGEX)

/**
 * @brief A wrapper to support multiple kinds of string matching algorithms.
 */
typedef struct string_matcher {
    int flags;
    union {
        bmopt_t bmopt;
        rregex_t regex;
        uint8_t* glob_pattern;
    } data;
    uint8_t* scratch;
} string_matcher_t;


/**
 * @brief an in-memory file record
 *
 * Represents a file record in a tree-like data structure.
 * a vector of nodes makes up the tree where
 * the index points to a parent in the tree, and the root of the tree
 * is always zero. The actual names are assumed to be stored as a list
 * of strings in memory, relative to some base address. This base address
 * plus the offset will yield a pointer to the name.
 *
 * This struct is used for building the binarized index, which is latter
 * memmory mapped. This avoids needing to use string parsing operations.
 *
 * N.B. the tree is split into two files, the set of directories and the set
 * of files. the root of the tree is the root directory, at index 0 in the
 * directory list. the file list indexes reference entries in the directory
 * list
 */
typedef struct roke_entry {

    uint32_t index;     // the parent index of this file or directory
    uint64_t f_size;    // file: size of the file, directory: sum of contained files
    uint16_t namelen;   // the length of the file name
    uint32_t offset;    // the offset (from the start of the memory address
                        // where the name can be found.

} roke_entry_t;

/**
 * @brief represents a memory mapped file index
 *
 * This struct maintains the set of pointers associated with a memory mapped
 * index file. The index file is a binary file containing a list of
 * roke_entry_t, followed by a list of null terminated string names.
 *
 * opening a roke_index_t will map the file and set the pointers appropriatly
 */
typedef struct roke_index {

    FILE* fp;
    size_t fsize;
    void* data;
    uint32_t nitems;
    roke_entry_t* entries;
    uint8_t* strings;

} roke_index_t;

ROKE_INTERNAL_API int roke_index_open(roke_index_t* idx, uint8_t* path);
ROKE_INTERNAL_API int roke_index_close(roke_index_t* idx);

ROKE_INTERNAL_API int roke_get_config_dir(char* s, size_t slen, char* default_path);
ROKE_INTERNAL_API int roke_set_build_cancel_for_test(int value);
ROKE_INTERNAL_API int roke_parse_entry(uint8_t* str, uint32_t* index, uint64_t* f_size, uint8_t** name);
ROKE_INTERNAL_API int roke_binarize_index(uint8_t* index_path, uint32_t nitems);

ROKE_INTERNAL_API int string_matcher_init(string_matcher_t* matcher,
    const uint8_t* pattern, size_t patlen, int flags);
ROKE_INTERNAL_API int string_matcher_match(string_matcher_t* matcher,
    const uint8_t* pattern, size_t patlen);
ROKE_INTERNAL_API int string_matcher_free(string_matcher_t* matcher);

ROKE_INTERNAL_API int roke_build_index_impl(FILE* output,
    const char* config_dir, const char* name, const char* root,
    char** blacklist, int verbose);

ROKE_INTERNAL_API int roke_rebuild_index(char* config_dir, char* name,
    char** blacklist);

ROKE_INTERNAL_API int roke_locate_impl(FILE* output,
    const uint8_t* config_dir, string_matcher_t** bmopts, int limit);

ROKE_INTERNAL_API int roke_dirent_info(
    struct dirent *dir, uint8_t* path, int* is_dir, off_t* size);

#endif