#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#include "dir.h"
#include "debug.h"

struct dir_s {
    DIR *dir;
};

dir_t*
dir_open(char *path) {
    dir_t *d=0;
    int err = -1;
    do {
	d = calloc(1, sizeof(*d));
	assertb(d);
	d->dir = opendir(path);
	assertb(d->dir);
	err = 0;
    } while(0);
    if( err ) {
	dir_close(d);
	d = 0;
    }
    return d;
}

// returns 0 on eof, >0 on read, <0 on error
int
dir_read_dots(dir_t *d, char *buf, int len) {
    int err=-1;
    struct dirent *dent;

    do {
	dent = readdir(d->dir);
	if( !dent ) {
	    return 0;
	}
	assertb_syserr(dent);
	strncpy(buf, dent->d_name, len);
	err = 1;
    } while(0);
    return err;
}

int
dir_close(dir_t *d) {
    if( d ) {
	if(d->dir) {
	    closedir(d->dir);
	}
	free(d);
    }
    return 0;
}

int 
names_cmp(const void *a, const void *b ) {
    return strcmp(*(char**)a, *(char**)b);
}


char*
dir_where_exe(char *exename, char *pathbuf, int pathlen) {
    char *p;
    int i;
    struct stat st;


    p = getenv("PATH");
    if( !p ) return 0;

    while(1) {
	for(i=0; i<pathlen-1 && *p && *p!=':'; p++, i++) {
	    pathbuf[i] = *p;
	}
	snprintf(pathbuf+i, pathlen-i, "/%s", exename);
	i = stat(pathbuf, &st);
	if( i == 0 && S_ISREG(st.st_mode) ) {
	    return pathbuf;
	}
	if( *p != ':' ) break;
	p++;
    }
    return 0;
}
