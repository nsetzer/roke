
#include "roke/libroke_internal.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "list information about existing indexes"},

    {0, 0, 0, "Optional Arguments:"},
    {"config", 0, 0, "path to the configuration directory."},

    {0, 0, 0, "Other:"},
    {0, 'v', 0, "verbose"},
    {0, 0, 0, 0},
};

/**
 * @brief print basic information about exiting index files
 * @param config_dir null terminated string ending in a path separator
 *                   the directory path containing index files
 * @return
 *
 * Prints the name, root directory, and age of the index
 */
int
roke_list(
    char* config_dir)
{

    char scratch[ROKE_PATH_MAX];
    char name[ROKE_NAME_MAX];
    DIR *d = NULL;
    struct dirent *dir;

    d = opendir(config_dir);
    if (!d) {
        fprintf(stderr, "failed to open config directory\n");
        return 1;
    }

    uint64_t ndirs_total=0, nfiles_total=0;
    while ((dir = readdir(d)) != NULL) {

        // find all databases,
        size_t name_len = strlen(dir->d_name);
        if (!has_suffix((uint8_t*)dir->d_name, name_len, (uint8_t*)".d.bin", 6)) {
            continue;
        }

        strcpy_safe((uint8_t*)name, sizeof(name), (uint8_t*)dir->d_name);
        name[strlen(name) - 6] = '\0';

        roke_index_dirinfo(config_dir, name,
            scratch, sizeof(scratch));

        uint32_t ndirs, nfiles;
		uint64_t mtime;
        roke_index_info(config_dir, name, &ndirs, &nfiles, &mtime);

        uint64_t ntime = time(NULL) - mtime;
        float age = ntime / 60.0f / 60.0f;

        ndirs_total += ndirs;
        nfiles_total += nfiles;

        if (age > 24) {
            age /= 24.0f;
            fprintf(stdout, "%s:\n   root: %s\n   dirs:  %d\n   files: %d\n   age: %.2f days\n", name, scratch, ndirs, nfiles, age);
        } else {
            fprintf(stdout, "%s:\n   root: %s\n   dirs:  %d\n   files: %d\n   age: %.2f hours\n", name, scratch, ndirs, nfiles, age);
        }

    }

    fprintf(stdout, "total dirs: %" PRIu64 "\ntotal files: %" PRIu64 "\n", ndirs_total, nfiles_total);


    if (d != NULL) {
        closedir(d);
    }

    return 0;
}

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

    fprintf(stdout, "%s\n", config_dir);

    roke_list(config_dir);

exit:
    argparser_delete(&argparse);
    return err;
}