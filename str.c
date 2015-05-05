#include <ctype.h>
#include <string.h>
#include "bklib/str.h"

char*
str_trim(char *str) {
    char *s, *p;
    for(p=str; *p && isspace(*p); p++);
    s = p;
    for(p=s+strlen(s)-1; p>=s && isspace(*p); p--);
    p++;
    *p = 0;
    return s;
}


char*
str_collapse(char *str, char *chars, char *buf, int len) {
    char *end = buf+len-1, *orig=buf, *dst=buf, *src;
    for(src=str; *src && dst<end; src++) {
	if( strchr(chars, *src) ) {
	    *dst = *src;
	    dst++;
	}
    }
    *dst = 0;
    return orig;
}
