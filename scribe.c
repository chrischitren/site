#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "librarian.h"

#define MAXLINE 1024
#define PAGEWIDTH_PX 512

/*
struct page { 
	FILE *fp;
	char *cleanname;
	
} */

int parsewholeline(char *out, char *line, int max);
int sanitizehtml(char *out, char *line, int maxdest, int n);

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
	const char *_htmloutdir = "main";

/*	Loop through the lines of the list to retrieve the directories.
	Open file pointers to each markdown source in sequence. */
	char indexlinedir[MAXDIR];
	while (fgets(indexlinedir, MAXDIR, _index) != NULL) {
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
			fprintf(stderr, "could not open file \x1b[31m%s\x1b[0m\n", indexlinedir);
			continue;
		}
		
		printf("\t\x1b[36m%s\x1b[0m\n", indexlinedir);
		printf("\t-> %s\n", output);
		
		/* * * * * * * * * * * * * * * */
		/* fetch each line of the file */
		char l[MAXLINE];
		char lnew[MAXLINE];
		while (fgets(l, MAXLINE, tempfile) != NULL) {
			printf("%d %c\n", l[0], (l[0]=='\n') ? ' ' : l[0]);
			if (l[0] == '!' || l[0] == '#') {
				if (parsewholeline(lnew, l, MAXLINE) == 0){
					printf("\t\x1b[32m%s\x1b[0m\n", lnew);
				}
			}
		}
		/* * * * * * * * * * * * * * * */
		
		fclose(tempfile);
	}
	
	fclose(_index);
	
	return(0);
}

int sanitizehtml(char *out, char *line, int maxdest, int n) {

	if (n < 0) {
		n = strlen(line);
	}
	
	char outbuf[maxdest];
	memset(outbuf, 0, maxdest);
	int i = 0, j = 0;
	for (; i < n && j < maxdest-6; i++) {
		switch (line[i]) {
			case '<':
				memcpy(outbuf+j, "&lt;", 4);
				j += 4;
				break;
			case '>':
				memcpy(outbuf+j, "&gt;", 4);
				j += 4;
				break;
			case '&':
				memcpy(outbuf+j, "&amp;", 5);
				j += 5;
				break;
			case '\"':
				memcpy(outbuf+j, "&quot;", 6);
				j += 6;
				break;
			case '\'':
				memcpy(outbuf+j, "&#39;", 5);
				j += 5;
				break;
			default:
				memset(outbuf+j, line[i], 1);
				j++;
		}
	}
	memset(outbuf+j, '\0', 1);

	strncpy(out, outbuf, maxdest);

	return(0);
}

int parsewholeline(char *out, char *line, int max) {
	char outbuf[max];
	memset(outbuf, 0, max);

	if (line[0] == '!') { /* Image line */

		const char *htmlpatternimg = "<a href=\"%s\"><img src=\"%s\" alt=\"%s\" width=%d></a>";

		/* greedy, but with standards: check if a valid substring exists */
		int i = 1;
		int indices[4] = {0,0,0,0};
		/* string:   ![link text goes here](link_url)                    */
		/* index:      0                  1 2       3                    */
		while (i < strlen(line)) {
			if (line[i] == '[' && !indices[0]) {
				indices[0] = i+1;
			} else if (line[i] == '(' && line[i-1] == ']' && indices[0]) {
				indices[1] = i-1;
				indices[2] = i+1;
			} else if (line[i] == ')' && indices[2]) {
				indices[3] = i;
				break;
			}
			i++;
		}

		if(!indices[0] || !indices[1] || !indices[3]) { 
			fprintf(stderr, "ERROR: malformed image link\n"); return(1);
		}

		char alt[indices[1]-indices[0]+1];
		char url[indices[3]-indices[2]+1];
		char urlpreview[indices[3]-indices[2]+8];

		strncpy(alt, line+indices[0], indices[1]-indices[0]);
		alt[indices[1]-indices[0]] = '\0';
		strncpy(url, line+indices[2], indices[3]-indices[2]);
		url[indices[3]-indices[2]] = '\0';

		char *lastslash = strrchr(url, '/');
		strncpy(urlpreview, url, strlen(url)-strlen(lastslash)+1);
		urlpreview[strlen(url)-strlen(lastslash)+1] = '\0';
		strcat(urlpreview, "preview_");
		strcat(urlpreview, lastslash+1);
		urlpreview[indices[3]-indices[2]+8] = '\0';

		snprintf(outbuf, max, htmlpatternimg, url, urlpreview, alt, PAGEWIDTH_PX);

	} else if (line[0] == '#') { /* Header line */
		
		int i = 0;
		while (line[i] == '#' && i < max) {
			i++;
		}
		
		if (i > 6) { fprintf(stderr, "ERROR: header larger than 6\n"); return(1); }
		if (line[i] != ' ') { fprintf(stderr, "ERROR: no space after header\n"); return(1); }
		
		const char htmlhpre[4] = {'<','h',i+48,'>'};
		const char htmlhpost[5] = {'<','/','h',i+48,'>'};

		size_t lenheader = strcspn(line+i+1, "\n\r");
		
		if (lenheader > MAXLINE - 10) { fprintf(stderr, "ERROR: header too long\n"); return(1); }

/*		strcpy(outbuf, htmlhpre);*/
		/************************* SANITIZE!!! ****************************/
		char sanitized[lenheader*6];
		
		sanitizehtml(sanitized, line+i+1, lenheader*6, lenheader);
		
		strncpy(outbuf+4, sanitized, strlen(sanitized));
		/******************************************************************/
/*		strcpy(outbuf+4+strlen(sanitized), htmlhpost);
		outbuf[strlen(sanitized)+9] = '\0';*/

		snprintf(outbuf, max, "<h%d>%s</h%d>", i, sanitized, i);
	} else {
		return(1);
	}

	strncpy(out, outbuf, max);

	return(0);
}
