#ifndef ROKE_COMMON_UNITTEST_H
#define ROKE_COMMON_UNITTEST_H

/**
 *
 * @file roke/common/unittest.h
 * @brief framework for writing unit tests in C
 */

#include "roke/common/compat.h"
#include "roke/common/argparse.h"
#include "roke/common/strutil.h"

ROKE_INTERNAL_API extern int verbose_logging;

#define tprintf(fmt,...)                                                       \
    if (verbose_logging > 0) {                                                 \
        fprintf(stderr, fmt, ##__VA_ARGS__);                                   \
    };

#define begin_test(_argc, _argv, _spec)                                        \
    int error           = 0;                                                   \
    argparser_t *argparse = newArgParse(_argc, _argv, _spec);                  \
    verbose_logging = argparser_get_flag(argparse, 'v');                       \
    tprintf("Test %s begin.\n", _argv[0]);

#define run_test(test, ...)                                                    \
    if (!argparser_has_kwarg(argparse, "pattern") ||                           \
        strglob((uint8_t*)argparser_get_kwarg(argparse, "pattern"),            \
                (uint8_t*) #test)==0) {                                        \
        tprintf("%s", "\n *** running " #test "\n");                           \
        if ((error = test(__VA_ARGS__)) != 0) {                                \
            fprintf(stderr, "%s", "\n *** Test " #test " failed.\n");          \
            goto end;                                                          \
        }                                                                      \
    } else {                                                                   \
        tprintf("%s", "\n *** skipping " #test "\n");                          \
    }

#define end_test(...)                                                          \
    end:                                                                       \
    argparser_delete(&argparse);                                               \
    if (error || verbose_logging) { fprintf(stderr, "\n"); }                   \
    fprintf(stderr, "Test %s exited with status %d\n", argv[0], error);        \
    return error;

#define T_EQ 0
#define T_NE 1
#define T_LT 2
#define T_GT 3

ROKE_INTERNAL_API int tassert_impl(int mode, const char *filename, int line,
	const char* sx, const char* sy,
    int64_t x, int64_t y);

ROKE_INTERNAL_API int tassert_impl_float(int mode, const char *filename,
             int line, const char* sx, const char* sy,
             double x, double y, double tolerance);

#define tassert_macro_impl(m, x, y)                                            \
    if (tassert_impl(m, __FILE__, __LINE__,                                    \
        #x, #y, (int64_t)(x), (int64_t)(y))) {                                 \
        err = 1;                                                               \
        goto end;                                                              \
    }

#define tassert_macro_impl_f(m, x, y, t)                                       \
    if (tassert_impl_float(m, __FILE__, __LINE__,                              \
        #x, #y, (double)(x), (double)(y), t)) {                                \
        err = 1;                                                               \
        goto end;                                                              \
    }

// (signed) integer comparisons
#define tassert_equal(x,y)        tassert_macro_impl(T_EQ,x,y)
#define tassert_notequal(x,y)     tassert_macro_impl(T_NE,x,y)
#define tassert_lessthan(x,y)     tassert_macro_impl(T_LT,x,y)
#define tassert_greaterthan(x,y)  tassert_macro_impl(T_GT,x,y)
#define tassert_true(x)           tassert_macro_impl(T_NE,x,0)
#define tassert_false(x)          tassert_macro_impl(T_EQ,x,0)
#define tassert_nonzero(x)        tassert_macro_impl(T_NE,x,0)
#define tassert_zero(x)           tassert_macro_impl(T_EQ,x,0)
#define tassert_nonnull(x)        tassert_macro_impl(T_NE,x,NULL)
#define tassert_null(x)           tassert_macro_impl(T_EQ,x,NULL)

// string comparisons
#define tassert_str_equal(a,b)    tassert_zero(strcmp(a,b))
#define tassert_str_notequal(a,b) tassert_nonzero(strcmp(a,b))
#define tassert_str_in(a,b)       tassert_nonnull(strstr(a,b))
#define tassert_str_notin(a,b)    tassert_null(strstr(a,b))

// floating point comparisons
#define tassert_near(x,y)             tassert_macro_impl_f(T_EQ,x,y,0.000001)
#define tassert_equal_f(x,y,t)        tassert_macro_impl_f(T_EQ,x,y,t)
#define tassert_notequal_f(x,y,t)     tassert_macro_impl_f(T_NE,x,y,t)
#define tassert_lessthan_f(x,y)     tassert_macro_impl_f(T_LT,x,y,0)
#define tassert_greaterthan_f(x,y)  tassert_macro_impl_f(T_GT,x,y,0)

ROKE_INTERNAL_API void print_buffer(const char *prefix, size_t len, const uint8_t *buf);

#define define_test(testname, ...) \
    int testname(##__VA_ARGS__) {  \
        int err=0;

#define exit_test() \
    } end: \
    return err;

#endif