#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../monkdown/parse.h"
#include "../monkdown/write.h"
#include "fnodes.h"

/******************** PAGE BUILDING *********************/
typedef struct page Page;
struct page {
	char path[512];
	char *title;
	Node *doc;
	Page **links;
};

/* Page building */
int mdtohtml(char *readdir, char *writedir);
Node *makenavtree(Node *doc);
/********************************************************/




int main() {
	Filenode *fnode_sources;
/*	Filenode *fnode_config;*/

	int i;
	Filenode *fnodes_md[20];

	for (i = 0; i < 20; i++) {
		fnodes_md[i] = NULL;
	}

	fnode_sources = fnodeinit(NULL, "../sources", FTYPE_DIR);
/*	fnode_config = fnodeinit(NULL, "../config", FTYPE_DIR);*/
	
	buildfnodetree(fnode_sources, fnode_sources->path);
/*	buildfnodetree(fnode_config, fnode_config->path);*/

	fnodeprint(fnode_sources, 0);
/*	fnodeprint(fnode_config, 0);*/

	/* do something */
	printf("\x1b[96m%d results for \"%s\"\x1b[0m\n",
	fnodesearch(fnode_sources, ".md", fnodes_md, 20), ".md");

	for (i = 0; i < 20; i++) {
		if (fnodes_md[i] != NULL) {
			printf("%s\n", fnodes_md[i]->name);
		} else {
			printf("NULL\n");
		}
	}
	
	/****************/
	
	fnodefree(fnode_sources);
/*	fnodefree(fnode_config);*/
	
/*	mdtohtml("../sources/monk.md", "../output/monk.html"); */
	return(0);
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

int mdtohtml(char *readdir, char *writedir) {
	Node *doc = NULL;
	Node *navtreenode = NULL;	

	char *templatedir = "../config/_template.html";
	
	FILE *readfp;
	FILE *writefp;
	FILE *templatefp;
	
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

	/* Open file pointers to relevant files */	
	if ((readfp = fopen(readdir, "r")) == NULL) {
		printf("\x1b[31m[pages] [mdtohtml] "
				"failed to open for reading: \"%s\"\x1b[0m\n", readdir);
		return(1);
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
	
	navtreenode = makenavtree(doc);
	
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

Node *makenavtree(Node *doc) {
	Node *ntnode = NULL;
	int i, j;
	Node *n;
	Node *a;
	char buf[32];

	ntnode = createnode("", 0, NULL, TYPE_MAXTYPE);

	for (i = 0; i < doc->numchildren; i++) {
		if (doc->children[i] != NULL) {
			n = doc->children[i];
			if (n->type >= TYPE_H1
					&& n->type <= TYPE_H6) {
				
				printf("%*s%s%s%s\n", 2*(n->type - TYPE_H1), "",
								"# ", n->children[0]->content,
								n->children[1]->attributes);
				for (j = 0; j < 2*(n->type - TYPE_H1); j++) {
					buf[j] = '#';
				}
				buf[j] = '\0';
				a = createnode(n->children[0]->content,
								n->children[0]->contentlen,
								createnode(buf,strlen(buf),ntnode,TYPE_P),
								TYPE_A);
				a->attributes = 
					malloc(sizeof(char)*(strlen(n->children[1]->attributes)+1));
				strcpy(a->attributes, n->children[1]->attributes);
			}
		}
	}
	return(ntnode);
}
