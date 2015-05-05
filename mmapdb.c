

typedef struct {
    unsigned long ptr_file;
    unsigned long ptr_off;
};

mmapdb_init(mmapdb *mdb, char *file, int mode, size_t len) {
    int f, prot, flags, fmode;
    
    do {
	prot = PROT_READ;
	fmode = O_RDONLY;
	flags = 0;
	if(mode & MMAPDB_CREATE) {
	    fmode |= O_CREAT;
	}
	if(mode & MMAPDB_WRITE) {
	    fmode |= O_WRONLY;
	    prot |= PROT_WRITE;
	}
	if(mode & MMAPDB_SHARED) {
	    flags |= MAP_SHARED;
	}
	if(mode & MMAPDB_PRIVATE) {
	    flags |= MAP_PRIVATE;
	}

	f = open(file, fmode);
	assertb_syserr(f>=0);

	/* map the whole file */
	if( !len ) {
	    fstat(f, &st);
	    len = st.st_len;
	}
	/* round up to a multiple of the page size */
	i = getpagesize();
	mdb->mmap_len = ((len + i-1)/i) * i;
	mdb->mmap_ptr = mmap(0, mdb->mmap_len, prot, flags, f, 0);
	assertb_syserr(mdb->mmap_ptr);
	
	
	p = mdb->mmap_ptr;
	n = mdb->mmap_len;
	mdb->hdr_len = sizeof(mdb->hdr);
	if( mode & MMAPDB_CREATE ) {
	    pack_u32(PACK_WRITE, &p, &n, &mdb->hdr_len);
	    pack_mmdb_hdr(PACK_WRITE, &p, &n, &mdb->hdr);
	}
	else {
	    pack_u32(PACK_READ, &p, &n, &mdb->hdr_len);
	    pack_mmdb_hdr(PACK_READ, &p, &n, &mdb->hdr);
	}
	mdb->data_ptr = mdb->mmap_ptr + mdb->hdr_len;

	
	err = 0;

    } while(0);
    if( f>=0 ) {
	close(f);
    }

}

mmapdb_alloc(mmapdb *mdb, size_t sz, mmapdb_ptr *p) {
}

