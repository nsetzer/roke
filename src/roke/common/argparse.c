//#! gcc -std=c11 $this -o argparse.exe && argparse.exe -vvv  --help --kind=0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roke/common/argparse.h"

// #define diag()
// #define diag() if (logging_verbosity>1){ printf(); }

/*
    specification:
        flags: e.g. -h
        keyval pairs e.g. --key=value
        comsumming flags: e.g. -l mylib or -lmylib
        flag smashing: e.g. -abcd or -abcl mylib or -abcdl=mylib)
    defaults:
        -h --help : print help
        -v verbosity
*/

// cflags converts A-Z to 1-26 and a-z to 27-53 or returns 0
// rcflags reverses the operation
#define cflags(c) \
    ((c >= 'A' && c <= 'Z') ? (c - 'A' + 1) : \
                              (c >= 'a' && c <= 'z') ? (c - 'a' + 27) : 0)
#define rcflags(c) \
    ((c >= 1 && c <= 26) ? (c - 1 + 'A') : \
                           (c > 26 && c < 53) ? (c - 27 + 'a') : \
                                                '!')

void
_set_kwargs(argparser_t *argparse, const char *key, const char *value) {
    size_t i;
    for (i = 0; i < argparse->kwargc; i++) {
        if (strcmp(argparse->kwarg[i].key, key) == 0) {
            if (argparse->kwarg[i].val)
                free((void *) argparse->kwarg[i].val);
            argparse->kwarg[i].val = (const char *) strdup_safe((uint8_t*) value);
        }
    }
}

// returns true for a null struct.
int
_is_zero_spec(argparse_spec_t *s) {
    return ((s->opt_long == NULL) && (s->opt_short == 0) && (s->type == NULL) &&
            (s->helpmsg == NULL));
}

const char *
_spec_has_flag(argparse_spec_t *spec, char flag) {
    int i;
    if (flag == 'h')
        return "help";
    for (i = 0; !_is_zero_spec(&spec[i]); i++) {
        if (spec[i].opt_short == flag)
            return spec[i].opt_long;
    }
    return NULL;
}

int
_parse(int argc,
       const char *argv[],
       int idx,
       argparser_t *argparse,
       argparse_spec_t *spec) {
    int consumed = 0;
    size_t i = 0, j = 0;
    size_t len = 0;
    // char flag;
    const char *arg = argv[idx] + 1;
    char keybuf[128]; // temporary
    len             = strlen(arg);
    const char *key = NULL;
    key = _spec_has_flag(spec, *arg);
    if (*arg == '-') { // scan for a keyword argument
        arg++;
        for (j = 0; arg[j] != 0 && arg[j] != '=' && j < 127;)
            j++;
        memcpy(keybuf, arg, j);
        keybuf[j] = 0;

        if (len == j + 1)
            // undefined behavior if passing arg+j+1,
            // usually grabs the next argv. instead of empty behavior.
            _set_kwargs(argparse, keybuf, "true");
        else
            _set_kwargs(argparse, keybuf, arg + j + 1);
    } else if (key != NULL && len > 1) {
        // consume single letter flags of type -lmylib
        _set_kwargs(argparse, key, arg + 1);

    } else {

        for (i = 0; i < len - 1; i++) {
            if (*(arg + i + 1) == '=')
                break;
            // flag = arg[i];
            argparse->flags[cflags(*(arg + i))]++;
        }
        // the last single letter flag can consume a value if it is a kwarg
        key = _spec_has_flag(spec, *(arg + i));
        if (key != NULL) {
            if (i + 1 >= len) { // special case if h is given
                _set_kwargs(argparse, key, "");
            } /*else if (*(arg+i+1)=='=') { // if separated by an equals
                 _set_kwargs(argparse,key,arg+i+2);
             } else { // if separated by white space
                 _set_kwargs(argparse,key,argv[idx+1]);
                 consumed += 1;
             }*/
        } else {
            argparse->flags[cflags(*(arg + i))]++;
        }
    }

    return consumed;
}

int
argparser_get_flag(argparser_t *argparse, char flag) {
    return argparse->flags[cflags(flag)];
}

int
argparser_has_kwarg(argparser_t *argparse, const char *key) {
    size_t i;
    for (i = 0; i < argparse->kwargc; i++) {
        if (strcmp(argparse->kwarg[i].key, key) == 0 &&
            argparse->kwarg[i].val != NULL) {
            return 1;
        }
    }
    return 0;
}

