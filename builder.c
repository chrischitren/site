/* 
	FIRST PASS CHUNKS:

			Standard: 		<p>
								text
							</p>
	
	>		Blockquote:		<blockquote>
								text
							</blockquote>
	
	-		List:			<ul>
								<li>text</li>
							</ul>

	1.		Ordered list:	<ol>
								<li>text</li>
							</ol>
	
	#		Heading:		<h1>text</h1>
							...
							<h6>text</h6>

	!		Image:			<a href="link"><img src="link" name="text"></a>

	```		Code block:		<pre>
	text						text
	```						</pre>

*/

/*
	After parsing the first pass, we will not have a tree. We will have a
	list. Chunks are found using the following algorithm:

FIND CHUNKS:

	Start at the beginning of the document.

	Skip to the first character that is not a newline or space. 

		Are the next three characters exactly "```"? Yes:
			
			Mark the beginning of a code block.
			
			Skip to the next occurrence of the sequence "```\n.
				
				If the end of the document is reached, end the block.
			
			Mark the end of the code block.
		
		No:
			
			Mark the beginning of a block.
			
			Identify the block based on	its first character.

			Skip to the next occurrence of the sequence "\n\n".
	
				If the end of the document is reached, end the block.

			Mark the end of the block.

	Is it the end of the document? No:

		GOTO "Skip to the first character that is not a newline or space"

	Yes:
		
		END

*/

/*
	
	We now have a list of chunks that are contiguous regions of similarly-
	formatted content. They are useful because they allow us to prune the
	transition table; for example, we will never go from being in a heading
	to being in a list element. We can define a recursive routine to parse
	inline text, which contains only literal characters & control sequences
	with comprehensible nesting rules.

	The inline parser can "eat" symbols greedily. Take the example:
	
		Styles draw the ***eye.*** This [*italic* is in a link](/link). This
		*[link](/link)* is in an italic.

	Our inline parser is a simple state machine. It knows the context of the
	chunk it is in, and has a transition table based on this context, the
	current state, and the next character. 

	Our inline parser is a recursive function. When it consumes a character,
	it may create a new instance of itself with context informed by its own.
	The things it can do are:

		Consume a character and do nothing.
		                        append it to self->content.
		                        append it to self->attributes.
		                        give birth.
		                        ascend.

*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_CHILDREN   32
#define MAX_CHUNK_SZ 2048

#define L_P             0
#define L_LI            1
#define L_STRONG_EM     2
#define L_STRONG        3
#define L_EM            4
#define L_CODE          5
#define L_A             6
#define L_A_TEXT        8
#define L_A_HREF        7

#define L_ROOT          9

typedef struct _ielem iElement;

struct _ielem {
	iElement *parent;
	iElement *children[MAX_CHILDREN];
	char *content;
	int level;
};


int parseinline(void *parent[], int parentsz, 
						char *chunk,  int level, int depth); 
int parseasterisk(void *parent[], int parentsz, 
						char *chunk, int chunksz, int level, int depth); 
int checkdelim(const char *delim, char *string, int n);
int realloc_i(iElement *e, char *cont, char *attr);
iElement *alloc_i(void *parent[], int parentsz,
					char *cont, int level);
int cleantree(iElement *root);
int printtree(iElement *root, int depth);

int main() {
	char *strtest = "If a [plain link](link_url) is wrapped like `[plain link](link_url)` it should not strip any of the brackets. Adjacent styles like *emphasis*`code` probably do not work! Well, **what about***things that crawl...*";

	char *strtest21 = "*italic* to start; **bold** next; ***both***; *italic **bold*** ***let's hope\\\\* this works** as well as ***t\\*his** one* . . . one*three***two**";	

	int i = 0;

	iElement *rt[64];
	
	for (; i < 64; i++) {	
		rt[i] = malloc(sizeof(iElement));
		rt[i] = NULL;
	}
	
	alloc_i((void **)rt, 64, strtest, 9);
	
	parseasterisk((void **)rt, 64, strtest21, strlen(strtest21), L_ROOT, 0);

	for (i = 0; i < 64; i++) {	
		cleantree(rt[i]);
	}

	printf("\n");	

	return(0);
}

/*
	We need to define a way to allow or disallow recursion based on the
	current context of parseinline. We need to check the next two characters
	or else we cannot properly parse ***text*** and [links](link_url).
	
	The thing to do is to break the text into iElements containing substrings
	of the original. To do this we loop over the entire thing looking for
	the delimiters (in order of precedence):

         0123         0123        01         012        01         01
            .            .         .           .         .          .       
		\n-...\n+    \n>...\n+    `+...`+    **...**    *+...*+    [+...)+
		li           p            code       strong     em         a      
		^ul          ^blockquote                                           

	where + indicates that any character matches. It is important to check for
	** before * because we don't want to wind up with three nested <em> tags
	where there should be a <strong><em>...</em></strong>. 

	There is no valid markdown that generates <a><li>text</li></a>, and
	similarly our parser should *not* generate <code><em>text</em></code>.
	Thus we must decide which elements can be matched inside which others.

	         inside this,         
	         li p  cd st em a  
	 a    li -- -- -- -- -- -- 
     l t   p -- -- -- -- -- -- 
     l h  cd YY YY -- YY YY YY 
     o i  st YY YY -- -- -- YY 
     w s  em YY YY -- YY -- YY 
       ?   a YY YY -- YY YY -- 

	UNFORTUNATELY this nice little table is insufficient to parse some of the
	more obtuse parts of the syntax, at least as far as I can see from my
	attempts to hack it into order. 
	
	Hitting a delimiter will need to call a function (of bespoke design) that
	inspects the rest of the chunk to find the end of the sequence. Some of
	these will be very simple functions, like for code blocks: check for the
	next '`' that isn't escaped. The one for em/strong is less pretty.

	These functions should return a few values: a length (or error code, -1?),
	whether the inner string is frozen (contains no further unescaped control 
	characters), an element type... maybe they should just construct the inner
	string themselves and pass back a pointer to a parent-linked iElement?

	The great value of just returning positions and line lengths is avoiding
	ever allocating (whether in the heap or in the stack) short-lived buffers.
	All we do is read, decide, and return an instruction to the primary "read-
	write head" that has a preloaded copy of the chunk to reference quickly.

	As written, parseasterisk gobbles up MAX_CHUNK_SZ bytes immediately, and
	any other function will also need to do this. If they just had to read
	bytes from a pointer+offset and make a stop/continue decision at each char
	they would (presumably) be much more efficient.

*/

