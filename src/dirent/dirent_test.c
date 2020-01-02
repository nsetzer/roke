
#include "roke/libroke_internal.h"
#include "roke/common/unittest.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "libroke test"},
    {0, 0, "source_directory", "source directory"},

    {0, 0, 0, "Optional Arguments"},
    {0, 'v', 0, "verbose"},
    {"pattern", 'p', 0, "run tests that match the given glob-like pattern."},
    {0, 0, 0, 0},
};

int test_dirent_list(const char* source_directory)
{
    int err=0;
    DIR *d = NULL;
    struct dirent *dir;
    uint8_t temp_path[ROKE_PATH_MAX];

    d = opendir(source_directory);
    tassert_nonnull(d);

    int found_abc = 0;
    int found_a = 0;
    int found_n = 0;

    int count = 0;
    while ((dir = readdir(d)) != NULL) {

        int is_dir = 0;
        off_t f_size = 0;

        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
            continue;
        }

        // check that any sub directories can also be opened.
        {
            // todo check for errors
            const uint8_t* parts[] = { (uint8_t*)source_directory,
                                       (uint8_t*)dir->d_name };
            _joinpath(parts, 2, temp_path, sizeof(temp_path));
        }

        fflush(stdout);

        tassert_zero(roke_dirent_info(dir, temp_path, &is_dir, &f_size));

        // the test directory is expected to have specially named files

        if (strstr(dir->d_name, "abc") != NULL)
            found_abc = 1;

        if (strstr(dir->d_name, "\xe3\x81\x82") != NULL)
            found_a = 1;

        if (strstr(dir->d_name, "\xe3\x81\xae") != NULL)
            found_n = 1;

        if (is_dir) {
            DIR *d2 = opendir((char*)temp_path);
            tassert_nonnull(d2);
            closedir(d2);
        }

        if (++count > 10)
            break;

    }

    closedir(d);

    // the counter prevents infinite loops,
    // if an infinite loop occured, this test will catch it
    // bad readdir implementations will trip this alarm
    tassert_true(count < 10);

    tassert_true(found_abc);
    // prove that unicode directories can be listed correctly
    // without wide character support on windows, these would not be found
    tassert_true(found_a);
    tassert_true(found_n);

  end:
    return err;
}

int
main(int argc, const char **argv) {
    begin_test(argc, argv, spec);

    char source_dir[ROKE_PATH_MAX];
    size_t len;
    len = _abspath((uint8_t*)argparse->argv[1], strlen(argparse->argv[1]),
                   (uint8_t*)source_dir, sizeof(source_dir));
    if (len >= sizeof(source_dir)) {
        fprintf(stderr, "source directory path too long.\n");
        error = 1;
        goto exit;
    }

    run_test(test_dirent_list, source_dir);

  exit:
    end_test();
}