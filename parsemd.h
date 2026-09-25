#ifndef PARSEMD_H
#define PARSEMD_H

#include <stdio.h>

#include "chunks.h"

#define MAX_BUFFERSZ 32768 
#define MAX_DOCSZ 128

/* read an md file located at readdir and write and html file to writedir */
int mdtohtml(char *readdir, char *writedir);

/* load the file's contents to the stack */
int readtobuffer(char *readdir, char *_buffer, int _buffersz);

/* parsing functiosn */
int parsefile(ChunkElement **_doc, int _docsz, char *_buffer, int _buffersz);
int decidetype(char *_chunkstr, int _chunksz);

/* write functions */
int writedoc_html(ChunkElement **_doc, int _docsz, char *writedir);
void writechunk_html(ChunkElement *c, FILE *_fp, int depth);

/* document handling functions */
void initdoc(ChunkElement **_doc, int _docsz);
void freedoc(ChunkElement **_doc, int _docsz);

void printlinks(ChunkElement *c);

#endif
