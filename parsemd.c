#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "chunks.h"

#define MAX_BUFFERSZ 32768 
#define MAX_DOCSZ 128

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

int main() {

/*	ChunkElement **doc = mdtohtml("src/test.md", "_testhtmlout.html");*/

	/* everybody's got to work from the same copy of the buffer, otherwise
	   someone might point to it, not knowing that it has been popped off the
	   stack! */
	char buffer[MAX_BUFFERSZ];
	int chunksz;	

	/* an array of ChunkElements in the order they appear in the .md source */
	ChunkElement *doc[MAX_DOCSZ];
	
	/* initialize document */
	initdoc(doc, MAX_DOCSZ);
	
	/* directory string -> FILE* -> into the buffer */
	chunksz = readtobuffer("README.md", buffer, MAX_BUFFERSZ);
	if (chunksz == -1) { return(-1); }
	
	/* (blank document, buffer) -> into the parser! -> filled document */
	parsefile(doc, MAX_DOCSZ, buffer, chunksz);

	/************************************************************************/
	/* WE NOW HAVE ChunkElements THAT ALL REFERENCE buffer (SELF-CONTAINED) */
	/************************************************************************/
	
	/* (document, read directory) -> [FILE*] <- writechunk_html  */
	writedoc_html(doc, MAX_DOCSZ,  "_testhtmlout.html");
	
	/* filled document -> free all its malloc'd ChunkElements */
	freedoc(doc, MAX_DOCSZ);
	
	return(0);
}


int readtobuffer(char *readdir, char *_buffer, int _buffersz) {
	int i, readsz;
	FILE *rfp;

	if ((rfp = fopen(readdir, "r")) == NULL) {
		fprintf(stderr, "\x1b[31m[parsemd] [readtobuffer]"
						" failed to open file %s\x1b[0m\n", readdir);
		return(-1);
	}
	/*	Do you still live in the Surf?
		battered by the violent Froth?
		at the whims of the Tides and Winds?
		do you still live in fear?
		
		With other Creatures wearing Shells,
		hiding under the wet Sand,
		scurrying out in search of debris,
		deposited by Great Forces? */	
	
	
	readsz = fread(_buffer, sizeof(char), _buffersz-1, rfp);
	_buffer[readsz] = '\0';
	
	fclose(rfp);
	
    return(readsz);
}


