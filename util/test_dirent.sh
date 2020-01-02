

CC=${1:-gcc}
# /opt/rh/devtoolset-2/root/usr/bin/gcc

cat <<EOF | $CC -o a.out -xc -
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#ifndef _DIRENT_HAVE_D_TYPE
    #define TEST 1
#else
    #define TEST 0
#endif
int main(void) {
    struct dirent de;
    de.d_type = 0;
    printf("_DIRENT_HAVE_D_TYPE defined: %d\n", 0==TEST);
    return 0;
}
EOF

if [ -e ./a.out ]; then

    if ./a.out; then
        rm a.out
        exit 0
    fi

    rm a.out
    exit 1
else
    exit 1
fi