const char *
argparser_get_kwarg(argparser_t *argparse, const char *key) {
    size_t i;
    for (i = 0; i < argparse->kwargc; i++) {
        if (strcmp(argparse->kwarg[i].key, key) == 0) {
            return argparse->kwarg[i].val;
        }
    }
    return NULL;
}

void
argparser_default_kwarg(argparser_t *argparse,
                        const char *key,
                        const char** out) {
    const char *val = argparser_get_kwarg(argparse, key);
    if (val != NULL)
        *out = val;
}

void
argparser_default_kwarg_i(argparser_t *argparse, const char *key, int32_t *out) {
    const char *val = argparser_get_kwarg(argparse, key);
    if (val != NULL) {
        *out = atoi(val);
    }
}

void
argparser_default_kwarg_d(argparser_t *argparse, const char *key, double *out) {
    const char *val = argparser_get_kwarg(argparse, key);
    if (val != NULL)
        *out = atof(val);
}

void
_print_help(argparser_t *argparse, argparse_spec_t *spec) {
    int i;
    // for(i=0;!_is_zero_spec(&spec[i]);i++) {
    i = 0;
    if (spec[i].opt_long == NULL && spec[i].opt_short == 0 &&
        spec[i].type == 0) {
        printf("%s\n", spec[i].helpmsg);
        // break;
    }
    //}
    printf("\n");
    printf(" Usage:\n");
    printf("    %s [ -h --opt=value ] ", argparse->argv[0]);
    for (i = 1; !_is_zero_spec(&spec[i]); i++) {
        if (spec[i].opt_long == NULL && spec[i].opt_short <= 0 &&
            spec[i].type != NULL) {
            if (spec[i].opt_short == ARGPARSE_MANY)
                printf("%s... ", spec[i].type);
            else if (spec[i].opt_short == ARGPARSE_OPT)
                printf("[%s] ", spec[i].type);
            else if (spec[i].opt_short == ARGPARSE_OPTMANY)
                printf("[%s...] ", spec[i].type);
            else
                printf("%s ", spec[i].type);
        }
    }
    printf("\n\n");

    for (i = 1; !_is_zero_spec(&spec[i]); i++) {
        if (spec[i].opt_long == NULL && spec[i].opt_short <= 0 &&
            spec[i].type != NULL) {
            printf("  %s : %s\n", spec[i].type, spec[i].helpmsg);
        } else if (spec[i].opt_long == NULL && spec[i].opt_short > 0) {
            printf("  -%c : %s\n", spec[i].opt_short, spec[i].helpmsg);
        } else if (spec[i].opt_long != NULL) {
            if (spec[i].type != NULL) {

                if (spec[i].opt_short != 0)
                    printf("  [-%c] --%s=<%s> : %s\n", spec[i].opt_short,
                           spec[i].opt_long, spec[i].type, spec[i].helpmsg);
                else
                    printf("       --%s=<%s> : %s\n", spec[i].opt_long,
                           spec[i].type, spec[i].helpmsg);
            } else {
                if (spec[i].opt_short != 0)
                    printf("  [-%c] --%s : %s\n", spec[i].opt_short,
                           spec[i].opt_long, spec[i].helpmsg);
                else
                    printf("       --%s : %s\n", spec[i].opt_long,
                           spec[i].helpmsg);
            }
        } else if (spec[i].opt_long == NULL && spec[i].opt_short == 0 &&
                   spec[i].type == 0) {
            printf("\n%s\n", spec[i].helpmsg);
        }
    }
    printf("  [-h] --help : display this message and quit.\n");
    printf("\n");
}

void
argparser_print_args(argparser_t *argparse) {
    size_t i;
    printf("\n");
    for (i = 0; i < argparse->argc; i++) {
        printf(" %" PFMT_SIZE_T ": %s\r\n", i, argparse->argv[i]);
    }
    for (i = 0; i < 53; i++) {
        size_t j = argparse->flags[i];
        if (j > 0)
            printf(" -%c given %" PFMT_SIZE_T " times.\r\n", (char) rcflags(i), j);
    }
    for (i = 0; i < argparse->kwargc; i++) {
        printf(" --%s=%s\r\n", argparse->kwarg[i].key, argparse->kwarg[i].val);
    }
    printf("\n");
}

