#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "chunks.h"

#define MAX_BUFFER 8192

int parsefile(ChunkElement **_doc, int _docsz, FILE *fp);
int decidetype(char *_chunkstr, int _chunksz);

int main() {
	FILE *fp;
	ChunkElement *document[32];
	int i;
	const char *fdir = "src/test.md";
	
	if ((fp = fopen(fdir, "r")) == NULL) {
		fprintf(stderr, "\x1b[31m[parsemd] [main]"
						" failed to open file %s\n", fdir);
		return(1);
	}
	/*
	while (fread(chunkbuffer, sizeof(char), 2048, fp) == 2048) {
		for (i = 0; i < 2047; i++) {
			if (chunkbuffer[i] == '\n' && chunkbuffer[i+1] == '\n') {*/
			/*	Do you still live in the Surf?
				battered by the violent Froth?
				at the whims of the Tides and Winds?
				do you still live in fear?
				
				With other Creatures wearing Shells,
				hiding under the wet Sand,
				scurrying out in search of debris,
				deposited by the Tides and Winds? */
	/*
			}
		}
	}*/
	
	
	for (i = 0; i < 32; i++) {
		document[i] = NULL;
	}
	
	parsefile(document, 32, fp);
/*
	for (i = 0; i < 32; i++) {
		if (document[i] != NULL) {
			printchunktree(document[i], 0);
		}
		printf("---------------------------------\n");
	}
	*/
	/* garbage collection */
	for (i = 0; i < 32; i++) {
		rfree(document[i]);
	}
	
	fclose(fp);
	
	return(0);
}

int parsefile(ChunkElement **_doc, int _docsz, FILE *fp) {
	int doci = 0;
	int i = 0;
	int readsz;
	char buffer[MAX_BUFFER];
	int inchunk = 0;
	int chunkstart = 0;
	
	readsz = fread(buffer, sizeof(char), MAX_BUFFER-1, fp);
	buffer[readsz] = '\0';
	
	while (i < readsz) {
		if (!inchunk
				&& buffer[i] != ' '
				&& buffer[i] != '\n'
				&& buffer[i] != '\t') {
			inchunk = 1;
			chunkstart = i;
		}
		if (inchunk) {
			if (i == chunkstart && i < readsz-2 && buffer[i] == '`') {
				if (buffer[i+1] == '`' && buffer[i+2] == '`') {
					inchunk = 2;
					i += 3;
				}
			}
			while(i < readsz) {
				i++;
				if (inchunk == 1 && i > 0) {
					if (buffer[i] == '\n' && buffer[i-1] == '\n') {
						inchunk = 0;
					/*	printf("\"\x1b[33m%.*s\x1b[0m\"",
								 i-chunkstart-1, buffer+chunkstart);
						printf("%d", decidetype(buffer+chunkstart, i-chunkstart-1));*/
						if (doci < _docsz) {
							_doc[doci] = parsechunk(buffer+chunkstart,
												i-chunkstart-1,
												decidetype(buffer+chunkstart,
															i-chunkstart-1),
												16);
						/*	printchunktree(_doc[doci], 0);*/
							doci++;
						}
						break;
					}
				}
				if (inchunk == 2 && i > 2) {
					if (buffer[i] == '`' && buffer[i-1] == '`'
										 && buffer[i-2] == '`') {
						inchunk = 0;
						i++;
					/*	printf("\"\x1b[33m%.*s\x1b[0m\"",
								i-chunkstart, buffer+chunkstart);
						printf("%d", decidetype(buffer+chunkstart, i-chunkstart));*/
						if (doci < _docsz) {
							_doc[doci] = parsechunk(buffer+chunkstart+3,
												i-chunkstart-6,
												decidetype(buffer+chunkstart,
															i-chunkstart),
												16);
						/*	printchunktree(_doc[doci], 0);*/
							doci++;
						}
						break;
					}
				}
			}
			if (i == readsz && inchunk) {
				inchunk = 0;
			}
		}
		i++;
	}
	return(0);
}

int decidetype(char *_chunkstr, int _chunksz) {
	int i = 0;
	if (_chunkstr[0] == '-') {
		return(TYPE_UL);
	} else if (_chunkstr[0] == '>') {
		return(TYPE_BLOCKQUOTE);
	} else if (_chunkstr[0] == '#') {
		i = 1;
		while (_chunkstr[i] == '#' && i < 6 && i < _chunksz) {
			i++;
		}
		return(TYPE_H1+i-1);
	} else if (_chunkstr[0] == '`' && _chunksz >= 6) {
		if (_chunkstr[1] == '`' && _chunkstr[2] == '`') {
			return(TYPE_PRE);
		}
	} else {
		return(TYPE_P);
	}
	return(-100);
}