int parseasterisk(void *parent[], int parentsz,
						char *chunk, const int chunksz, int level, int depth) {
	int i, j, k;
	char innerbuffer[MAX_CHUNK_SZ];

	int escape = 0;

	int bi = 0;

	int tag = 0;

	int inside = 0;
	int start_n = 0;
	int depth_e = 0;	
	int last_n = 0;
	
	printf("\x1b[35m%s\x1b[0m\n\n", chunk);

	for (i = 0; i < strlen(chunk); ) {
/*tst*/	if (!inside) {
/*tst*/		memset(innerbuffer, '#', MAX_CHUNK_SZ);
/*tst*/	}

		if (chunk[i] == '\\' && !escape) {
			escape = 2;
		}
		if (chunk[i] == '*' && !escape) {
			for (j = 0; i+j < strlen(chunk); j++) { 
				if (chunk[i+j] != '*') {
					break;
				}
				if (j > 3) {
					break;
				}
			}
			if (!inside) {
				bi = 0;
				inside = 1;
				start_n = j;
				depth_e = j;
				if (j < 3) {
					i += start_n;
					tag = start_n;
				} else if (j == 3) {
					memset(innerbuffer+bi, '*', 3);
					bi += 3;
					i += 3;
				}
			} else {
				if (start_n < 3) {
					if (j == start_n) {
		/*write*/		innerbuffer[bi] = '\0';
		/*write*/		printf("(s<3exact %d \"%s\")\n", tag, innerbuffer);
		/*write*/		inside = 0; 
		/*write*/		bi = 0; 
						i += start_n;
					} else if (j == 3) {
						if (depth_e == 3) {
							i += 3-start_n;
							memset(innerbuffer+bi, '*', 3-start_n);
							bi += 3-start_n;
			/*write*/		innerbuffer[bi] = '\0'; 
			/*write*/		printf("(s<3 d=3  %d \"%s\")\n", tag, innerbuffer);
			/*write*/		inside = 0; 
			/*write*/		bi = 0; 
							i += start_n;
						} else if (depth_e < 3) {
			/*write*/		innerbuffer[bi] = '\0'; 
			/*write*/		printf("(s<3 d<3  %d \"%s\")\n", tag, innerbuffer);
			/*write*/		inside = 0; 
			/*write*/		bi = 0; 
							i += start_n;
						}
					} else {
						depth_e += j;
						i += j;
						memset(innerbuffer+bi, '*', j);
						bi += j;
					}
				} else if (start_n == 3) {
					if (j == 3) {
						i += 2;
						tag = 1;
						memset(innerbuffer+bi, '*', 2);
						bi += 2;
		/*write*/		innerbuffer[bi] = '\0'; 
		/*write*/		printf("(s=3exact %d \"%s\")\n", tag, (innerbuffer+1));
		/*write*/		inside = 0; 
		/*write*/		bi = 0; 
						i += 1;
					} else {
						if (depth_e-j <= 0) {
							tag = j;
			/*write*/		innerbuffer[bi] = '\0'; 
			/*write*/		printf("(s=3 d>=3 %d \"%s\")\n", tag, 
										(innerbuffer+j));
			/*write*/		inside = 0; 
			/*write*/		bi = 0; 
							i += j;
						} else {
							last_n = j;
							depth_e -= j;
							i += j;
							memset(innerbuffer+bi, '*', j);
							bi += j;
						}
					}
				}
			}
		} else {
			innerbuffer[bi] = chunk[i];
			bi++;
			i++;
		}
		if (escape) {
			escape--;
		}
	}
}



