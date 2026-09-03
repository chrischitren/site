#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAXLEN 256
#define MODEBITS S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH

/*  Walks through a given directory recursively and records all
    the files there with a given extension (.md in this case).
    Makes an index file that contains the relative paths to these. */

void walkfiles(char *dir, char *ext, char *parent, FILE *writefile);
FILE *makemdindex(char *filename, char *srcdir);

int main() {
	
	FILE *_index = makemdindex("_index.txt", "src");

	char indexlinedir[MAXLEN];

	while (fgets(indexlinedir, MAXLEN, _index) != NULL) {
		FILE *tempfile;

		size_t len = strcspn(indexlinedir, "\n\r");
		indexlinedir[len] = '\0';

		if ((tempfile = fopen(indexlinedir, "r")) == NULL) {
			printf("could not open file \x1b[31m%s\x1b[0m\n", indexlinedir);
			continue;
		}

		printf("\x1b[32m%s\x1b[0m\n", indexlinedir);

		char c;
		while ((c = fgetc(tempfile)) != EOF) {
			fputc(c, stdout);
		}

		fclose(tempfile);
	}
	
	fclose(_index);
	
	return(0);
}

/*  Wrapper for walkfiles that only requires two arguments, [filename]
    for the index and [srcdir] for the directory to walk. Assumes that
    we are looking for markdown files. */
FILE *makemdindex(char *filename, char *srcdir) {
	
	char workingdir[MAXLEN];

	getcwd(workingdir, MAXLEN);

	FILE *index;
	
	if ((index = fopen(filename, "w+")) == NULL) {
		printf("failed to open %s\n", filename);
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
	
	if (chdir(dir)) {
		fprintf(stderr, "err at cd to %s", dir);
	}
	
	f = opendir(".");
	if (f == NULL) {
		fprintf(stderr, "err opening . directory");
	}
	
	char tempdir[MAXLEN];
	
	while (entry = readdir(f)) {
		stat(entry->d_name, &filestat);
		if (S_ISDIR(filestat.st_mode)) {	
			if (strncmp(entry->d_name, ".", 1) == 0 
				|| strcmp(entry->d_name, "..") == 0) {
				continue;
			} else {
				char reldir[strlen(parent)+1+strlen(entry->d_name)];
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
