#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define C_E_MAXCHILDREN 16
#define MAX_CHUNK_SZ 2048

enum CEType {
	TYPE_PLAIN = 0, /* plain text!!! */
	
/****** ROOT NODE (FULL CHUNK) TYPES ******/
	TYPE_H1, /****************************/
	TYPE_H2, /*                         */
	TYPE_H3, /*     headings must      */
	TYPE_H4, /*     be ordered.       */
	TYPE_H5, /*                      */
	TYPE_H6, /***********************/
	TYPE_IMG,
	TYPE_BLOCKQUOTE,
	TYPE_PRE,
	TYPE_UL,
	TYPE_OL,
	
/************* ERRANT ONES. *************/
	TYPE_P, /* whuh?!? */

/************* INLINE TYPES *************/
	TYPE_LI,
	TYPE_EM,     /*   strong must    */
	TYPE_STRONG, /*   follow em.    */
	TYPE_A,
	TYPE_CODE
};

/*
----- FULL CHUNK TYPES -----

TYPE_PLAIN
	{}
TYPE_H1
	{TYPE_PLAIN, TYPE_EM, TYPE_STRONG, TYPE_A, TYPE_CODE}
TYPE_H2
	{TYPE_PLAIN, TYPE_EM, TYPE_STRONG, TYPE_A, TYPE_CODE}
TYPE_H3
	{TYPE_PLAIN, TYPE_EM, TYPE_STRONG, TYPE_A, TYPE_CODE}
TYPE_H4
	{TYPE_PLAIN, TYPE_EM, TYPE_STRONG, TYPE_A, TYPE_CODE}
TYPE_H5
	{TYPE_PLAIN, TYPE_EM, TYPE_STRONG, TYPE_A, TYPE_CODE}
TYPE_H6
	{TYPE_PLAIN, TYPE_EM, TYPE_STRONG, TYPE_A, TYPE_CODE}
TYPE_IMG
	{TYPE_PLAIN}
TYPE_BLOCKQUOTE
	{TYPE_P}
TYPE_PRE
	{TYPE_PLAIN}
TYPE_UL
	{TYPE_LI}
TYPE_P
	{TYPE_PLAIN, TYPE_EM, TYPE_STRONG, TYPE_A, TYPE_CODE}
TYPE_LI
	{TYPE_P}
TYPE_EM
	{TYPE_PLAIN, TYPE_STRONG, TYPE_A, TYPE_CODE}
TYPE_STRONG
	{TYPE_PLAIN, TYPE_EM, TYPE_A, TYPE_CODE}
TYPE_A
	{TYPE_EM, TYPE_STRONG, TYPE_CODE}
TYPE_CODE
	{TYPE_PLAIN}

*/



typedef struct _celem ChunkElement;

struct _celem {
	char *chunkstr;   /* pointer to the string of the *full* chunk */
	int chunksz;      /*                 (a string of this length) */
	ChunkElement *parent;
	ChunkElement *children[C_E_MAXCHILDREN];
	int position;     /* the element's location in chunkstr        */
	int length;       /* the element's length in bytes             */
	int type;         /* semantic identifier (e.g. HTML tag)       */
	int open;         /* 0: no further subelements to be parsed
	                     1: not yet frozen, try to parse           */
};

ChunkElement *initchunk(char *_chunkstr, int _chunksz, int _chunkpos,
						int _type, int _open, ChunkElement *_parent);
ChunkElement *parsechunkelement(ChunkElement *c);

int parsesimple(char *chunk, int *_contentsz, char delim1, char delim2);
int parseasterisk(char *chunk, int *_contentsz, int *_delimsz);

int countopenleaves(ChunkElement *c);
void printchunktree(ChunkElement *c, int depth);
int rfree(ChunkElement *c);

