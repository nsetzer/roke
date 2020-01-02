

gcc -g -std=c11 -I. roke/common/*.c  roke/bin/build.c  -o cbuild || exit 1
gcc -g -std=c11 -I. roke/common/*.c  roke/bin/find.c  -o cfind || exit 1

./cbuild src $PWD/..
echo "expected:"
find $PWD/.. -name "*CMakeCache*" | sort
echo "actual:"
./cfind CMakeCache | sort
