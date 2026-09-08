#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG_BODY  0
#define TAG_MAIN  1
#define TAG_P     2
#define TAG_A     3
#define TAG_EM    4

#define MAX_CHILDREN 64

typedef struct element Element;

Element *allocelem(Element *p, int t);
int parsetree(Element *root, const char *source, int startreadhead);

struct element {
	Element *parent;                     /*     8   */
	Element *children[MAX_CHILDREN];     /*  64*8   */
/*	int nchildren;                              4   */
	int tag;                             /*     4   */
	long start, end,                     /*   6*8   */
			startc, endc, 
			starta, enda;
};

int main() {
	/* Initialize the root of the document tree. This should be the only
       Element that does not have a parent. */
	Element *body = NULL;
	body = malloc(sizeof(Element));
	body->tag = TAG_BODY;

	Element *main = allocelem(body, TAG_MAIN);

	const char *test = 
		"markdown is *quite* hard! here's a [s*pe*c](https://commonmark.org/)\n";
	const int testlen = strlen(test);

	/* on new line, make <p> if not some other kind of line */

	Element *current = allocelem(main, TAG_P); 
	
	int ci = 0;
	current->start = ci;
	current->startc = ci;
	
	for (; ci < testlen; ci++) {
		if (test[ci] == '*') {
			current = allocelem(current, TAG_EM);
			current->start = ci;
			current->startc = ci+1;
			ci++;
			while (test[ci] != '*' && test[ci] != '\n' && ci < testlen) {
				ci++;
			}
			current->endc = ci-1;
			current->end = ci;
			printf("\x1b[32m%c\x1b[0m", test[ci]);
			current = current->parent;	
		} else if (test[ci] == '[') {
			current = allocelem(current, TAG_A);
			current->start = ci;
			current->startc = ci+1;
			ci++;
			while (test[ci] != ')' && test[ci] != '\n' && ci < testlen) {
				if (test[ci] == ']') {
					current->endc = ci-1;
				}
				if (test[ci] == '(') {
					current->starta = ci+1;
				}
				ci++;
			}
			current->enda = ci;
			current->end = ci+1;
			printf("\x1b[33m%c\x1b[0m", test[ci]);
			current = current->parent;
		} else {
			printf("%c", test[ci]);
		}
	}
	
	current->endc = ci;
	current->end = ci;

	parsetree(body, test, 0);
	printf("\n");

	return(0);
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

Element *allocelem(Element *p, int t) {
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