int main() {
	int i = 0;
/*	char *teststr = 
		"> ***emstrong** em* noemph `code` *em **emstrong notcode** em*\n"
		"> *`emcode`* none **strong *emstrong* strong**.\n"
		"> hello world!\n"
		"this is a newline...\n"
		"> but there is a new element after it!!!";*/

char *teststr = 
		"- this is a list item\n"
		"containing a paragraph break\n"
		"- followed by another list item."
		"- followed by a third-whoa! --\n";
	ChunkElement *rt;
	
	if ((rt = initchunk(teststr, strlen(teststr), 0, TYPE_UL, 1, NULL)) != NULL) {
		printf("open leaves: %d\n", countopenleaves(rt));
		printf("parsing\n");
		
		parsechunkelement(rt);
		for (i = 0; i < C_E_MAXCHILDREN; i++) {
			if (rt->children[i] != NULL) {
				parsechunkelement(rt->children[i]);
			}
		}
		
		printchunktree(rt, 0);
		
		
		
		printf("open leaves: %d\n", countopenleaves(rt));
		
		rfree(rt);
	}
	
	printf("%ld\n", sizeof(ChunkElement));
	
	return(0);
}

/*	initchunk will take a string (the chunk) and return a root ChunkElement
	whose children correspond to the structure of the chunk. */
ChunkElement *initchunk(char *_chunkstr, int _chunksz, int _chunkpos, 
						int _type, int _open, ChunkElement *_parent) {
	int i;
	
	ChunkElement *root;
	if ((root = malloc(sizeof(ChunkElement))) == NULL) {
		fprintf(stderr, "\x1b[31m[chunks] [initchunk]"
						" failed to malloc root node\x1b[0m\n");
		return(NULL);
	}
	
	root->parent = _parent;
	if (_parent != NULL) {
		for (i = 0; i < C_E_MAXCHILDREN; i++) {
			if (_parent->children[i] == NULL) {
				_parent->children[i] = root;
				break;
			}
		}
		if (i == C_E_MAXCHILDREN) {
			fprintf(stderr, "\x1b[31m[chunks] [initchunk]"
							" parent has too many children\x1b[0m\n");
			free(root);
			return(NULL);
		}
	}
	
	for(i = 0; i < C_E_MAXCHILDREN; i++) {
		root->children[i] = NULL;
	}
	
	root->chunkstr = _chunkstr;
	root->position = _chunkpos; /* root starts at beginning of chunk  */
	root->length = _chunksz;    /*      and runs through end of chunk */
	root->type = _type;
	root->open = _open;
	
	return(root);
}



/*	parsechunkelement takes an open ChunkElement found at c and reads its
	associated chunk string. It appends as children any valid subelements in
	the form of ChunkElement pointers. NOT RECURSIVE; the handling function
	must check if there are open leaves and call parsechunkelement on each. */
