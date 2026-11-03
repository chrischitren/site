#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>

#include "fnodes.h"

/* Recursively search a directory (Filenode with ftype=FTYPE_DIR) for
   FTYPE_REG Filenodes with [query] in their name. Returns number of hits,
   capped at ressz, and places pointers to hit Filenodes in res. */
int fnodesearch(Filenode *fnode, char *query, Filenode **res, int ressz) {
	int i = 0, n = 0;
	if (fnode->ftype != FTYPE_DIR) {
		printf("\x1b[31m[pages] [fnodesearch]"
				" provided path \"%s\" is not a directory\n", fnode->path);
		return(-1);
	}
	while (i < fnode->numchildren) {
		if (fnode->children[i] != NULL) {
			if (fnode->children[i]->ftype == FTYPE_DIR) {
				n += fnodesearch(fnode->children[i], query, res+n, ressz-n);
			} else if (fnode->children[i]->ftype == FTYPE_REG) {
				if (strstr(fnode->children[i]->name, query) != NULL) {
					if (n < ressz) {
						res[n] = fnode->children[i];
						n += 1;
					} else {
						printf("Search results overflowed provided buffer\n");
					}
				}
			}
		}
		i++;
	}
	return(n);
}

/* Recursively print a Filenode and its children */
void fnodeprint(Filenode *fnode, int depth) {
	int i = 0;
	if (fnode->ftype == FTYPE_DIR) {
		printf("\x1b[94m");
	} else if (fnode->ftype == FTYPE_REG) {
		if (strstr(fnode->name, ".md") != NULL) {
			printf("\x1b[93m");
		}
	} else {
		printf("\x1b[31m");
	}
	printf("%*s%s\x1b[0m\n",
			depth*4, "", fnode->name);
	while (i < fnode->numchildren) {
		if (fnode->children[i] != NULL) { 
			fnodeprint(fnode->children[i], depth+1);
		}
		i++;
	}
}



int buildfnodetree(Filenode *root, char *srcdir) {
	
	DIR *f;
	struct dirent *entry; /* for getting filenames and directory info */
	struct stat filestat; /* for checking if a file is a directory */
	Filenode *fnp;

	if ((f = opendir(srcdir)) == NULL) {
		printf("\x1b[31m[pages] [buildfnodetree] "
				"could not open directory \"%s\"", srcdir);
	}
	
	while ( (entry = readdir(f)) ) {
		if (entry->d_name[0] == '.') {
			continue;
		}
		fnp = fnodeinit(root, entry->d_name, FTYPE_IGNORE);
		stat(fnp->path, &filestat);
		if (S_ISREG(filestat.st_mode)) {
			fnp->ftype = FTYPE_REG;
		}
		if (S_ISDIR(filestat.st_mode)) {
			fnp->ftype = FTYPE_DIR;
			buildfnodetree(fnp, fnp->path);
		}
	}
	closedir(f);
	return(0);
}

/* Create a child Filenode and append it to _parent's children. The child's
   path is constructed by appending "/[_name]" to the parent's path. */
Filenode *fnodeinit(Filenode *_parent, char *_name, int _ftype) {
	int namesz;
	Filenode *fnode;

	namesz = strlen(_name) + 1;
	
	fnode = malloc(sizeof(Filenode));

	fnode->parent = _parent;

	fnode->children = NULL;
	fnode->numchildren = 0;
	
	fnode->name = malloc(sizeof(char) * namesz);
	strcpy(fnode->name, _name);

	fnode->ftype = _ftype;

	if (_parent == NULL) {
		/* A parentless Filenode's path is just its name */
		fnode->path = malloc(sizeof(char) * namesz);
		strcpy(fnode->path, _name);
	} else {
		/* Build the path from the name and the parent's name */
		fnode->path = malloc(sizeof(char) * (namesz+strlen(_parent->path)+1));
		strcpy(fnode->path, _parent->path);
		strcat(fnode->path, "/");
		strcat(fnode->path, _name);
		
		/* Ask to be made a child of your parent */
		_parent->children = realloc(_parent->children, 
							sizeof(Filenode*) * (_parent->numchildren+1));
		_parent->children[_parent->numchildren] = fnode;
		_parent->numchildren += 1;
	}
	
	return(fnode);
}

/* Recursively free a Filenode, its contents, its children, and their contents.
   Should only be used on a top-level filenode (one that is not the child of
   any other) since this function does not edit the parent's children list. */
int fnodefree(Filenode *fnode) {
	/* TODO write a better free function that can remove dangling pointers
	   from parent Filenodes' children lists. */
	int i = 0;
	while (i < fnode->numchildren) {
		if(fnode->children[i] != NULL) {
			fnodefree(fnode->children[i]);
		}
		i++;
	}
	if (fnode->children != NULL) {
		free(fnode->children);
	}
	free(fnode->name);
	free(fnode->path);
	free(fnode);
	return(0);
}
