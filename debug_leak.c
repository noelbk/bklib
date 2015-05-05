#include <stdint.h>

#include "bklib/debug_leak.h"
#include "bklib/debug.h"
#include "bklib/array.h"
#include "bklib/hash.h"

static hash_t debug_leak_hash;
static int debug_leak_init_done = 0;
int debug_leak_counter = 0;

/*------------------------------------------------------------------*/
typedef struct debug_leak_stack_rec_t {
    char *file;
    int line;
} debug_leak_stack_rec_t;

typedef struct debug_leak_rec_t {
    void *ptr;
    int counter;
    array_t m_stack;
} debug_leak_rec_t;

void
debug_leak_rec_init(debug_leak_rec_t *rec, void *ptr) {
    array_init(&rec->m_stack, sizeof(debug_leak_stack_rec_t), 1);
    rec->ptr = ptr;
    rec->counter = ++debug_leak_counter;
}

void
debug_leak_rec_free(debug_leak_rec_t *rec) {
    array_clear(&rec->m_stack);
}

void
debug_leak_rec_dump(debug_leak_rec_t *rec) {
    int i;
    debug_leak_stack_rec_t *loc;
    
    for(i=array_count(&rec->m_stack)-1; i>=0; i--) {
	loc = (debug_leak_stack_rec_t*)array_get(&rec->m_stack, i);
	debug_hdr(DEBUG_WARN, loc->file, loc->line);
	debugf("debug_leak_dump: ptr=%p counter=%d\n", rec->ptr, rec->counter);
    }
}

/*------------------------------------------------------------------*/
int
debug_leak_init_f() {
    if( debug_leak_init_done ) return 0;
    debug_leak_init_done = 1;
    hash_init(&debug_leak_hash, hash_hash_int, hash_cmp_int, 0, sizeof(debug_leak_rec_t), 0);
    return 0;
}

int
foreach_debug_leak_rec_free(hash_t *hash, hash_node_t *node, void *arg) {
    debug_leak_rec_t *rec = (debug_leak_rec_t *)node->node_val;
    debug_leak_rec_free(rec);
    return 0;
}


void
debug_leak_fini_f() {
    if( !debug_leak_init_done ) return;
    hash_foreach(&debug_leak_hash, foreach_debug_leak_rec_free, 0);
    hash_clear(&debug_leak_hash);
    debug_leak_init_done = 0;
}

void
debug_leak_create_f(void *ptr, char *file, int line) {
    hash_node_t *node;
    debug_leak_rec_t *rec;

    debug_leak_init_f();
    node = hash_get(&debug_leak_hash, ptr);
    if( node ) {
	rec = (debug_leak_rec_t*)node->node_val;

	debug_hdr(DEBUG_WARN, file, line);
	debugf("debug_leak: ptr=%p allocated twice\n", ptr);
	debug_leak_rec_dump(rec);
	debug_leak_rec_free(rec);
	hash_free(&debug_leak_hash, node);
    }    
    node = hash_put(&debug_leak_hash, ptr, 0);
    rec = (debug_leak_rec_t*)node->node_val;
    debug_leak_rec_init(rec, ptr);
    
    debug_leak_stack_f(ptr, file, line);
}

void
debug_leak_stack_f(void *ptr, char *file, int line) {
    hash_node_t *node;
    debug_leak_rec_t *rec;
    debug_leak_stack_rec_t *loc;
    
    node = hash_get(&debug_leak_hash, ptr);
    if( !node ) {
	debug_hdr(DEBUG_WARN, file, line);
	debugf("debug_leak_stack: unknown ptr=%p stacked\n", ptr);
    }
    else {
	rec = (debug_leak_rec_t*)node->node_val;
	loc = (debug_leak_stack_rec_t*)array_add(&rec->m_stack, 1);
	loc->file = file;
	loc->line = line;
    }
}

void
debug_leak_delete_f(void *ptr, char *file, int line) {
    hash_node_t *node;
    debug_leak_rec_t *rec;

    node = hash_get(&debug_leak_hash, ptr);
    if( !node ) {
	debug_hdr(DEBUG_WARN, file, line);
	debugf("debug_leak_delete: unknown ptr=%p deleted\n", ptr);
    }
    else {
	rec = (debug_leak_rec_t*)node->node_val;
	debug_leak_rec_free(rec);
	hash_free(&debug_leak_hash, node);
    }
}

typedef struct debug_leak_by_line_t {
    char *file;
    int  line;
    int count;
} debug_leak_by_line_t;

hash_val_t
debug_leak_by_line_hash_hash(const void *a) {
    return 
	hash_hash_str(((debug_leak_by_line_t *)a)->file)
	+ hash_hash_int((void*)(intptr_t)((debug_leak_by_line_t *)a)->line)
	;
}

