#include <stdio.h>
#include <string.h>
#include "librarian.h"

/*
struct page { 
	FILE *fp;
	char *cleanname;
	
} */

int main() {

/*	Use makemdindex from librarian to walk the src folder
	and make a list of md files. */	
	FILE *_index = makemdindex("_index.txt", "src");
/*	FILE *_head;
	if ((_head = fopen("head.html", "r")) == NULL) {
		printf("could not find head.html");
		exit(1);
	}
*/
	char *_htmloutdir = "main";

/*	Loop through the lines of the list to retrieve the directories.
	Open file pointers to each markdown source in sequence. */
	char indexlinedir[MAXLEN];
	while (fgets(indexlinedir, MAXLEN, _index) != NULL) {
		FILE *tempfile;
		
		size_t len = strcspn(indexlinedir, "\n\r");
		indexlinedir[len] = '\0';
		
		size_t lenfilename = strcspn((strrchr(indexlinedir, '/') + 1), ".");
		char output[lenfilename+5]; /* five extra chars for ".html" */
		strncpy(output, (strrchr(indexlinedir, '/') + 1), lenfilename);
		output[lenfilename] = '\0';
		char *cleanname = output;
		strcat(output, ".html");
		
		
		if ((tempfile = fopen(indexlinedir, "r")) == NULL) {
			printf("could not open file \x1b[322m%s\x1b[0m\n", indexlinedir);
			continue;
		}
		
		printf("\t\x1b[36m%s\x1b[0m\n", indexlinedir);
		printf("\t-> %s\n", output);
		/*
		char c;
		while ((c = fgetc(tempfile)) != EOF) {
			fputc(c, stdout);
		}
		*/
		fclose(tempfile);
	}
	
	fclose(_index);
	
	return(0);
}
