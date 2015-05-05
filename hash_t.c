// hash_t.c - test hash.c
// Noel Burton-Krahn
// Jan 26, 2002
//
// Copyright 2002 Burton-Krahn, Inc
//

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bklib/hash.h"
#include "bklib/debug.h"

typedef struct _elt {
    int in_hash;
    int got_foreach;
} elt;


int
verify_foreach(hash_t *hash, hash_node_t *node, void *varg) {
    elt *elts = (elt*)varg, 
	*p = (elt*)node->node_val;
    
    if( p->got_foreach ) {
	printf("error in foreach: elt enumerated twice"
	       " at i=%d, key=%p, val=%p got_foreach=%d\n",
	       (int)(p - elts), node->node_key, node->node_val, 
	       p->got_foreach);
    }
    p->got_foreach++;
    return 0;
}


int
verify(hash_t *hash, elt *elts, int nelts, char *msg) {
    int i, err;
    hash_node_t *node=0, tmp;
    elt *p;

    memset(&tmp, 0, sizeof(tmp));

    printf("%s: ", msg);

    err = 0;
    for(i=0, p=elts; i<nelts; i++, p++) {
	p->got_foreach = 0;
	node = hash_get(hash, p);
	if( !p->in_hash && node ) {
	    printf("error: p found in hash"
		   " at %d: p=%p, in_hash=%d, key=%p val=%p",
		   i, p, p->in_hash, node->node_key, node->node_val);
	    err = 1;
	}
	if( p->in_hash && (!node || node->node_val != p) ) {
	    node = &tmp;
	    err = 1;
	    printf("error: p not found in hash"
		   " at %d: p=%p, in_hash=%d, key=%p val=%p\n",
		   i, p, p->in_hash, node->node_key, node->node_val);
	}
    }
    
    hash_foreach(hash, verify_foreach, elts);
    for(i=0, p=elts; i<nelts; i++, p++) {
	if( p->in_hash && !p->got_foreach ) {
	    printf("error in foreach: elt missed"
		   " at i=%d, key=%p, val=%p, got_foreach=%d\n",
		   i, node->node_key, node->node_val, 
		   p->got_foreach);
	    err = 1;
	}
    }

    if( !err ) {
	hash_stat_t st;
	i = hash_stat(hash, &st);
	printf("ok\n  count=%d buckets=%d used=%d(%0.1f%%) max=%d avg=%.1f",
	       hash_count(hash), st.count, st.used, 
	       (float)100.0*st.used/st.count, 
	       st.max, st.avg);
    }
    printf("\n");

    return err;
}

#define KEY_MAX 20+1
#define VAL_MAX 20+1
void
test_string(int nelts) {
    hash_t h;
    hash_node_t *node;
    char key[KEY_MAX], val[VAL_MAX];
    int i, j; //, t;

    hash_init(&h, hash_hash_str, hash_cmp_str, KEY_MAX, VAL_MAX, 1);
    for(i=0; i<nelts; i++) {
	sprintf(key, "%dk%d", i, i);
	sprintf(val, "%dv%d", i, i);
	node = hash_put(&h, key, val);

	//t = 1;
	for(j=0; j<hash_count(&h); j++) {
	    sprintf(key, "%dk%d", j, j);
	    sprintf(val, "%dv%d", j, j);
	    node = hash_get(&h, key);
	    if( !node ) {
		fprintf(stderr, "error: i=%d j=%d key=%s not found\n", 
			i, j, key);
	    }
	    else if( strcmp(key, node->node_key)
		     || strcmp(val, node->node_val) ) {
		fprintf(stderr, "error: key=%s val=%s node=[%s %s]\n",
			key, val, (char*)node->node_key, (char*)node->node_val);
		//t = 0;
	    }
	}
    }
}

int 
main(int argc, char **argv) {
    hash_t h;
    elt *elts, *p;
    int nelts = 0;
    char *c;
    int i;

    do {
	hash_zero(&h);

	if( argc>1 ) {
	    nelts = strtoul(argv[1], &c, 0);
	}
	if( nelts == 0 ) {
	    nelts = 1000;
	}
	elts = (elt*)calloc(nelts*sizeof(*elts),1);

	test_string(nelts);

	hash_init(&h, hash_hash_int, hash_cmp_int, 0, 0, 1);

	// put everything in
	for(i=0, p=elts; i<nelts; i++, p++) {
	    p->in_hash = 1;
	    hash_put(&h, p, p);
	}
	verify(&h, elts, nelts, "put");
	verify(&h, elts, nelts, "put");
	
	// delete every even element
	for(i=0, p=elts; i<nelts; i+=2, p+=2) {
	    p->in_hash = 0;
	    hash_free(&h, hash_get(&h, p));
	}
	verify(&h, elts, nelts, "delete even");

	// delete every odd element
	for(i=1, p=elts+1; i<nelts; i+=2, p+=2) {
	    p->in_hash = 0;
	    hash_free(&h, hash_get(&h, p));
	}
	verify(&h, elts, nelts, "delete odd");

	// randomly insert and delete stuff
	for(i=0; i<3*nelts; i++) {
	    int j = (int)(1.0*nelts*rand()/RAND_MAX);
	    p = elts + j;
	    if( (int)(10.0*rand()/RAND_MAX)+1 > 1 ) {
		p->in_hash = 1;
		hash_put(&h, p, p);
	    }
	    else {
		p->in_hash = 0;
		hash_free(&h, hash_get(&h, p));
	    }
	}
	verify(&h, elts, nelts, "random insert/delete");
	hash_clear(&h);

	/* loop forever looking for memory leak */
	if( 0 ) {
	    hash_init(&h, hash_hash_int, hash_cmp_int, 0, 0, 1);
	    for(i=0; ;i++) {
		hash_node_t *node;
		hash_remove(&h, (void*)(intptr_t)(i-1));
		hash_put(&h, (void*)(intptr_t)i, (void*)(intptr_t)i);
		node = hash_get(&h, (void*)(intptr_t)(i-1));
		assertb(!node);
		node = hash_get(&h, (void*)(intptr_t)i);
		assertb(node);
		assertb((intptr_t)node->node_val == i);
	    }
	}

    } while(0);
    
    return 0;
}

