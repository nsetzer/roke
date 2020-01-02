
#include "roke/libroke_internal.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "build an index"},

    {0, 0, 0, "Positional Arguments:"},
    {0, 0, "name", "the name of the index to build"},
    {0, 0, "root", "directory to scan"},

    {0, 0, 0, "Optional Arguments:"},
    {"config", 0, 0, "path to the configuration directory."},

    {0, 0, 0, "Other:"},
    {0, 'v', 0, "verbose"},
    {0, 0, 0, 0},
};

char* blacklist[] = {".", "..", ".git", ".svn", ".dropbox", ".dropbox.cache", NULL};

int main(int argc, char** argv)
{
    int err=0;
    char config_dir[ROKE_PATH_MAX];
    char root[ROKE_PATH_MAX];
    char* name;

    argparser_t *argparse = newArgParse(argc, (const char**) argv, spec);

    char* pconfig = NULL;
    argparser_default_kwarg(argparse, "config", (const char**) &pconfig);
    if (roke_get_config_dir(config_dir, sizeof(config_dir), pconfig) == 0) {
        err = 1;
        goto exit;
    }

    makedirs((uint8_t*)config_dir);

    _abspath((uint8_t*)argparse->argv[2], strlen(argparse->argv[2]),
             (uint8_t*)root, sizeof(root));

    name = (char*) argparse->argv[1];

    fprintf(stdout, "Building Index: %s %s\n", name, root);
    roke_build_index(config_dir, name, root, blacklist);

exit:
    argparser_delete(&argparse);
    return err;
}