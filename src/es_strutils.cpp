//=========================================================================
// Includes
//=========================================================================
#include "es_strutils.h"

//=========================================================================
// Removes escape characters from a string.
//=========================================================================
void unescape(char * s, const char * t) {
	int i, j;
	i = j = 0;

	while ( t[i] ) {
		switch ( t[i] ) {
			//TODO: "Make sure this doesn't crash with \ at end of line"
		case '\\':

			/*  We've found an escape sequence, so translate it  */

			switch( t[++i] ) {
		case 'n':
			s[j] = '\n';
			break;

		case 't':
			s[j] = '\t';
			break;

		case 'a':
			s[j] = '\a';
			break;

		case 'b':
			s[j] = '\b';
			break;

		case 'f':
			s[j] = '\f';
			break;

		case 'r':
			s[j] = '\r';
			break;

		case 'v':
			s[j] = '\v';
			break;

		case '\\':
			s[j] = '\\';
			break;

		case '\"':
			s[j] = '\"';
			break;

		default:

			/*  We don't translate this escape
			sequence, so just copy it verbatim  */

			s[j++] = '\\';
			s[j] = t[i];
			}
			break;

		default:

			/*  Not an escape sequence, so just copy the character  */

			s[j] = t[i];
		}
		++i;
		++j;
	}
	s[j] = t[i];    /*  Don't forget the null character  */
}



//=========================================================================
// String replace.
//=========================================================================
char *str_replace(const char *subject, const char *search, const char *replace, char *replaced, int size)
{
	if (search == NULL || replace == NULL || subject == NULL) return NULL;
	if (strlen(search) == 0 || strlen(subject) == 0) return NULL;

	char *temp = NULL;
	const char *p = subject, *p3 = subject, *p2;
	int  found = 0;

	while ( (p = strstr(p, search)) != NULL) {
		found = 1;
		strncat(replaced, p - (p - p3), p - p3);
		strcat(replaced, replace);
		p3 = p + strlen(search);
		p += strlen(search);
		p2 = p;
	}

	if (found == 1) {
		if (strlen(p2) > 0) {
			strcat(replaced, p2);
		}
	} else {
		strcpy(replaced, subject);
	}
	return replaced;
}