argparser_t *
newArgParse(int argc, const char *argv[], argparse_spec_t *spec) {
    argparser_t *argparse = malloc(sizeof(argparser_t));
    argparser_init(argparse, argc, argv, spec);
    return argparse;
}

uint8_t
argparser_init(argparser_t *argparse,
               int argc,
               const char *argv[],
               argparse_spec_t *spec) {
    int i, j;
    size_t len;
    size_t spec_size    = 0;
    size_t num_pos_args = 1; // 1 always for exe name

    if (!argparse)
        return 1;

    for (unsigned int k = 0; k < sizeof(argparse->flags); k++)
        argparse->flags[k] = 0;

    argparse->argv                   = malloc(sizeof(char *) * argc);
    argparse->argc                   = 0;
    argparse->argv[argparse->argc++] = (const char*) strdup_safe((uint8_t*) argv[0]);

    // get the length of the spec.
    spec_size = 0;
    int _opt = 0;
    for (i = 0; !_is_zero_spec(&spec[i]); i++) {
        spec_size += spec[i].opt_long != 0;
        // if (spec[i].type&&spec[i].type[0]=='?')
        if (spec[i].opt_short == ARGPARSE_OPTMANY && _opt < 3)
            _opt = 3;
        else if (spec[i].opt_short == ARGPARSE_MANY && _opt < 2) {
            num_pos_args += 1;
            _opt = 2;
        } else if (spec[i].opt_short < 0 && _opt < 1) {
            _opt = 1;
        } else if (_opt > 0 && spec[i].type != NULL &&
                   spec[i].opt_long == NULL && spec[i].opt_short==0) {
            printf(" !! Invalid Arg Spec (item %d '%s' cannot be optional)\n\n",
                   i, spec[i].type);
            goto show_help_and_quit;
        } else
            num_pos_args += (spec[i].opt_long == NULL &&
                             spec[i].opt_short <= 0 && spec[i].type != NULL);
    }
    argparse->argp = num_pos_args;

    argparse->kwarg  = malloc(sizeof(argparse_kvpair_t) * (spec_size + 1));
    argparse->kwargc = (spec_size + 1);
    for (i = 0, j = 0; !_is_zero_spec(&spec[i]) && ((unsigned) j) < spec_size;
         i++) {

        if (spec[i].opt_long == NULL)
            continue;
        argparse->kwarg[j].key = spec[i].opt_long;
        argparse->kwarg[j].val = NULL;
        j++;
    }
    // TODO: check if help is already in the list
    argparse->kwarg[j].key = "help";
    argparse->kwarg[j].val = NULL;
    argparse->kwargc       = j + 1;

    for (i = 1; i < argc; i++) {

        if (*argv[i] == '-') {
            len = strlen((argv[i]));
            if (len > 1) {
                i += _parse(argc, argv, i, argparse, spec);
            } else {
                argparse->argv[argparse->argc++] = (const char*) strdup_safe((uint8_t*) argv[i]);
            }
        } else {
            argparse->argv[argparse->argc++] = (const char*) strdup_safe((uint8_t*) argv[i]);
        }
    }

    if (argparser_get_kwarg(argparse, "help") != NULL)
        goto show_help_and_quit;

    if (argparse->argc < num_pos_args) {
        if (argparse->argc < num_pos_args)
            printf(" !! Missing positional argument (received: %" PRIu64
                   "/%" PRIu64 ")\n\n",
                   (uint64_t)(argparse->argc - 1), (uint64_t)(num_pos_args - 1));
    show_help_and_quit:
        _print_help(argparse, spec);
        exit(1);
    }

    return 0;
}

void
argparser_delete(argparser_t **argparse) {
    argparser_free(*argparse);
    free(*argparse);
    *argparse = NULL;
}

void
argparser_free(argparser_t *argparse) {
    if (argparse) {
        uint32_t i;
        for (i = 0; i < argparse->argc; i++) {
            free((char *) argparse->argv[i]);
        }
        free((char**)argparse->argv); // cast away const
        argparse->argv = NULL;
        if (argparse->kwarg)
            for (i = 0; i < argparse->kwargc; i++)
                free((void *) argparse->kwarg[i].val);
        free(argparse->kwarg);
        argparse->kwarg = NULL;
    }
}
