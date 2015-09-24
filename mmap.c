#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "debug.h"
#include "mmap.h"

static
int
init_mmap(mmap_t *mm) {
    int err = -1;
    int prot;
    int flags;
    
    do {
	if( mm->max == 0 ) {
	    err = 0;
	    break;
	}
	prot = PROT_READ;
	if( mm->mode & MMAP_MODE_WRITE ) {
	    prot |= PROT_WRITE;
	}
	if( mm->mode & MMAP_MODE_PRIVATE ) {
	    flags = MAP_PRIVATE;
	}
	else {
	    flags = MAP_SHARED;
	}
	mm->start = mmap(0, mm->max, prot, flags, mm->fd, 0);
	assertb_syserr(mm->start);
	err = 0;
    } while(0);
    return err;
}


int
mmap_open(mmap_t *mm, const char *path, int mode, int blocksz) {
    int err=-1;
    int flags;
    int fd;
    do {
	memset(mm, 0, sizeof(*mm));
	if( mode & MMAP_MODE_WRITE ) {
	    flags = O_RDWR | O_CREAT;
	}
	else {
	    flags = O_RDONLY;
	}
	fd = open(path, flags, 0777);
	assertb_syserr(fd >= 0);
	mm->close_fd = 1;
	
	err = mmap_fd(mm, fd, mode, blocksz);
    } while(0);
    
    return err;
}

int
mmap_fd(mmap_t *mm, int fd, int mode, int blocksz) {
    int e, err=-1;
    struct stat st;
       
    do {
	memset(mm, 0, sizeof(*mm));
	mm->fd = fd;
	mm->mode = mode;
	if( blocksz == 0 ) {
	    blocksz = 4 * sysconf(_SC_PAGE_SIZE);
	}
	mm->blocksz = blocksz;
	e = fstat(mm->fd, &st);
	assertb_syserr(!e);
	mm->max = st.st_size;
	mm->len = mm->max;
	err = init_mmap(mm);
    } while(0);
    return err;
}

void*
mmap_write(mmap_t *mm, size_t off, size_t len) {
    int e;
    void *ret = 0;
    size_t old_max;
    do {
	assertb(mm->mode & MMAP_MODE_WRITE);
	if( off == MMAP_APPEND ) {
	    off = mm->len;
	}

	if(!mm->start || off + len > mm->max) {
	    old_max = mm->max;
	    mm->max = off + len;
	    mm->max = (mm->max + mm->blocksz - 1) / mm->blocksz * mm->blocksz;
	    assertb(mm->max >= off + len);
	    
	    e = lseek(mm->fd, mm->max-1, SEEK_SET);
	    assertb_syserr(e == mm->max-1);
	    e = write(mm->fd, &ret, 1);
	    assertb_syserr(e == 1);
	    
	    if( mm->start ) {
		mm->start = mremap(mm->start, old_max, mm->max, MREMAP_MAYMOVE);
		assertb_syserr(mm->start);
	    }
	    else {
		e = init_mmap(mm);
		assertb(!e)
	    }
	}
	
	if(off + len > mm->len ) {
	    mm->len = off + len;
	}

	ret = mm->start + off;
    } while(0);
    return ret;
}

size_t
mmap_offset(mmap_t *mm, void *ptr) {
    return ptr - mm->start;
}

void*
mmap_ptr(mmap_t *mm, size_t len) {
    return mm->start + len;
}

int
mmap_close(mmap_t *mm) {
    int e, err = -1;
    do {
	if( mm->start ) {
	    e = munmap(mm->start, mm->max);
	    if( e != 0 ) {
		debugf("munmap(%d, %p) failed", mm->fd, mm->start);
	    }
	    mm->start = 0;
	}
	
	if( mm->fd >= 0 ) {
	    if( (mm->mode & MMAP_MODE_WRITE) && (mm->len != mm->max) ) {
		e = ftruncate(mm->fd, mm->len);
		if( e != 0 ) {
		    debugf("ftruncate(%d, %d) failed", mm->fd, (int)mm->len);
		}
	    }
	    if( mm->close_fd ) {
		close(mm->fd);
	    }
	}

	err = 0; 
    } while(0);
    return err;
}

