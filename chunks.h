#ifndef CHUNKS_H
#define CHUNKS_H
#define C_E_MAXCHILDREN 16

enum _cetype {
	TYPE_ATTRIBUTE = -1,
	TYPE_PLAIN = 0, /* plain text!!! */
	
/****** ROOT NODE (FULL CHUNK) TYPES ******/
	TYPE_H1, /****************************/
	TYPE_H2, /*                         */
	TYPE_H3, /*     headings must      */
	TYPE_H4, /*     be ordered.       */
	TYPE_H5, /*                      */
	TYPE_H6, /***********************/
	TYPE_IMG,TYPE_BLOCKQUOTE,TYPE_PRE,TYPE_UL,TYPE_OL,
	
/************* ERRANT ONES. *************/
	TYPE_P, /* whuh?!? */

/************* INLINE TYPES *************/
	TYPE_LI,
	TYPE_EM,     /*   strong must    */
	TYPE_STRONG, /*   follow em.    */
	TYPE_A,TYPE_CODE,
	TYPE_MAXTYPE /* used for indexing */
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

ChunkElement *parsechunk(char *_chunkstr, int _chunksz, int _type, int _maxdepth);
int parseopenleaves(ChunkElement *c);
ChunkElement *parsechunkelement(ChunkElement *c);

int scansimple(char *chunk, int *_contentsz, char delim1, char delim2);
int scanasterisk(char *chunk, int *_contentsz, int *_delimsz);

int countopenleaves(ChunkElement *c);

ChunkElement *initchunk(char *_chunkstr, int _chunksz, int _chunkpos,
						int _type, int _open, ChunkElement *_parent);
int rfree(ChunkElement *c);

#endif
