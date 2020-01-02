
#include "roke/libroke_internal.h"
#include "roke/common/unittest.h"
#include "roke/common/pathutil.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "libroke test"},
    {0, 0, "config_directory", "config directory"},
    {0, 0, "source_directory", "source directory"},

    {0, 0, 0, "Optional Arguments"},
    {0, 'v', 0, "verbose"},
    {"pattern", 'p', 0, "run tests that match the given glob-like pattern."},
    {0, 0, 0, 0},
};

int test_build_index(const char* config_directory, const char* source_directory)
{
    int err=0;

    char* blacklist[] = {".", "..", NULL};

    makedirs((uint8_t*)config_directory);

    roke_build_index(config_directory, "test", source_directory, blacklist);

    //const char* patterns[] = {"",};
    //roke_locate(config_directory, patterns, 1, 0, 0);

    tassert_zero(0);

  end:
    return err;
}

int
test_get_config_1(void) {
    int err = 0;
    char dir[ROKE_PATH_MAX];
    tassert_nonzero(roke_default_config_dir(dir, sizeof(dir)));
  end:
    return err;
}

int
test_get_config_2(void) {
    int err = 0;
    char dir1[ROKE_PATH_MAX];
    char dir2[ROKE_PATH_MAX];
    char dir3[ROKE_PATH_MAX];
#ifdef _WIN32
    char *test = "C:\\test\\";
#else
	char *test = "/test/";
#endif
    tassert_nonzero(roke_default_config_dir(dir1, sizeof(dir1)));
    tassert_nonzero(roke_get_config_dir(dir2, sizeof(dir2), NULL));
    tassert_nonzero(roke_get_config_dir(dir3, sizeof(dir3), test));

    tassert_str_equal(dir1, dir2);
    tassert_str_equal(dir3, test);
  end:
    return err;
}


#ifndef _WIN32

int
locate_test_helper(const char* config_dir, char* pattern, char* buffer, size_t bufferlen)
{
    /*
    fork and run locate, collect stdout into the buffer
    write the first match, if any to the buffer, ignoring all other results
    */
    int out_pipe[2];
    pid_t   pid1, pid2;
    int status = 0;

        // make a pipe
    if( pipe(out_pipe) != 0 ) {
      return -2;
    }

    pid1 = fork();

    if (pid1 < 0) {
        return 1;
    } else if (pid1 == 0) {
        // child
        // close read fd
        close(out_pipe[0]);

        // redirect stdout to the pipe
        dup2(out_pipe[1], STDOUT_FILENO);
        close(out_pipe[1]);

        // anything sent to printf should now go down the pipe
        const char* patterns[] = {pattern, NULL};
        roke_locate(config_dir, patterns, 1, 0, 1);
        exit(0);

    } else {

        // parent
        // close write fd
        close(out_pipe[1]);

        pid2 = wait(&status);

        status = ((status&0xFF00) >> 8)&0xFF;

        // todo assert that pid1 and pid2 are the same
        printf(">wait %d/%d\n", pid1, pid2);

        // read from pipe into buffer
        ssize_t len = read(out_pipe[0], buffer, bufferlen-1);
        if (len > 0) {
            buffer[len] = '\0';
        } else {
            buffer[0] = '\0';
        }

    }

    // return an error code if exit status is negative
    return status;
}

int
locate_test_helper_2(const char* config_dir, char* pattern, char* buffer, size_t bufferlen)
{
    /*
    fork and run locate, collect stdout into the buffer
    write the first match, if any to the buffer, ignoring all other results
    */
    int out_pipe[2];

    // make a pipe
    if( pipe(out_pipe) != 0 ) {
      return -2;
    }

    // write results to the pipe
    const char* patterns[] = {pattern, NULL};
    roke_locate_fd(out_pipe[1],config_dir, patterns, 1, 0, 1);

    // read from pipe into buffer
    ssize_t len = read(out_pipe[0], buffer, bufferlen-1);
    if (len > 0) {
        buffer[len] = '\0';
    } else {
        buffer[0] = '\0';
    }

    // return an error code if exit status is negative
    return 0;
}

int
fork_locate_test_1(char* config_base) {
    int err = 0;

    int status;
    char buffer[ROKE_PATH_MAX] = {0};
    uint8_t cfgdir[ROKE_PATH_MAX];

    const uint8_t *parts[] = {(uint8_t*) config_base, (uint8_t*) "linux-v4.16"};
    _joinpath(parts, 2, cfgdir, sizeof(cfgdir));
    printf("read:\n%s\n", cfgdir);

    status = locate_test_helper((char*)cfgdir, "linux", buffer, sizeof(buffer));
    tassert_zero(status);
    printf("read:\n%s\n", buffer);
    tassert_nonnull(strstr(buffer, "linux"));

  end:
    if (err>0) {
        printf("read:\n%s\n", buffer);
    }
    return err;
}

int
fork_locate_test_2(char* config_base) {

    int err = 0;

    int status;
    char buffer[ROKE_PATH_MAX] = {0};
    uint8_t cfgdir[ROKE_PATH_MAX];

    const uint8_t *parts[] = {(uint8_t*) config_base, (uint8_t*) "linux-v4.16"};
    _joinpath(parts, 2, cfgdir, sizeof(cfgdir));
    printf("read:\n%s\n", cfgdir);

    status = locate_test_helper_2((char*)cfgdir, "linux", buffer, sizeof(buffer));
    tassert_zero(status);
    printf("read:\n%s\n", buffer);
    tassert_nonnull(strstr(buffer, "linux"));

  end:
    if (err>0) {
        printf("read:\n%s\n", buffer);
    }
    return err;

}

#endif


int
main(int argc, const char **argv) {
    begin_test(argc, argv, spec);

    char config_dir[ROKE_PATH_MAX];
    char source_dir[ROKE_PATH_MAX];
    size_t len;
    // normalize the input paths

    len = _abspath((uint8_t*)argparse->argv[1], strlen(argparse->argv[1]),
                   (uint8_t*)config_dir, sizeof(config_dir));
    if (len >= sizeof(config_dir)) {
        fprintf(stderr, "config directory path too long.\n");
        error = 1;
        goto exit;
    }
    // add a final slash at the end of the path
    config_dir[len++] = '/';
    config_dir[len] = '\0';

    len = _abspath((uint8_t*)argparse->argv[2], strlen(argparse->argv[2]),
                   (uint8_t*)source_dir, sizeof(source_dir));
    if (len >= sizeof(source_dir)) {
        fprintf(stderr, "source directory path too long.\n");
        error = 1;
        goto exit;
    }


    run_test(test_build_index, config_dir, source_dir);

    run_test(test_get_config_1);
    run_test(test_get_config_2);

#ifndef _WIN32



    run_test(fork_locate_test_1, config_dir);
    run_test(fork_locate_test_2, config_dir);
#endif
  exit:
    end_test();
}