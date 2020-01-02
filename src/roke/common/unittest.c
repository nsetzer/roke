
#include "roke/common/unittest.h"



int verbose_logging = 0;

int
tassert_impl(int mode,
             const char *filename,
             int line,
             const char* sx,
             const char* sy,
             int64_t x,
             int64_t y) {
    int result = 0;
    char *opstr = "does not equal";
    char *op = "!=";

    switch (mode) {

        case T_GT:
            result = x > y;
            opstr = "not greater than";
            op = "<";
            break;
        case T_LT:
            result = x < y;
            opstr = "not less than";
            op = ">";
            break;
        case T_NE:
            result = x != y;
            opstr = "equals";
            op = "=";
            break;
        case T_EQ:
        default:
            result = x == y;
            break;
    }

    if (result==0) {
        fprintf(stderr,
                "\nAssertion Error:\n FILE: %s\n LINE: %d\n"
                " EXPR: `%s` %s `%s`\n VAL : %"PRId64" %s %"PRId64"\n",
                filename,line,sx,opstr,sy,x,op,y);
        return 1;
    }
    return 0;

}


int
tassert_impl_float(int mode,
             const char *filename,
             int line,
             const char* sx,
             const char* sy,
             double x,
             double y,
             double tolerance) {
    double result = 0;
    char *opstr = "does not equal";
    char *op = "!=";

    switch (mode) {

        case T_GT:
            result = x > y;
            opstr = "not greater than";
            op = "<";
            break;
        case T_LT:
            result = x < y;
            opstr = "not less than";
            op = ">";
            break;
        case T_NE:
            result = fabs(x - y) >= tolerance;
            opstr = "equals";
            op = "=";
            break;
        case T_EQ:
        default:
            result = fabs(x - y) < tolerance;
            break;
    }

    if (result==0) {
        fprintf(stderr,
                "\nAssertion Error:\n FILE: %s\n LINE: %d\n"
                " EXPR: `%s` %s `%s`\n VAL : %f %s %f\n",
                filename,line,sx,opstr,sy,x,op,y);
        return 1;
    }
    return 0;

}

void
print_buffer(const char *prefix, size_t len, const uint8_t *buf) {
    printf("%s", prefix);
    for (int i = 0; i < len; i++) {
        if (i % 16 == 0 && i > 0)
            printf("\n%s", prefix);
        printf("%.2X", (unsigned) (0xFF & buf[i]));
        if (i % 2 == 1)
            printf(" ");
    }
    printf("\n");
}