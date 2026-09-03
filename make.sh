gcc -c librarian.c scribe.c
gcc librarian.o scribe.o -o buildsite
./buildsite
find . -name "*.o" -delete
