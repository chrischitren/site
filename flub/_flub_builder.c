#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define C_E_MAXCHILDREN 32
#define MAX_CHUNK_SZ 2048

typedef struct _celem ChunkElement;

struct _celem {
	char *chunkstr;   /* pointer to the string of the *full* chunk */
	int chunksz;      /*                 (a string of this length) */
	void *parent;
	ChunkElement *children[C_E_MAXCHILDREN];
	int position;     /* the element's location in chunkstr        */
	int length;       /* the element's length in bytes             */
	int type;         /* semantic identifier (e.g. HTML tag)       */
	int open;         /* 0: no further subelements to be parsed
	                     1: not yet frozen, try to parse           */
};

ChunkElement *initchunk(char *_chunkstr, int _chunksz, int _chunkpos);

int countopenleaves(ChunkElement *c);

int parseinline(void *parent[], int parentsz, 
						char *chunk,  int level, int depth);
int parseasterisk(void *parent[], int parentsz, 
						char *chunk, int chunksz, int level, int depth);
int checkdelim(const char *delim, char *string, int n);

int main() {
	int i = 0;
	char *teststr = "test *string* here";
	ChunkElement *rt;
	
	if ((rt = initchunk(teststr, strlen(teststr), 0)) != NULL) {
		printf("open leaves: %d\n", countopenleaves(rt));
		for (i = rt->position; i < rt->position+rt->length; i++) {
			printf("%c", teststr[i]);
		}
		printf("\n");
		free(rt);
	}
	
	return(0);
}

/*	initchunk will take a string (the chunk) and return a root ChunkElement
	whose children correspond to the structure of the chunk. */
ChunkElement *initchunk(char *_chunkstr, int _chunksz, int _chunkpos) {
	int i, j;

	const int[9][9] nestingtable = {	
		{0,0,0,0,0,0,0,0,0}, /*                             */ 
		{0,0,0,0,0,0,0,0,0}, /*                             */ 
		{0,0,0,0,0,0,0,0,0}, /*                             */ 
		{0,0,0,0,0,0,0,0,0}, /*                             */ 
		{0,0,0,0,0,0,0,0,0}, /*                             */ 
		{0,0,0,0,0,0,0,0,0}, /*                             */ 
		{0,0,0,0,0,0,0,0,0}, /*                             */ 
		{0,0,0,0,0,0,0,0,0}, /*                             */ 
		{0,0,0,0,0,0,0,0,0}  /*                             */ 
	};

/*
	
	void *parent
	ChunkElement *children[C_E_MAXCHILDREN];
	int position;
	int length;
	int type;
	int open;
*/
	ChunkElement *root;
	if ((root = malloc(sizeof(ChunkElement))) == NULL) {
		fprintf(stderr, "[builder] [initchunk] failed to malloc root node\n");
		return(NULL);
	}
	
	root->parent = NULL;
	root->chunkstr = _chunkstr;
	root->position = _chunkpos;	/* root starts at beginning of chunk  */
	root->length = _chunksz;	/*      and runs through end of chunk */
	root->type = 0;				/* root type is root                  */
	root->open = 1;				/* root must start open to find elems */
	
	/* loop through the . . . */
	for (i = 0; i < chunksz; i++) {
		
	}
	
	return(root);
}


/*	parsechunktree takes a root ChunkElement pointer (p) and reads its
	associated chunk string. It appends as children any valid subelements,
	in the form of ChunkElement pointers. NOT RECURSIVE; the handling
	function must check if there are open leaves and call parsechunktree
	on each. */
ChunkElement *parsechunktree(int chunksz,
								int chunkpos, ChunkElement *p) {
	
}

/* 	countopenleaves searches a tree of ChunkElements for nodes that don't have
	any children, but for which open==1. It returns the sum of c->open ints
	for these terminal/leaf nodes */ 
int countopenleaves(ChunkElement *c) {
	int f = 0;
	int i = 0;
	
	while (i < C_E_MAXCHILDREN && c->children[i] != NULL) {
		f += countopenleaves(c->children[i], 0);
		i++;
	}
	
	if (i == 0) {
		return(c->open);
	}
	
	return(f);
}


/*
int *parsebacktick(char *chunk, const int chunksz) {
	int i = 0;
	int escape = 0;
	
	
	
	return(0);
}
*/

