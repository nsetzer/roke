

#ifndef ROKE_COMMON_COMPAT_H
#define ROKE_COMMON_COMPAT_H
/**
 *
 * @file roke/common/compat.h
 * @brief provide multi-platform compatability
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

// defined for fileno(), MAP_, and DT_ defines
// dump default compiler defines
// gcc -dM -E - < /dev/null | grep GNU
#ifdef __APPLE__
// apple does not define MAP_POPULATE, and it is not necessarily needed.
#define MMAP_FLAGS MAP_PRIVATE
#include <sys/types.h>
#include <sys/dir.h>
#else
#define MMAP_FLAGS (MAP_PRIVATE | MAP_POPULATE)

// for fileno (defined in stdio.h)
#ifdef __MINGW64__
#define _POSIX_SOURCE
#endif

//#ifdef __GNUC_RH_RELEASE__
//#define _BSD_SOURCE
//#endif

// centos won't compile without the _BSD_SOURCE flag set
// arch complains about the flag, and suggests _DEFAULT_SOURCE
// this check was made by comparing various default defines in gcc
// but may not be valid for all redhat versions
// gcc -dM -E - < /dev/null | grep GNU
// /opt/rh/devtoolset-2/root/usr/bin/gcc -dM -E - < /dev/null | grep GNU
//#ifdef __GNUC_RH_RELEASE__
//#define _BSD_SOURCE
//#else
//#define _DEFAULT_SOURCE
//#endif

#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#if defined(_MSC_VER)
    #include "dirent/dirent.h"
    #include <direct.h> // _mkdir
#elif defined(_WIN32)
    #include "dirent/dirent_msys.h"
#else
    #include <unistd.h>
    #include <dirent.h>
    #define stat_utf8 stat
    #define stat64_utf8 stat
    #define stat64_t stat
#endif


#ifndef _WIN32
    #include <sys/wait.h>
#endif

#include <string.h>
#include <ctype.h>

#ifdef __APPLE__
    #include <mach/error.h>
#else
	#if ! defined(_MSC_VER)
		#include <error.h>
	#endif
#endif

#include <errno.h>
#include <sys/stat.h>

#include <limits.h>
#include <stddef.h>

#if defined(__APPLE__) || defined(__clang__)
    // EEXIST, EACCESS, errno
    #include <errno.h>
#endif

#ifdef _WIN32
#include <mman/mman.h>
#define MAP_POPULATE 0
#else
#include <sys/mman.h>
#endif

#ifdef _WIN32
    // hmm...
    #define PFMT_SIZE_T "zu"

    #define roke_getcwd(dst, dstlen) _getcwd(dst, dstlen)
    #define roke_fileno(x) _fileno(x)
    #define roke_fdopen(_fd,mode) _fdopen(_fd,mode)

    #include <io.h>
    #define roke_isatty(_fn) _isatty(_fn)
#else
    #define PFMT_SIZE_T "zu"

    #define roke_getcwd(dst, dstlen) getcwd(dst, dstlen)
    #define roke_fileno(x) fileno(x)
    #define roke_fdopen(_fd,mode) fdopen(_fd,mode)
    #define roke_isatty(_fn) isatty(_fn)
#endif

#include <time.h>
#include <math.h>


#endif