/*	BROKEN (BUT THE GIST IS THERE) PSEUDOCODE FOR PARSEASTERISK

	INSIDE = 0;
	N = 0;
	START_N = 0;
	DEPTH = 0;
	LAST = 0;
	
	if you match,
		N = [length of match]
		and !INSIDE,
			START_N = N
			DEPTH = N
			and N < 3,
				skip START_N
				TAG <- START_N
			and N == 3,
				write "***" to buffer
				skip START_N
		and INSIDE,
			and START_N < 3,
				and N == START_N,
					write *(content)
					skip START_N
				and N == 3,
					and DEPTH == 3,
						skip 3-START_N
						write *(content)
						skip START_N
					AND DEPTH < 3,
						skip START_N
						write *(content)
				and N < 3 && N != START_N,
					DEPTH += N
					skip N
			and START_N == 3,
				and N == 3,
					skip 1
					TAG <- 1
					write *(content+1)
					skip 2
				and N < 3,
					and DEPTH + N >= 3,
						TAG <- LAST
						write *(content+LAST)
						skip LAST
					and DEPTH + N < 3,
						LAST = N
						DEPTH += N
						skip N
*/
/*
	
	next time you find a run longer than the current depth, pop up to toplevel,
	but do so at the *end* of this new run. e.g:
	
		*italic **bold text***
  			-> <em>italic <strong>bold text</strong></em>
    	
		*italic**bold text***
			-> <em>italic<strong>bold text</strong></em>
			-> <em>italic</em><em>bold text</em></strong>
	
	Which parse is "correct"? As a browser, we know that the second HTML is
	invalid. As backtrackers, we know that there was an alternative parse of
	the markdown that would have produced valid HTML.
	
	The simple solution is to only allow * to be ended by * or ***, and if the
	latter, then we must "consume" the final asterisk to ensure that subparses
	are successful. Similarly, ** can only be ended by ** or ***, and we must
	consume the end of the triplet to leave inner strings well-formed. 
	
		+- consumed         +- consumed
		|                   |
		*italic**bold text***
		|      |          |
		|      |          +- see triplet; can end; skip ahead by 3-depth
		|      |
		|      +- cannot end depth 1 with runlength 2      
		|
		+- start with depth 1
	
	The next problem is starting at depth 3. We cannot know whether we should
	be in a 1-depth or 2-depth on the first pass, so we cannot know how many
	characters to consume.
	
	We can hack it in the following way:
	
		Set the depth to 3 and consume {all 3 asterisks, no asterisks}
		As we read other runs, count down the depth.
		When the depth reaches zero, consume the run that did so;
			calculate what should have been consumed at the start;	
			prepend asterisks to correct the inner string.
	
	This sucks because we have to backtrack (sort of; we can write,
	
		***text with** italic*
		^^^^^^^^^^^^^^^^^^^^^
	
	to the buffer, compute that we should have consumed one, and then pass
	*(buffer+1) as the inner string), but I don't think it's possible to avoid
	in this case.
	
*/