int parseasterisk(void *parent[], int parentsz,
						char *chunk, const int chunksz, int level, int depth) {
	int i, j;
	char innerbuffer[MAX_CHUNK_SZ];
	
	int escape = 0;		/* bool; 1:last char was escape, 0:wasn't  */
	
	int bi = 0;			/* position of write head in output buffer */
	
	int tag = 0;		/* calculated depth of run (might not be   
						   equal to start_n if start_n==3)         */
	
	int inside = 0;		/*                                         */
	int start_n = 0;	/* length of starting run                  */
	int depth_e = 0;	/* current depth of emphasis (0, 1, 2, 3)  */
	
	printf("\x1b[35m%s\x1b[0m\n\n", chunk);
	
	for (i = 0; i < strlen(chunk); ) {
/*tst*/	if (!inside) {
/*tst*/		memset(innerbuffer, '#', MAX_CHUNK_SZ);
/*tst*/	}
		
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
					tag = start_n;
				} else if (j == 3) {
					memset(innerbuffer+bi, '*', 3);
					bi += 3;
					i += 3;
				}
			} else {
				if (start_n < 3) {
					if (j == start_n) {
		/*write*/		innerbuffer[bi] = '\0';
		/*write*/		printf("(s<3exact %d \"%s\")\n", tag, innerbuffer);
		/*write*/		inside = 0; 
		/*write*/		bi = 0; 
						i += start_n;
					} else if (j == 3) {
						if (depth_e == 3) {
							i += 3-start_n;
							memset(innerbuffer+bi, '*', 3-start_n);
							bi += 3-start_n;
			/*write*/		innerbuffer[bi] = '\0'; 
			/*write*/		printf("(s<3 d=3  %d \"%s\")\n", tag, innerbuffer);
			/*write*/		inside = 0; 
			/*write*/		bi = 0; 
							i += start_n;
						} else if (depth_e < 3) {
			/*write*/		innerbuffer[bi] = '\0'; 
			/*write*/		printf("(s<3 d<3  %d \"%s\")\n", tag, innerbuffer);
			/*write*/		inside = 0; 
			/*write*/		bi = 0; 
							i += start_n;
						}
					} else {
						depth_e += j;
						i += j;
						memset(innerbuffer+bi, '*', j);
						bi += j;
					}
				} else if (start_n == 3) {
					if (j == 3) {
						i += 2;
						tag = 1;
						memset(innerbuffer+bi, '*', 2);
						bi += 2;
		/*write*/		innerbuffer[bi] = '\0'; 
		/*write*/		printf("(s=3exact %d \"%s\")\n", tag, (innerbuffer+1));
		/*write*/		inside = 0; 
		/*write*/		bi = 0; 
						i += 1;
					} else {
						if (depth_e-j <= 0) {
							tag = j;
			/*write*/		innerbuffer[bi] = '\0'; 
			/*write*/		printf("(s=3 d>=3 %d \"%s\")\n", tag, 
										(innerbuffer+j));
			/*write*/		inside = 0; 
			/*write*/		bi = 0; 
							i += j;
						} else {
							depth_e -= j;
							i += j;
							memset(innerbuffer+bi, '*', j);
							bi += j;
						}
					}
				}
			}
		} else {
			innerbuffer[bi] = chunk[i];
			bi++;
			i++;
		}
		if (escape) {
			escape--;
		}
	}
	return(bi);
}


