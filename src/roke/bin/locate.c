
#include "roke/libroke_internal.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "quickly find files by name"},
    {0, 0, 0, "Regular Expression Help:\n"
        "todo\n"},
    {0, 0, 0, "Positional Arguments:"},
    {0, 0, "pattern", "file name pattern."},
    {0, ARGPARSE_OPT, "pattern2", "file path pattern."},

    {0, 0, 0, "Optional Arguments:"},
    {0, 'i', 0, "case insensitive matching"},
    {0, 'I', 0, "case sensitive matching"},
    {0, 'g', 0, "use shell-like glob matching (ignores -i switch)"},
    {0, 'r', 0, "use regular expression matching (ignores -i switch)"},
    {"config", 0, 0, "path to the configuration directory."},

    {0, 0, 0, "Other:"},
    {0, 'v', 0, "verbose"},
    {0, 0, 0, 0},
};

int main(int argc, char** argv)
{
    int err = 0;
    char config_dir[ROKE_PATH_MAX];

    argparser_t *argparse = newArgParse(argc, (const char**) argv, spec);

    char* pconfig = NULL;
    argparser_default_kwarg(argparse, "config", (const char**) &pconfig);
    if (roke_get_config_dir(config_dir, sizeof(config_dir), pconfig)==0) {
        err = 1;
        goto exit;
    }

    int flags = 0;
    if (argparser_get_flag(argparse, 'i')) {
        flags |= ROKE_CASE_INSENSITIVE;
    }

    if (argparser_get_flag(argparse, 'g')) {
        flags |= ROKE_GLOB;
    }

    if (argparser_get_flag(argparse, 'r')) {
        flags |= ROKE_REGEX;
    }

    err = roke_locate(config_dir, argparse->argv + 1, argparse->argc - 1, flags, 0);

  exit:
    argparser_delete(&argparse);
    return err;
}

