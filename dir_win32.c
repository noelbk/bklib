#include "bkwin32.h"
#include <errno.h>
#include <direct.h>

#include "dir.h"
#include "debug.h"

struct dir_s {
    HANDLE h;
    WIN32_FIND_DATA fi;
    int first;
};

dir_t*
dir_open(char *path) {
    dir_t *d=0;
    int i, err = -1;
    char *p=0;
    do {
	i = strlen(path)+4;
	p = malloc(i);
	assertb(p);
	strncpy(p, path, i);
	strncat(p, "/*", i);

	d = calloc(1, sizeof(*d));
	assertb(d);
	d->h = INVALID_HANDLE_VALUE;

	d->h = FindFirstFile(p, &d->fi);
	if(d->h == INVALID_HANDLE_VALUE 
	   && GetLastError() == ERROR_NO_MORE_FILES) {
	    break;
	}
	assertb_syserr(d->h);
	d->first = 1;
	err = 0;
    } while(0);
    if( p ) {
	free(p);
    }
    if( err ) {
	dir_close(d);
	d = 0;
    }
    return d;
}

// returns 0 on eof, >0 on read, <0 on error
int
dir_read_dots(dir_t *d, char *buf, int len) {
    int i, err=-1;

    do {
	if( d->first ) {
	    d->first = 0;
	}
	else {
	    i = FindNextFile(d->h, &d->fi);
	    if(i == 0 && GetLastError() == ERROR_NO_MORE_FILES) {
		err = 0;
		break;
	    }
	    assertb_syserr(i);
	}
	strncpy(buf, d->fi.cFileName, len);
	err = 1;
    } while(0);
    return err;
}

int
dir_close(dir_t *d) {
    if( d ) {
	if(d->h != INVALID_HANDLE_VALUE) {
	    FindClose(d->h);
	    d->h = INVALID_HANDLE_VALUE;
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
    int i, err=-1;
    char exebuf[4096], *p;
    HINSTANCE h=0;

    do {
	i = snprintf(exebuf, sizeof(exebuf), "%s", exename);
	assertb(i>0);
	p = dir_filename(exebuf, 0);
	assertb(p);
	if( !strchr(p, '.') ) {
	    strncat(exebuf, ".exe", sizeof(exebuf)-strlen(exebuf)-1);
	}
	for(p=exebuf; *p; p++) {
	    if( *p == '\\' ) *p = '/';
	}

	h = LoadLibrary(exebuf);
	assertb_syserr(h);

	i = GetModuleFileName((HMODULE)h, pathbuf, pathlen);
	assertb_syserr(i>0);
	
	err = 0;
    } while(0);
    if( h ) {
	FreeLibrary(h);
    }

    return err ? 0 : pathbuf;
}

