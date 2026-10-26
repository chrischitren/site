gcc -Wpedantic -Wall -std=c89 -c -g ../monkdown/parse.c ../monkdown/write.c
gcc -Wpedantic -Wall -std=c89 -c -g ../amanuensis/pages.c
gcc -Wpedantic -Wall -std=c89 parse.o write.o pages.o -o _run
find . -name "*.o" -delete
