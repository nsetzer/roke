

gcc -g -std=c11 -I. boyer_moore.c cfind.c -o cfind || exit 1

queryA=png
queryB=PNG

echo ./cfind $queryA
./cfind   $queryA | wc -l
grep -Irn $queryA ~/.config/explor/index/*.f.idx | wc -l

echo ./cfind $queryB
./cfind   $queryB | wc -l
grep -Irn $queryB ~/.config/explor/index/*.f.idx | wc -l

echo ./cfind -i $queryA
./cfind -i $queryA | wc -l
grep -Iirn $queryA ~/.config/explor/index/*.f.idx | wc -l

echo ./cfind -i $queryB
./cfind -i $queryB | wc -l
grep -Iirn $queryB ~/.config/explor/index/*.f.idx | wc -l

