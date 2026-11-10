#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../monkdown/nodes.h"
#include "../monkdown/parse.h"
#include "../monkdown/write.h"
#include "fnodes.h"

#define MAX_PAGES 256
#define MAX_URLSZ 256

typedef struct page Page;
struct page {
	Filenode *fnode;

	Node *nodeheader;
	Node *nodemain;
	Node *nodefooter;
	
	Page *outlinks[MAX_PAGES];
	Page *backlinks[MAX_PAGES];
};

/* wrong place */
void nodeprintlinks(Node *n);

enum linktypes { LINKTYPE_WEB = 0, LINKTYPE_PAGE,
				LINKTYPE_FRAGMENT, LINKTYPE_OTHER };
int guesslink(char *linkstr);


/* PAGE HANDLING FUNCTIONS */
Page *pageinit(Filenode *fnode);
void pagefree(Page *pg);
void pageprint(Page *pg);

/* DEPRECATED MISCELLANEA */
int mdtohtml(char *readdir, char *writedir);
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
	
	int i, j, k;
	Filenode *fnodelistmd[MAX_PAGES];
	Page *pagelist[MAX_PAGES];

	Filenode *fnodelistother[MAX_PAGES];

	Node *nptr;
	Page *pgptr;
	Filenode *fnptrbuf[MAX_PAGES];
	Filenode *fnptr;

	char buf[256];
	char buf2[256];
	char *p;
	int exactmatchfound = 0;

