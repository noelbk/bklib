#include <sys/types.h>
#include <dirent.h>

struct cfg_enum_s {
    char path[4096];
    DIR *dir;
    int ok;
};

cfg_enum_t*
cfg_enum_open(cfg_t *cfg, cfg_dir_where_t where, char *subdir) {
    char buf[4096];
    cfg_enum_t *e=0;
    do {
	if( where == CFG_DIR_NONE ) {
	    e = cfg_enum_open(cfg, CFG_DIR_USER, subdir);
	    if( e ) return e;
	    e = cfg_enum_open(cfg, CFG_DIR_GLOBAL, subdir);
	    if( e ) return e;
	    return 0;
	}

	e = (cfg_enum_t*)calloc(1, sizeof(*e));
	assertb(e);
	snprintf(e->path, sizeof(e->path), "%s/%s", 
		 cfg_dir(cfg, where, buf, sizeof(buf)), subdir);
	e->dir = opendir(e->path);
	
    } while(0);
    if( e && !e->dir ) {
	free(e);
	e = 0;
    }
    return e;
}

char*
cfg_enum_read(cfg_enum_t *e, char *buf, int len) {
    struct dirent *d;

    while(1) {
	d = readdir(e->dir);	
	if( !d ) {
	    break;
	}
	if( strcmp(d->d_name, ".") != 0 
	    && strcmp(d->d_name, "..") != 0 
	    ) {
	    /* skip "." and ".." */
	    break;
	}
    }
    if( !d ) {
	return 0;
    }
    
    snprintf(buf, len, "%s/%s", e->path, d->d_name);
    return buf;
}

void
cfg_enum_close(cfg_enum_t *e) {
    if( e ) {
	if( e->dir ) {
	    closedir(e->dir);
	}
	free(e);
    }
}
