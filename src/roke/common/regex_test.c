#include "roke/common/argparse.h"
#include "roke/common/unittest.h"
#include "roke/common/regex.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "Test Regular Expressions Parser"},
    {0, 'v', 0, "verbose"},
    {"pattern", 'p', 0, "run tests that match the given glob-like pattern."},
    {0, 0, 0, 0},
};

#define _rematch(str) (regex_match(&regex, (uint8_t*) str, strlen(str)))
#define _recomp(str) (regex_compile(&regex, (uint8_t*) str, strlen(str)))

int
regex_simple_test(void) {
    int err = 0;

    rregex_t regex;
    _recomp("abc");

    tassert_zero(_rematch("abc"));

    tassert_nonzero(_rematch("123"));

    regex_free(&regex);

  end:
    return err;
}


int
regex_inset_test(void) {
    int err = 0;

    rregex_t regex;
    _recomp("[a-zA-Z0-9]+");
    tassert_zero(_rematch("abc"));
    tassert_zero(_rematch("ABC"));
    tassert_zero(_rematch("123"));
    tassert_zero(_rematch("aA0"));

    tassert_nonzero(_rematch(" "));
    tassert_nonzero(_rematch("#"));
    regex_free(&regex);

  end:
    return err;
}

int
regex_outset_test(void) {
    int err = 0;

    rregex_t regex;
    _recomp("[^a-zA-Z0-9]+");
    tassert_nonzero(_rematch("abc"));
    tassert_nonzero(_rematch("ABC"));
    tassert_nonzero(_rematch("123"));
    tassert_nonzero(_rematch("aA0"));

    tassert_zero(_rematch(" "));
    tassert_zero(_rematch("#"));
    regex_free(&regex);

  end:
    return err;
}

int
regex_exception_test(void) {
    int err = 0;

    rregex_t regex;
    _recomp("^.*\\.c$");

    tassert_zero(_rematch("misc.c"));
    tassert_zero(_rematch("test.c"));
    tassert_zero(_rematch("test1.c"));
    tassert_zero(_rematch("test12.c"));
    tassert_zero(_rematch("test123.c"));

    regex_free(&regex);

  end:
    return err;
}


int
main(int argc, const char *argv[]) {

    begin_test(argc, argv, spec);

    run_test(regex_simple_test);
    run_test(regex_inset_test);
    run_test(regex_outset_test);
    run_test(regex_exception_test);

    end_test();
}