int
debug_leak_by_line_hash_cmp(const void *va, const void *vb) {
    debug_leak_by_line_t *a = (debug_leak_by_line_t *)va;
    debug_leak_by_line_t *b = (debug_leak_by_line_t *)vb;
    int i;
    i = hash_cmp_str(a->file, b->file);
    if( i == 0 ) {
	i = hash_cmp_int((void*)(intptr_t)a->line, (void*)(intptr_t)b->line);
    }
    return i;
}

int
debug_leak_by_line_cmp_count(const void *va, const void *vb) {
    debug_leak_by_line_t *a = *(debug_leak_by_line_t **)va;
    debug_leak_by_line_t *b = *(debug_leak_by_line_t **)vb;
    int i;
    i = b->count - a->count;
    if( i==0 ) {
	i = hash_cmp_str(a->file, b->file);
    }
    return i;
}

/* count allocated pointers by file:line in a hash table */
int
debug_leak_by_line_foreach_hash(hash_t *hash, hash_node_t *node, void *arg) {
    hash_t *by_line_hash = (hash_t*)arg;
    debug_leak_rec_t *rec = (debug_leak_rec_t*)node->node_val;
    debug_leak_by_line_t by_line_rec, *by_line_ptr = &by_line_rec;
    debug_leak_stack_rec_t *loc;

    do {
	loc = (debug_leak_stack_rec_t*)array_get(&rec->m_stack, 0);
	assertb(loc);
	by_line_rec.file = loc->file;
	by_line_rec.line = loc->line;
	by_line_rec.count = 0;
	node = hash_get(by_line_hash, &by_line_rec);
	if( !node ) {
	    node = hash_put(by_line_hash, &by_line_rec, &by_line_rec);
	}
	assertb(node);
	by_line_ptr = (debug_leak_by_line_t*)node->node_val;
	by_line_ptr->count++;
    } while(0);

    return 0;
}

/* put file:line recs into an array */
int
debug_leak_by_line_foreach_array(hash_t *hash, hash_node_t *node, void *arg) {
    array_t *by_line_array = (array_t *)arg;
    debug_leak_by_line_t *by_line_ptr = (debug_leak_by_line_t*)node->node_val;
    *(debug_leak_by_line_t**)array_add(by_line_array, 1) = by_line_ptr;
    return 0;
}

/* print the number of allocated pointers by file:line */
void
debug_leak_by_line_f() {
    hash_t by_line_hash;
    array_t by_line_sort;
    int i;

    hash_init(&by_line_hash, 
	      debug_leak_by_line_hash_hash, debug_leak_by_line_hash_cmp, 
	      sizeof(debug_leak_by_line_t), sizeof(debug_leak_by_line_t),
	      0);
    array_init(&by_line_sort, sizeof(debug_leak_by_line_t*), 0);
    do {
	/* collect all debug_leak record file file:line into a hash */
	hash_foreach(&debug_leak_hash, 
		     debug_leak_by_line_foreach_hash, &by_line_hash);

	/* sort file:line recs by count */
	hash_foreach(&by_line_hash, 
		     debug_leak_by_line_foreach_array, &by_line_sort);
	array_qsort(&by_line_sort, debug_leak_by_line_cmp_count);

	/* print nonzero file:line recs */
	debug(debug_level, ("debug_leak_by_line begin\n"));
	for(i=0; i<array_count(&by_line_sort); i++) {
	    debug_leak_by_line_t *by_line_ptr
		= *(debug_leak_by_line_t**)array_get(&by_line_sort, i);
	    debugf("  debug_leak_by_line: %s:%d count=%d\n", 
		   by_line_ptr->file, by_line_ptr->line, by_line_ptr->count);
	
	}
	debug(debug_level, ("debug_leak_by_line end\n"));
	
    } while(0);

    array_clear(&by_line_sort);
    hash_clear(&by_line_hash);
}


int
debug_leak_dump_foreach(hash_t *hash, hash_node_t *node, void *arg) {
    array_t *sort = (array_t*)arg;
    debug_leak_rec_t *rec = (debug_leak_rec_t*)node->node_val;
    *(debug_leak_rec_t **)array_add(sort, 1) = rec;
    return 0;
}

int
debug_leak_rec_cmp_counter(const void *ai, const void *bi) {
    debug_leak_rec_t **a = (debug_leak_rec_t**)ai;
    debug_leak_rec_t **b = (debug_leak_rec_t**)bi;
    return (*a)->counter - (*b)->counter;
}

void
debug_leak_dump_f() {
    array_t sort;
    int i;

    /* collect all records in an array and sort by rec->counter */
    array_init(&sort, sizeof(debug_leak_rec_t*), 0);
    hash_foreach(&debug_leak_hash, debug_leak_dump_foreach, &sort);
    array_qsort(&sort, debug_leak_rec_cmp_counter);

    for(i=0; i<array_count(&sort); i++) {
	debug_leak_rec_dump(*(debug_leak_rec_t**)array_get(&sort, i));
    }
    array_clear(&sort);
}

