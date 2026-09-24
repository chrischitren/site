gcc -Wpedantic -Wall -std=c89 -c -g chunks.c parsemd.c
gcc -Wpedantic -Wall -std=c89 chunks.o parsemd.o -o _run
./_run
find . -name "*.o" -delete
