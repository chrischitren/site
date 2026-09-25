gcc -Wpedantic -Wall -std=c89 -c -g chunks.c parsemd.c librarian.c
gcc -Wpedantic -Wall -std=c89 chunks.o parsemd.o librarian.o -o _run
./_run
find . -name "*.o" -delete
