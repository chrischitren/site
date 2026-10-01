#ifndef LIBRARIAN_H
#define LIBRARIAN_H

#include <stdio.h>
#define MAXDIR 256
#define MAX_FILES 64

void buildindex(char _index[MAX_FILES][MAXDIR], char *srcdir);
void recursivefilesearch(char *searchdir, char *parent, 
					char index[MAX_FILES][MAXDIR], int *i);
void walkfiles(char *dir, char *ext, char *parent, FILE *writefile);
FILE *makemdindex(char *filename, char *srcdir);

#endif
