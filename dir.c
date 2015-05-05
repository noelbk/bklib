#include <string.h>
#include "config.h"

#if OS & OS_UNIX
#include "dir_unix.c"
#include <sys/stat.h>
#include <sys/types.h>
#endif

#if OS & OS_WIN32
#include "dir_win32.c"
#endif

#include <stdlib.h>

// skip "." and ".."
int
dir_read(dir_t *d, char *buf, int len) {
    int i;
    while(1) {
	i = dir_read_dots(d, buf, len);
	if( i <= 0 ) break;
	if( !strcmp(buf, ".") || !strcmp(buf, "..") )
	    continue;
	break;
    }
    return i;
}

int
dir_sorted(char *path, char **names, int namelen, char *buf, int buflen) {
    int nname=0;
    char *p=buf, *end = buf+buflen;
    int i, err=-1;
    dir_t *d=0;

    do {
	d = dir_open(path); 
	assertb(d);
	while(1) {
	    assertb(nname<namelen);
	    assertb(p<end);
	    i = dir_read(d, p, end-p);
	    assertb(i>=0);
	    if( i == 0 ) {
		err = 0;
		break;
	    }
	    names[nname++] = p;
	    p += strlen(p)+1;
	}
	assertb(!err);
	qsort(names, nname, sizeof(*names), names_cmp);
    } while(0);
    dir_close(d);
    return err ? err : nname;
}

// return the last component of dir, optionally truncating dir
char *
dir_filename(char *dir, int chop) {
    char *p, *q;
    p = dir;
    q = 0;
    while(*p) {
	p += 1+strcspn(p+1, "/\\");
	if( !p || !*p ) {
	    break;
	}
	q = p;
    }
    if( q ) {
	if( chop ) {
	    *q = 0;
	}
	q++;
    }
    return q ? q : dir;
}

// make all directories in path, except for the last component
int
dir_mkpath(char *dir, int last) {
    char *p, c;
    int err=-1;

    p = dir;
    while(*p) {
	err = -1;
	p += 1+strcspn(p+1, "/\\");
	if( !p || !*p ) {
	    if( last ) {
		err = dir_mkdir(dir);
	    }
	    break;
	}
	c = *p;
	*p = 0;
	err = dir_mkdir(dir);
	*p = c;
	err = 0;
    }
    if( err && errno != EEXIST ) {
	do {
	    assertb_syserr(0);
	} while(0);
	return err;
    }
    return 0;
}

int
dir_mkdir(char *dir) {
#if OS == OS_UNIX
    return mkdir(dir, 0777);
#endif 
#if OS == OS_WIN32
    return mkdir(dir);
#endif 

}

#define PATH_SEP "\\/"

char*
dir_basename(char *path, char *basename, int len) {
    char *p;
    p=path+strlen(path);
    
    // skip trailing "/"
    for(;p>path && strchr(PATH_SEP, *p); p--);

    // seek back to last pathsep
    for(;p>path && !strchr(PATH_SEP, *p); p--);
    if( strchr(PATH_SEP, *p) ) {
	p++;
    }
    
    // copy into basename
    strncpy(basename, p, len);

    return basename;
}
