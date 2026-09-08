#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG_MAIN  0
#define TAG_P     1
#define TAG_A     2
#define TAG_EM    3




#define MAX_CHILDREN 64

typedef struct element Element;

Element *allocelem(Element *p, int t);
int parsetree(Element *root, const char *source, int startreadhead);
long buildtree(Element *root, Element *current, const char *source, 
									long size, int allowinline, long rpos);

struct element {
	Element *parent;                     /*     8   */
	Element *children[MAX_CHILDREN];     /*  64*8   */
	int tag;                             /*     4   */
	long start, end,                     /*   6*8   */
			startc, endc, 
			starta, enda;
};

int main() {
	Element *main = NULL;
	main = malloc(sizeof(Element));
	main->tag = TAG_BODY;

	FILE *fp = fopen("src/test.md", "r");

	fseek(fp, 256, SEEK_SET);

	char fstr[256];

	fread(fstr, sizeof(char), 256, fp);

	int i = 0;
	for (; i < 256; i++) {
		putc(fstr[i], stdout);
	}
	putc('\n', stdout);

	buildtree(main, main, fstr, 256, 0, 0);
/*	parsetree(main, fstr, 0);*/

	return(0);
}

long buildtree(Element *root, Element *current, const char *source, 
								long size, int allowinline, long rpos) {
	
	/*  We will use allowinline to indicate whether we should recurse
		to look for substyles in the content of the current element.  */

	/*  I am itching to make this thing recursive because the type of
		output we're looking for is so well-suited to recursion!  */

	int pop = 0; /* 0:continue looping ; 1:pop to parent */
	char controlstr[] = "\n\n\n";
	if (rpos != 0 || rpos != 1) {
		controlstr[0] = source[rpos-2];
		controlstr[1] = source[rpos-1];
		controlstr[2] = source[rpos];
	}

	while (pop == 0 && rpos < size) {
		memmove(controlstr, controlstr+1, 2);
		controlstr[2] = source[rpos];
		
		if (current == root) {
			/*  Two consecutive newlines always end the current context. 
				Sequences of this form should  */
			if (controlstr[0] == '\n' && controlstr[1] == '\n') {
				pop = 1;
				switch (controlstr[2]) { /* check what is happening next */
					case '#':
						/* h start */
						
						break;
					case '-':
						/* ul start */
						
						break;
					case '>':
						/* blockquote start */
						
						break;
					case '!':
						/* img start */
						
						break;
					case '`':
						/* code block? */
						
						break;
					case '\n':
						/* skip one */
						
						break;
					default:
						/* p start */
						
				}
			}
		} else {
			/* Check what kind of element we're in */
			switch (current->tag) {
				case TAG_P:
				 	allowinline = 1;
					
					break;
				case TAG_EM:
					allowinline = true;
					
					break;
				case TAG_STRONG:
				 	
					break;
				case TAG_CODE:
				 	
					break;
				case TAG_A:
				 	
					break;
				case TAG_H:
				 	
					break;
				case TAG_UL:
				 	
					break;
				case TAG_LI:
				 	
					break;
				case TAG_PRE:
				 	
					break;
				case TAG_IMG:
				 	
					break;
			}
		}
		printf("%s", controlstr);
		printf("\n");
		rpos++;

	}

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
