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
	int i, j, wi;
	char *wptr;
	char mdfiledir[MAXDIR];
	char htmlfiledir[MAXDIR];
	char srclist[MAX_LINKS][MAXDIR];
	struct stat st;
	FILE *ifp;
	char *readpath = ".";
	const char *writepath = "html2";
	Document doc;
	
	
	ifp = makemdindex("_index.txt", readpath);
	
	i = 0;
	while (fgets(srclist[i], MAXDIR, ifp) != NULL) {
		srclist[i][strcspn(srclist[i], "\n")] = '\0';
		
		strcpy(strstr(srclist[i], ".md"), ".html");
		i++;
	}
	srclist[i][0] = '\0';
	rewind(ifp);
	i = 0;
	
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

	/* Writing  */
	while (fgets(mdfiledir, MAXDIR, ifp) != NULL) {
		mdfiledir[strcspn(mdfiledir, "\n")] = '\0';
		printf("\n\x1b[33m%s\x1b[0m\n", mdfiledir);

		strcpy(htmlfiledir, writepath);
		strcat(htmlfiledir, strrchr(mdfiledir, '/'));
		strcpy(strstr(htmlfiledir, ".md"), ".html");
		printf(" -> \x1b[33m%s\x1b[0m\n", htmlfiledir);

		readdoc_md(&doc, mdfiledir);
		
		i = 0;
		while (doc.outlinks[i][0] != '\0') {
			if (strstr(doc.outlinks[i], "//") == NULL) {
				printf("  %s\n", strrchr(doc.outlinks[i], '/')+1);
				j = 0;
				while (srclist[j][0] != '\0') {
					if (strcmp( strrchr(srclist[j], '/')+1,
					            strrchr(doc.outlinks[i], '/')+1) == 0) {
						printf("\x1b[32m found \"%s\"\x1b[0m\n", srclist[j]);
						j = -1;
						break;
					}
					wi = 0;
					wptr = srclist[j];
					printf("     %s", readpath);
					while ((wptr = strchr(wptr, '/')) != NULL) {
						/*wi = wi << 8;
						for (; (wi & 255) < (wi >> 8); wi++) { printf("/"); }
						wi = wi >> 8;*/
						wptr++;
						if (strchr(wptr, '/') == NULL) {
							printf("/\x1b[36m");
						} else {
							printf("/\x1b[34m");
						}
						printf("%.*s\x1b[0m",
								(int) strcspn(wptr, "/"), wptr);
					}
					printf("\n");
					j++;
				}
				if (j != -1) {
					printf("\x1b[31m[librarian]"
							" broken link in \"%s\": \"%s\"\x1b[0m\n",
							mdfiledir, doc.outlinks[i]);
				}
			}
			i++;
		}

		
		
		writedoc_html(&doc, htmlfiledir);
		freedoc(&doc);
		
	}
	
	fclose(ifp);
	return(0);
}


/* It could be so beautiful. It is so beautiful. We have to bring its elegant
   form into existence, to solidify the ephemeral flutterings-through. */
 


/*  Wrapper for walkfiles that only requires two arguments, [filename]
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
