#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "chunks.h"

ChunkElement *parsechunk(char *_chunkstr, int _chunksz, int _type, int _maxdepth) {
	
	ChunkElement *rt;
	int finalopenleaves = -1;
	
	if ((rt = initchunk(_chunkstr, _chunksz,
												0, _type, 1, NULL)) == NULL) {
		fprintf(stderr, "\x1b[31m[chunks] [parsechunk]"
						" failed to allocate root node\x1b[0m\n");
		return(NULL);
	}
	
	while (_maxdepth > 0 && countopenleaves(rt) > 0) {
		parseopenleaves(rt);
		_maxdepth--;
	}
	
	finalopenleaves = countopenleaves(rt);
	
	if (finalopenleaves != 0) {
		fprintf(stderr, "\x1b[31m[chunks] [parsechunk]"
						" returned chunk with open leaves\x1b[0m\n");
	}

	return(rt);
}


int parseopenleaves(ChunkElement *c) {
	int i = 0;
	
	if (c->children[0] == NULL && c->open) {
		parsechunkelement(c);
		return(0);
	}
	
	while (i < C_E_MAXCHILDREN) {
		if (c->children[i] != NULL) {
			parseopenleaves(c->children[i]);
		}
		i++;
	}
	
	return(1);
}


/*	initchunk will take a string (the chunk) and return a root ChunkElement
	whose children correspond to the structure of the chunk. */
ChunkElement *initchunk(char *_chunkstr, int _chunksz, int _chunkpos, 
						int _type, int _open, ChunkElement *_parent) {
	int i;
	
	ChunkElement *root;
	if ((root = malloc(sizeof(ChunkElement))) == NULL) {
		fprintf(stderr, "\x1b[31m[chunks] [initchunk]"
						" failed to malloc root node\x1b[0m\n");
		return(NULL);
	}
	
	root->parent = _parent;
	if (_parent != NULL) {
		for (i = 0; i < C_E_MAXCHILDREN; i++) {
			if (_parent->children[i] == NULL) {
				_parent->children[i] = root;
				break;
			}
		}
		if (i == C_E_MAXCHILDREN) {
			fprintf(stderr, "\x1b[31m[chunks] [initchunk]"
							" parent has too many children\x1b[0m\n");
			free(root);
			return(NULL);
		}
	}
	
	for(i = 0; i < C_E_MAXCHILDREN; i++) {
		root->children[i] = NULL;
	}
	
	root->chunkstr = _chunkstr;
	root->position = _chunkpos; /* root starts at beginning of chunk  */
	root->length = _chunksz;    /*      and runs through end of chunk */
	root->type = _type;
	root->open = _open;
	
	return(root);
}


/*	parsechunkelement takes an open ChunkElement found at c and reads its
	associated chunk string. It appends as children any valid subelements in
	the form of ChunkElement pointers. NOT RECURSIVE; the handling function
	must check if there are open leaves and call parsechunkelement on each. */
