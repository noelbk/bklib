struct cfg_enum_s {
    char path[4096];
    HANDLE find_handle;
    WIN32_FIND_DATA find_data;
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
	snprintf(e->path, sizeof(e->path), "%s/%s/*", 
		 cfg_dir(cfg, where, buf, sizeof(buf)), subdir);
	e->find_handle = FindFirstFile(e->path, &e->find_data);
	e->ok = 1;
	
	/* strip off trailing "/*" */
	e->path[strlen(e->path)-2] = 0;
    } while(0);
    if( e && (!e->find_handle || e->find_handle == INVALID_HANDLE_VALUE) ) {
	free(e);
	e = 0;
    }
    return e;
}

char*
cfg_enum_read(cfg_enum_t *e, char *buf, int len) {
    while(e->ok) {
	if( strcmp(e->find_data.cFileName, ".") != 0 
	    && strcmp(e->find_data.cFileName, "..") != 0 
	    ) {
	    /* skip "." and ".." */
	    break;
	}
	e->ok = FindNextFile(e->find_handle, &e->find_data);
    }
    if( !e->ok ) {
	return 0;
    }
    
    snprintf(buf, len, "%s/%s", e->path, e->find_data.cFileName);

    e->ok = FindNextFile(e->find_handle, &e->find_data);
    return buf;
}

void
cfg_enum_close(cfg_enum_t *e) {
    if( e ) {
	if( e->find_handle ) {
	    FindClose(e->find_handle);
	}
	free(e);
    }
}
