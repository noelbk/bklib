//--------------------------------------------------------------------
// pool_t.cpp - test pool.cpp
// Noel Burton-Krahn
// Jan 24, 2002
//
// Copyright 2002 Burton-Krahn, Inc.
//

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "pool.h"
#include "pool.h" // include twice to test header #ifdefs
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
    int should_free, is_free;
} elt_t;

void
elt_free(void *velt, void *arg) {
    ((elt_t*)velt)->is_free = 1;
}

int
check(elt_t *e, int n, char *msg) {
    int i, err;

    printf("%s: ", msg);
    err = 0;
    for(i=0; i<n; i++, e++) {
	if( e->should_free == e->is_free ) {
	    continue;
	}
	
	printf("error at %d: should_free=%d is_free=%d\n", 
	       i, e->should_free, e->is_free);
	err = -1;
    }
    if( !err ) {
	printf("ok");
    }
    printf("\n");
    return err;
}

int
main(int argc, char **argv) {
    pool_t *pool;
    int i, n, *p;
    elt_t *e, *elts;
    int nelts = 100;

    elts = (elt_t*)calloc(nelts*sizeof(*elts), 1);
    
    pool = pool_new();
    for(i=0, e=elts; i<nelts; i++, e++) {
	e->is_free = 0;
	e->should_free = 0;
	pool_add(pool, e, elt_free, 0);
    }
    check(elts, nelts, "add");
    
    for(i=0, e=elts+6; i<7; i++, e++) {
	e->should_free = 1;
	pool_free(pool, e);
    }
    check(elts, nelts, "free");
    
    for(i=0, e=elts+14; i<7; i++, e++) {
	e->should_free = 0;
	e->is_free = 0;
	pool_remove(pool, e);
	pool_free(pool, e);
    }
    check(elts, nelts, "remove");
    for(i=0, e=elts+14; i<7; i++, e++) {
	e->is_free = 1;
    }

    for(i=0, e=elts; i<nelts; i++, e++) {
	if( e->is_free ) {
	    e->should_free = 0;
	    e->is_free = 0;
	}
	else {
	    e->should_free = 1;
	}
    }
    pool_delete(pool);
    check(elts, nelts, "clear");

    pool = pool_new();

    n = 4096*16;
    p = (int*)pool_malloc(pool, n*sizeof(*p));
    p[n-1] = 1;
    printf("malloc: ok\n");

    n *= 2;
    p = (int*)pool_realloc(pool, p, n*sizeof(*p));
    p[n-1] = 1;
    printf("realloc: ok\n");
    pool_free(pool, p);

    pool_delete(pool);
    return 0;
}


