#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 16384

#define MAX_CHILDREN 64

typedef struct element Element;

typedef enum {
	TAG_MAIN,
	TAG_P,
	TAG_EM,
	TAG_STRONG,
	TAG_A,
	TAG_H1, TAG_H2, TAG_H3, TAG_H4, TAG_H5, TAG_H6,
	TAG_CODE,
	TAG_BLOCKQUOTE,
	TAG_UL,
	TAG_LI,
	TAG_PRE
} Tag;

Element *allocelem(Element *p, Tag t);
int parsetree(Element *root, const char *source, int startreadhead);

struct element {
	Element *parent;                     /*     8   */
	Element *children[MAX_CHILDREN];     /*  64*8   */
	Tag tag;                             /*     4   */
	long start, end,                     /*   6*8   */
			startc, endc, 
			starta, enda;
};

int main() {
	Element *emain = NULL;
	emain = malloc(sizeof(Element));
	emain->tag = TAG_MAIN;

/*	allocelem(emain, TAG_P);*/
	
	/* READ THE FILE INTO A BUFFER (FIXED LENGTH, ONE READ)
		TODO: READ AS SLIDING WINDOW/STREAM. */

	FILE *fp = fopen("README.md", "r");
	
	if (fp == NULL) {
		fprintf(stderr, "Failed to open file");
	}

	char secbuf[BUFFER_SIZE];
	long gotlength = 0;
	if (ftell(fp) == 0) { /* first char is newline to simplify processing */
		secbuf[0] = '\n';
		if ((gotlength = fread(secbuf+1, sizeof(char), BUFFER_SIZE-1, fp))
			< BUFFER_SIZE-1) {
			secbuf[gotlength+1] = '\0';
		}
	} else {
		if ((gotlength = fread(secbuf, sizeof(char), BUFFER_SIZE, fp)) 
			< BUFFER_SIZE) {
			secbuf[gotlength] = '\0';
		}
	}
	
	int codeblock = 0;
	int ci = 0;
	while (ci < BUFFER_SIZE-1 && secbuf[ci]!='\0') {
		/* SKIP ALL CONSECUTIVE NEWLINES */
		while (ci < BUFFER_SIZE-1 
				&& secbuf[ci]=='\n' && secbuf[ci+1]=='\n') {
			ci++;
		}
		
		/* CHECK IF NEXT BLOCK WILL BE A CODE BLOCK */
		if (secbuf[ci+1]=='`' && secbuf[ci+2]=='`' && secbuf[ci+3]=='`') {
			codeblock = 1;
			ci += 4;
		}
		
		(codeblock == 1) ? printf("\x1b[32m\n\\")
						 : printf("\x1b[31m\n\\");
		
		/* READ BLOCK */
		while (ci < BUFFER_SIZE-1 && secbuf[ci]!='\0') {
			if (codeblock == 0) {
				if (secbuf[ci]=='\n' && secbuf[ci+1]=='\n') {
					break;
				}
			} else {
				if (secbuf[ci]=='\n' && secbuf[ci+1]=='`'
					 		   && secbuf[ci+2]=='`' && secbuf[ci+3]=='`') {
					codeblock = 0;
					ci += 4;
					break;
				}
			}
			printf("%c", secbuf[ci]);
			ci++;
		}
	}
	printf("\x1b[0m");

	free(emain);
	fclose(fp);
}

/* 
	Chunk types to be handled:

	#		TAG_Hn
	>		TAG_BLOCKQUOTE
	-		TAG_UL
	!		image
	
	Other first characters will go into a <p>.
*/

Element *chunk(Element *p, long chunkstart, char *chunk, long chunklen) {
	switch(chunk[0]) {
		case '#':
			return(build_h(p, chunkstart, chunk, chunklen));
			break;
		case '>':
			return(build_blockquote(p, chunkstart, chunk, chunklen));
			break;
		case '-':
			return(build_ul(p, chunkstart, chunk, chunklen));
			break;
		case '!':
			return(build_img(p, chunkstart, chunk, chunklen));
			break;
		default:
			return(build_inline(p, chunkstart, chunk, chunklen));
	}
}

Element *build_h(Element *p, long chunkstart, char *chunk, long chunklen) {
	int i = 0;
	for (; i < chunklen && chunk[i] == '#'; i++);
	Element *ret = allocelem(p, TAG_H1+i-1);
	/* what we really need is to store content as strings.
		hopefully this doesn't violate our principles... */ 
	return(ret);
}

Element *build_blockquote(Element *p, long chunkstart, char *chunk, long chunklen) {
	Element *ret = allocelem(p, TAG_BLOCKQUOTE);
	
	return(ret);
}

Element *build_ul(Element *p, long chunkstart, char *chunk, long chunklen) {
	Element *ret = allocelem(p, TAG_UL);

	return(ret);
}

Element *build_pre(Element *p, long chunkstart, char *chunk, long chunklen) {
	Element *ret = allocelem(p, TAG_PRE);
	
	return(ret);
}

int parsetree(Element *root, const char *source, int startreadhead) {
	/* this should be an enum or something along with the tag defs */
	const char *tagstrlist[5] = {"body", "main", "p", "a", "em"};

	char tagstr[64]; /* WARNING: overflow possible with a big attribute */
	strcpy(tagstr, "<"); /* null terminates; henceforth we can use strcat */
	strcat(tagstr, tagstrlist[root->tag]);

	char tagendstr[64];
	strcpy(tagendstr, "</");
	strcat(tagendstr, tagstrlist[root->tag]);
	strcat(tagendstr, ">");

	/* build tag */
	if (root->starta == 0 && root->enda == 0) {
		strcat(tagstr, ">");
	} else {
		strcat(tagstr, " href=\"");
		strncat(tagstr, source+(root->starta), (root->enda)-(root->starta));
		strcat(tagstr, "\">");
	}
	int ci = root->startc;
	
	printf("%s", tagstr);
	int i = 0;
	while (root->children[i] != NULL && i < MAX_CHILDREN) {
		for (; ci < root->children[i]->start; ci++) {
			fputc(source[ci], stdout);
		}
		parsetree(root->children[i], source, startreadhead);
		ci += (root->children[i]->end) - (root->children[i]->start) + 1;
		i++;
	}
	for (; ci <= root->endc; ci++) {
		fputc(source[ci], stdout);
	}
	printf("%s", tagendstr);
}


Element *allocelem(Element *p, Tag t) {
	Element *ret = NULL;
	ret = malloc(sizeof(Element));
	if (ret == NULL) {
		fprintf(stderr, "ERROR: allocelem: returned a null pointer!\n");
		return(NULL);
	}
	
	ret->tag = t;
	ret->parent = p;
	
	int i = 0;
	for (; i < MAX_CHILDREN; i++) {
		if (p->children[i] != NULL) {
			continue;
		} else {
			p->children[i] = ret;
			return(ret);
		}
	}
	
	fprintf(stderr, "ERROR: allocelem: returned a null pointer!\n");
	return(NULL);
}