int parseinline(void *parent[], int parentsz,
						char *chunk,  int level, int depth) {
	
	int i, j, k;
	const char *delims3[18] = {	 "> ." , "\n.." ,   /*  0   blockquote>p  */ 
								 "- ." , "\n.." ,   /*  1   ul>li         */
								 "`.." ,  "`.." ,   /*  2   code          */
								 "***" ,  "***" ,   /*  3   strong>em     */
								 "**." ,  "**." ,   /*  4   strong        */
								 "*.." ,  "*.." ,   /*  5   em            */
								 "[.." ,  ").." ,   /*  6   a             */
								 "(.." ,  ".\0.",   /*  7   a>href        */
								 "[.." ,  "].." };  /*  8   a>text        */
	
	/*        transition[parent][child] is 1 if parent can contain child  */
	const int transition[10][9] =                   /*  ---- PARENT ----  */
	/*               bq li cd se st em  a ah at                           */
				{	{ 0, 0, 1, 1, 1, 1, 1, 0, 0},   /*  0   blockquote>p  */
					{ 0, 0, 1, 1, 1, 1, 1, 0, 0},   /*  1   ul>li         */
					{ 0, 0, 0, 0, 0, 0, 0, 0, 0},   /*  2   code          */
					{ 0, 0, 1, 0, 0, 1, 1, 0, 0},   /*  3   strong>em     */
					{ 0, 0, 1, 0, 0, 1, 1, 0, 0},   /*  4   strong        */
					{ 0, 0, 1, 0, 1, 0, 1, 0, 0},   /*  5   em            */
					{ 0, 0, 0, 0, 0, 0, 0, 1, 1},   /*  6   a             */
	                { 0, 0, 0, 0, 0, 0, 0, 0, 0},   /*  7   a>href        */
					{ 0, 0, 1, 1, 1, 1, 0, 0, 0},   /*  8   a>text        */
													/*                    */	
					{ 1, 1, 1, 1, 1, 1, 1, 0, 0} }; /*  9   toppest       */

	const char *types[10] = 
	  { "bq>", "li-", "cde", "sem", "str", " em", "a[)", "a()", "a[]", "TOP" };

	int toplevel = level;
	int mode = toplevel; /* plain text */

	int escape = 0;

	int subparses = 0;

	char contentbuffer[128];
	int ci = 0;

	int offset = 0;

	if (toplevel == L_A) {
		mode = L_A_TEXT;
	}
	
	for (i = 0; i <= strlen(chunk); ) {
		offset = 0;
		if (escape == 0 && chunk[i] == '\\') {
			escape = 1;
			i++;
		}
		if (escape == 0) {
			if (mode == toplevel) { /* check for any starting sequences */
				for (j = 0; j < 9; j++) {
					if (transition[toplevel][j] == 1) {
						/* get number of matched characters */
						k = checkdelim(delims3[2*j], chunk+i, 3);
						if (k != -1) {
							subparses++;
							contentbuffer[ci] = '\0';
							ci = 0;
							for (ci = 0; ci < depth; ci++) { printf("---"); }
							ci = 0;
							printf("\"\x1b[36m%s\x1b[0m\"\nst %s->%s\n",
									contentbuffer, types[mode], types[j]);
							i += k;
							mode = j;
							break;
						}
					}
				}
			} else { /* check for ending sequence */
				k = checkdelim(delims3[2*mode+1], chunk+i, 3);
				if (k != -1) {
					contentbuffer[ci] = '\0';
					ci = 0;
					offset = 1;
					for (ci = 0; ci < depth; ci++) { printf("---"); }
					ci = 0;
					printf("\t\"\x1b[36m%s\x1b[0m\"\nen %s<-%s\n", 
							contentbuffer, types[toplevel], types[mode]);
					i += k;
					mode = toplevel;
				}
			}
		}
		escape = 0;
		if (offset != 1) {
			contentbuffer[ci] = chunk[i];
			i++;
			ci++;
		}
	}

	if (subparses == 0) {
		for (ci = 0; ci < depth; ci++) { printf("==="); }
		printf("nf in   %s \"\x1b[33m%s\x1b[0m\"\n", types[mode], contentbuffer);
	}

	if (mode != toplevel) {
		fprintf(stderr, "\x1b[31m[builder] [parseinline] "
							"line ended below toplevel\x1b[0m\n");
	}
/*	printf("\n%d\n", mode);*/
/*                       						
	strcpy(ret->content, c);
	strcpy(ret->attributes, a);

	free(c);
	free(a);
*/

	return(0); /* TODO return number of subelements so we know when to stop */
}