int parsefile(ChunkElement **_doc, int _docsz, char *_buffer, int _buffersz) {
	int di = 0;
	int i = 0;
	int inchunk = 0;
	int chunkstart = 0;
	
	while (i < _buffersz) {
		if (!inchunk
				&& _buffer[i] != ' '
				&& _buffer[i] != '\n'
				&& _buffer[i] != '\t') {
			inchunk = 1;
			chunkstart = i;
		}
		if (inchunk) {
			if (i == chunkstart && i < _buffersz-2 && _buffer[i] == '`') {
				if (_buffer[i+1] == '`' && _buffer[i+2] == '`') {
					inchunk = 2;
					i += 3;
				}
			}
			while(i < _buffersz) {
				i++;
				if (inchunk == 1 && i > 0) {
					if (_buffer[i] == '\n' && _buffer[i-1] == '\n') {
						inchunk = 0;
					/*	printf("\"\x1b[33m%.*s\x1b[0m\"",
								 i-chunkstart-1, _buffer+chunkstart);
						printf("%d", decidetype(_buffer+chunkstart, i-chunkstart-1));*/
						if (di < _docsz) {
							_doc[di] = parsechunk(_buffer+chunkstart,
												i-chunkstart-1,
												decidetype(_buffer+chunkstart,
															i-chunkstart-1),
												16);
						/*	writechunk_html(_doc[di], 0);*/
							di++;
						}
						break;
					}
				}
				if (inchunk == 2 && i > 3) {
					if (_buffer[i] == '`' && _buffer[i-1] == '`'
										 && _buffer[i-2] == '`'
										 && _buffer[i-3] == '\n') {
						inchunk = 0;
						i++;
					/*	printf("\"\x1b[33m%.*s\x1b[0m\"",
								i-chunkstart, _buffer+chunkstart);
						printf("%d", decidetype(_buffer+chunkstart, i-chunkstart));*/
						if (di < _docsz) {
							_doc[di] = parsechunk(_buffer+chunkstart+3,
												i-chunkstart-6,
												decidetype(_buffer+chunkstart,
															i-chunkstart),
												16);
						/*	writechunk_html(_doc[di], 0);*/
							di++;
						}
						break;
					}
				}
			}
			if (i == _buffersz && inchunk) {
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


int writedoc_html(ChunkElement **_doc, int _docsz, char *writedir) {
	int i;
	FILE *wfp;
	struct stat st;

	if (stat(writedir, &st) == 0) {
		fprintf(stderr, "\x1b[31m[parsemd] [writedoc_html]"
						" refusing to overwrite \"%s\", exiting\x1b[0m\n",
						writedir);
		return(1);
	}
	
	if ((wfp = fopen(writedir, "a")) == NULL) {
		fprintf(stderr, "\x1b[31m[parsemd] [writedoc_html]"
						" failed to open file %s\x1b[0m\n", writedir);
		return(1);
	}
	
	for (i = 0; i < _docsz; i++) {
		if (_doc[i] != NULL) {
			writechunk_html(_doc[i], wfp, 0);
		}
	}
	
	fclose(wfp);
	return(0);
}


void writechunk_html(ChunkElement *c, FILE *_fp, int depth) {
	int i = 0;
	int pathisurl = 0;
	char tags[TYPE_MAXTYPE][12];

	strcpy(tags[TYPE_H1], "h1");
	strcpy(tags[TYPE_H2], "h2");
	strcpy(tags[TYPE_H3], "h3");
	strcpy(tags[TYPE_H4], "h4");
	strcpy(tags[TYPE_H5], "h5");
	strcpy(tags[TYPE_H6], "h6");
	strcpy(tags[TYPE_IMG], "img");
	strcpy(tags[TYPE_BLOCKQUOTE], "blockquote");
	strcpy(tags[TYPE_PRE], "pre");
	strcpy(tags[TYPE_UL], "ul");
	strcpy(tags[TYPE_P], "p");
	strcpy(tags[TYPE_LI], "li");
	strcpy(tags[TYPE_EM], "em");
	strcpy(tags[TYPE_STRONG], "strong");
	strcpy(tags[TYPE_A], "a");
	strcpy(tags[TYPE_CODE], "code");
	
	if (c->type == TYPE_ATTRIBUTE || depth == -1) {
		for (i = c->position; i < c->position + c->length; i++) {
			if (c->type == TYPE_ATTRIBUTE) {
				if (*((c->chunkstr)+i) == ':' && i < (c->position)+(c->length)-3) {
					if (*((c->chunkstr)+i+1) == '/' 
							&& *((c->chunkstr)+i+2) == '/'  ) {
						pathisurl = 1;
					}
				}
				if (!pathisurl) {
					if (*((c->chunkstr)+i) == '.'
							&& i < (c->position) + (c->length) - 3) {
						if (*((c->chunkstr)+i+1) == 'm'
								&& *((c->chunkstr)+i+2) == 'd') {
							fprintf(_fp, "%s", ".html");
							break;
						}
					}
				}
			}
			switch ( *((c->chunkstr)+i) ) {
				case '&':
					fprintf(_fp, "%s", "&amp;");
					break;
				case '<':
					fprintf(_fp, "%s", "&lt;");
					break;
				case '>':
					fprintf(_fp, "%s", "&gt;");
					break;
				case '"':
					fprintf(_fp, "%s", "&quot;");
					break;
				case '\'':
					fprintf(_fp, "%s", "&#39;");
					break;
				case '\t':
					fprintf(_fp, "    ");
					break;
				default:
					fprintf(_fp, "%c", *((c->chunkstr)+i));
					break;
			}
		}
		return;
	}
	
	if (c->open != 0 && c->type != TYPE_PLAIN) {
		fprintf(_fp, "<%s", tags[c->type]);
		if (c->type == TYPE_IMG) {
			fprintf(_fp, " src=\"");
			writechunk_html(c->children[1], _fp, -1);
			fprintf(_fp, "\" title=\"");
			writechunk_html(c->children[0], _fp, -1);
			fprintf(_fp, "\" alt=\"");
			writechunk_html(c->children[0], _fp, -1);
			fprintf(_fp, "\"");
		}
		if (c->type == TYPE_A) {
			fprintf(_fp, " href=\"");
			writechunk_html(c->children[1], _fp, -1);
			fprintf(_fp, "\"");
		}
		fprintf(_fp, ">");
	}
	
	if (c->open == 0) {
		for (i = c->position; i < c->position + c->length; i++) {
			switch ( *((c->chunkstr)+i) ) {
				case '&':
					fprintf(_fp, "%s", "&amp;");
					break;
				case '<':
					fprintf(_fp, "%s", "&lt;");
					break;
				case '>':
					fprintf(_fp, "%s", "&gt;");
					break;
				case '"':
					fprintf(_fp, "%s", "&quot;");
					break;
				case '\'':
					fprintf(_fp, "%s", "&#39;");
					break;
				case '\t':
					fprintf(_fp, "    ");
					break;
				default:
					fprintf(_fp, "%c", *((c->chunkstr)+i));
					break;
			}
		}
	}
	if (c->type != TYPE_IMG) {
		i = 0;
		while (i < C_E_MAXCHILDREN) {
			if (c->children[i] != NULL) {
				if (c->children[i]->type != TYPE_ATTRIBUTE) {
	/*			fprintf(_fp, "%*s%p\n", depth*4, " ", (void *)(c->children[i]));*/
					writechunk_html(c->children[i], _fp, depth+1);
				}
			}
			i++;
		}
		if (c->open != 0 && c->type != TYPE_PLAIN) {
			if (c->type != TYPE_IMG) {
				fprintf(_fp, "</%s>", tags[c->type]);
			}
		}
	}
}


void initdoc(ChunkElement **_doc, int _docsz) {
	int i;
	for (i = 0; i < _docsz; i++) {
		_doc[i] = NULL;
	}
}


void freedoc(ChunkElement **_doc, int _docsz) {
	int i;
	/* garbage collection */
	for (i = 0; i < MAX_DOCSZ; i++) {
		rfree(_doc[i]);
	}
}