/*	char *outdir = "../output";*/

	/* Initialize empty lists of Pages and Filenodes */
	for (i = 0; i < MAX_PAGES; i++) {
		fnodelistmd[i] = NULL;
		pagelist[i] = NULL;
		fnodelistother[i] = NULL;
	}
	
	/* Open the "config" directory */
	fnode_config = fnodeinit(NULL, "../config", FTYPE_DIR);
	fnodebuildtree(fnode_config, fnode_config->path);
	fnodeprint(fnode_config, 0);

	printf("\n");
	
	/* Open the source directory */
	fnode_sources = fnodeinit(NULL, "/mnt/c/Users/1poof/Documents/aaaeph", FTYPE_DIR);
	fnodebuildtree(fnode_sources, fnode_sources->path);
	fnodeprint(fnode_sources, 0);
	
	printf("\n");
	
	/* Search the source directory for .md files */
	printf("Found %d results for \"%s\" in \"%s\"\n",
	fnodesearch(fnode_sources, ".md", fnodelistmd, MAX_PAGES), ".md",
				fnode_sources->path);

	printf("\n");
		
	/* Make a Page for each source file */
	for (i = 0; i < MAX_PAGES; i++) {
		if (fnodelistmd[i] != NULL
				&& *(strstr(fnodelistmd[i]->path, ".md")+3) == '\0') {
			
			printf("Parsing source file \"%s\"\n", fnodelistmd[i]->path);
/*			fnoderelativepath(buf, 256, fnodelistmd[i], fnode_sources);
			printf("(relative path: \"\x1b[32m%s\x1b[0m/%s\")\n", outdir, buf);
*/			
			if ((pagelist[i] = pageinit(fnodelistmd[i])) == NULL) {
				printf("\x1b[31m[pages] "
						"received NULL Page for path %s\x1b[0m\n", 
						fnodelistmd[i]->path);
			}
			/*printf("\n");*/
		}
	}
	
	printf("\n");
	
	/* Get the links in every page. We need to store them... */
	for (i = 0; i < MAX_PAGES; i++) {
		if (pagelist[i] == NULL) {
			continue;
		}
		
/*		pageprint(pagelist[i]);*/
		strcpy(buf, pagelist[i]->fnode->name);
		if ((p = strrchr(buf, '/')) != NULL) {
			memmove(buf, p+1, strlen(buf)+1);
		}
		if ((p = strrchr(buf, '.')) != NULL) {
			*p = '\0';
		}

		printf("Page source at %s (\"%s\")\n",
				pagelist[i]->fnode->path, buf);
		
		/* Find all the hrefs */
		nptr = pagelist[i]->nodemain;
		while ((nptr = nextnodewithattributekey(nptr, "href")) != NULL) {
			strcpy(buf, getattributebykey(nptr, "href")->value);
			if (guesslink(buf) == LINKTYPE_PAGE) {
				if ((p = strrchr(buf, '/')) != NULL) {
					memmove(buf, p+1, strlen(buf)+1);
				}
				if ((p = strrchr(buf, '.')) != NULL) {
					*p = '\0';
				}
				printf("    Link to \"%s\", searching for page \"%s\"\n",
						getattributebykey(nptr, "href")->value, buf);
				pgptr = NULL;
				exactmatchfound = 0;
				/* Search all the Pages for one with name containing the link
				   referent's filename, stopping if we find an exact match. */
				for (j = 0; j < MAX_PAGES; j++) {
					if (i == j || pagelist[j] == NULL) {
						continue;
					}
					strcpy(buf2, pagelist[j]->fnode->name);
					if ((p = strrchr(buf2, '.')) != NULL) {
						*p = '\0';
					}
					if (strstr(buf2, buf) != NULL) {
						if (strcmp(buf2, buf) == 0) {
							printf("      %c\x1b[32m page at "
									"%s (\"%s\")\x1b[0m\n",
								(exactmatchfound==0) ? '>' : ' ',
								pagelist[j]->fnode->path, buf2);
							if (exactmatchfound == 1) {
								printf("        \x1b[31mWARNING! Multiple "
										"exact matches found!\x1b[0m\n");
							} else {
								exactmatchfound = 1;
								pgptr = pagelist[j];
							}
						} else if (exactmatchfound == 0) {
							printf("      ?\x1b[33m page at "
										"%s (\"%s\")?\x1b[0m\n",
								pagelist[j]->fnode->path, buf2);
							pgptr = pagelist[j];
						}
					}
				}
				/* If we found a match for the referent, */
				if (pgptr != NULL) {
					if (exactmatchfound == 0) {
						printf("        \x1b[31mWARNING! "
								"Approximate match at \"%s\" used!\x1b[0m\n",
								pgptr->fnode->path);
					}
					/* Update Node's Attribute to conform to the output
					   directory structure */
					printf("        Updating Node's Attribute value for key "
							"'href' to \"%s\"\n", pgptr->fnode->name);
					removeattribute(nptr, "href");
					addattribute(nptr, "href",
							pgptr->fnode->name, strlen(pgptr->fnode->name));
					/* Store pointer to referent's Page in outlinks */
					for (j = 0; j < MAX_PAGES; j++) {
						if (pagelist[i]->outlinks[j] == NULL) {
							pagelist[i]->outlinks[j] = pgptr;
							break;
						} else {
							if (pagelist[i]->outlinks[j] == pgptr) {
								break;
							}
						}
					}
					if (j == MAX_PAGES) {
						printf("\x1b[31mWARNING! Page %s"
								" has full outlink list\x1b[0m\n",
								pagelist[i]->fnode->name);
					}
					/* Store pointer to the Page we're checking in the ref-
					   erent's list of backlinks ('pages that link here') */
					for (j = 0; j < MAX_PAGES; j++) {
						if (pgptr->backlinks[j] == NULL) {
							pgptr->backlinks[j] = pagelist[i];
							break;
						} else {
							if (pgptr->backlinks[j] == pagelist[i]) {
								break;
							}
						}
					}
					if (j == MAX_PAGES) {
						printf("\x1b[31mWARNING! Page %s"
								" has full backlink list\x1b[0m\n",
								pgptr->fnode->name);
					}
				/* Warn if we could not find even an approximate match */
				} else {
					printf("        \x1b[31mWARNING! "
							"No match found!\x1b[0m\n");
				}
			} else if (guesslink(buf) == LINKTYPE_WEB) {
				printf("    Link to \"%s\" - web\n", buf);
				/* TODO Perhaps use libcurl to check for dead links */
			} else if (guesslink(buf) == LINKTYPE_FRAGMENT) {
				printf("    Link to \"%s\" - fragment\n", buf);
			} else if (guesslink(buf) == LINKTYPE_OTHER) {
				printf("    Link to \"%s\" - attachment?\n", buf);
				k = fnodesearch(fnode_sources, buf, fnptrbuf, MAX_PAGES);
				exactmatchfound = 0;
				for (j = 0; j < k; j++) {
					if (strcmp(buf, fnptrbuf[j]->name) == 0) {
						printf("      %c \x1b[32m%s\x1b[0m\n",
							(exactmatchfound==0) ? '>' : ' ',
							fnptrbuf[j]->path);
						if (exactmatchfound == 1) {
							printf("        \x1b[31mWARNING! Multiple "
									"exact matches found!\x1b[0m\n");
						} else {
							exactmatchfound = 1;
							fnptr = fnptrbuf[j];
						}
					} else if (exactmatchfound == 0) {
						printf("      ?\x1b[33m %s\x1b[0m\n",
								fnptrbuf[j]->path);
						fnptr = fnptrbuf[j];
					}
				}
				if (j > 0) {
					/* TODO Update link in Node; store link/file info. */
					if (exactmatchfound == 0) {			
						printf("        \x1b[31mWARNING! "
								"Approximate match at \"%s\" used!\x1b[0m\n",
								fnptr->path);
					}
					printf("        Updating Node's Attribute \"href\" to"
							" \"%s\"\n", fnptr->name);
					removeattribute(nptr, "href");
					addattribute(nptr, "href",
									fnptr->name, strlen(fnptr->name));
				} else {
					printf("        \x1b[31mWARNING! "
							"No match found!\x1b[0m\n");
				}
				for (j = 0; j < nptr->numchildren; j++) {
					if (nptr->children[j]->type == TYPE_IMG) {
						printf("        Link has image requesting %s\n",
							getattributebykey(nptr->children[j],"src")->value);
					}
				}
			} else {
				printf("\x1b[31mUnidentifiable link type: "
						"\"%s\"\x1b[0m\n", buf);
			}
		}
		/*
		nptr = pagelist[i]->nodemain;
		while ((nptr = nextnodewithattributekey(nptr, "src")) != NULL) {
			printf("    Link to \"%s\" - image?\n",
					getattributebykey(nptr, "src")->value);
			
			printf("        \x1b[31mWARNING! Not handled\x1b[0m\n");
		}*/
	}

	printf("\n");
	
	for (i = 0; i < MAX_PAGES; i++) {
		if (pagelist[i] != NULL) {
			if (pagelist[i]->backlinks[0] == NULL) {
				printf("\x1b[31m");
			}
			printf("%s\x1b[0m\n", pagelist[i]->fnode->name);
			for (j = 0; j < MAX_PAGES; j++) {
				if (pagelist[i]->outlinks[j] != NULL) {
					printf("    %s\n",
							pagelist[i]->outlinks[j]->fnode->name);
				}
			}
			for (j = 0; j < MAX_PAGES; j++) {
				if (pagelist[i]->backlinks[j] != NULL) {
					printf("    \x1b[93m%s\x1b[0m\n",
							pagelist[i]->backlinks[j]->fnode->name);
				}
			}
		}
	}	

