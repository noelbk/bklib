#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "debug.h"
#include "mmap.h"
#include "mstime.h"
#include "readable.h"
#include "defutil.h"

FILE *tmpfile(void);

typedef struct {
    enum {
	OP_END = 0
	,OP_READ
	,OP_WRITE
    } op;
    size_t off;
    size_t len;
} op_t;

#define OP_READ_MAX 10000
#define NOPS 10000
#define SIZE_MAX 1000000000

int
test_mmap(int fd, op_t *op) {
    mmap_t mm;
    int e, err=-1;
    int i;
    char *p;
    long sum = 0;
    
    do {
	e = mmap_fd(&mm, fd, MMAP_MODE_WRITE, 0);
	assertb(!e);

	for(; op->op != OP_END; op++) {
	    switch(op->op) {
	    case OP_END: {
		break;
	    }

	    case OP_READ: {
		p = mmap_ptr(&mm, op->off);
		assertb(p);
		for(i=0; i<op->len; i++) {
		    sum += *p++;
		}
		break;
	    }
	    
	    case OP_WRITE: {
		p = mmap_write(&mm, op->off, op->len);
		assertb(p);
		for(i=0; i<op->len; i++) {
		    sum += *p++ = (char)i;
		}
		break;
	    }
	    }
	}
	
	err = sum;
    } while(0);
    mmap_close(&mm);
    return err;
}

int
test_read(int fd, op_t *op) {
    int e, err=-1;
    int i, n;
    char *p;
    char buf[OP_READ_MAX];
    long sum = 0;
    
    do {
	assertb_syserr(fd>=0);

	for(; op->op != OP_END; op++) {
	    switch(op->op) {
	    case OP_END: {
		break;
	    }

	    case OP_READ: {
		e = lseek(fd, op->off, SEEK_SET);
		assertb_syserr(e == op->off);
		n = read(fd, buf, op->len);
		assertb_syserr(n == op->len);
		p = buf;
		for(i=0; i<op->len; i++) {
		    sum += *p++;
		}
		break;
	    }
	    
	    case OP_WRITE: {
		e = lseek(fd, op->off, SEEK_SET);
		assertb_syserr(e == op->off);
		p = buf;
		for(i=0; i<op->len; i++) {
		    sum += *p++ = (char)i;
		}
		n = write(fd, buf, op->len);
		assertb_syserr(n == op->len);
		break;
	    }
	    }
	}
	
	err = sum;
    } while(0);
    return err;
}

// Assumes 0 <= max <= RAND_MAX
// Returns in the half-open interval [0, max]
long randi(long max) {
    unsigned long
	// max <= RAND_MAX < ULONG_MAX, so this is okay.
	num_bins = (unsigned long) max + 1,
	num_rand = (unsigned long) RAND_MAX + 1,
	bin_size = num_rand / num_bins,
	defect   = num_rand % num_bins;

    long x;
    do {
	x = random();
    }
    // This is carefully written not to overflow
    while (num_rand - defect <= (unsigned long)x);

    // Truncated division is intentional
    return x/bin_size;
}

int
main(int argc, char **argv) {
    int e, err=1;
    mstime_t t1, dt;
    op_t *ops;
    size_t maxlen = 0;
    int i;
    struct stat st;
    char path_read[256], path_mmap[256];
    int fd_read=-1, fd_mmap=-1;
    mmap_t mm_mmap;
    mmap_t mm_read;
    long seed;
    int num_read=0;
    int num_write=0;
    long mmap_sum, read_sum;
    
    do {
	debug_init(DEBUG_INFO, 0, 0);
	
	ops = calloc((NOPS+1)*sizeof(*ops), 1);
	assertb(ops);

	seed = (long)(mstime() * 1000000);
	srand(seed);
	
	for(i=0; i<NOPS; i++) {
	    ops[i].off = randi(SIZE_MAX);
	    ops[i].len = 1 + randi(MIN(OP_READ_MAX, SIZE_MAX - ops[i].off) - 1);
	    ops[i].op = OP_READ + randi(1);
	    if( i==0 || ops[i].off + ops[i].len > maxlen ) {
		ops[i].op = OP_WRITE;
		maxlen = ops[i].off + ops[i].len;
	    }
	    switch(ops[i].op) {
	    case OP_READ: num_read++; break;
	    case OP_WRITE: num_write++; break;
	    default: assertbf(0, ("ERROR: bad op: %d", ops[i].op));
	    }
	}
	ops[i].op = OP_END;
    
	debugf("random read=%d write=%d op_read_max=%d\n",
	       num_read, num_write, OP_READ_MAX);

	snprintf(path_read, sizeof(path_read), "/tmp/t_readXXXXXX");
	fd_read = mkstemp(path_read);
	assertb_syserr(fd_read>=0);
	t1 = mstime();
	read_sum = test_read(fd_read, ops);
	dt = mstime() - t1;
	debugf("read: %0.6fs\n", dt);
	e = fstat(fd_read, &st);
	assertb_syserr(!e);
	assertb(st.st_size == maxlen);

	snprintf(path_mmap, sizeof(path_mmap), "/tmp/t_mmapXXXXXX");
	fd_mmap = mkstemp(path_mmap);
	assertb_syserr(fd_mmap>=0);
	t1 = mstime();
	mmap_sum = test_mmap(fd_mmap, ops);
	dt = mstime() - t1;
	debugf("mmap: %0.6fs\n", dt);
	e = fstat(fd_mmap, &st);
	assertb_syserr(!e);
	assertb(st.st_size == maxlen);

	assertb(mmap_sum == read_sum);
	debugf("op checksum ok: %lx\n", mmap_sum);
	
	e = mmap_fd(&mm_mmap, fd_mmap, MMAP_MODE_READ, 0);
	assertb(!e);
	e = mmap_fd(&mm_read, fd_read, MMAP_MODE_READ, 0);
	assertb(!e);
	e = memcmp(mmap_ptr(&mm_mmap, 0), mmap_ptr(&mm_read, 0), maxlen);
	assertb(!e);
	debugf("file cmp ok: %lu\n", maxlen);

	err = 0;
    } while(0);

    if( fd_read >= 0 ) {
	mmap_close(&mm_read);
	close(fd_read);
	unlink(path_read);
    }
    if( fd_mmap >= 0 ) {
	mmap_close(&mm_mmap);
	close(fd_mmap);
	unlink(path_mmap);
    }
	
    return err;
}