ChunkElement *parsechunkelement(ChunkElement *c) {
	
	int i = 0;
	int escape = 0;
	
	int lastwritepos = 0;
	int skippedsz = 0;
	
	int offset = c->position; /* start pos in c->chunkstr   */
	int contentsz = 0;
	int delimsz = 0;          /* length of delimiter        */
	char *pci = "";           /* 'p'ointer to 'c'har at 'i' */
	
	if (c->open == 0) {
		return(NULL);
	}
	
	while (i <= c->length) {
		pci = ((c->chunkstr)+offset+i);
		
		if (*pci == '\\') {
			escape = 2;
		}
		if (escape && *pci != '\n') {
			i++;
			escape--;
			continue;
		} else {
			escape = 0;
		}
		
		/* Split <blockquote> into <p>s; linebreaks cause new <p> children. */
		if (c->type == TYPE_BLOCKQUOTE) {
			if (*pci == '>' && i == lastwritepos) {
				lastwritepos += 2;
				i += 2;
			}
			if ((*pci == '\n' || *pci == '\0') && i != lastwritepos) {
				initchunk(c->chunkstr, i-lastwritepos, offset+lastwritepos,
					TYPE_P, 1, c);
				lastwritepos = i+1;
			}
		}
		
		/* Splits <ul> into <li>s; linebreaks can't directly bear children
		   because they imply multiple <p>s nested inside one <li>. */
		if (c->type == TYPE_UL) {
			if (*pci == '-' && i == lastwritepos) {
				lastwritepos += 2;
				i += 2;
			}
			if (((*pci == '-' && *(pci-1) == '\n') || *pci == '\0') && i != lastwritepos) {
				initchunk(c->chunkstr, i-lastwritepos-1, offset+lastwritepos,
					TYPE_LI, 1, c);
				lastwritepos = i+2;
			}
		}
		
		/* Split the aforementioned nested <p>s up inside an <li> */
		if (c->type == TYPE_LI) {
			if ((*pci == '\n' || i == c->length) && i != lastwritepos) {
				initchunk(c->chunkstr, i-lastwritepos, offset+lastwritepos,
							TYPE_P, 1, c);
				lastwritepos = i+1;
			}
		}
		
		/* Parse inline elements <em>, <strong>, <code>, and <a> (TODO WIP) */
		if (c->type == TYPE_P || (c->type >= TYPE_H1 && c->type <= TYPE_H6)) {
			if ((*pci == '\0' || i == c->length) && i != lastwritepos) {
				initchunk(c->chunkstr, i-lastwritepos, offset+lastwritepos,
							TYPE_PLAIN, 0, c);
			}
			if (*pci == '`') {
				if (parsesimple(pci, &contentsz, '`', '`') == 0) {
					/* write content up to now into plaintext ChunkElement */
					if (i != lastwritepos) {
						initchunk(c->chunkstr, i-lastwritepos, offset+lastwritepos,
								TYPE_PLAIN, 0, c);
						lastwritepos = i;
					}
					
					/* write ChunkElement and skip */
					initchunk(c->chunkstr, contentsz, offset+i+1,
							TYPE_CODE, 0, c);
					skippedsz = contentsz + 2;
					i += skippedsz;
					lastwritepos += skippedsz;
				}
			}
			
			if (*pci == '*') {
				if (parseasterisk(pci, &contentsz, &delimsz) == 0) {
					/* write content up to now into plaintext ChunkElement */
					if (i != lastwritepos) {
						initchunk(c->chunkstr, i-lastwritepos, offset+lastwritepos,
								TYPE_PLAIN, 0, c);
						lastwritepos = i;
					}
					
					/* write asterisk-delimited ChunkElement and skip */
					initchunk(c->chunkstr, contentsz, offset+i+delimsz,
							TYPE_EM-1+delimsz, 1, c);
					skippedsz = contentsz + 2*delimsz;
					i += skippedsz;
					lastwritepos += skippedsz;
				}
			}
		}
		
		if (!skippedsz) {
			i++;
		/*	contentsz++;*/
		}
		skippedsz = 0;
	}
	return(NULL);
}



int parsesimple(char *chunk, int *_contentsz, char delim1, char delim2) {
	int i = 0;
	int escape = 1;

	int sz;
	sz = strlen(chunk);

	if (chunk[0] != delim1) {
		fprintf(stderr, "\x1b[31m[chunks] [parsesimple]"
						" string does not start with given delim\x1b[0m\n");
		return(1);
	}
	
	for (i = 0; i < sz; ) {
		if (chunk[i] == '\\' && !escape) {
			escape = 2;
		}
		if (chunk[i] == delim2 && !escape) {
			*_contentsz = i-1;
			return(0);
		} else {
			i++;
		}
		if (escape) {
			escape--;
		}
	}
	
	fprintf(stderr, "\x1b[31m[chunks] [parsesimple]"
					" reached end of string without closing\x1b[0m\n");
	return(1);
}


/*  parseasterisk finds the length of the next and returns innerlength; needs
	to return length of delimiter as well */
