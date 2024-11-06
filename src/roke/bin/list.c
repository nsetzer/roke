
#include <string.h>

#include "roke/libroke_internal.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "list information about existing indexes"},

    {0, 0, 0, "Optional Arguments:"},
    {"config", 0, 0, "path to the configuration directory."},

    {0, 0, 0, "Other:"},
    {0, 'v', 0, "verbose"},
    {0, 't', 0, "sort by timestamp"},
    {0, 0, 0, 0},
};

typedef struct roke_db_entry_s {
    char* name;
    char* path;
    int ndirs;
    int nfiles;
    float age;
} roke_db_entry_t;

static int sort_by_path(const void *a, const void *b)
{
    const roke_db_entry_t* ea = (const roke_db_entry_t*) a;
    const roke_db_entry_t* eb = (const roke_db_entry_t*) b;
    return strcmp(ea->path, eb->path);
}

static int sort_by_age(const void *a, const void *b)
{
    const roke_db_entry_t* ea = (const roke_db_entry_t*) a;
    const roke_db_entry_t* eb = (const roke_db_entry_t*) b;
    float v = ea->age - eb->age;
    if (v < 0) {
        return -1;
    } else if (v > 0) {
        return 1;
    }
    return 0;
}

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
    char* config_dir,
    int sort_flag)
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

    roke_db_entry_t entries[100];
    memset(entries, 0, sizeof(entries));
    int entry_count;


    while ((dir = readdir(d)) != NULL && entry_count < 100) {

        // find all databases,
        size_t name_len = strlen(dir->d_name);
        if (!has_suffix((uint8_t*)dir->d_name, name_len, (uint8_t*)".d.bin", 6)) {
            continue;
        }

        strcpy_safe((uint8_t*)name, sizeof(name), (uint8_t*)dir->d_name);
        int pos = ((int)strlen(name)) - 6;
        if (pos > 0) {
            name[strlen(name) - 6] = '\0';
        }

        memset(scratch, 0, sizeof(scratch));
        int rv = roke_index_dirinfo(config_dir, name,
            scratch, sizeof(scratch));
        if (rv <= 0) {
            fprintf(stderr, "failed to get index info\n");
            continue;
        }

        uint32_t ndirs, nfiles;
		uint64_t mtime;
        roke_index_info(config_dir, name, &ndirs, &nfiles, &mtime);

        uint64_t ntime = time(NULL) - mtime;
        float age = ntime / 60.0f / 60.0f;

        entries[entry_count].name = strdup(name);
        entries[entry_count].path = strdup(scratch);
        entries[entry_count].ndirs = ndirs;
        entries[entry_count].nfiles = nfiles;
        entries[entry_count].age = age;
        entry_count++;
    }

    // sort the entries by path
    if (sort_flag) {
        qsort(entries, entry_count, sizeof(roke_db_entry_t), sort_by_age);
    } else {
        qsort(entries, entry_count, sizeof(roke_db_entry_t), sort_by_path);
    }

    fprintf(stdout, "%-24s %9s %9s%7s        %s\n", "Name", "#Dirs", "#Files", "Age", "Root");
    for (int i=0; i<entry_count; i++) {
        roke_db_entry_t* entry = &entries[i];

        ndirs_total += entry->ndirs;
        nfiles_total += entry->nfiles;

        if (entry->age > 24) {
            fprintf(stdout, "%-24s %9d %9d %7.2f days  %s\n", entry->name, entry->ndirs, entry->nfiles, entry->age/24.0f, entry->path);
        } else {
            fprintf(stdout, "%-24s %9d %9d %7.2f hours %s\n", entry->name, entry->ndirs, entry->nfiles, entry->age, entry->path);
        }
    }
    fprintf(stdout, "total dirs: %" PRIu64 "\ntotal files: %" PRIu64 "\n", ndirs_total, nfiles_total);

    for (int i=0; i<entry_count; i++) {
        free(entries[i].name);
        free(entries[i].path);
    }

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

    int sort_flag = 0;
    if (argparser_get_flag(argparse, 't')) {
        sort_flag = 1;
    }

    fprintf(stdout, "%s\n", config_dir);

    roke_list(config_dir, sort_flag);

exit:
    argparser_delete(&argparse);
    return err;
}