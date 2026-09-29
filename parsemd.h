#ifndef PARSEMD_H
#define PARSEMD_H

#include <stdio.h>

#include "chunks.h"

#define MAX_BUFFERSZ 32768 
#define MAX_DOCSZ 128
#define MAX_LINKS 64

#ifndef MAXDIR
#define MAXDIR 256
#endif

typedef struct _documenttype Document;

struct _documenttype {
	ChunkElement *chunklist[MAX_DOCSZ];
	int chunklistlen;
	char buffer[MAX_BUFFERSZ];
	int bufferlen;
	char outlinks[MAX_LINKS][MAXDIR];
	int outlinkslen;
};

/* Document handling functions */
int readdoc_md(Document *_doc, char *readdir);
int writedoc_html(Document *_doc, char *writedir);
void freedoc(Document *_doc);

/* Internal functions */
int readtobuffer(char *readdir, char *_buffer, int _buffersz);
int parsefile(ChunkElement **_chunklist, int _chunklistsz,
												char *_buffer, int _buffersz);
int decidetype(char *_chunkstr, int _chunksz);
void writechunk_html(ChunkElement *c, FILE *_fp, int depth);

void populateoutlinks(Document *_doc); 
int graboutlinks(ChunkElement *c, Document *_doc, int *oli);

#endif
