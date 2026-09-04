#ifndef LIBRARIAN_H
#define LIBRARIAN_H

#include <stdio.h>
#define MAXDIR 256
void walkfiles(char *dir, char *ext, char *parent, FILE *writefile);
FILE *makemdindex(char *filename, char *srcdir);

#endif
