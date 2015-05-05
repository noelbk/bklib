/* vercmp.c - compare version strings like debian's dpkg
 *
 * Noel Burton-Krahn <noel@burton-krahn.com>
 * Feb 16, 2005
 *
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "config.h"
#include "vercmp.h"

typedef enum version_part_t {
    VERSION_PART_STR
    ,VERSION_PART_INT
} version_part_t;

version_part_t
version_part(char **pv, char *buf, int len) {
    int i;
    *buf = 0;
    *pv += strspn(*pv, ".-");
    i = strcspn(*pv, ".-");
    strncpy(buf, *pv, len);
    *pv += i;
    if( i >= len ) { i = len-1; }
    buf[i] = 0;
    for(i=0; buf[i]; i++) {
	if( !isdigit(buf[i]) ) { 
	    return VERSION_PART_STR;
	}
    }
    return VERSION_PART_INT;
}

int
vercmp(char *v1, char *v2) {
    char buf1[1024], buf2[1024];
    char *p;
    version_part_t t1, t2;
    int i;
    
    do {
	t1 = version_part(&v1, buf1, sizeof(buf1));
	t2 = version_part(&v2, buf2, sizeof(buf2));
    
	if( t1 == t2 && t1 == VERSION_PART_INT ) {
	    i = strtol(buf1, &p, 10) - strtol(buf2, &p, 10);
	}
	else {
	    i = strcasecmp(buf1, buf2);
	}
	if( i != 0 ) {
	    return i;
	}
    } while(*v1 && *v2);
    return *v1 ? 1 : *v2 ? -1 : 0;
}


