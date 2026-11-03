#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../monkdown/parse.h"
#include "../monkdown/write.h"
#include "fnodes.h"

#define MAX_PAGES 64

typedef struct page Page;
struct page {
	Filenode *fnode;

	Node *nodeheader;
	Node *nodemain;
	Node *nodefooter;
	
	Page *links[MAX_PAGES];
	Page *backlinks[MAX_PAGES];
};


/* PAGE HANDLING FUNCTIONS */
Page *pageinit(Filenode *fnode);
void pagefree(Page *pg);
void pageprint(Page *pg);

/* DEPRECATED MISCELLANEA */
int mdtohtml(char *readdir, char *writedir);
Node *makenavtree(Node *doc);
void getcleannodetext(char *dest, int destsz, Node *n, int toplevel);


/* A Page is a collection of the things required to write a page (an HTML
   document) in situ. Thus it must possess information about links, backlinks,
   where the file it references is, etc. */

/* A Node is a syntax tree describing the formatting of text and objects in a
   document. It does not contain metadata about the document. */

/* A Filenode is an element of a tree whose structure is representative of an
   existing or hypothetical directory structure. It knows nothing of Nodes or
   Pages; it is an interface to the filesystem. */

/* The default behavior for resolving broken links should be to search for the
   *name* given in the offending file. This should be robust enough that if we
   want to lean on it as a crutch forever, giving everything a unique name and
   only ever referring to these names, we should be able to without issue. */

/* To preserve link integrity, duplicate page names are not tolerated, and no
   reference is made to the directory structure in the output folder. All
   apparent structuring of the site is illusory and for the benefit of a user
   navigating the site alone. As a consequence, internal links do not have to
   be well-formed, nor do they have to be repaired if the directory structure
   is altered. */

/* Good software depends on good specification. The implementation details of
   how character arrays are manipulated is always uncertain and should remain
   up to the discretion of someone who knows the hardware well enough to write
   efficient, reliable machine code. The deciding factor for portability and
   reliability is that the *behaviors* of routines like strcpy and strrchr, as
   well as the behaviors of any routines they depend on, are well-defined, and
   that any implementation of such standard routines exactly replicates this
   behavior. */

int main() {
	Filenode *fnode_sources;
	Filenode *fnode_config;
	
	int i;
	Filenode *fnodelistmd[MAX_PAGES];
	Page *pagelist[MAX_PAGES];
	
	char buf[256];
	char *outdir = "../output";

	fnode_config = fnodeinit(NULL, "../config", FTYPE_DIR);
	fnodebuildtree(fnode_config, fnode_config->path);
	fnodeprint(fnode_config, 0);
	
	for (i = 0; i < MAX_PAGES; i++) {
		fnodelistmd[i] = NULL;
		pagelist[i] = NULL;
	}
	
	fnode_sources = fnodeinit(NULL, "..", FTYPE_DIR);
	fnodebuildtree(fnode_sources, fnode_sources->path);
	fnodeprint(fnode_sources, 0);
	
	printf("\n");
	
	/* do something */
	printf("\x1b[96m%d results for \"%s\"\x1b[0m\n",
	fnodesearch(fnode_sources, ".md", fnodelistmd, MAX_PAGES), ".md");

	for (i = 0; i < MAX_PAGES; i++) {
		if (fnodelistmd[i] != NULL
				&& *(strstr(fnodelistmd[i]->path, ".md")+3) == '\0') {
			
			printf("Parsing source file \"%s\"\n", fnodelistmd[i]->path);
			fnoderelativepath(buf, 256, fnodelistmd[i], fnode_sources);
			printf("(relative path: \"\x1b[32m%s\x1b[0m/%s\")\n", outdir, buf);
			
			pagelist[i] = pageinit(fnodelistmd[i]);
			pageprint(pagelist[i]);
		}
	}
	
	/****************/
	
	/* Clean up all our allocated things */
	for (i = 0; i < MAX_PAGES; i++) {
		if (pagelist[i] != NULL) {
			printf("freeing page at %p\n", (void*) pagelist[i]);
			pagefree(pagelist[i]);
		}
	}
	
	fnodefree(fnode_sources);
	fnodefree(fnode_config);
	
	return(0);
}


