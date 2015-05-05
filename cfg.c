
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>

#include "dir.h"
#include "config.h"
#include "cfg.h"
#include "warn.h"
#include "defutil.h"

#if OS & OS_WIN32
#include "bkwin32.h"
#endif


#ifndef MAX_PATH
#define MAX_PATH 4096
#endif // MAX_PATH

struct cfg_s {
    char app_name[256];
    char etc_dir[MAX_PATH];
    char user_dir[MAX_PATH];
    char var_dir[MAX_PATH];
    char log_dir[MAX_PATH];
};

cfg_t*
cfg_new(char *appname) {
    cfg_t *cfg;
    int err=-1;
    char buf[4096];
    
    do {
	cfg = calloc(1, sizeof(*cfg));
	strncpy(cfg->app_name, appname, sizeof(cfg->app_name));

	*buf = 0;

#if OS & OS_WIN32
	// use a subdirectory next to the current executable.  If I'm
	// in "dir/foo.exe", then config goes in "dir/etc/foo"
	n = (int)GetModuleFileName(0, buf, sizeof(buf));
	assertb(n>0);
	for(i=n-1; i>=0; i--) {
	    if( i<n-1 && strchr("/\\", buf[i]) ) break;
	}
	buf[i] = 0;
#endif /* OS & OS_WIN32 */

	snprintf(cfg->etc_dir, sizeof(cfg->etc_dir), "%s/etc/%s", 
		 buf, cfg->app_name);
	
	snprintf(cfg->var_dir, sizeof(cfg->var_dir), "%s/var/lib/%s", 
		 buf, cfg->app_name);

	snprintf(cfg->log_dir, sizeof(cfg->log_dir), "%s/var/log/%s", 
		 buf, cfg->app_name);

	if( getenv("HOME") ) {
	    snprintf(cfg->user_dir, sizeof(cfg->user_dir), 
		     "%s/.%s", getenv("HOME"), appname);
	}
	else {
	    strncpy(cfg->user_dir, cfg->etc_dir, sizeof(cfg->user_dir));
	}

	err = 0;
    } while(0);
    if( err ) {
	if( cfg ) {
	    cfg_delete(cfg);
	    cfg = 0;
	}
    }

    return cfg;
}

cfg_t*
cfg_new_rootdir(char *rootdir) {
    cfg_t *cfg;
    int err=0;
    
    do {
	cfg = cfg_new(rootdir);
	assertb(cfg);
	strncpy(cfg->etc_dir, rootdir, sizeof(cfg->etc_dir));
	strncpy(cfg->user_dir, rootdir, sizeof(cfg->user_dir));
	strncpy(cfg->var_dir, rootdir, sizeof(cfg->var_dir));
	strncpy(cfg->log_dir, rootdir, sizeof(cfg->log_dir));
	err = 0;
    } while(0);
    if( err ) {
	cfg_delete(cfg);
	cfg = 0;
    }
    return cfg;
}

void
cfg_delete(cfg_t *cfg) {
    free(cfg);
}

int
cfg_get_val_or_path(cfg_t *cfg, const char *key, char *val, int len, int path_only) {
    char buf[MAX_PATH];
    int i, n=0, err=-1;
    FILE *f = 0;

    do {
	val[0] = 0;

	// try user dir
	if( *cfg->user_dir ) {
	    i = snprintf(buf, sizeof(buf), "%s/%s", cfg->user_dir, key);
	    assertb(i>=0);
	    f = fopen(buf, "rb");
	}

	// try /etc
	if( !f && *cfg->etc_dir ) {
	    i = snprintf(buf, sizeof(buf), "%s/%s", cfg->etc_dir, key);
	    assertb(i>=0);
	    f = fopen(buf, "rb");
	}
	
	if( f ) {
	    if( path_only ) {
		n = snprintf(val, len, "%s", buf);
	    }
	    else if( val ) {
		n = fread(val, 1, len, f);
		    val[n<len ? n : len-1] = 0;
	    }	 
	    else {
		struct stat st;
		n = 0;
		i = stat(buf, &st);
		if( i == 0 ) {
		    n = st.st_size;
		}
	    }
	}
	err = 0;
    } while(0);
    if( f ) {
	fclose(f);
    }
    return err ? err : n;
}

char*
cfg_path(cfg_t *cfg, const char *key, char *path_buf, int path_len) {
    int i;
    i = cfg_get_val_or_path(cfg, key, path_buf, path_len, 1);
    return i > 0 ? path_buf : 0;
}

int
cfg_get(cfg_t *cfg, const char *key, char *val, int len) {
    return cfg_get_val_or_path(cfg, key, val, len, 0);
}

