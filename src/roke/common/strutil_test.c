
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roke/common/unittest.h"
#include "roke/common/strutil.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "pathutil test"},
    {0, 'v', 0, "verbose"},
    {"pattern", 'p', 0, "run tests that match the given glob-like pattern."},
    {0, 0, 0, 0},
};

#define utf8_strglob(a,b) strglob((uint8_t*)a, (uint8_t*)b)

int
strutil_utf8_strglob_test(void) {

    int err = 0;

    tassert_zero(utf8_strglob("aaa", "aaa"));
    tassert_zero(utf8_strglob("a*a", "a123a"));
    tassert_zero(utf8_strglob("a*a", "a.a"));
    tassert_zero(utf8_strglob("a?a", "aaa"));
    tassert_zero(utf8_strglob("???", "aaa"));
    tassert_zero(utf8_strglob("\\a", "a"));

    tassert_zero(utf8_strglob("*", "abc"));
    tassert_zero(utf8_strglob("a*", "abc"));
    tassert_zero(utf8_strglob("ab*", "abc"));
    tassert_zero(utf8_strglob("*c", "abc"));
    tassert_zero(utf8_strglob("*bc", "abc"));

    // test multiple wild cards
    tassert_zero(utf8_strglob("a*b*c", "a123b123c"));
    tassert_zero(utf8_strglob("*a*", "123a123"));

    tassert_nonzero(utf8_strglob("abc", "123"));
    tassert_nonzero(utf8_strglob("abc", ""));
    tassert_nonzero(utf8_strglob("*", ""));
    tassert_nonzero(utf8_strglob("?", ""));

    // test escape sequences
    tassert_zero(utf8_strglob("*\\.c", "test.c"));
    tassert_nonzero(utf8_strglob("*\\.c", "test.h"));
    tassert_nonzero(utf8_strglob("*\\.c", "test"));

    // test greedy match
    tassert_zero(utf8_strglob("*c", "struct.c"));

end:
    return err;
}
/*
uint8_t* tolowercase2(uint8_t* str)
{
      utf8proc_ssize_t inplen = strlen((char*) str);
      utf8proc_int32_t codepoint;
      utf8proc_ssize_t count;
      utf8proc_uint8_t* inptmp = str;

      // an extra byte for null, and 4 extra bytes for utf-8
      utf8proc_ssize_t outlen = 1 + 4 + inplen;
      utf8proc_uint8_t* out = malloc(sizeof(utf8proc_uint8_t) * outlen);
      utf8proc_uint8_t* tmpout = out;
      utf8proc_ssize_t total=0;

      // decode the first code point in the string
      count = utf8proc_iterate(inptmp, inplen, &codepoint);
      while (inplen>0 && codepoint!=-1) {
        inplen -= count;
        inptmp += count;

        // convert the code point to lowercase if possible
        codepoint = utf8proc_tolower(codepoint);

        // resize the output array as needed
        if (outlen - total < 4) {
            outlen = outlen + (outlen > 1);
            out = realloc(out, sizeof(utf8proc_uint8_t) * outlen);
            if (!out) {
                return NULL;
            }
            tmpout = out + total;
        }
        // reencode the character to a byte string
        count = utf8proc_encode_char(codepoint, tmpout);
        tmpout += count;
        total += count;

        // decode the next code point
        count = utf8proc_iterate(inptmp, inplen, &codepoint);
      }

      return out;
}
*/

int test_string_escape(void)
{
    int err = 0;

    uint8_t buffer[32];

    strcpy((char*)buffer, "\\x20");
    strescape(buffer, 4);
    tassert_str_equal((char*)buffer, " ");

    strcpy((char*)buffer, "\\xd0\\x9c\\xd0\\x98\\xd0\\xa0");
    strescape(buffer, 4*6);
    tassert_str_equal((char*)buffer, "\xd0\x9c\xd0\x98\xd0\xa0");

    /*uint8_t * tmp = buffer;
    printf("\n[");
    while (*tmp != '\0')
        printf(" %02X", *tmp++);
    printf("]\n");*/

   end:
     return err;
}

define_test(strutil_utf8_lowercase)
{

    uint8_t* str = (uint8_t*) "HELLO WORLD";
    uint8_t* exp = (uint8_t*) "hello world";
    uint8_t buf[1024];

    tolowercase(buf, sizeof(buf), str);

    tassert_str_equal((char*) buf, (char*) exp);

    exit_test();
}

int
main(int argc, const char **argv) {
    begin_test(argc, argv, spec);

    run_test(strutil_utf8_strglob_test);
    run_test(test_string_escape);
	run_test(strutil_utf8_lowercase);

    end_test();
}