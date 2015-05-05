//--------------------------------------------------------------------
// pool.cpp - a pool to hold pointers which must be freed
// Noel Burton-Krahn
// Jan 24, 2002
//
// Copyright 2002 Burton-Krahn, Inc.
//
// see pool.h
//

#include <stdlib.h>
#include <string.h>
#include "pool.h"
#include "hash.h"
#include "debug.h"

struct pool_s {
    hash_t hash;
};

typedef struct {
    void *ptr;
    int  refcount;
    pool_free_func_t free;
    void *arg;
} pool_node_t;

pool_t*
pool_new() {
    pool_t *pool=0;
    pool = (pool_t*)malloc(sizeof(*pool));
    memset(pool, 0, sizeof(*pool));
    hash_init(&pool->hash, hash_hash_int, hash_cmp_int, 0, sizeof(pool_node_t), 0);
    return pool;
}

static int
pool_foreach_free(hash_t *hash, hash_node_t *node, void *arg) {
    pool_node_t *p = (pool_node_t*)node->node_val;
    if( p->free ) {
	(*p->free)(p->ptr, p->arg);
    }
    else {
	free(p->ptr);
    }
    return 0;
}

void
pool_delete(pool_t *pool) {
    hash_foreach(&pool->hash, pool_foreach_free, pool);
    hash_clear(&pool->hash);
    free(pool);
}

void
pool_clear(pool_t *pool) {
    hash_foreach(&pool->hash, pool_foreach_free, pool);
    hash_clear(&pool->hash);
    hash_init(&pool->hash, hash_hash_int, hash_cmp_int, 0, sizeof(pool_node_t), 0);
}

pool_node_t*
pool_find(pool_t *pool, void *ptr) {
    hash_node_t *node;
    node = hash_get(&pool->hash, ptr);
    return node ? (pool_node_t *)node->node_val : 0;
}

void
pool_remove(pool_t *pool, void *ptr) {
    hash_remove(&pool->hash, ptr);
}

int
pool_addref(pool_t *pool, void *ptr) {
    pool_node_t *p;

    if( !ptr ) return 0;
    p = pool_find(pool, ptr);
    if( p ) {
	p->refcount++;
    }
    return p ? p->refcount : 0;
}

void
pool_free(pool_t *pool, void *ptr) {
    pool_node_t *p;

    if( !ptr ) return;
    p = pool_find(pool, ptr);
    if( p ) {
	p->refcount--;
	if(p->refcount <= 0 ) { 
	    if( p->free ) {
		(*p->free)(p->ptr, p->arg);
	    }
	    else {
		free(p->ptr);
	    }
	    pool_remove(pool, ptr);
	}
    }
}

void*
pool_add(pool_t *pool, void *ptr, pool_free_func_t free, void *arg) {
    pool_node_t *p;
    hash_node_t *node;

    pool_free(pool, ptr);
    node = hash_put(&pool->hash, ptr, 0);
    p = (pool_node_t*)node->node_val;
    p->ptr = ptr;
    p->free = free;
    p->arg = arg;
    p->refcount = 1;
    return ptr;
}

void*
pool_malloc(pool_t *pool, size_t len) {
    return pool_add(pool, calloc(len, 1), 0, 0);
}

char*
pool_strdup(pool_t *pool, const char *str) {
    if( !str ) return 0;
    return (char*)pool_add(pool, strdup(str), 0, 0);
}

char *
pool_memdup(pool_t *pool, const char *str, int len) {
    char *p=0, *q=0;
    int err=-1;
    do {
	assertb(str);
	p = (char*)malloc(len);
	assertb(p);
	memcpy(p, str, len);
	q = (char*)pool_add(pool, p, 0, 0);
	err = 0;
    } while(0);
    if( err ) {
	if( q ) {
	    pool_free(pool, q);
	}
	else if ( p ) {
	    free(p);
	}
	p = q = 0;
    }
    return q;
}


void*
pool_realloc(pool_t *pool, void *ptr, size_t len) {
    pool_remove(pool, ptr);
    ptr = realloc(ptr, len);
    return pool_add(pool, ptr, 0, 0);
}


char*
pool_copy_str(pool_t *pool, char **dst, const char *str) {
    char *copy;
    copy = pool_strdup(pool, str);
    pool_free(pool, *dst);
    *dst = copy;
    return *dst;
}

// does pool_free(*dst), then *dst = malloc(len)
void*
pool_copy_buf(pool_t *pool, void **dst, const void *buf, size_t len) {
    pool_free(pool, *dst);
    *dst = pool_malloc(pool, len);
    memcpy(*dst, buf, len);
    return *dst;
}
