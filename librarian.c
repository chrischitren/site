/*
	Walks through a given directory recursively and records all
	the files there with a given extension (.md in this case).
	Makes an index file that contains the relative paths to these.
*/
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "librarian.h"
#include "chunks.h"
#include "parsemd.h"

int main() {
	int i, j, k, matchcount, nfiles;
	
	struct stat st;
	FILE *tmpfp;
	FILE *fp;
	
	char *readpath = "src";
	char srclist[MAX_LINKS][MAXDIR];
	
	char *writepath = "html";
	char outlist[MAX_LINKS][MAXDIR];
	
	char headerbuffer[4096];

	int edgematrix[MAX_LINKS][MAX_LINKS];

	Document doc;

/* HTML header? */
/*	fputs(
	"<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
	"<meta charset=\"UTF-8\">\n<title>chrischitren</title>\n"
	"<link rel=\"icon\" type=\"image/png\" href=\"/images/favicon_32.png\">\n"
	"<link rel=\"stylesheet\" href=\"main.css\">\n</head>\n<body>",
	wfp);*/
	
/*	if (readpath[strlen(readpath)-1] == '/') {
		fprintf(stderr, "\x1b[31m[librarian]"
						" source dir \"%s\" has trailing slash\x1b[0m\n",
						readpath);
		return(1);
	}*/

	for (i = 0; i < MAX_LINKS; i++) {
		for (j = 0; j < MAX_LINKS; j++) {
			edgematrix[i][j] = 0;
		}
	}
	
	/* Checking if directory exists */	
	if(stat(writepath, &st) == 0) {
		printf("[librarian] write path \"%s\" already exists", writepath);
		if (S_ISDIR(st.st_mode) == 0) {
			printf(" but is not a directory\n");
			fprintf(stderr, "\x1b[31m[librarian] [main]"
				" existing path \"%s\" is not a directory, exiting\x1b[0m\n",
				writepath);
			return(1);
		} else {
			printf(" and will be used\n");
		}
	} else {
		printf("[librarian] making directory \"%s\"\n", writepath);
		mkdir(writepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}

	/* build an index of the source directory, including *all* files */
	buildindex(srclist, readpath);
	
	/* build output paths for markdown files that need to get parsed */
	for (i = 0; i < MAX_LINKS; i++) {
		outlist[i][0] = '\0';
	}	
	i = 0;
	while (srclist[i][0] != '\0' && i < MAX_LINKS) {
		strcpy(outlist[i], writepath);
		if (writepath[strlen(writepath)-1] != '/') {
			strcat(outlist[i], "/");
		}
		strcat(outlist[i], strrchr(srclist[i], '/')+1);
		if (strstr(srclist[i], ".md") != NULL) {
			strcpy(strstr(outlist[i], ".md"), ".html");
		}
		i++;
	}
	nfiles = i;
	
	/**************  printing for testing  *****************/
	printf("\n%-*s->  OUTPUT DESTINATION\n", 32, "SOURCE FILE FOUND");
	printf("------------------------------------------------------\n");
	i = 0;
	while (srclist[i][0] != '\0' && i < MAX_LINKS) {
		printf("%s", srclist[i]);
		if (outlist[i][0] != '\0') {
			printf("%*s->  \x1b[33m%s\x1b[0m",
					(int) (32-strlen(srclist[i])), "", outlist[i]);
		}
		printf("\n");
		i++;
	} 
	printf("\n");
	/*******************************************************/

	/* We now have a list of input and output files. The next step is to
	   parse the markdown documents and record their outgoing link lists. */
	for (i = 0; i < nfiles; i++) {
		if (strstr(srclist[i], ".md") != NULL) {
			printf("\x1b[36mparsing \"%s\" (\x1b[33m\"%s\"\x1b[36m)\x1b[0m\n",
				srclist[i], outlist[i]);
			readdoc_md(&doc, srclist[i]);
			if (writedoc_html(&doc, outlist[i]) != 0) {
				freedoc(&doc);
				return(1);
			}
			j = 0;
			while (doc.outlinks[j][0] != '\0') {
				if (strstr(doc.outlinks[j], "//") != NULL) {
					printf("  \x1b[34m%s\x1b[0m\n", doc.outlinks[j]);
				} else {
					matchcount = 0;
					printf("  %s", strrchr(doc.outlinks[j], '/'));
					for (k = 0; k < MAX_FILES; k++) {
						if (outlist[k][0] != '\0') {
							if (strcmp(strrchr(outlist[k], '/'),
							           strrchr(doc.outlinks[j], '/')) == 0) {
								matchcount++;
							}
						}
					}
					if (matchcount == 0) {
						printf("\x1b[31m FILE NOT FOUND!!!\x1b[0m");
					} else if (matchcount > 1) {
						printf("\x1b[31m NAME COLLISION!!!\x1b[0m");
					} else {
						printf("\x1b[32m one match found\x1b[0m");
						/* edgematrix[i][j] = 1
						     =>  file i contains link to file j */
						edgematrix[i][j] = 1;
					}
					printf("\n");
				}
				j++;
			}
			freedoc(&doc);
			printf("\n");
		}
	}
	for (i = 0; i < nfiles; i++) {
		printf("%-32s  ", outlist[i]);
		for (j = 0; j < nfiles; j++) {
			printf("%d ", edgematrix[i][j]);
		}
		printf("\n");
	}
	printf("\n");

	/* Build headers, but... you have to prepend!! */
	for (i = 0; i < nfiles; i++) {
		if (strstr(srclist[i], ".md") != NULL) {
			strcpy(headerbuffer, 
			"<!DOCTYPE html>\n"
			"<html lang=\"en\">\n"
			"<head>\n"
			  "<meta charset=\"UTF-8\">\n"
			  "<title>");
			
			strcat(headerbuffer, strrchr(outlist[i], '/')+1);
		
			strcat(headerbuffer,
			  "</title>\n"
			  "<link rel=\"icon\" type=\"image/png\" "
								 "href=\"/images/favicon_32.png\">\n"
			  "<link rel=\"stylesheet\" href=\"main.css\">\n"
			"</head>\n"
			"<body>");
			
			tmpfp = tmpfile();
			fp = fopen(outlist[i], "r+");
			
			fwrite(headerbuffer, sizeof(char), strlen(headerbuffer), tmpfp);

			while ((j = fread(headerbuffer, sizeof(char), 4096, fp)) != 0) {
				fwrite(headerbuffer, sizeof(char), j, tmpfp);
			}

			fflush(tmpfp);

			rewind(fp);
			rewind(tmpfp);

			while ((j = fread(headerbuffer, sizeof(char), 4096, tmpfp)) != 0) {
				fwrite(headerbuffer, sizeof(char), j, fp);
			}
			
			fclose(tmpfp);
			fclose(fp);
		}
	}

	
	/* Build footers by reading columns of edgematrix to find backlinks. */
	for (i = 0; i < nfiles; i++) {
		if (strstr(srclist[i], ".md") != NULL) {
			for (j = 0; j < nfiles; j++) {
				if (edgematrix[j][i] == 1) {
					j = -1;
					break;
				}
			}
			fp = fopen(outlist[i], "a");
			fseek(fp, 0, SEEK_END);
			if (j == -1) {
				fprintf(fp, "<footer><p>pages that link here:</p><p>");
				for (j = 0; j < nfiles; j++) {
					if (edgematrix[j][i] == 1 && j != i) {
						/* if there is a backlink j -> i, */
						fprintf(fp, "<a href=\"%s\">%s</a>",
								strrchr(outlist[j], '/')+1,
								strrchr(outlist[j], '/')+1);
					}
				}
			} else {
				fprintf(fp, "<footer><p>this page is an orphan!");
			}
			fprintf(fp, "</p></footer></body></html>");
			fclose(fp);
		}
	}
	
	return(0);
}


/* It could be so beautiful. It is so beautiful. We have to bring its elegant
   form into existence, to solidify the ephemeral flutterings-through. */

/* Read the source directory and find all the files. Write their paths
   to pointers in an array. */
void buildindex(char _index[MAX_FILES][MAXDIR], char *srcdir) {
	char workingdir[MAXDIR];
	int n = 0;
	
	getcwd(workingdir, MAXDIR); /* store the cwd to make sure we get back */

	/* walk the directory recursively and save all the filenames */
	recursivefilesearch(srcdir, srcdir, _index, &n);
	_index[n][0] = '\0';

	chdir(workingdir); /* leave no trace */
}

/* Walks a directory recursively. Writes file paths to index[i] and increments
   i as they are found. */
void recursivefilesearch(char *searchdir, char *parent, 
					char index[MAX_FILES][MAXDIR], int *i) {
	DIR *f;
	struct dirent *entry; /* for getting filenames and directory info */
	struct stat filestat; /* for checking if a file is a directory */
	char reldir[MAXDIR];  /* buffer for building the next directory name
	                         (to be passed as "parent" when recursing) */

	if (chdir(searchdir)) {
		fprintf(stderr, "\x1b[31m[librarian] [recursivefilesearch]"
						" could not cd to source directory \"%s\"\x1b[0m\n",
						searchdir);
	}

	if ((f = opendir(".")) == NULL) {
		fprintf(stderr, "\x1b[31m[librarian] [recursivefilesearch]"
						" could not open directory at \".\"\x1b[0m\n");
	}
	
	while ( (entry = readdir(f)) ) {
		stat(entry->d_name, &filestat); /* inspect file information */
		if (S_ISDIR(filestat.st_mode)) { /* if the file is a directory, */
			if (strncmp(entry->d_name, ".", 1) == 0 
				|| strcmp(entry->d_name, "..") == 0) {
				/* and it's either "." or "..", don't follow it! */
				continue;
			} else {
				/* write the new directory (entry->d_name)
				   onto the end of the previous one (parent) */
				strcpy(reldir, parent);
				strcat(reldir, "/");
				strcat(reldir, entry->d_name);
				/* recurse into the new directory (entry->d_name) with
				   knowledge of where we came from (reldir) */
				recursivefilesearch(entry->d_name, reldir, index, i);
			}
		} else { /* if the file is NOT a directory, */
			if (entry->d_name[0] == '.') {
				continue;
			} else {
				if (*i < MAX_LINKS) {
					strcpy(index[*i], parent);
					strcat(index[*i], "/");
					strcat(index[*i], entry->d_name);
					*i += 1;
				}
			}
		}
	}
	closedir(f);
	chdir("..");
}


/*  Walk recursively through [dir], looking for files ending in [ext].
    Write each file as it is found to an index [writefile]. */
void walkfiles(char *dir, char *ext, char *parent, FILE *writefile) {
	
	DIR *f;
	struct dirent *entry;
	struct stat filestat;
	char reldir[MAXDIR];
	
	if (chdir(dir)) {
		fprintf(stderr, "\x1b[31m[librarian] [walkfiles]"
			" could not cd to \"%s\"\x1b[0m\n", dir);
	}
	
	f = opendir(".");
	if (f == NULL) {
		fprintf(stderr, "\x1b[31m[librarian] [walkfiles]"
			" failed to open \".\"\x1b[0m\n");
	}
	
/*	char tempdir[MAXDIR];*/
	
	while ( (entry = readdir(f)) ) {
		stat(entry->d_name, &filestat);
		if (S_ISDIR(filestat.st_mode)) {	
			if (strncmp(entry->d_name, ".", 1) == 0 
				|| strcmp(entry->d_name, "..") == 0) {
				continue;
			} else {
				strcpy(reldir, parent);
				strcat(reldir, "/"); strcat(reldir, entry->d_name);
				walkfiles(entry->d_name, ext, reldir, writefile);
			}
		} else {
			if (entry->d_name[0] == '.') {
				continue;
			}
			if (strstr(entry->d_name, ext)) {
				fprintf(writefile, "%s/%s\n", parent, entry->d_name);
			}
		}
	}
	closedir(f);
	chdir("..");
}



/* Wrapper for walkfiles that only requires two arguments, [filename]
   for the index and [srcdir] for the directory to walk. Assumes that
   we are looking for markdown files. */
FILE *makemdindex(char *filename, char *srcdir) {
	
	char workingdir[MAXDIR];
	FILE *index;
	getcwd(workingdir, MAXDIR);
	
	printf(
		"[librarian] making index \"%s\" for dir \"%s\"\n",
		filename, srcdir);
	
	if ((index = fopen(filename, "w+")) == NULL) {
		fprintf(stderr, "\x1b[31m[librarian] [makemdindex]"
			" failed to open \"%s\"\x1b[0m\n", 
			filename);
		return(NULL);
	}
	
	walkfiles(srcdir, ".md", srcdir, index);
	
	rewind(index);
	
	chdir(workingdir);

	return(index);
}