ChunkElement *parsechunkelement(ChunkElement *c) {
	
	int i = 0;
	int escape = 0;
	
	int lastwritepos = 0;
	int skippedsz = 0;
	
	int offset = c->position; /* start pos in c->chunkstr   */
	int contentsz = 0;
	int delimsz = 0;          /* length of delimiter        */
	char *pci = "";           /* 'p'ointer to 'c'har at 'i' */
	
	if (c->open == 0) {
		return(NULL);
	}
	
	while (i <= c->length) {
		pci = ((c->chunkstr)+offset+i);
		
		if (*pci == '\\') {
			escape = 2;
		}
		if (escape && *pci != '\n') {
			i++;
			escape--;
			continue;
		} else {
			escape = 0;
		}

		if (c->type == TYPE_PRE || c->type == TYPE_CODE) {
			if (*pci == '\0' || i == c->length) {
				initchunk(c->chunkstr, i-lastwritepos, offset+lastwritepos,
					TYPE_PLAIN, 0, c);
			}
		}
		
		/* Split <blockquote> into <p>s; linebreaks cause new <p> children. */
		if (c->type == TYPE_BLOCKQUOTE) {
			if (*pci == '>' && i == lastwritepos) {
				lastwritepos += 2;
				i += 2;
			}
			if ((*pci == '\n' || *pci == '\0') && i != lastwritepos) {
				initchunk(c->chunkstr, i-lastwritepos, offset+lastwritepos,
					TYPE_P, 1, c);
				lastwritepos = i+1;
			}
		}
		
		/* Splits <ul> into <li>s; linebreaks can't directly bear children
		   because they imply multiple <p>s nested inside one <li>. */
		if (c->type == TYPE_UL) {
			if (*pci == '-' && i == lastwritepos) {
				lastwritepos += 2;
				i += 2;
			}
			if ((*pci == '-' && *(pci-1) == '\n') && i != lastwritepos) {
				initchunk(c->chunkstr, i-lastwritepos-1, offset+lastwritepos,
					TYPE_LI, 1, c);
				lastwritepos = i+2;
			}
			if (i == c->length && i != lastwritepos) {
				initchunk(c->chunkstr, i-lastwritepos, offset+lastwritepos,
					TYPE_LI, 1, c);
				lastwritepos = i+2;
			}
		}
		
		/* Split the aforementioned nested <p>s up inside an <li> */
		if (c->type == TYPE_LI) {
			if ((*pci == '\n' || i == c->length) && i != lastwritepos) {
				initchunk(c->chunkstr, i-lastwritepos, offset+lastwritepos,
							TYPE_P, 1, c);
				lastwritepos = i+1;
			}
		}

		if (c->type == TYPE_A || c->type == TYPE_IMG) {
			if (i == 0 && *pci == '!' && c->type == TYPE_A) {
				initchunk(c->chunkstr, c->length, offset, TYPE_IMG, 1, c);
				scansimple(pci, &contentsz, '!', ']');
				skippedsz = contentsz + 2;
				i += skippedsz;
			} else {
				if (*pci == '[') {
					if (scansimple(pci, &contentsz, '[', ']') == 0) {
						if (c->type == TYPE_IMG) {
							initchunk(c->chunkstr, contentsz, offset+i+1,
									TYPE_PLAIN, 0, c);
						} else {
							initchunk(c->chunkstr, contentsz, offset+i+1,
									TYPE_PLAIN, 1, c);
						}
						skippedsz = contentsz + 2;
						i += skippedsz;
					}
				}
			}
			if (*pci == '(') {
				if (scansimple(pci, &contentsz, '(', ')') == 0) {
					initchunk(c->chunkstr, contentsz, offset+i+1,
									TYPE_ATTRIBUTE, 0, c);
					skippedsz = contentsz + 2;
					i += skippedsz;
				}
			}
		}
		
		/* Parse inline elements <em>, <strong>, <code>, and <a> */
		if (c->type == TYPE_P
			|| c->type == TYPE_EM
			|| c->type == TYPE_STRONG
			|| (c->type >= TYPE_H1 && c->type <= TYPE_H6)
			|| (c->type == TYPE_PLAIN && c->open == 1)) {
			if ((*pci == '\0' || i == c->length) && i != lastwritepos) {
				initchunk(c->chunkstr, i-lastwritepos, offset+lastwritepos,
							TYPE_PLAIN, 0, c);
			}
	
			if (c->type >= TYPE_H1 && c->type <= TYPE_H6 && i == 0) {
				skippedsz = c->type - TYPE_H1 + 2;
				i += skippedsz;
				lastwritepos = i;
			}
			
			if (*pci == '[' && i < c->length-2) {
				if (scansimple(pci, &contentsz, '[', ']') == 0) {
					delimsz = contentsz+2;
					if (scansimple(pci+delimsz, &contentsz, 
							'(', ')') == 0) {
						/* if the link is preceded by a bang, check if we
						   should be escaped (unless we're at the start of
						   a line, then just go for it because we can't be!) */
						if ((i == 1 && *(pci-1) == '!') ||
							(i > 1 && *(pci-2) != '\\' && *(pci-1) == '!')) {
							/* write content up to now to plain ChunkElement */
							if (i != lastwritepos) {
								initchunk(c->chunkstr, i-lastwritepos-1, 
									offset+lastwritepos, TYPE_PLAIN, 0, c);
								lastwritepos = i-1;
							}
							
							/* write <a> ChunkElement INCLUDING '!' & skip */
							skippedsz = delimsz + contentsz + 3;
							initchunk(c->chunkstr, skippedsz, offset+i-1,
								TYPE_A, 1, c);
							i += skippedsz;
							lastwritepos += skippedsz;
						} else {
							/* write content up to now to plain ChunkElement */
							if (i != lastwritepos) {
								initchunk(c->chunkstr, i-lastwritepos, 
									offset+lastwritepos, TYPE_PLAIN, 0, c);
								lastwritepos = i;
							}
							
							/* write <a> ChunkElement and skip */
							skippedsz = delimsz + contentsz + 2;
							initchunk(c->chunkstr, skippedsz, offset+i,
								TYPE_A, 1, c);
							i += skippedsz;
							lastwritepos += skippedsz;
						}
					}
				}
			}
			
			if (*pci == '`') {
				if (scansimple(pci, &contentsz, '`', '`') == 0) {
					/* write content up to now into plain ChunkElement */
					if (i != lastwritepos) {
						initchunk(c->chunkstr, i-lastwritepos, 
								offset+lastwritepos, TYPE_PLAIN, 0, c);
						lastwritepos = i;
					}
					/* write ChunkElement and skip */
					initchunk(c->chunkstr, contentsz, offset+i+1,
							TYPE_CODE, 1, c);
					skippedsz = contentsz + 2;
					i += skippedsz;
					lastwritepos += skippedsz;
				}
			}
			
			if (*pci == '*') {
				if (scanasterisk(pci, &contentsz, &delimsz) == 0) {
					skippedsz = 0;
					if ( (delimsz == 1 && c->type != TYPE_EM)
						|| (delimsz == 2 && c->type != TYPE_STRONG) ) {
						/* write content up to now into plain ChunkElement */
						if (i != lastwritepos) {
							initchunk(c->chunkstr, i-lastwritepos,
									offset+lastwritepos, TYPE_PLAIN, 0, c);
							lastwritepos = i;
						}
						
						/* write asterisk-delimited ChunkElement and skip */
						initchunk(c->chunkstr, contentsz, offset+i+delimsz,
								TYPE_EM-1+delimsz, 1, c);
						skippedsz = contentsz + 2*delimsz;
						i += skippedsz;
						lastwritepos += skippedsz;
					}
				}
			}
		}	
		if (!skippedsz) {
			i++;
		}
		skippedsz = 0;
	}
	return(NULL);
}