int checkdelim(const char *delim, char *string, int n) {
	int i = 0;
	int j = 0;
/*
	printf("checking %3d,%3d,%3d against %3d,%3d,%3d\n",
				delim[0],delim[1],delim[2],string[0],string[1],string[2]);
	printf("i.e.     %3c,%3c,%3c against %3c,%3c,%3c\n\n",
				delim[0],delim[1],delim[2],string[0],string[1],string[2]);
*/	
	while (i < n) {
		if (*(delim+i) != '.') {
			if (*(delim+i) != *(string+i)) {
/*				printf("\x1b[34m(%d!=%d)\x1b[0m", *(delim+i), *(string+i));*/
				return(-1);
			} else {
/*				printf("\x1b[32m(m: %d with %d)\x1b[0m\n", 
							*(delim+i), *(string+i));
*/				j++;
			}
		}
		i++;
	}
	return(j);
}

iElement *alloc_i(void *parent[], int parentsz, 
					char *cont, int lvl) {
	int sizec = strlen(cont);
	iElement *ret = NULL;
	int i = 0;
	while (parent[i] != NULL && i < parentsz) {
		i++;
	}
	if (i == parentsz) {
		fprintf(stderr, "[builder] [alloc_i] failed append to full parent\n");
		return(NULL);
	}
	if ((ret = malloc(sizeof(iElement))) == NULL) {
		fprintf(stderr, "[builder] [alloc_i] failed to malloc iElement\n");
		return(NULL);
	}
	parent[i] = ret;
	ret->content = malloc(sizeof(char) * sizec);
	strcpy(ret->content, cont);
	ret->level = lvl;
	return(ret);
}

int cleantree(iElement *root) {	
	int i = 0;
	if (root == NULL) {
/*		free(root); */
		return(0);
	}
	while (root->children[i] != NULL && i < MAX_CHILDREN) {
		cleantree(root->children[i]);
		i++;
	}
	free(root->content);
	free(root);
	return(0);
}

int printtree(iElement *root, int depth) {
	int i = 0;

	int d = 0;
	for (; d < depth; d++) {
		printf("\t");
	}
	printf("level:   \x1b[36m%d\x1b[0m\n", root->level);
	for (d = 0; d < depth; d++) {
		printf("\t");
	}
	printf("content: \x1b[36m%s\x1b[0m\n", root->content);

	while (root->children[i] != NULL && i < MAX_CHILDREN) {
		printtree(root->children[i], depth+1);
		i++;
	}
}
