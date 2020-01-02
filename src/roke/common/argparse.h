
#ifndef ROKE_COMMON_ARGPARSE_H
#define ROKE_COMMON_ARGPARSE_H

/**
 *
 * @file roke/common/argparse.h
 * @brief A framework for command line argument parsing
 *
 * Example Spec:
 *    argparse_spec_t spec[] = {
 *        { 0, 0, 0, "program description"},
 *        { 0, 0, "pos1", "1st positional argument"},
 *        { 0, 0, "pos2", "2nd positional argument"},
 *        { 0, ARGPARSE_OPT, "pos3", "optional argument"},
 *        { 0, ARGPARSE_OPTMANY, "pos3", "optional argument repeats"},
 *        { 0, 'a', 0, "optional flag 'a'"},
 *        { "inFile", 'i', "file","parameter that consumes a value"},
 *        { "outFile",  0, "file","parameter with no short cut"},
 *        {0,0,0,0},
 *    }
 *
 */

#include "roke/common/strutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define ARGPARSE_MANY -1
#define ARGPARSE_OPT -2
#define ARGPARSE_OPTMANY -3

typedef struct argparse_spec {
    const char *opt_long;
    const char opt_short;
    const char *type;
    const char *helpmsg;
} argparse_spec_t;

typedef struct argparse_kvpair {
    const char *key;
    const char *val;
} argparse_kvpair_t;

typedef struct argparser {
    const char **argv; // positional arguments
    argparse_kvpair_t *kwarg;     // paired parameters
    unsigned char flags[53];
    size_t argc;
    size_t argp;
    size_t kwargc;
} argparser_t;

// flags return a count of the number of times they were provided
ROKE_INTERNAL_API int argparser_get_flag(argparser_t *argparse, char flag);

ROKE_INTERNAL_API int argparser_has_kwarg(argparser_t *argparse, const char *key);
ROKE_INTERNAL_API const char* argparser_get_kwarg(argparser_t *argparse, const char *key);
// default_* only write to 'out' if a kwarg with key exists.
ROKE_INTERNAL_API void
argparser_default_kwarg(argparser_t *argparse, const char *key, const char** out);
ROKE_INTERNAL_API void
argparser_default_kwarg_i(argparser_t *argparse, const char *key, int32_t *out);
ROKE_INTERNAL_API void
argparser_default_kwarg_d(argparser_t *argparse, const char *key, double *out);

// pretty-print the contents of the ArgParse structure
ROKE_INTERNAL_API void  argparser_print_args(argparser_t *argparse);

ROKE_INTERNAL_API argparser_t* newArgParse(int argc, const char *argv[], argparse_spec_t *spec);
ROKE_INTERNAL_API uint8_t argparser_init(argparser_t *argparse,
                       int argc,
                       const char *argv[],
                       argparse_spec_t *spec);

ROKE_INTERNAL_API void argparser_delete(argparser_t **argparse);
ROKE_INTERNAL_API void argparser_free(argparser_t *argparse);
#endif
