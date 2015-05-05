#include "bklib/rand.h"
#include "bklib/array.h"
#include "bklib/debug.h"

int
array_cmp_int(const void *a, const void *b) {
    return *(int*)a - *(int*)b;
}

#define NELTS 100

void
test(array_t *a, int r) {
    int n;
    do {
	n = array_bsearch(a, array_cmp_int, &r);

	debug(DEBUG_INFO, 
	      ("array_search r=%d array[%d]=%d\n",
	       r, n, *(int*)array_get(a, n)));
	
	if( n>=0 ) {
	    assertb(*(int*)array_get(a, n) <= r);
	    if( n<NELTS-1 ) {
		assertb(*(int*)array_get(a, n+1) >= r);
	    }
	    if( n > 0 ) {
		assertb(*(int*)array_get(a, n-1) < r);
	    }
	}
    } while(0);
}

int
main() {
    array_t a;
    int i, e, r;

    debug_init(DEBUG_INFO, 0, 0);

    array_init(&a, sizeof(int), NELTS);

    /* make a sorted random array with elts 0..10 apart */
    e = 1;
    for(i=0; i<NELTS; i++) {
	e += rand_u32(10);
	*(int*)array_add(&a, 1) = e;

	debug(DEBUG_INFO, ("array[%d] = %d\n", i, e));
    }

    test(&a, 0);
    test(&a, 72);
    test(&a, 74);
    test(&a, e+10);    
    test(&a, e);

    /* search for elts in array.  Make sure found is always <=
       search. */
    for(i=0; i<NELTS; i++) {
	r = rand_u32(e);
	test(&a, r);
    }

    return 0;
}
