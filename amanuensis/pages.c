#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../monkdown/parse.h"
#include "../monkdown/write.h"

int mdtohtml(char *readdir, char *writedir);

int main() {
	mdtohtml("README.md", "README.html");
	return(0);
}

int mdtohtml(char *readdir, char *writedir) {
	Node *doc = NULL;
	FILE *wfp;
	struct stat filestat;
	char *tdir = "_template.html";
	char title[128];
	char *p;
	
	/* Make a title by finding just the file name (no leading directories),
	   stripping file extension, and replacing '_' with ' ' */
	if ((p = strrchr(writedir, '/')) != NULL) {
		strcpy(title, p);
	} else {
		strcpy(title, writedir);
	}
	
	if ((p = strrchr(title, '.')) != NULL) {
		*p = '\0';
	}
	
	while ((p = strchr(title, '_')) != NULL) {
		*p = ' ';
	}
	
	/* Parse the markdown source to a Node tree using parse.c */
	printf("Parsing \"%s\"\n", readdir);
	if ((doc = parsemd(readdir)) == NULL) {
		printf("Failed to parse, exiting\n");
		return(1);
	} else {
		printf("Found %d Nodes (1 document, %d blocks)\n",
				countnodes(doc), doc->numchildren); 
	}
	
	printf("Destination: \"%s\"\n", writedir);
	
	/* Ask user for permission to overwrite file at writedir if it exists */	
	if (stat(writedir, &filestat) == 0) {
		printf("\x1b[31mWARNING! Overwrite \"%s\"? (y/N)\x1b[0m ", writedir);
		if (getchar() != 'y') {
			printf("Exiting\n");
			return(1);
		}
	}
	
	/* Open writedir */
	if ((wfp = fopen(writedir, "w")) == NULL) {
		printf("\x1b[31m[write] [mdtohtml] "
				"failed to open for writing: \"%s\"\x1b[0m\n", writedir);
		return(1);
	}
	
	printf("Writing to \"%s\" with template \"%s\"\n", writedir, tdir);
	if (writehtmlfromtemplate(wfp, tdir, title, NULL, doc, NULL) == 0) {
		fclose(wfp);
		stat(writedir, &filestat);
		printf("Wrote %ld bytes to \"%s\"\n",
				(long int) filestat.st_size, writedir);
	} else {
		printf("Failed to write.\n");
	}
	
	/* Clean up Nodes on the heap */
	freerecursive(doc);	
	return(0);
}
