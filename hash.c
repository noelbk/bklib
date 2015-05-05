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
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

#include "hash.h"
#include "warn.h"

int
hash_zero(hash_t *self) {
    memset(self, 0, sizeof(*self));
    return 0;
}

int
hash_init(hash_t *self, hash_func hash, hash_cmp_func cmp, 
	  int key_sz, int val_sz, int buckets) {
    hash_zero(self);
    self->m_hash = hash;
    self->m_cmp = cmp;
    self->m_key_sz = key_sz;
    self->m_val_sz = val_sz;
    self->m_node_sz = sizeof(hash_node_t) + key_sz + val_sz;
    if( buckets <= 0 ) {
	buckets = HASH_DEFAULT_BUCKETS;
    }
    return hash_resize(self, buckets);
}

int
hash_set_free(hash_t *self
	      ,hash_free_func_t free_key_func, void *free_key_farg
	      ,hash_free_func_t free_val_func, void *free_val_farg
	      ) {
    self->m_free_key_func = free_key_func;
    self->m_free_key_farg = free_key_farg;
    self->m_free_val_func = free_val_func;
    self->m_free_val_farg = free_val_farg;
    return 0;
}

void
hash_free_node(hash_t *self, hash_node_t *node) {
    if( self->m_free_key_func ) {
	self->m_free_key_func(node->node_key, self->m_free_key_farg);
    }
    if( self->m_free_val_func ) {
	self->m_free_val_func(node->node_val, self->m_free_val_farg);
    }
}

int
hash_foreach_free_node(hash_t *hash, hash_node_t *node, void *arg) {
    hash_free_node(hash, node);
    return 0;
}

int
hash_clear(hash_t *self) {
    int i;
    hash_foreach(self, hash_foreach_free_node, 0);
    if( self->m_bucket ) {
	for(i=0; i<self->m_bucket_count; i++) {
	    array_clear(&self->m_bucket[i]);
	}
	free(self->m_bucket);
    }
    memset(self, 0, sizeof(*self));
    return 0;
}

int
hash_count(hash_t *self) {
    return self->m_node_count;
}

int
hash_stat(hash_t *self, hash_stat_t *st) {
    int i, n, max;
    float avg;
    array_t *a;

    memset(st, 0, sizeof(*st));
    st->count = self->m_bucket_count;
    if( self->m_bucket_count < 1 ) {
	return 0;
    }
    
    max = 0;
    avg = 0;
    st->used = 0;
    max = array_count(&self->m_bucket[0]);
    for(i=0, a=self->m_bucket; i<self->m_bucket_count; i++, a++) {
	n = array_count(a);
	avg += n;
	if( n > max ) { 
	    max = n; 
	}
	if( n > 0 ) {
	    st->used++;
	}
    }
    st->max = max;
    if( st->used > 0 ) {
	st->avg = avg / st->used;
    }
    return 0;
}

hash_node_t*
hash_put(hash_t *self, void *key, void *val) {
    hash_node_t *node;
    hash_val_t h;
    int i;

    hash_free(self, hash_get(self, key));

    if( self->m_node_count > 3*self->m_bucket_count ) {
	i = hash_resize_next(self, self->m_bucket_count);
	hash_resize(self, i);
    }

    h = (*self->m_hash)(key);
    i = h % self->m_bucket_count;
    node = (hash_node_t*)array_add(&self->m_bucket[i], 1);
    node->node_hash = h;
    if( self->m_key_sz ) {
	node->node_key = (char*)node + sizeof(hash_node_t);
	memcpy(node->node_key, key, self->m_key_sz);
    }
    else {
	node->node_key = key;
    }

    if( self->m_val_sz ) {
	node->node_val = (char*)node + sizeof(hash_node_t) + self->m_key_sz;
	if( val ) {
	    memcpy(node->node_val, val, self->m_val_sz);
	}
    }
    else {
	node->node_val = val;
    }
    self->m_node_count++;
    return node;
}

hash_node_t*
hash_get(hash_t *self, void *key) {
    hash_node_t *node=0;
    array_t *array;
    hash_val_t h;
    int i;

    h = (*self->m_hash)(key);
    i = h % self->m_bucket_count;
    array = &self->m_bucket[i];
    for(node=(hash_node_t*)array_get(array, -1), i=array_count(array)-1; 
	i>=0; 
	i--, node = (hash_node_t*)((char*)node - self->m_node_sz)) {

	if( h != node->node_hash ) continue;
	if( self->m_key_sz ) {
	    node->node_key = (char*)node + sizeof(hash_node_t);
	}
	if( (*self->m_cmp)(key, node->node_key)==0 ) {
	    if( self->m_val_sz ) {
		node->node_val = 
		    (char*)node + sizeof(hash_node_t) + self->m_key_sz;
	    }
	    return node;
	}
    }
    return 0;
}

