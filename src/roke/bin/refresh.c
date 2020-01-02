
#include "roke/libroke_internal.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "rebuild existing indexes"},

    {0, 0, 0, "Positional Arguments:"},
    {0, ARGPARSE_OPTMANY, "name", "the name of an index"},

    {0, 0, 0, "Optional Arguments:"},
    {"config", 0, 0, "path to the configuration directory."},

    {0, 0, 0, "Other:"},
    {0, 'v', 0, "verbose"},
    {0, 0, 0, 0},
};

char* blacklist[] = {".", "..", ".git", ".svn", ".dropbox", ".dropbox.cache", NULL};

/**
 * @brief rebuild all index files found in the config directory
 * @param config_dir null terminated string ending in a path separator
 *                   the directory path containing index files
 * @param blacklist null terminated list of strings. directories that exactly
 *                  match a name in this list will not be searched.
 * @return
 *
 */
int
roke_rebuild_all(
    char* config_dir,
    char** blacklist)
{

    char name[256];
    DIR *d = NULL;
    struct dirent *dir;

    d = opendir(config_dir);
    if (!d) {
        fprintf(stderr, "failed to open config directory\n");
        return 1;
    }

    while ((dir = readdir(d)) != NULL) {

        // find all databases,
        size_t name_len = strlen(dir->d_name);
        if (!has_suffix((uint8_t*)dir->d_name, name_len, (uint8_t*)".d.bin", 6)) {
            continue;
        }

        strcpy_safe((uint8_t*)name, sizeof(name), (uint8_t*)dir->d_name);
        name[strlen(name)-6] = '\0';

        roke_rebuild_index(config_dir, name, blacklist);

    }

    if (d != NULL) {
        closedir(d);
    }

    return 0;
}

int main(int argc, char** argv)
{
    int err=0;
    int i;
    char config_dir[ROKE_PATH_MAX];

    argparser_t *argparse = newArgParse(argc, (const char**) argv, spec);

    char* pconfig = NULL;
    argparser_default_kwarg(argparse, "config", (const char**) &pconfig);
    if (roke_get_config_dir(config_dir, sizeof(config_dir), pconfig)==0) {
        err = 1;
        goto exit;
    }

    if (argparse->argc == 1) {
        err += roke_rebuild_all(config_dir, blacklist);
    } else {
        for (i=1; i<argparse->argc; i++) {
            char * name = (char*) argparse->argv[i];
            err += roke_rebuild_index(config_dir, name, blacklist);
        }
    }

exit:
    argparser_delete(&argparse);
    return err;
}