/****************************************************************************/
/* PAGE HANDLING FUNCTIONS                                                  */
/****************************************************************************/

/* Initialize a Page from a .md source file at *fnode by opening the file,
   calling parsemd, and setting metadata. */
Page *pageinit(Filenode *fnode) {
	Page *pg;
	FILE *readfp = NULL;

	if (fnode == NULL) {
		printf("\x1b[31m[pages] [pageinit] "
				"received NULL Filenode, cannot create page\x1b[0m\n");
		return(NULL);
	}

	pg = malloc(sizeof(Page));
	
	pg->fnode = fnode;

	pg->nodeheader = NULL;
	pg->nodefooter = NULL;

	/* Open file pointers to relevant files */	
	if ((readfp = fopen(fnode->path, "r")) == NULL) {
		printf("\x1b[31m[pages] [pageinit] "
				"failed to open for reading: \"%s\"\x1b[0m\n", fnode->path);
		return(NULL);
	}
	if ((pg->nodemain = parsemd(readfp)) == NULL) {
		printf("\x1b[31m[pages] [pageinit] "
				"failed to parse: \"%s\"\x1b[0m\n", fnode->path);
		return(NULL);
	}
	
	fclose(readfp);
	return(pg);	
}

/* Free a Page's Nodes (if they exist) and then free the Page itself */
void pagefree(Page *pg) {
	if (pg->nodeheader != NULL) {
		freenodes(pg->nodeheader);
	}
	if (pg->nodemain != NULL) {
		freenodes(pg->nodemain);
	}
	if (pg->nodefooter != NULL) {
		freenodes(pg->nodefooter);
	}
	free(pg);
}

/* Print all the data of a Page for debugging */
void pageprint(Page *pg) {
	printf("\x1b[93m------ PAGE ------ (%p)\n", (void *)pg);

	printf("fnode->path: \"%s\"\n", pg->fnode->path);
	printf("fnode->name: \"%s\"\n", pg->fnode->name);
	printf("nodeheader: %4d nodes (%p)\n", countnodes(pg->nodeheader),
			(void *)pg->nodeheader);
	printf("nodemain:   %4d nodes (%p)\n", countnodes(pg->nodemain),
			(void *)pg->nodemain);
	printf("nodefooter: %4d nodes (%p)\n", countnodes(pg->nodefooter),
			(void *)pg->nodefooter);
	printf("links: ?\n"); /* TODO link printing */
	printf("backlinks: ?\n"); /* TODO backlink printing */

	printf("------------------\x1b[0m\n\n");
}

/****************************************************************************/
/* DEPRECATED MISCELLANEA                                                   */
/****************************************************************************/

