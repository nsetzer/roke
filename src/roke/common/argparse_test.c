
#include "roke/common/argparse.h"
#include "roke/common/unittest.h"

argparse_spec_t spec[] = {
    {0, 0, 0, "Test Argument Parser"},
    {0, 'v', 0, "verbose"},
    {"pattern", 'p', 0, "run tests that match the given glob-like pattern."},
    {0, 0, 0, 0},
};

int
parse_positional_simple(void) {
    int err = 0;
    argparser_t *args;
    argparse_spec_t test_spec[] = {
        {0, 0, 0, "Test Argument Parser"},
        {0, 0, 0, 0},
    };

    // expect fail
    tassert_nonzero(argparser_init(NULL, 0, NULL, NULL));

    const char *argv1[] = {"dummy.exe", "one", "two", "three"};
    args                = newArgParse(4, argv1, test_spec);

    tassert_str_equal("one", args->argv[1]);

    argparser_delete(&args);

  end:
    return err;
}

int
parse_flag_simple(void) {
    int err = 0;
    argparser_t *args;
    argparse_spec_t test_spec[] = {
        {0, 0, 0, "Test Argument Parser"},
        {0, 'v', 0, "verbose"},
        {0, 0, 0, 0},
    };

    const char *argv1[] = {"dummy.exe", "-vvv"};
    args = newArgParse(2, argv1, test_spec);
    tassert_equal(argparser_get_flag(args, 'v'), 3);
    argparser_delete(&args);

    const char *argv2[] = {"dummy.exe", "-v", "-v", "-v"};
    args = newArgParse(4, argv2, test_spec);
    tassert_equal(argparser_get_flag(args, 'v'), 3);
    argparser_delete(&args);

  end:
    return err;
}

int
parse_kwarg_simple(void) {
    int err = 0;
    argparser_t *args;
    int32_t tmp;
    argparse_spec_t test_spec[] = {
        {0, 0, 0, "Test Argument Parser"},
        {"verbose", 'v', 0, "verbose"},
        {0, 0, 0, 0},
    };

    const char *argv1[] = {"dummy.exe", "--verbose=3"};
    args                = newArgParse(2, argv1, test_spec);
    tmp = 42;
    argparser_default_kwarg_i(args, "verbose", &tmp);
    tassert_equal(tmp, 3);
    argparser_delete(&args);

    // this should fail
    const char *argv4[] = {"dummy.exe", "--verbose", "3"};
    args                = newArgParse(3, argv4, test_spec);
    tmp = 42;
    argparser_default_kwarg_i(args, "verbose", &tmp);
    tassert_equal(tmp, 0);
    argparser_delete(&args);

    const char *argv2[] = {"dummy.exe", "-v2"};
    args                = newArgParse(2, argv2, test_spec);
    tmp = 42;
    argparser_default_kwarg_i(args, "verbose", &tmp);
    tassert_equal(tmp, 2);
    argparser_delete(&args);

    // this should fail
    const char *argv3[] = {"dummy.exe", "-v", "7"};
    args                = newArgParse(3, argv3, test_spec);
    tmp = 42;
    argparser_default_kwarg_i(args, "verbose", &tmp);
    tassert_equal(tmp, 0);
    argparser_delete(&args);
  end:
    return err;
}

int
parse_kwarg_hard(void) {

    // const char *argv3[] = {"dummy.exe","-abcv7"};
    // set a b c to 1 and v equal to 7

    int err = 0;
    argparser_t *args;
    int32_t tmp;
    double tmpd;
    const char *tmps;

    argparse_spec_t test_spec[] = {
        {0, 0, 0, "Test util.c functions"},
        {"verbose", 'v', 0, "verbose"},
        {0, 0, 0, 0},
    };

    const char *argv1[] = {"dummy.exe", "-v", "1", "--verbose=2"};
    args                = newArgParse(4, argv1, test_spec);
    tmp                 = 42;

    argparser_default_kwarg_i(args, "verbose", &tmp);
    tassert_equal(tmp, 2);

    argparser_default_kwarg_d(args, "verbose", &tmpd);
    tassert_near(tmpd, 2)

    argparser_default_kwarg(args, "verbose", &tmps);
    tassert_str_equal(tmps, "2");

    tassert_equal(argparser_has_kwarg(args, "verbose"), 1)
    tassert_equal(argparser_has_kwarg(args, "dne"), 0)
    tassert_null(argparser_get_kwarg(args, "dne"))

    argparser_delete(&args);

  end:
    return err;
}

