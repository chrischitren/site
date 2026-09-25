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
#include "parsemd.h"

int main() {
	char mdfiledir[MAXDIR];
	char htmlfiledir[MAXDIR];
	struct stat st;
	FILE *ifp;
	const char *writepath = "html2";
	
	
	ifp = makemdindex("_index.txt", "src");
		
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
	
	while (fgets(mdfiledir, MAXDIR, ifp) != NULL) {
		mdfiledir[strcspn(mdfiledir, "\n")] = '\0';
		printf("\x1b[33m%s\x1b[0m\n", mdfiledir);

		strcpy(htmlfiledir, writepath);
		strcat(htmlfiledir, strrchr(mdfiledir, '/'));
		strcpy(strstr(htmlfiledir, ".md"), ".html");
		printf(" -> \x1b[33m%s\x1b[0m\n\n", htmlfiledir);

		mdtohtml(mdfiledir, htmlfiledir);
	}
	
	fclose(ifp);
	return(0);
}


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
			if (strstr(entry->d_name, ext)) {
				fprintf(writefile, "%s/%s\n", parent, entry->d_name);
			}
		}
	}
	closedir(f);
	chdir("..");
}
