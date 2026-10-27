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
	
	printf("Destination: \"%s\"\n", writedir);
	
	
	printf("Writing to \"%s\" with template \"%s\"\n", writedir, templatedir);
	if (writehtmlfromtemplate(writefp, templatefp, title, NULL, doc, NULL) == 0) {
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
	freerecursive(doc);	
	return(0);
}