/* scansimple should be passed a string that starts with delim1. It will
   terminate and write the inner length to *_contentsz when it hits delim2. */
int scansimple(char *chunk, int *_contentsz, char delim1, char delim2) {
	int i = 0;
	int escape = 1;

	int sz;
	sz = strlen(chunk);

	if (chunk[0] != delim1) {
		fprintf(stderr, "\x1b[31m[chunks] [scansimple]"
						" string does not start with given delim\x1b[0m\n");
		return(1);
	}
	
	for (i = 0; i < sz; ) {
		if (chunk[i] == '\\' && !escape) {
			escape = 2;
		}
		if (chunk[i] == delim2 && !escape) {
			*_contentsz = i-1;
			return(0);
		} else {
			i++;
		}
		if (escape) {
			escape--;
		}
	}
	
	fprintf(stderr, "\x1b[31m[chunks] [scansimple]"
					" reached end of string without closing\x1b[0m\n");
	return(1);
}


/*  scanasterisk finds the length of the next and returns inner length; needs
	to return length of delimiter as well */
int scanasterisk(char *chunk, int *_contentsz, int *_delimsz) {
	int i, j;
	
	int escape = 0;     /* bool; 1:last char was escape, 0:wasn't  */
	
	int bi = 0;         /* position of write head in output buffer */
	
	int inside = 0;     /*                                         */
	int start_n = 0;    /* length of starting run                  */
	int depth_e = 0;    /* current depth of emphasis (0, 1, 2, 3)  */
	
	int sz;
	sz = strlen(chunk);
	
	for (i = 0; i < sz; ) {
		if (chunk[i] == '\n') {
			return(1);
		}
		if (chunk[i] == '\\' && !escape) {
			escape = 2;
		}
		if (chunk[i] == '*' && !escape) {
			for (j = 0; i+j < sz; j++) { 
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
					*_delimsz = start_n;
				} else if (j == 3) {
					bi += 3;
					i += 3;
				}
			} else {
				if (start_n < 3) {
					if (j == start_n) {
						*_contentsz = bi; /* w */
						return(0);        /* w */
					} else if (j == 3) {
						if (depth_e == 3) {
							i += 3-start_n;
							bi += 3-start_n;
							*_contentsz = bi; /* w */
							return(0);        /* w */
						} else if (depth_e < 3) {
							*_contentsz = bi; /* w */
							return(0);        /* w */
						} else {
							return(1);
						}
					} else {
						depth_e += j;
						i += j;
						bi += j;
					}
				} else if (start_n == 3) {
					if (j == 3) {
						i += 2;
						bi += 2;
						*_delimsz = 1;      /* w */
						*_contentsz = bi-1; /* w */
						return(0);          /* w */
					} else {
						if (depth_e-j <= 0) {
							*_delimsz = j;      /* w */
							*_contentsz = bi-j; /* w */
							return(0);          /* w */
						} else {
							depth_e -= j;
							i += j;
							bi += j;
						}
					}
				}
			}
		} else {
			bi++;
			i++;
		}
		if (escape) {
			escape--;
		}
	}
	fprintf(stderr, "\x1b[31m[chunks] [scanasterisk]"
					" reached end of string without closing\x1b[0m\n");
	return(1);
}

/* countopenleaves searches a tree of ChunkElements for nodes that don't have
   any children, but for which open==1. It returns the sum of c->open ints
   for these terminal/leaf nodes */ 
int countopenleaves(ChunkElement *c) {
	int f = 0;
	int i = 0;
	
	if(c->children[0] == NULL) {
		return(c->open);
	}
	
	while (i < C_E_MAXCHILDREN) {
		if (c->children[i] != NULL) {
			f += countopenleaves(c->children[i]);
		}
		i++;
	}
	
	return(f);
}
int rfree(ChunkElement *c) {
	int i = 0;
	if (c == NULL) {
		return(0);
	}
	while (i < C_E_MAXCHILDREN) {
		if (c->children[i] != NULL) {
			rfree(c->children[i]);
		}
		i++;
	}
	free(c);
	return(0);
}

