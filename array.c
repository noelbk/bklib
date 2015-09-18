/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

#include <string.h>
#include <stdlib.h>
#include "array.h"
#include "warn.h"

int
array_init(array_t *self, int eltsz, int blocksz) {
    memset(self, 0, sizeof(*self));
    if( blocksz <= 0 ) {
	blocksz = ARRAY_DEFAULT_BLOCKSZ;
    }
    self->m_max = 0;
    self->m_eltsz = eltsz;
    self->m_blocksz = blocksz;
    return 0;
}

void
array_free_noop(array_t *array, void *farg) {
}

int
array_import(array_t *self
	     , void *array, int nelts
	     , int eltsz, int blocksz
	     , array_free_func_t free_func
	     , void *free_farg
	     ) {
    memset(self, 0, sizeof(*self));
    if( blocksz <= 0 ) {
	blocksz = ARRAY_DEFAULT_BLOCKSZ;
    }
    self->m_count = nelts;
    self->m_max = nelts;
    self->m_eltsz = eltsz;
    self->m_blocksz = blocksz;
    self->m_array = array;
    self->m_free_func = free_func;
    self->m_free_farg = free_farg;
    return 0;
}

void*
array_extract(array_t *self) {
    void *array = self->m_array;
    self->m_array = 0;
    self->m_max = 0;
    self->m_count = 0;
    self->m_free_func = 0;
    self->m_free_farg = 0;
    return array;
}

int
array_set_free(array_t *self, array_free_func_t func, void *farg) {
    self->m_free_func = func;
    self->m_free_farg = farg;
    return 0;
}


void
array_clear(array_t *self) {
    if( self->m_free_func ) {
	self->m_free_func(self, self->m_free_farg);
    }
    else {
	free(self->m_array);
    }
    self->m_array = 0;
    self->m_count = self->m_max = 0;
}

void*
array_add(array_t *self, int num) {
    int err=-1;
    char *p=0;

    do {
	assertb(self->m_eltsz>0);
	if( self->m_count+num <= self->m_max ) {
	    err = 0;
	    break;
	}
	self->m_max = self->m_count 
	    + (num > self->m_blocksz ? num : self->m_blocksz);
	self->m_array = (char*)realloc(self->m_array, 
				       self->m_max * self->m_eltsz);
	assertb_syserr(self->m_array);
	memset(self->m_array+(self->m_count * self->m_eltsz), 
	       0, (self->m_max-self->m_count)*self->m_eltsz);

	err = 0;
    } while(0);
    if( err ) {
	p = 0;
    }
    else if( num > 0 ) {
	p = self->m_array + (self->m_count * self->m_eltsz);
	memset(p, 0, (num * self->m_eltsz));
	self->m_count += num;
    }

    return p;
}

void
array_remove_ptr(array_t *self, void *ptr) {
    array_remove_idx(self, array_index(self, ptr), 1);
}

int
array_index(array_t *self, void *ptr) {
    return (int)((char*)ptr-(char*)self->m_array)/self->m_eltsz;
}

void
array_remove_idx(array_t *self, int i, int len) {
    char *p;
    if( i < 0 ) {
	i += self->m_count;
    }
    if( len <= 0 || i+len > self->m_count ) {
	len = self->m_count-i;
    }
    p = (char*)array_get(self, i);
    if( !p ) {
	return;
    }

    if( i < self->m_count-1 ) {
	memmove(p, p+(len*self->m_eltsz), 
		(self->m_count-i-len)*self->m_eltsz);
    }
    self->m_count -= len;
}

int
array_eltsz(array_t *self) { 
    return self->m_eltsz; 
}

// to be called by pool_free();
void
pool_free_array(void *array, void *arg) {
    array_clear((array_t*)array);
    if( arg ) {
	free(array);
    }
}


