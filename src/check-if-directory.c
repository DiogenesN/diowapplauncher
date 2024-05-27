/* Check is the given path is a valid path, exists and is a directory
 * Udage:
 * const char *path = "/home/diogenes/Exec"; 
 * if (is_directory(path)) {
 *		printf("The path '%s' is a valid directory.\n", path);
 *	}
 *	else {
 *		printf("The path '%s' does not exist or is not a directory.\n", path);
 *	}
 */

#include <stdio.h>
#include <sys/stat.h>

int is_directory(const char *path) {
	struct stat path_stat;

  	 // Use stat to get information about the file at 'path'
	if (stat(path, &path_stat) != 0) {
		// stat() failed, the path does not exist or is inaccessible
		perror("stat");
		return 0; // Path does not exist or is not accessible
	}

	// Check if the path is a directory
	if (S_ISDIR(path_stat.st_mode)) {
		return 1; // Path exists and is a directory
	}
	else {
		return 0; // Path exists but is not a directory
	}
}
