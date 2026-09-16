#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define C_E_MAXCHILDREN 8
#define MAX_CHUNK_SZ 2048

enum CEType {
	TYPE_PLAIN = 0,
	TYPE_P,
	TYPE_EM,
	TYPE_STRONG
};

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
int countopenleaves(ChunkElement *c);
void printchunktree(ChunkElement *c, int depth);
int rfree(ChunkElement *c);
int parseasterisk(char *chunk, int *_contentsz, int *_delimsz);

int main() {
/*
	const int nestingtable[9][9] = {
		{0,0,0,0,0,0,0,0,0}, /~*                             *~/ 
		{0,0,0,0,0,0,0,0,0}, /~*                             *~/ 
		{0,0,0,0,0,0,0,0,0}, /~*                             *~/ 
		{0,0,0,0,0,0,0,0,0}, /~*                             *~/ 
		{0,0,0,0,0,0,0,0,0}, /~*                             *~/ 
		{0,0,0,0,0,0,0,0,0}, /~*                             *~/ 
		{0,0,0,0,0,0,0,0,0}, /~*                             *~/ 
		{0,0,0,0,0,0,0,0,0}, /~*                             *~/ 
		{0,0,0,0,0,0,0,0,0}  /~*                             *~/ 
	};*/
	int i = 0;
	char *teststr = 
		"***emstrong** em* noemph *em **emstrong** em* none **strong *emstrong* strong**.";
	ChunkElement *rt;
	
	if ((rt = initchunk(teststr, strlen(teststr), 0, 0, 1, NULL)) != NULL) {
		printf("open leaves: %d\n", countopenleaves(rt));
		for (i = rt->position; i < rt->position+rt->length; i++) {
		/*	printf("%c", teststr[i]);*/
		}
		printf("parsing\n");
		
		parsechunkelement(rt);
		printchunktree(rt, 0);
		
		printf("open leaves: %d\n", countopenleaves(rt));
		
		rfree(rt);
	}
	
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
	
	
	
	int offset = c->position;
	int contentlength = 0;
	int skiplength = 0;
/*	int childindex = 0;*/
	
	if (c->open == 0) {
		return(NULL);
	}

	while(i < c->length) {
		
		if (*((c->chunkstr)+offset+i) == '*') {
			/* TODO: standardize parse functions as writing to 
			         int pointers; use return value as error code. */
			if (parseasterisk((c->chunkstr)+offset+i, 
							&contentlength, &skiplength) == 0) {
				initchunk(c->chunkstr, contentlength, offset+i+skiplength,
						TYPE_EM-1+skiplength, 1, c);
				i += contentlength + 2*skiplength;
			} else {
				/* skip this asterisk if parseasterisk returns error code */
				i++;
			}
		} else {
			i++;
		}
	}
	printf("\n");
	return(NULL);
}
/*
int *parsebacktick(char *chunk, const int chunksz) {
	int i = 0;
	int escape = 0;
	
	
	
	return(0);
}
*/


/*  parseasterisk finds the length of the next and returns innerlength; needs
	to return length of delimiter as well */
int parseasterisk(char *chunk, int *_contentsz, int *_delimsz) {
	int i, j;
	
	int escape = 0;     /* bool; 1:last char was escape, 0:wasn't  */
	
	int bi = 0;         /* position of write head in output buffer */
	
	int inside = 0;     /*                                         */
	int start_n = 0;    /* length of starting run                  */
	int depth_e = 0;    /* current depth of emphasis (0, 1, 2, 3)  */
	
	for (i = 0; i < strlen(chunk); ) {
		if (chunk[i] == '\\' && !escape) {
			escape = 2;
		}
		if (chunk[i] == '*' && !escape) {
			for (j = 0; i+j < strlen(chunk); j++) { 
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
	
	printf("%*s%p\n", depth*4, " ", (void *)c);
	printf("%*stype=%d\n", depth*4, " ", c->type);
	printf("%*sopen=%d\n", depth*4, " ", c->open);
	printf("%*s\"", depth*4, " ");
	for (i = c->position; i < c->position + c->length; i++) {
		printf("%c", *((c->chunkstr)+i));
	}
	printf("\"\x1b[0m\n");
	i = 0;
	while (i < C_E_MAXCHILDREN) {
		if (c->children[i] != NULL) {
			printf("%*s%p\n", depth*4, " ", (void *)(c->children[i]));
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