/* reinitialize dst, and copy src to it */
int
array_copy(array_t *dst, array_t *src) {
    int i, n, err=-1;
    do {
	i = array_init(dst, src->m_eltsz, src->m_blocksz);
	assertb(i==0);
	n = array_count(src);
	if( n > 0 ) {
	    array_append(dst, array_get(src, 0), n);
	}
	err = 0;
    } while(0);
    return err;
}

/* insert elts into dst starting at idx */
void*
array_insert(array_t *dst, int idx, void *elts, int nelts) {
    int err=-1;
    char *start=0, *end;

    do {
	if( nelts <=0 ) {
	    err = 0;
	    break;
	}

	end = array_add(dst, nelts);
	assertb(end);
	start = array_get(dst, idx);
	assertb(start);
	memmove(end, start, end-start);
	memcpy(start, elts, nelts * dst->m_eltsz);

	err = 0;
    } while(0);
    return err ? 0 : start;
}

/* add one elt to dst */
void*
array_append(array_t *dst, void *elt, int nelts) {
    int err=-1;
    char *p=0;

    do {
	if( nelts <=0 ) {
	    err = 0;
	    break;
	}
	p = array_add(dst, nelts);
	assertb(p);
	memcpy(p, elt, nelts * dst->m_eltsz);
	err = 0;
    } while(0);
    return err ? 0 : p;
}


/* copy all elts from src to the end of dst  */
void*
array_concat(array_t *dst, array_t *src) {
    int n, err=-1;
    char *p;
    do {
	assertb(dst->m_eltsz == src->m_eltsz);
	n = array_count(src);
	p = array_add(dst, n);
	assertb(p);
	memcpy(p, array_get(src, 0), n*src->m_eltsz);
	err = 0;
    } while(0);
    return err ? 0 : p;
}

int
array_find(array_t *array, array_cmp_func cmp, const void *needle) {
    int i;
    void *p;
    for(i=array_count(array)-1; i>=0; i--) {
	p = array_get(array, i);
	if( cmp(needle, p)==0 ) {
	    return i;
	}
    }
    return -1;
}

int
array_add_nodup(array_t *array, array_cmp_func cmp, const void *elt)
{
    int i, err=-1;
    void *p;
    
    do {
	i = array_find(array, cmp, elt);
	if( i >= 0 ) {
	    /* already there, don't add again */
	    err = i;
	    break;
	}
	p = array_add(array, 1);
	assertb(p);
	memcpy(p, elt, array->m_eltsz);
	err = 0;
    } while(0);
    return err;
}

int
array_subset(array_t *subset, array_t *superset, array_cmp_func cmp) {
    int i;
    for(i=array_count(subset)-1; i>=0; i--) {
	if( array_find(superset, cmp, array_get(subset, i)) < 0 ) {
	    return 0;
	}
    }
    return 1;
}

int
array_bsearch(array_t *array, array_cmp_func cmp, void *cmp_elt) {
    int a, b, m, i;

    a=0;
    b=array_count(array)-1;

    if( b<0 || cmp(cmp_elt, array_get(array, 0)) < 0 ) {
	return -1;
    }
    while(1) {
	m = (a+b)/2;
	if( a >= b ) {
	    break;
	}
	i = cmp(cmp_elt, array_get(array, m));
	if( i==0 ) {
	    a = m;
	    break;
	}
	if( i < 0 ) {
	    if( b == m ) {
		a = b;
		break;
	    }
	    b = m;
	}
	if( i > 0 ) {
	    if( a == m ) {
		if( cmp(cmp_elt, array_get(array, b)) >= 0 ) {
		    a = b;
		}
		break;
	    }
	    a = m;
	}
    }
    while(a>0 && cmp(cmp_elt, array_get(array, a-1))==0) {
	a--;
    }
    return a;
}

typedef int (*qsort_compare_t)(const void *, const void *);

int
array_qsort(array_t *array, array_cmp_func cmp) {
    qsort(array->m_array, array->m_count, array->m_eltsz, (qsort_compare_t)cmp);
    return 0;
}

/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

