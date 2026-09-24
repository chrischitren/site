#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "chunks.h"

#define MAX_BUFFER 8192
#define MAX_DOCSZ 64

ChunkElement **readdoc(char *readdir, int _docsz);
int writedoc_html(ChunkElement **_doc, int _docsz, char *writedir);
void freedoc(ChunkElement **_doc, int _docsz);
int parsefile(ChunkElement **_doc, int _docsz, FILE *fp);

int decidetype(char *_chunkstr, int _chunksz);

void writechunk_html(ChunkElement *c, FILE *_fp, int depth);

int main() {

/*	ChunkElement **doc = mdtohtml("src/test.md", "_testhtmlout.html");*/

	ChunkElement **doc = readdoc("src/test.md", MAX_DOCSZ);
	writedoc_html(doc, MAX_DOCSZ, "_testhtmlout.html");
	freedoc(doc, MAX_DOCSZ);
	return(0);
}


void freedoc(ChunkElement **_doc, int _docsz) {
	int i;
	/* garbage collection */
	for (i = 0; i < MAX_DOCSZ; i++) {
		rfree(_doc[i]);
	}
	free(_doc);
}


ChunkElement **readdoc(char *readdir, int _docsz) {
	int i;
	ChunkElement **document;
	FILE *rfp;

	document = malloc(sizeof(ChunkElement*)*_docsz);
	
	if ((rfp = fopen(readdir, "r")) == NULL) {
		fprintf(stderr, "\x1b[31m[parsemd] [mdtohtml]"
						" failed to open file %s\x1b[0m\n", readdir);
		return(NULL);
	}
	/*	Do you still live in the Surf?
		battered by the violent Froth?
		at the whims of the Tides and Winds?
		do you still live in fear?
		
		With other Creatures wearing Shells,
		hiding under the wet Sand,
		scurrying out in search of debris,
		deposited by Great Forces? */	
	
	for (i = 0; i < _docsz; i++) {
		document[i] = NULL;
	}
	
	parsefile(document, _docsz, rfp);
	
	fclose(rfp);
	
	return(document);
}


int writedoc_html(ChunkElement **_doc, int _docsz, char *writedir) {
	int i;
	FILE *wfp;
	struct stat st;

	if (stat(writedir, &st) == 0) {
		fprintf(stderr, "\x1b[31m[parsemd] [mdtohtml]"
						" refusing to overwrite \"%s\", exiting\x1b[0m\n",
						writedir);
		return(1);
	}
	
	if ((wfp = fopen(writedir, "a")) == NULL) {
		fprintf(stderr, "\x1b[31m[parsemd] [mdtohtml]"
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
						/*	writechunk_html(_doc[doci], 0);*/
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
						/*	writechunk_html(_doc[doci], 0);*/
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


void writechunk_html(ChunkElement *c, FILE *_fp, int depth) {
	int i = 0;
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
				if (*((c->chunkstr)+i) == '.' && i < c->position + c->length - 2) {
					if (*((c->chunkstr)+i+1) == 'm' && *((c->chunkstr)+i+2) == 'd') {
						fprintf(_fp, "%s", ".html");
						break;
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