int parseasterisk(char *chunk, int *_contentsz, int *_delimsz) {
	int i, j;
	
	int escape = 0;     /* bool; 1:last char was escape, 0:wasn't  */
	
	int bi = 0;         /* position of write head in output buffer */
	
	int inside = 0;     /*                                         */
	int start_n = 0;    /* length of starting run                  */
	int depth_e = 0;    /* current depth of emphasis (0, 1, 2, 3)  */
	
	int sz;
	sz = strlen(chunk);
	
	for (i = 0; i < sz; ) {
		if (chunk[i] == '\\' && !escape) {
			escape = 2;
		}
		if (chunk[i] == '*' && !escape) {
			for (j = 0; i+j < sz; j++) { 
				if (chunk[i+j] != '*') {
					break;
				}
				if (j > 3) {
					break;
				}
			}
			if (!inside) {
				bi = 0;
				inside = 1;
				start_n = j;
				depth_e = j;
				if (j < 3) {
					i += start_n;
					*_delimsz = start_n;
				} else if (j == 3) {
					bi += 3;
					i += 3;
				}
			} else {
				if (start_n < 3) {
					if (j == start_n) {
						*_contentsz = bi; /* w */
						return(0);        /* w */
					} else if (j == 3) {
						if (depth_e == 3) {
							i += 3-start_n;
							bi += 3-start_n;
							*_contentsz = bi; /* w */
							return(0);        /* w */
						} else if (depth_e < 3) {
							*_contentsz = bi; /* w */
							return(0);        /* w */
						}
					} else {
						depth_e += j;
						i += j;
						bi += j;
					}
				} else if (start_n == 3) {
					if (j == 3) {
						i += 2;
						bi += 2;
						*_delimsz = 1;    /* w */
						*_contentsz = bi-1; /* w */
						return(0);        /* w */
					} else {
						if (depth_e-j <= 0) {
							*_delimsz = j;    /* w */
							*_contentsz = bi-j; /* w */
							return(0);        /* w */
						} else {
							depth_e -= j;
							i += j;
							bi += j;
						}
					}
				}
			}
		} else {
			bi++;
			i++;
		}
		if (escape) {
			escape--;
		}
	}
	fprintf(stderr, "\x1b[31m[chunks] [parseasterisk]"
					" reached end of string without closing\x1b[0m\n");
	return(1);
}

/* 	countopenleaves searches a tree of ChunkElements for nodes that don't have
	any children, but for which open==1. It returns the sum of c->open ints
	for these terminal/leaf nodes */ 
int countopenleaves(ChunkElement *c) {
	int f = 0;
	int i = 0;
	
	if(c->children[0] == NULL) {
		return(c->open);
	}
	
	while (i < C_E_MAXCHILDREN) {
		if (c->children[i] != NULL) {
			f += countopenleaves(c->children[i]);
		}
		i++;
	}
	
	return(f);
}

void printchunktree(ChunkElement *c, int depth) {
	int i = 0;
	if (c->open == 0) {
		printf("\x1b[36m");
	} else {
		printf("\x1b[33m");
	}
	
/*	printf("%*s%p\n", depth*4, " ", (void *)c);*/
	printf("%*s(type=%d, ", depth*4, " ", c->type);
	printf("open=%d) ", c->open);
	printf("\"");
	for (i = c->position; i < c->position + c->length; i++) {
		if (*((c->chunkstr)+i) != '\n') {
			printf("%c", *((c->chunkstr)+i));
		} else {
			putc(0xC2, stdout);
			putc(0xB6, stdout);
		}
	}
	printf("\"\x1b[0m\n");
	i = 0;
	while (i < C_E_MAXCHILDREN) {
		if (c->children[i] != NULL) {
/*			printf("%*s%p\n", depth*4, " ", (void *)(c->children[i]));*/
			printchunktree(c->children[i], depth+1);
		}
		i++;
	}
}

int rfree(ChunkElement *c) {
	int i = 0;
	while (i < C_E_MAXCHILDREN) {
		if (c->children[i] != NULL) {
			rfree(c->children[i]);
		}
		i++;
	}
	free(c);
	return(0);
}

