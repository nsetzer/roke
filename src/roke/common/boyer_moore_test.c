#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roke/common/argparse.h"
#include "roke/common/unittest.h"
#include "roke/common/boyer_moore.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "boyer moore test"},
    {0, 'v', 0, "verbose"},
    {"pattern", 'p', 0, "run tests that match the given glob-like pattern."},
    {0, 0, 0, 0},
};

#define bmmatch(s) boyer_moore_match(&bmopt, (uint8_t*) s, strlen((char*)s))

int
boyer_moore_match_0_test(void) {

    int err = 0;
    bmopt_t bmopt;
    uint8_t * pat = (uint8_t*) "abc";

    boyer_moore_init(&bmopt, pat, strlen((char*)pat));

    tassert_nonnull(bmmatch("abc"));
    tassert_nonnull(bmmatch("   abc"));
    tassert_nonnull(bmmatch("abc   "));

    tassert_null(bmmatch("def"));
    tassert_null(bmmatch("   def"));
    tassert_null(bmmatch("def   "));

    tassert_null(bmmatch("ABC"));

    boyer_moore_free(&bmopt);
end:
    return err;
}

int
boyer_moore_match_utf8_test(void) {

    int err = 0;
    uint8_t * str = (uint8_t*) "\xe3\x83\x87\xe3\x83\xb3\xe3\x82\xb8"
                               "\xe3\x83\xa3\xe3\x83\xbc\xe2\x98\x86"
                               "\xe3\x82\xae\xe3\x83\xa3\xe3\x83\xb3"
                               "\xe3\x82\xb0";
    uint8_t * pat = (uint8_t*) "\xe3\x83\x87\xe3\x83\xb3\xe3\x82\xb8"
                               "\xe3\x83\xa3\xe3\x83\xbc";
    bmopt_t bmopt;

    boyer_moore_init(&bmopt, pat, strlen((char*)pat));
    tassert_nonnull(bmmatch(str));
    boyer_moore_free(&bmopt);

  end:
    return err;
}

int
main(int argc, const char **argv) {
    begin_test(argc, argv, spec);

    run_test(boyer_moore_match_0_test);
    run_test(boyer_moore_match_utf8_test);

    end_test();
}