gcc -c librarian.c scribe.c
gcc librarian.o scribe.o -o _buildsite
./_buildsite
find . -name "*.o" -delete