int mdtohtml(char *readdir, char *writedir) {
	Node *doc = NULL;
	Node *navtreenode = NULL;	

	char *templatedir = "../config/_template.html";
	
	FILE *readfp = NULL;
	FILE *writefp = NULL;
	FILE *templatefp = NULL;
	
	struct stat filestat;
	
	char title[128];
	char *p;
	
	/* Make a title by finding just the file name (no leading directories),
	   stripping file extension, and replacing '_' with ' ' */
	if ((p = strrchr(writedir, '/')) != NULL) {
		strcpy(title, p+1);
	} else {
		strcpy(title, writedir);
	}
	
	if ((p = strrchr(title, '.')) != NULL) {
		*p = '\0';
	}
	
	while ((p = strchr(title, '_')) != NULL) {
		*p = ' ';
	}

	/* Ask user for permission to overwrite file at writedir if it exists */	
	if (stat(writedir, &filestat) == 0) {
		printf("\x1b[31mWARNING! Overwrite \"%s\"? (y/N)\x1b[0m ", writedir);
		if (getchar() != 'y') {
			printf("Exiting\n");
			return(1);
		}
	}
	if ((writefp = fopen(writedir, "w")) == NULL) {
		printf("\x1b[31m[pages] [mdtohtml] "
				"failed to open for writing: \"%s\"\x1b[0m\n", writedir);
		return(1);
	}
	if ((templatefp = fopen(templatedir, "r")) == NULL) {
		printf("\x1b[31m[pages] [mdtohtml] "
				"failed to open template: \"%s\"\x1b[0m\n", templatedir);
		return(1);
	}
	
	/* Parse the markdown source to a Node tree using parse.c */
	printf("Parsing \"%s\"\n", readdir);
	if ((doc = parsemd(readfp)) == NULL) {
		printf("Failed to parse, exiting\n");
		return(1);
	} else {
		printf("Found %d Nodes (1 document, %d blocks)\n",
				countnodes(doc), doc->numchildren); 
	}
	
	navtreenode = NULL;/* makenavtree(doc);*/ /* TODO fix everything here */

		

	printf("Destination: \"%s\"\n", writedir);
	
	
	printf("Writing to \"%s\" with template \"%s\"\n", writedir, templatedir);
	if (writehtmlfromtemplate(writefp, templatefp, title, navtreenode, doc, NULL) == 0) {
		fclose(writefp);
		stat(writedir, &filestat);
		printf("Wrote %ld bytes to \"%s\"\n",
				(long int) filestat.st_size, writedir);
	} else {
		fclose(writefp);
		printf("Failed to write.\n");
	}

	/* Close any files we might have left open */
	fclose(readfp);
	fclose(templatefp);
	
	/* Clean up Nodes on the heap */
	freenodes(doc);	
	freenodes(navtreenode);
	return(0);
}


Node *makenavheader(Page *pg) {
	
	return(NULL);
}


Node *makenavtree(Node *doc) {
	Node *ntnode = NULL;
	int i, j;
	Node *n;
	Node *a;
	char buf[32];
	char bigbuf[1024];

	ntnode = createnode("", 0, NULL, TYPE_MAXTYPE);

	for (i = 0; i < doc->numchildren; i++) {
		if (doc->children[i] != NULL) {
			n = doc->children[i];
			if (n->type >= TYPE_H1
					&& n->type <= TYPE_H6) {
				
				getcleannodetext(bigbuf, 1024, n, 1);
				printf("\x1b[33m%s\x1b[0m\n", bigbuf);

			/*	printf("%*s%s%s%s\n", 2*(n->type - TYPE_H1), "",
								"# ", n->children[0]->content,
								n->children[1]->attributes);*/
				for (j = 0; j < 2*(n->type - TYPE_H1); j++) {
					buf[j] = '#';
				}
				buf[j] = '\0';
				a = createnode(n->children[0]->content,
								n->children[0]->contentlen,
								createnode(buf,strlen(buf),ntnode,TYPE_P),
								TYPE_A);
				printnodes(ntnode, 0, 1); /* TODO debug */
				a->attributes = 
					malloc(sizeof(char)*(strlen(n->children[1]->attributes)+1));
				strcpy(a->attributes, n->children[1]->attributes);
			}
		}
	}
	return(ntnode);
}

void getcleannodetext(char *dest, int destsz, Node *n, int toplevel) {
	int i = 0;
	if (destsz <= 0) {
		return;
	}
	if (toplevel == 1) {
		strncpy(dest, n->content, destsz);
	} else {
		strncat(dest, n->content, destsz);
	}
	while (i < n->numchildren) {
		if (n->children[i] != NULL) {
			getcleannodetext(dest+strlen(dest), destsz-strlen(dest)-1, n->children[i], 0);
		}
		i++;
	}
}

/*

We need
	to build an index of files
		to make nav headers
		to make nav footers

We need to read all the files 
	to know the directory structure
		to make nav headers
	to extract all their links
		to build an adjacency matrix to find orphans
		(to curl urls to check for dead links)
		to find backlinks
			to make nav footers

We need to copy relevant files to the output directory
	like stylesheets, attachments, etc.
	everything that isn't a markdown file?

Let's talk about these nav headers. How will they look? 

*/
