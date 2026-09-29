#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "parsemd.h"

/*****************************************************************************
                         DOCUMENT HANDLING FUNCTIONS
The librarian will be expected to create Document objects and initilize them
using readdoc_md, which populates all relevant fields and builds ChunkElement
trees to represent the document structure.
*****************************************************************************/

int readdoc_md(Document *_doc, char *readdir) {
	int i;
	for (i = 0; i < MAX_DOCSZ; i++) {
		_doc->chunklist[i] = NULL;
	}

	for (i = 0; i < MAX_LINKS; i++) {
		_doc->outlinks[i][0] = '\0';
	}

	

	/* directory string -> FILE* -> into the buffer */
	_doc->bufferlen = readtobuffer(readdir, _doc->buffer, MAX_BUFFERSZ);
	if (_doc->bufferlen == -1) {
		return(-1);
	}

	/* (blank document, buffer) -> into the parser! -> filled document */
	parsefile(_doc->chunklist, MAX_DOCSZ,
				 _doc->buffer, _doc->bufferlen);
	
	populateoutlinks(_doc);

	/* we now have a document that contains:
	    a list of root ChunkElement nodes, and
	    a buffer holding the Markdown these all reference. */
	
	return(0);
}

int writedoc_html(Document *_doc, char *writedir) {
	int i;
	FILE *wfp;
	struct stat st;

	if (stat(writedir, &st) == 0) {
		fprintf(stderr, "\x1b[31m[parsemd] [writedoc_html]"
						" refusing to overwrite \"%s\"\x1b[0m\n",
						writedir);
		return(1);
	}
	
	if ((wfp = fopen(writedir, "a")) == NULL) {
		fprintf(stderr, "\x1b[31m[parsemd] [writedoc_html]"
						" failed to open file %s\x1b[0m\n", writedir);
		return(1);
	}

	fputs(
	"<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
	"<meta charset=\"UTF-8\">\n<title>chrischitren</title>\n"
	"<link rel=\"icon\" type=\"image/png\" href=\"/images/favicon_32.png\">\n"
	"<link rel=\"stylesheet\" href=\"main.css\">\n</head>\n<body>",
	wfp);
	
	for (i = 0; i < MAX_DOCSZ; i++) {
		if (_doc->chunklist[i] != NULL) {
			writechunk_html(_doc->chunklist[i], wfp, 0);
		}
	}

	fputs("</body>\n</html>", wfp);
	
	fclose(wfp);
	return(0);
}


void freedoc(Document *_doc) {
	int i;
	for (i = 0; i < MAX_DOCSZ; i++) {
		rfree(_doc->chunklist[i]);
	}
}

/*****************************************************************************
                              INTERNAL FUNCTIONS
*****************************************************************************/


void populateoutlinks(Document *_doc) {
	int i;
	int oli = 0;
	for (i = 0; i < MAX_DOCSZ; i++) {
		if (_doc->chunklist[i] != NULL) {
			graboutlinks(_doc->chunklist[i], _doc, &oli);
		}
	}
}

int graboutlinks(ChunkElement *c, Document *_doc, int *oli) {
	int i;
	
	if (*oli >= MAX_LINKS) {
		return(1);
	}
	
	if (c->type == TYPE_ATTRIBUTE && c->open == 0) {
		for (i = 0; i<(c->length) && i<MAXDIR-1; i++) {
			_doc->outlinks[*oli][i] = *((c->chunkstr)+c->position+i);
		}
		_doc->outlinks[*oli][i] = '\0';
		if (strstr(_doc->outlinks[*oli], "//") == NULL) {
			if (strstr(_doc->outlinks[*oli], ".md") != NULL) {
				strcpy(strstr(_doc->outlinks[*oli], ".md"), ".html");
			}
		}
		*oli += 1;
	}
	for (i = 0; i < C_E_MAXCHILDREN; i++) {
		if (c->children[i] != NULL) {
			graboutlinks(c->children[i], _doc, oli);
		}
	}
	return(0);
}

int readtobuffer(char *readdir, char *_buffer, int _buffersz) {
	int readsz;
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


int parsefile(ChunkElement **_chunklist, int _chunklistsz,
								char *_buffer, int _buffersz) {
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
						if (di < _chunklistsz) {
							_chunklist[di] = parsechunk(_buffer+chunkstart,
												i-chunkstart-1,
												decidetype(_buffer+chunkstart,
															i-chunkstart-1),
												16);
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
						if (di < _chunklistsz) {
							_chunklist[di] = parsechunk(_buffer+chunkstart+3,
												i-chunkstart-6,
												decidetype(_buffer+chunkstart,
															i-chunkstart),
												16);
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
							&& i < (c->position) + (c->length) - 2) {
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