/*
	for (i = 0; i < MAX_PAGES; i++) {
		if (pagelist[i] != NULL) {
			pageprint(pagelist[i]);
		}
	}
*/

	/* Clean up all our allocated things */
	for (i = 0; i < MAX_PAGES; i++) {
		if (pagelist[i] != NULL) {
/*			printf("freeing page at %p\n", (void*) pagelist[i]);*/
			pagefree(pagelist[i]);
		}
	}
	fnodefree(fnode_sources);
	fnodefree(fnode_config);
	
	return(0);
}


/* We would really like a function that treats the Node as a stream; it stores
   a pointer to the last Node that returned a hit so that we can put it in a
   while loop until it returns a signal that no hits were found. */

int guesslink(char *linkstr) {
	if ( (strncmp(linkstr, "https://", 8) == 0)
			|| (strncmp(linkstr, "http://", 7) == 0)
			|| (strstr(linkstr, "//") != NULL)
			|| (strncmp(linkstr, "www.", 4) == 0) ) {
		return(LINKTYPE_WEB);
	}
	if (linkstr[0] == '#') {
		return(LINKTYPE_FRAGMENT);
	}
	if ( (strstr(linkstr, ".md") != NULL)
			|| (strstr(linkstr, ".html") != NULL)
			|| (strchr(linkstr, '.') == NULL) ) {
		return(LINKTYPE_PAGE);
	}
	return(LINKTYPE_OTHER);
}


/****************************************************************************/
/* PAGE HANDLING FUNCTIONS                                                  */
/****************************************************************************/

/* Initialize a Page from a .md source file at *fnode by opening the file,
   calling parsemd, and setting metadata. */
Page *pageinit(Filenode *fnode) {
	Page *pg;
	FILE *readfp = NULL;
	int i;

	if (fnode == NULL) {
		printf("\x1b[31m[pages] [pageinit] "
				"received NULL Filenode, cannot create page\x1b[0m\n");
		return(NULL);
	}

	pg = malloc(sizeof(Page));
	
	pg->fnode = fnode;

	pg->nodeheader = NULL;
	pg->nodefooter = NULL;

	for (i = 0; i < MAX_PAGES; i++) {
		pg->outlinks[i] = NULL;
		pg->backlinks[i] = NULL;
	}

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
	printf("outlinks: ?\n"); /* TODO link printing */
	printf("backlinks: ?\n"); /* TODO backlink printing */

	printf("------------------\x1b[0m\n");
}

/****************************************************************************/
/* DEPRECATED MISCELLANEA                                                   */
/****************************************************************************/

void nodeprintlinks(Node *n) {
	int i = 0;
	char buf[MAX_ATTRIBUTESZ];
	
	buf[0] = '\0';
	
	if (n == NULL) {
		return;
	}
	
	if (n->attributes != NULL) {
		if (n->type == TYPE_A) {
			writeattributebykey(buf, MAX_ATTRIBUTESZ, n, "href");
		}
		if (n->type == TYPE_IMG) {
			writeattributebykey(buf, MAX_ATTRIBUTESZ, n, "src");
		}
	}
	if (buf[0] != '\0') {
		printf("\x1b[33m%s\x1b[0m", buf);
		switch (guesslink(buf)) {
			case LINKTYPE_WEB:
				printf(" (web)\n");
				break;
			case LINKTYPE_PAGE:
				printf(" (page)\n");
				break;
			case LINKTYPE_FRAGMENT:
				printf(" (fragment)\n");
				break;
			case LINKTYPE_OTHER:
				printf(" (other)\n");
				break;
			default:
				break;
		}
	}
	while (i < n->numchildren) {
		if (n->children[i] != NULL) {
			nodeprintlinks(n->children[i]);
		}
		i++;
	}
}


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
