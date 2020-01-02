#include "roke/common/argparse.h"
#include "roke/common/unittest.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "Test Argument Parser"},
    {0, 'v', 0, "verbose"},
    {"pattern", 'p', 0, "run tests that match the given glob-like pattern."},
    {0, 0, 0, 0},
};

int
test_assert_ok(void) {
    int err = 0;

    // test that each assertion can return true
    // mostly for showing example usage

    tassert_equal(2,2);
    tassert_notequal(2,4);
    tassert_lessthan(2,4);
    tassert_greaterthan(4,2);
    tassert_true(1);
    tassert_false(0);
    tassert_nonzero(2);
    tassert_zero(0);
    tassert_nonnull(2);
    tassert_null(NULL);

    tassert_str_equal("abc","abc");
    tassert_str_notequal("abc","123");
    tassert_str_in("abc","b");
    tassert_str_notin("abc","2");

    tassert_near(6.0,6.0000001);
    tassert_equal_f(6.0,6.0,.1);
    tassert_notequal_f(6.0,6.2,.1);
    tassert_lessthan_f(6.0,7.0);
    tassert_greaterthan_f(7.0,6.0);

  end:
    return err;
}

int
test_assert_error(void) {
    int err = 0;

    // test error conditions, these assertions should fail
    // this will print an error message if the test succeeds

    tassert_nonzero(tassert_impl(T_EQ, __FILE__, __LINE__,"a", "b", 0, 1));
    tassert_nonzero(tassert_impl_float(T_EQ, __FILE__, __LINE__,"a", "b", 0.0, 1.0, .1));

  end:
    return err;
}

int
test_assert_tolerance(void) {
    int err = 0;

    // test that the floating point compare tolerance can change the result
    // this will print an error message if the test succeeds

    // the inputs are not 'equal'
    tassert_nonzero(tassert_impl_float(T_EQ, __FILE__, __LINE__,
        "a", "b", 5.0, 5.1, 0.01));

    // the inputs are 'equal'
    tassert_zero(tassert_impl_float(T_EQ, __FILE__, __LINE__,
        "a", "b", 5.0, 5.1, .25));

  end:
    return err;
}


int
main(int argc, const char *argv[]) {

    begin_test(argc, argv, spec);

    run_test(test_assert_ok);
    run_test(test_assert_error);
    run_test(test_assert_tolerance);

    end_test();
}