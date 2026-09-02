#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*adapted from
https://c-for-dummies.com/blog/?p=3246
and subsequent posts */

void rlisting(char *directory, int depth, char *pexcl);

int main() {
rlisting("src", 0, ".");

return(0);
}


/*This thing is a lot like a human. It navigates by changing the
directory and listing the files, checking if each one is itself
a directory. It is not complicated, but it is obtuse because it
must conform to the C language.  */
void rlisting(char *directory, int depth, char *pexcl) {

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
		printf("%*sD %s\n", depth*2, "", entry->d_name);
		rlisting(entry->d_name, depth+1, pexcl);
	}
} else {
	printf("%*sF %s\n", depth*2, "", entry->d_name);
}
}

chdir("..");
closedir(f);
}