int
cfg_put(cfg_t *cfg, const char *key, char *val, int len, cfg_put_where_t where) {
    char buf[MAX_PATH], *dir=0;
    int i, n=0, err=-1;
    FILE *f = 0;

    do {
	switch(where) {
	case CFG_PUT_USER:   dir = cfg->user_dir; break;
	case CFG_PUT_GLOBAL: dir = cfg->etc_dir; break;
	default: dir = ""; break;
	}
	
	if( !dir || !*dir ) break;

	i = snprintf(buf, sizeof(buf), "%s/%s", dir, key);
	assertb(i>=0);

	if( !val ) {
	    unlink(buf);
	    err = 0;
	    break;
	}
	
	i = dir_mkpath(buf, 0);
	assertb(!i);

	f = fopen(buf, "wb");
	assertb_syserr(f);
	
	len = len >= 0 ? len : strlen(val);
	n = fwrite(val, 1, len, f);
	assertb(n == len);
	
	err = 0;

    } while(0);
    if( f ) {
	fclose(f);
    }
    return err ? err : n;
}

int
cfg_put_int(cfg_t *cfg, const char *key, int val, cfg_put_where_t where) {
    char buf[1024];
    int i, err=-1;
    do {
	i = snprintf(buf, sizeof(buf), "%d", val);
	assertb(i>0);
	err = cfg_put(cfg, key, buf, i, where);
    } while(0);
    return err;
}


int
 cfg_get_int(cfg_t *cfg, const char *key, int *val) {
    char buf[1024], *p;
    int i, err=-1;
    do {
	*val = 0;

	i = cfg_get(cfg, key, buf, sizeof(buf));
	if(i <= 0) {
	    break;
	}
	
	*val = strtoul(buf, &p, 0);
	assertb(p>buf);
	err = i;
    } while(0);
    return err;
}

char *
cfg_dir(cfg_t *cfg, cfg_dir_where_t where, char *val, int len) {
    char *dir;
    switch(where) {
    case CFG_DIR_USER:   dir = cfg->user_dir; break;
    case CFG_DIR_GLOBAL: dir = cfg->etc_dir; break;
    case CFG_DIR_VAR:    dir = cfg->var_dir; break;
    case CFG_DIR_LOG:    dir = cfg->log_dir; break;
    default: dir = ""; break;
    }
    snprintf(val, len, "%s", dir);
    return val;
}

/* open, read, and close path, returns 0 iff ok */
int
cfg_cat(cfg_t *cfg, char *path, char *val, int len) {
    FILE *f=0;
    int err=-1;
    do {
	f = fopen(path, "rb");
	assertb(f);
	err = fread(val, 1, len, f);
	assertb(err > 0);
	for(; err>0 && isspace(val[err-1]); err--);
	if( err < len ) {
	    val[err]=0;
	}
    } while(0);
    if( f ) fclose(f);
    return err;
}

// put val at the top of the mru
char*
mru(char *mru, char *val, char *dstbuf, int dstlen, int max) {
    char srcbuf[4096], *dst, *src;
    int i, n, err=-1;
    int count;
    do {
	// put val at the top
	dst = dstbuf;
	n = dstlen;
	i = snprintf(dst, n, "%s\n", val);
	BUF_ADD(dst, n, i);

	// append all the other mru lines
	src = mru;
	count = 1;
	while(*src) {
	    if( count >= max ) break;

	    for(i=0; *src && *src != '\n'; i++, src++ ) {
		srcbuf[i] = *src;
	    }
	    if( *src ) {
		src++;
	    }
	    srcbuf[i] = 0;
	    if( !*srcbuf ) {
		continue;
	    }
	    if( strcasecmp(val, srcbuf)==0 ) {
		continue;
	    }
	    i = snprintf(dst, n, "%s\n", srcbuf);
	    BUF_ADD(dst, n, i);

	    count++;
	}
	
	err = 0;
	    
    } while(0);
    return err ? 0 : dstbuf;
}

int
cfg_list_pack(char **p, int *n, char *src) {
    int i;
    
    i = snprintf(*p, *n, "%s%c", src, '\n');
    if( i > 0 ) {
	*p += i;
	*n -= i;
    }
    return i;
}

char*
cfg_list_next(char **q, char *buf, int len) {
    char *p = *q;
    char *orig = buf;
    if( !p || !*p ) return 0;
    len--;
    for(; *p && *p != '\n'; p++) {
	if( len > 0 ) {
	    *buf = *p;
	    buf++;
	    len--;
	}
    }
    if( len > 0 ) {
	*buf = 0;
	len--;
    }
    if( *p ) {
	p++;
    }
    *q = p;
    return len >= 0 ? orig : 0;
}


#if OS & OS_WIN32
#include "cfg_win32.c"
#endif

#if OS & OS_UNIX
#include "cfg_unix.c"
#endif