int
parse_kwarg_hard_v2(void) {

    // const char *argv3[] = {"dummy.exe","-abcv7"};
    // set a b c to 1 and v equal to 7

    int err = 0;
    argparser_t *args;
    int32_t tmp=0;
    const char* tmps=NULL;

    argparse_spec_t test_spec[] = {
        {0, 0, 0, "Test util.c functions"},
        {"verbose", 'v', 0, "verbose"},
        {0, 0, 0, 0},
    };

    const char *argv1[] = {"dummy.exe", "-v5",};
    args                = newArgParse(2, argv1, test_spec);

    argparser_default_kwarg_i(args, "verbose", &tmp);
    tassert_equal(tmp, 5);

    const char *argv2[] = {"dummy.exe", "-vabc",};
    args                = newArgParse(2, argv2, test_spec);
    argparser_default_kwarg(args, "verbose", &tmps);
    tassert_str_equal(tmps, "abc");

    argparser_delete(&args);

  end:
    return err;
}

#ifndef _WIN32

int
argparse_test_helper(argparse_spec_t* spec, const char** argv, size_t nargv, char* buffer, size_t bufferlen)
{
    /*
    fork and then parse the argparse spec using argv.
    redirect stdout to the provided buffer, so that a test
    may parse the output
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

        newArgParse(nargv, argv, spec);

        // only reachable if pasing the argument list was successful.
        // note that the logic below will interpret the negative status
        // as -1 == 255.
        exit(-1);

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
parse_kwarg_help_1(void) {
    int err = 0;

    int status;
    char buffer[2048] = {0};

    // this spec order uncovered a bug in the code that validates
    // the spec. specifically how it was determined to print
    // the Invalid spec warning and quit.
    argparse_spec_t test_spec[] = {
        { 0, 0, 0, "Test util.c functions"},
        { 0, 0, 0, "program description"},
        { 0, 0, "pos1", "1st positional argument"},
        { 0, 0, "pos2", "2nd positional argument"},
        { 0, ARGPARSE_OPT, "pos3", "optional argument"},
        { 0, ARGPARSE_OPTMANY, "pos4", "optional argument repeats"},
        { 0, 'a', 0, "optional flag 'a'"},
        { "inFile", 'i', "infile","parameter that consumes a value"},
        { "outFile",  0, "outfile","parameter with no short cut"},
        { 0, 0, 0, 0},
    };

    const char *argv1[] = {"dummy", "-h", };

    status = argparse_test_helper(test_spec, argv1, 2, buffer, sizeof(buffer));

    // exit status of the executable should have been 1
    tassert_equal(status, 1);
    // assert that no warning messages were printed.
    tassert_str_notin(buffer, "!!");

    tassert_str_in(buffer, "pos1 : 1st positional argument");
    tassert_str_in(buffer, "pos2 : 2nd positional argument");
    tassert_str_in(buffer, "pos3 : optional argument");
    tassert_str_in(buffer, "pos4 : optional argument repeats");
    tassert_str_in(buffer,  "-a : optional flag 'a'");
    tassert_str_in(buffer,  "[-i] --inFile=<infile> : parameter that consumes a value");
    tassert_str_in(buffer,  "     --outFile=<outfile> : parameter with no short cut");

  end:
    if (err>0) {
        printf("read:\n%s\n", buffer);
    }
    return err;
}

int
parse_kwarg_help_2(void) {
    int err = 0;

    char buffer[2048] = {0};

    argparse_spec_t test_spec[] = {
        { 0, 0, 0, "Test util.c functions"},
        { 0, 0, 0, "program description"},
        { 0, 'a', 0, "optional flag 'a'"},
        { "inFile", 'i', "infile","parameter that consumes a value"},
        { "outFile",  0, "outfile","parameter with no short cut"},
        { 0, 0, "pos1", "1st positional argument"},
        { 0, 0, "pos2", "2nd positional argument"},
        { 0, ARGPARSE_OPT, "pos3", "optional argument"},
        { 0, ARGPARSE_OPTMANY, "pos4", "optional argument repeats"},

        { 0, 0, 0, 0},
    };

    const char *argv1[] = {"dummy", "-h", };
    argparse_test_helper(test_spec, argv1, 2, buffer, sizeof(buffer));

    tassert_str_notin(buffer, "!!");

    tassert_str_in(buffer,  "-a : optional flag 'a'");
    tassert_str_in(buffer,  "[-i] --inFile=<infile> : parameter that consumes a value");
    tassert_str_in(buffer,  "     --outFile=<outfile> : parameter with no short cut");
    tassert_str_in(buffer, "pos1 : 1st positional argument");
    tassert_str_in(buffer, "pos2 : 2nd positional argument");
    tassert_str_in(buffer, "pos3 : optional argument");
    tassert_str_in(buffer, "pos4 : optional argument repeats");

    const char *argv2[] = {"dummy",};
    argparse_test_helper(test_spec, argv2, 1, buffer, sizeof(buffer));

    tassert_str_in(buffer, "!! Missing positional argument");


    //printf("read: %s\n", buffer);

  end:
    if (err>0) {
        printf("read:\n%s\n", buffer);
    }
    return err;
}

int
parse_kwarg_help_3(void) {
    int err = 0;

    int status;
    char buffer[2048] = {0};

    // this spec order uncovered a bug in the code that validates
    // the spec. specifically how it was determined to print
    // the Invalid spec warning and quit.
    argparse_spec_t test_spec[] = {
        { 0, 0, 0, "Test util.c functions"},
        { 0, 0, 0, "program description"},
        { 0, ARGPARSE_MANY, "pos", "optional argument repeats"},
        { "opt1",  'o', 0, "unnamed flag1"},
        { "opt2",    0, 0, "unnamed flag2"},
        { 0, 0, 0, 0},
    };

    const char *argv1[] = {"dummy", "-h", };

    status = argparse_test_helper(test_spec, argv1, 2, buffer, sizeof(buffer));

    // exit status of the executable should have been 1
    tassert_equal(status, 1);
    // assert that no warning messages were printed.
    tassert_str_notin(buffer, "!!");
    tassert_str_in(buffer, "[-o] --opt1");
    tassert_str_in(buffer, "     --opt2");

  end:
    if (err>0) {
        printf("read:\n%s\n", buffer);
    }
    return err;
}

int
parse_kwarg_help_4(void) {
    int err = 0;

    int status;
    char buffer[2048] = {0};

    // this spec order uncovered a bug in the code that validates
    // the spec. specifically how it was determined to print
    // the Invalid spec warning and quit.
    argparse_spec_t test_spec[] = {
        { 0, 0, 0, "Test util.c functions"},
        { 0, 0, 0, "program description"},
        { 0, ARGPARSE_MANY, "pos1", "optional argument repeats"},
        { 0, 0, "pos2", "optional argument repeats"},
        { 0, 0, 0, 0},
    };

    const char *argv1[] = {"dummy", "-h", };

    status = argparse_test_helper(test_spec, argv1, 2, buffer, sizeof(buffer));

    // exit status of the executable should have been 1
    tassert_equal(status, 1);
    // assert that no warning messages were printed.
    tassert_str_in(buffer, "!! Invalid Arg Spec");

  end:
    if (err>0) {
        printf("read:\n%s\n", buffer);
    }
    return err;
}

#endif

int
main(int argc, const char *argv[]) {

    begin_test(argc, argv, spec);

    run_test(parse_positional_simple);
    run_test(parse_flag_simple);
    run_test(parse_kwarg_simple);
    run_test(parse_kwarg_hard);
    run_test(parse_kwarg_hard_v2);
#ifndef _WIN32
    run_test(parse_kwarg_help_1);
    run_test(parse_kwarg_help_2);
    run_test(parse_kwarg_help_3);
    run_test(parse_kwarg_help_4);
#endif

    end_test();
}
