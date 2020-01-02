

#ifdef _WIN32
    #define setenv(a, b, c) _putenv(a "=" b)
#endif

#include "roke/common/pathutil.h"
#include "roke/common/unittest.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "pathutil test"},
    {0, 'v', 0, "verbose"},
    {"pattern", 'p', 0, "run tests that match the given glob-like pattern."},
    {0, 0, 0, 0},
};

int _abspath_test(char* root, char * src, char* sol) {

    char dst[ROKE_PATH_MAX];
    size_t len =_abspath_impl((uint8_t*)src, strlen(src),
                              (uint8_t*)dst, sizeof(dst),
                              (uint8_t*)root, strlen(root));

    if (strcmp(dst, sol)==0) {
        return 0;
    } else if (strlen(dst) != len) {
        printf(" ERROR %" PFMT_SIZE_T " %" PFMT_SIZE_T "\n", strlen(dst), len);
        printf("source:   %s\n", src);
        printf("actual:   %s\n", dst);
        printf("expected: %s\n", sol);
        return 1;
    } else {
        printf(" ERROR\n");
        printf("source:   %s\n", src);
        printf("actual:   %s\n", dst);
        printf("expected: %s\n", sol);
        return 1;
    }
}

int
pathutil_abspath_test(void) {

    int err = 0;

#ifdef _WIN32
    setenv("USERPROFILE", "C:\\Users", 1);
    #define PREFIX "C:\\test"
    char* root = "C:\\test\\test";

    err += _abspath_test(root, "C:\\"       , "C:"       );
    err += _abspath_test(root, "C:\\a"      , "C:\\a"    );
    err += _abspath_test(root, "C:\\a\\"    , "C:\\a"    );
    err += _abspath_test(root, "C:\\a\\b\\" , "C:\\a\\b" );
    err += _abspath_test(root, "~/abc" , "C:\\Users\\abc");
#else
    setenv("HOME", "/root", 1);
    #define PREFIX "/test"
    char* root = "/test/test";

    err += _abspath_test(root, "/"       , "/");
    err += _abspath_test(root, "/a"      , "/a");
    err += _abspath_test(root, "/a/"     , "/a");
    err += _abspath_test(root, "/a/b/"   , "/a/b");
    err += _abspath_test(root, "~/abc" , "/root/abc");
#endif
    err += _abspath_test(root, ""        , PREFIX DSEP "test");
    err += _abspath_test(root, "a"       , PREFIX DSEP "test" DSEP "a");
    err += _abspath_test(root, "a/"      , PREFIX DSEP "test" DSEP "a");
    err += _abspath_test(root, "."       , PREFIX DSEP "test");
    err += _abspath_test(root, "./a"     , PREFIX DSEP "test" DSEP "a");
    err += _abspath_test(root, "./a/"    , PREFIX DSEP "test" DSEP "a");
    err += _abspath_test(root, "./a/b"   , PREFIX DSEP "test" DSEP "a" DSEP "b");
    err += _abspath_test(root, "./a/b/." , PREFIX DSEP "test" DSEP "a" DSEP "b");
    err += _abspath_test(root, "./a/b/./", PREFIX DSEP "test" DSEP "a" DSEP "b");
    err += _abspath_test(root, ".."      , PREFIX );
    err += _abspath_test(root, "../"     , PREFIX );
    err += _abspath_test(root, "./a/.."  , PREFIX DSEP "test");
    err += _abspath_test(root, "./a/../" , PREFIX DSEP "test");
    err += _abspath_test(root, "./../a"  , PREFIX DSEP "a");
    err += _abspath_test(root, "./../a/" , PREFIX DSEP "a");
    err += _abspath_test(root, ".abc" , PREFIX DSEP "test" DSEP ".abc");
    err += _abspath_test(root, "..abc" , PREFIX DSEP "test" DSEP "..abc");

    return err;
}

int _joinpath_test(const char** parts, size_t partslen, char* sol) {

    uint8_t dst[ROKE_PATH_MAX];

    _joinpath((const uint8_t**)parts, partslen, dst, sizeof(dst));

    if (strcmp((char*)dst, (char*)sol)==0) {
        return 0;
    } else {
        printf(" ERROR\n");
        printf("actual:   %s\n", dst);
        printf("expected: %s\n", sol);
        return 1;
    }
}


int
pathutil_joinpath_test(void) {

    int err = 0;

#ifdef _WIN32
    {
    const char* parts[] = {"C:\\",NULL};
    err += _joinpath_test(parts, 1, "C:");
    }

    {
    const char* parts[] = {"C:\\","test", NULL};
    err += _joinpath_test(parts, 2, "C:\\test");
    }

    {
    const char* parts[] = {"C:\\","test/", NULL};
    err += _joinpath_test(parts, 2, "C:\\test");
    }

    {
    const char* parts[] = {"C:\\","test", "", NULL};
    err += _joinpath_test(parts, 3, "C:\\test");
    }

    {
    const char* parts[] = {"C:\\test/", "file.txt", NULL};
    err += _joinpath_test(parts, 2, "C:\\test\\file.txt");
    }

    {
    const char* parts[] = {"C:\\","test", "", "file.txt", NULL};
    err += _joinpath_test(parts, 4, "C:\\test\\file.txt");
    }

    {
    const char* parts[] = {"~","test", NULL};
    err += _joinpath_test(parts, 2, "~\\test");
    }
#else
    {
    const char* parts[] = {"/",NULL};
    err += _joinpath_test(parts, 1, "/");
    }

    {
    const char* parts[] = {"/","test", NULL};
    err += _joinpath_test(parts, 2, "/test");
    }

    {
    const char* parts[] = {"/","test/", NULL};
    err += _joinpath_test(parts, 2, "/test");
    }

    {
    const char* parts[] = {"/","test", "", NULL};
    err += _joinpath_test(parts, 3, "/test");
    }

    {
    const char* parts[] = {"/test/", "file.txt", NULL};
    err += _joinpath_test(parts, 2, "/test/file.txt");
    }

    {
    const char* parts[] = {"/", "test", "", "file.txt", NULL};
    err += _joinpath_test(parts, 4, "/test/file.txt");
    }

    {
    const char* parts[] = {"~","test", NULL};
    err += _joinpath_test(parts, 2, "~/test");
    }
#endif

    return err;
}
int
main(int argc, const char **argv) {
    begin_test(argc, argv, spec);

    run_test(pathutil_abspath_test);
    run_test(pathutil_joinpath_test);

    end_test();
}