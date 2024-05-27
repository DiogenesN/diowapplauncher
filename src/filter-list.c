/* Give a list of strings and a key to search and it will create a filtered list (case insensitive)
 * Usage:
 * char *strings[] = { "Hello", "My", "dear", "Darling", NULL };
 * char *filteredStrings[2048];
 * filtered_list(strings, filteredStrings, "dar");
 * int i = 0;
 * while (filteredStrings[i] != NULL) {
 * 		printf("%s\n", filteredStrings[i]);
 *      i = i + 1;
 * }
 * the output: Darling
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

int strcasecmp(const char *s1, const char *s2) {
	while (*s1 && *s2 && tolower(*s1) == tolower(*s2)) {
		s1 = s1 + 1;
		s2 = s2 + 1;
	}
	return tolower(*s1) - tolower(*s2);
}

char *strcasestr(const char *haystack, const char *needle) {
	size_t len = strlen(needle);
	while (*haystack) {
		if (strncasecmp(haystack, needle, len) == 0) {
			return (char *)haystack;
		}
		haystack++;
	}
	return NULL;
}

void filtered_list(char *list[], char *filteredList[], char *key) {
	int i = 0, j = 0;
	while (list[i] != NULL) {
		if (strcasestr(list[i], key) != NULL) {
			filteredList[j++] = list[i];
		}
		i = i + 1;
	}
	filteredList[j] = NULL;  // Null-terminate the filtered list
}
