#ifndef FNODES_H
#define FNODES_H

/****************** FILENODE FUNCTIONS ******************/
typedef struct fnode Filenode;
struct fnode {
	Filenode *parent;
	Filenode **children; int numchildren;
	char *name; char *path;
	int ftype;
};

enum ftypes { FTYPE_IGNORE = -1, FTYPE_REG = 0, FTYPE_DIR = 1 };

/* Search for FTYPE_REG Filenodes with [query] in their name */
int fnodesearch(Filenode *fnode, char *query, Filenode **res, int ressz);
/* Printing */
void fnodeprint(Filenode *fnode, int depth);
/* Filenode manipulation functions */
Filenode *fnodeinit(Filenode *_parent, char *_name, int _ftype);
int fnodefree(Filenode *fnode);
/* Recursive directory search */
int buildfnodetree(Filenode *root, char *srcdir);
/********************************************************/

#endif