void
hash_free(hash_t *self, hash_node_t *node) {
    if( !node ) {
	return;
    }
    
    hash_free_node(self, node);

    array_remove_ptr(&self->m_bucket[node->node_hash % self->m_bucket_count],
		     node);
    self->m_node_count--;
}

int
hash_foreach(hash_t *self, hash_foreach_func func, void *arg) {
    int i, j, n;
    hash_node_t *node;
    array_t *bucket;

    for(i=0, bucket=self->m_bucket; i<self->m_bucket_count; i++, bucket++) {
	for(node=(hash_node_t*)array_get(bucket, -1), j=array_count(bucket)-1; 
	    j>=0; 
	    j--, node=(hash_node_t*)((char*)node - self->m_node_sz)) {
	    
	    if( self->m_key_sz ) {
		node->node_key = (char*)node + sizeof(hash_node_t);
	    }
	    if( self->m_val_sz ) {
		node->node_val = 
		    (char*)node + sizeof(hash_node_t) + self->m_key_sz;
	    }
	    n = (*func)(self, node, arg);
	    if( n ) { return n; }
	}
    }
    return 0;
}

int
hash_resize_next(hash_t *self, int buckets) {
    int i;

    /* choose the next bigger 2**n+1 */
    for(i=1; i && i<=buckets; i<<=1);
    if( !i ) i=buckets;
    return i+1;
}

int
hash_resize(hash_t *self, int buckets) {
    array_t* old_bucket = self->m_bucket;
    int    old_bucket_count = self->m_bucket_count;
    hash_node_t *old_node, *new_node;
    int i, j;
    
    self->m_bucket_count = buckets;
    self->m_bucket = (array_t*)malloc(self->m_bucket_count * sizeof(array_t));
    
    for(i=0; i<self->m_bucket_count; i++) {
	array_init(&self->m_bucket[i], self->m_node_sz, 1);
    }

    if( !old_bucket ) {
	return 0;
    }

    for(i=0; i<old_bucket_count; i++) {
	for(old_node = (hash_node_t*)array_get(&old_bucket[i], -1), 
		j=array_count(&old_bucket[i])-1; 
	    j >= 0; 
	    j--, old_node=(hash_node_t*)((char*)old_node-self->m_node_sz)) {
	    
	    new_node = (hash_node_t*)
		array_add(&self->m_bucket[old_node->node_hash % self->m_bucket_count], 1);
	    memcpy(new_node, old_node, self->m_node_sz);
	}
	array_clear(&old_bucket[i]);
    }
    free(old_bucket);

    return 0;
}

//---------------------------------------------------------------------
//from http://www.ddj.com/articles/1996/9604/9604b/9604b.htm#014e_0073
unsigned long
hash_str_elf_nocase(const char *q, unsigned long n, unsigned long h, int nocase)
{
    const unsigned char *p = (const unsigned char *)q;
    unsigned long  g;
    while( n-- > 0 ) {
	unsigned char c;
	c = *p++;
	if( nocase ) c = tolower(c);
	h = ( h << 4 ) + c;
	if ( (g = h & 0xF0000000) )
	    h ^= g >> 24;
	h &= ~g;
    }
    return h;
}

unsigned long
hash_str_elf(const char *q, unsigned long n, unsigned long h) {
    return hash_str_elf_nocase(q, n, h, 0);
}

   

hash_val_t
hash_hash_int(const void *key) {
    return (hash_val_t)key;
}

int
hash_cmp_int(const void *key1, const void *key2) {
    return (intptr_t)key1 - (intptr_t)key2;
}

hash_val_t
hash_hash_str(const void *key) {
    return (hash_val_t)hash_str_elf((char*)key, strlen((char*)key), 0);
}

int
hash_cmp_str(const void *key1, const void *key2) {
    return strcmp((char*)key1, (char*)key2);
}

hash_val_t
hash_hash_str_nocase(const void *key) {
    return (hash_val_t)hash_str_elf_nocase((char*)key, strlen((char*)key), 0, 1);
}

int
hash_cmp_str_nocase(const void *key1, const void *key2) {
    return strcasecmp((char*)key1, (char*)key2);
}

void
hash_remove(hash_t *self, void *key) { 
    hash_free(self, hash_get(self, key)); 
}

// to be called by pool_free();
void
pool_free_hash(void *hash, void *arg) {
    hash_clear((hash_t*)hash);
}


/* frees a malloc'ed pointer: free(ptr) */
void
hash_free_malloc(void *ptr, void *farg) {
    free(ptr);
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

