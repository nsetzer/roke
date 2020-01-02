
#ifndef ROKE_LIBROKE_H
#define ROKE_LIBROKE_H

/**
 *
 * @file roke/libroke.h
 * @brief A library for indexing and searching the filesystem
 */

#ifndef ROKE_API
#if defined(_MSC_VER)
#ifdef roke_EXPORTS
#define ROKE_API __declspec(dllexport)
#define ROKE_INTERNAL_API __declspec(dllexport)
#else
#define ROKE_API __declspec(dllimport)
#define ROKE_INTERNAL_API __declspec(dllimport)
#endif
#else
#define ROKE_API
#define ROKE_INTERNAL_API
#endif
#endif

#include <stdint.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ROKE_CASE_SENSITIVE  0
#define ROKE_CASE_INSENSITIVE  1
#define ROKE_GLOB  2
#define ROKE_REGEX  4

ROKE_API size_t roke_default_config_dir(char* dst, size_t dstlen);

ROKE_API int roke_build_cancel();

ROKE_API int roke_build_index(const char* config_dir,
    const char* name, const char* root, char** blacklist);

ROKE_API int roke_build_index_fd(int fd, const char* config_dir,
    const char* name, const char* root, char** blacklist);

// rename to find
ROKE_API int roke_locate(const char* config_dir, const char** patterns,
    size_t npatterns, int match_flags, int limit);

ROKE_API int roke_locate_fd(int fd, const char* config_dir, const char** patterns,
    size_t npatterns, int match_flags, int limit);

// todo merge these two api calls into 1, populate a structure?

typedef struct roke_info {
    char root[1024];
    uint32_t ndirs;
    uint32_t nfiles;
    uint64_t mtime;
} roke_info_t;

ROKE_API size_t roke_index_dirinfo(
    char* config_dir,
    char* name,
    /*out*/ char* root,
    size_t rootlen);

ROKE_API int roke_index_info(
    char* config_dir,
    char* name,
    /*out*/ uint32_t *ndirs,
    /*out*/ uint32_t *nfiles,
    /*out*/ uint64_t *mtime);

#ifdef __cplusplus
}
#endif
#endif