int parseinline(void *parent[], int parentsz,
						char *chunk,  int level, int depth) {
	
	int i, j, k;
	const char *delims3[18] = {	 "> ." , "\n.." ,   /*  0   blockquote>p  */ 
								 "- ." , "\n.." ,   /*  1   ul>li         */
								 "`.." ,  "`.." ,   /*  2   code          */
								 "***" ,  "***" ,   /*  3   strong>em     */
								 "**." ,  "**." ,   /*  4   strong        */
								 "*.." ,  "*.." ,   /*  5   em            */
								 "[.." ,  ").." ,   /*  6   a             */
								 "(.." ,  ".\0.",   /*  7   a>href        */
								 "[.." ,  "].." };  /*  8   a>text        */
	
	/*        transition[parent][child] is 1 if parent can contain child  */
	const int transition[10][9] =                   /*  ---- PARENT ----  */
	/*               bq li cd se st em  a ah at                           */
				{	{ 0, 0, 1, 1, 1, 1, 1, 0, 0},   /*  0   blockquote>p  */
					{ 0, 0, 1, 1, 1, 1, 1, 0, 0},   /*  1   ul>li         */
					{ 0, 0, 0, 0, 0, 0, 0, 0, 0},   /*  2   code          */
					{ 0, 0, 1, 0, 0, 1, 1, 0, 0},   /*  3   strong>em     */
					{ 0, 0, 1, 0, 0, 1, 1, 0, 0},   /*  4   strong        */
					{ 0, 0, 1, 0, 1, 0, 1, 0, 0},   /*  5   em            */
					{ 0, 0, 0, 0, 0, 0, 0, 1, 1},   /*  6   a             */
					{ 0, 0, 0, 0, 0, 0, 0, 0, 0},   /*  7   a>href        */
					{ 0, 0, 1, 1, 1, 1, 0, 0, 0},   /*  8   a>text        */
													/*                    */	
					{ 1, 1, 1, 1, 1, 1, 1, 0, 0} }; /*  9   toppest       */

	const char *types[10] = 
	  { "bq>", "li-", "cde", "sem", "str", " em", "a[)", "a()", "a[]", "TOP" };

	int toplevel = level;
	int mode = toplevel; /* plain text */

	int escape = 0;

	int subparses = 0;

	char contentbuffer[128];
	int ci = 0;

	int offset = 0;

	if (toplevel == L_A) {
		mode = L_A_TEXT;
	}
	
	for (i = 0; i <= strlen(chunk); ) {
		offset = 0;
		if (escape == 0 && chunk[i] == '\\') {
			escape = 1;
			i++;
		}
		if (escape == 0) {
			if (mode == toplevel) { /* check for any starting sequences */
				for (j = 0; j < 9; j++) {
					if (transition[toplevel][j] == 1) {
						/* get number of matched characters */
						k = checkdelim(delims3[2*j], chunk+i, 3);
						if (k != -1) {
							subparses++;
							contentbuffer[ci] = '\0';
							ci = 0;
							for (ci = 0; ci < depth; ci++) { printf("---"); }
							ci = 0;
							printf("\"\x1b[36m%s\x1b[0m\"\nst %s->%s\n",
									contentbuffer, types[mode], types[j]);
							i += k;
							mode = j;
							break;
						}
					}
				}
			} else { /* check for ending sequence */
				k = checkdelim(delims3[2*mode+1], chunk+i, 3);
				if (k != -1) {
					contentbuffer[ci] = '\0';
					ci = 0;
					offset = 1;
					for (ci = 0; ci < depth; ci++) { printf("---"); }
					ci = 0;
					printf("\t\"\x1b[36m%s\x1b[0m\"\nen %s<-%s\n", 
							contentbuffer, types[toplevel], types[mode]);
					i += k;
					mode = toplevel;
				}
			}
		}
		escape = 0;
		if (offset != 1) {
			contentbuffer[ci] = chunk[i];
			i++;
			ci++;
		}
	}

	if (subparses == 0) {
		for (ci = 0; ci < depth; ci++) { printf("==="); }
		printf("nf in   %s \"\x1b[33m%s\x1b[0m\"\n", types[mode], contentbuffer);
	}

	if (mode != toplevel) {
		fprintf(stderr, "\x1b[31m[builder] [parseinline] "
							"line ended below toplevel\x1b[0m\n");
	}
/*	printf("\n%d\n", mode);*/
/*
	strcpy(ret->content, c);
	strcpy(ret->attributes, a);

	free(c);
	free(a);
*/

	return(0); /* TODO return number of subelements so we know when to stop */
}

int checkdelim(const char *delim, char *string, int n) {
	int i = 0;
	int j = 0;
/*
	printf("checking %3d,%3d,%3d against %3d,%3d,%3d\n",
				delim[0],delim[1],delim[2],string[0],string[1],string[2]);
	printf("i.e.     %3c,%3c,%3c against %3c,%3c,%3c\n\n",
				delim[0],delim[1],delim[2],string[0],string[1],string[2]);
*/	
	while (i < n) {
		if (*(delim+i) != '.') {
			if (*(delim+i) != *(string+i)) {
/*				printf("\x1b[34m(%d!=%d)\x1b[0m", *(delim+i), *(string+i));*/
				return(-1);
			} else {
/*				printf("\x1b[32m(m: %d with %d)\x1b[0m\n", 
							*(delim+i), *(string+i));
*/				j++;
			}
		}
		i++;
	}
	return(j);
}

