#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*	adapted from
	https://c-for-dummies.com/blog/?p=3246
	and subsequent posts */

/*

struct mdfile {
	int id;
	char *path;
} */

int rlisting(char *directory, char *parent, int depth, char *pexcl);

int main() {
	
	printf("%d\n", rlisting("src", ".", 0, "."));
	
	return(0);
}

/*	This thing is a lot like a human. It navigates by changing the
	directory and listing the files, checking if each one is itself
	a directory. It is not complicated, but it is obtuse because it
	must conform to the C language.  

	pexcl is used to exclude directories that start with a prefix */
int rlisting(char *directory, char *parent, int depth, char *pexcl) {
	
	char fulldir[strlen(parent)+1+strlen(directory)];

	strcpy(fulldir, parent);
	strcat(fulldir, "/");
	strcat(fulldir, directory);

	int filecount = 0;
	DIR *f; /* a "directory" as opendir understands */
	struct dirent *entry; /* struct from dirent.h; a "file" as listed in a dir */
	struct stat filestat; /* struct from stat.h with info about filetypes */
	
	if (chdir(directory)) {
		fprintf(stderr, "error at chdir");
		exit(1);
	}
	
	f = opendir(".");
	if (f == NULL) {
		fprintf(stderr, "opendir returned NULL");
	}
	
	while (entry = readdir(f)) {
		stat(entry->d_name,&filestat);
		if ( S_ISDIR(filestat.st_mode) ) {
			if (strncmp(entry->d_name,pexcl,strlen(pexcl)) == 0){
				continue;
			} else {
				/* printf("%*sD %s\n", depth*2, "", entry->d_name); */
				filecount += rlisting(entry->d_name, fulldir, depth+1, pexcl);
			}
		} else {
			filecount++;
			printf("%*s%s/%s\n", depth*2, "", fulldir, entry->d_name);
			/* printf("%s\n", directory); */
		}
	}
	
	chdir("..");
	closedir(f);

	return filecount;
}
