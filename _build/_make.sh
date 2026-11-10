gcc -Wpedantic -Wall -std=c89 -c -g ../monkdown/parse.c ../monkdown/write.c ../monkdown/nodes.c
gcc -Wpedantic -Wall -std=c89 -c -g ../amanuensis/pages.c ../amanuensis/fnodes.c
gcc -Wpedantic -Wall -std=c89 parse.o write.o pages.o fnodes.o nodes.o -o _run
find . -name "*.o" -delete
