#include "roke/common/argparse.h"
#include "roke/common/unittest.h"
#include "roke/common/stack.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "Test the directory stack"},
    {0, 'v', 0, "verbose"},
    {"pattern", 'p', 0, "run tests that match the given glob-like pattern."},
    {0, 0, 0, 0},
};

int
test_stack_push_pop(void) {
    int err = 0;

    rstack_t stack;
    rstack_data_t* data;

    rstack_init(&stack);
    tassert_true(rstack_empty(&stack))

    rstack_push(&stack, 1, 0, (uint8_t*)"a");
    rstack_push(&stack, 2, 0, (uint8_t*)"b");
    rstack_push(&stack, 3, 0, (uint8_t*)"c");

    tassert_false(rstack_empty(&stack))

    data = rstack_head(&stack);
    tassert_str_equal((char*)data->path, "c");
    rstack_pop(&stack);

    data = rstack_head(&stack);
    tassert_str_equal((char*)data->path, "b");
    rstack_pop(&stack);

    data = rstack_head(&stack);
    tassert_str_equal((char*)data->path, "a");
    rstack_pop(&stack);

    tassert_true(rstack_empty(&stack))

    rstack_free(&stack);

  end:
    return err;
}

int
test_stack_resize(void) {
    int err = 0;

    rstack_t stack;
    rstack_init(&stack);

    uint32_t cap = stack.capacity;
    uint32_t i;
    for (i=0; i < cap+10; i++) {
        rstack_push(&stack, 1, 0, (uint8_t*)"a");
    }

    // check that the capacity was increased
    tassert_equal(stack.size, cap+10);
    tassert_lessthan(cap, stack.capacity);

    // this should  free the entire stack -- check with valgrind
    rstack_free(&stack);

  end:
    return err;
}

int
main(int argc, const char *argv[]) {

    begin_test(argc, argv, spec);

    run_test(test_stack_push_pop);
    run_test(test_stack_resize);

    end_test();
}