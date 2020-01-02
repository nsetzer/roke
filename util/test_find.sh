

gcc -O3 -std=c11 -I. boyer_moore.c cfind.c -o cfind || exit 1

queryA=png

echo ./cfind $queryA
time ./cfind $queryA | wc -l

