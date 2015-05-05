/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

/* 
=pod
=head1 NAME

hash.h - fetch values by keys

=head1 SYNOPSIS

    #include "hash.h"

    hash_t hash;
    hash_node_t *node;
    void *key, *val;

    hash_init(hash, my_hash_func, my_cmp_func, keysz, valsz, buckets);
    hash_put(hash, key, val);
    hash_foreach(hash, my_foreach_func, my_foreach_arg);
    node = hash_get(hash, key);
    hash_free(node);

=head1 AUTHOR

 Noel Burton-Krahn
 Jan 25, 2002
 Copyright 2002 Burton-Krahn, Inc.

=head1 SEE ALSO

 array.h pool.h

=cut
*/

#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED

#include "array.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define HASH_DEFAULT_BUCKETS 17

typedef unsigned long hash_val_t;

typedef hash_val_t (*hash_func)(const void *key);
typedef int    (*hash_cmp_func)(const void *key_find, const void *key_hash);

struct hash_node_s {
    void *node_key;
    void *node_val;
    hash_val_t node_hash;
};
typedef struct hash_node_s hash_node_t;

struct hash_stat_s {
    int count; // number of buckets
    int used;  // number of non-empty buckets
    int max;   // max number of elts/bucket
    float avg; // avg number of elts in non-empty buckets
}; 
typedef struct hash_stat_s hash_stat_t;

typedef array_free_func_t hash_free_func_t;

struct hash_s {
    array_t       *m_bucket;
    int            m_bucket_count;
    hash_cmp_func  m_cmp;
    hash_func      m_hash;
    int            m_node_count;
    int            m_key_sz, m_val_sz, m_node_sz;
    hash_free_func_t m_free_key_func;
    void*            m_free_key_farg;
    hash_free_func_t m_free_val_func;
    void*            m_free_val_farg;
};
typedef struct hash_s hash_t;

typedef int (*hash_foreach_func)(hash_t *hash, hash_node_t *node, void *arg);

unsigned long
hash_str_elf(const char *p, unsigned long len, unsigned long curr_hash);

unsigned long
hash_str_elf_nocase(const char *p, unsigned long len, unsigned long curr_hash, int nocase);

hash_val_t hash_hash_int(const void *key);
int    hash_cmp_int(const void *key_find, const void *key_hash);

hash_val_t hash_hash_str(const void *key);
int    hash_cmp_str(const void *key_find, const void *key_hash);

hash_val_t hash_hash_str_nocase(const void *key);
int    hash_cmp_str_nocase(const void *key_find, const void *key_hash);

// key_sz and val_sz are for copying fixed-length structs into the
// hash node itself.  It's useful for putting structs into a hash
// withough worrying about allocating copies
int    hash_zero(hash_t *self);
int    hash_init(hash_t *self, 
		 hash_func hash, hash_cmp_func cmp, 
		 int key_sz, int val_sz, int buckets);

int    hash_set_free(hash_t *self
		     ,hash_free_func_t free_key_func, void *free_key_farg
		     ,hash_free_func_t free_val_func, void *free_val_farg
		     );

int    hash_clear(hash_t *self);
int    hash_resize(hash_t *self, int buckets);

// should return a prime > cursize
int    hash_resize_next(hash_t *self, int cursize);
    
hash_node_t* hash_put(hash_t *self, void *key, void *val);
hash_node_t* hash_get(hash_t *self, void *key);
void      hash_remove(hash_t *self, void *key);

// you can call hash_free(hash, node) on a node returned from hash_get
void      hash_free(hash_t *self, hash_node_t *node);

// iterate over all hash elements.  hash_free(node) is ok as is
// hash_count and hash_stats.  All other hash functions are
// undefined. If func returns nonzero, exit the foreach loop and
// return func's value.
//
int       hash_foreach(hash_t *self, hash_foreach_func func, void *arg);

// return the number of elements
int       hash_count(hash_t *self);

int       hash_stat(hash_t *self, hash_stat_t *st);

// to be called by pool_free();
void pool_free_hash(void *hash, void *arg);

/* frees a malloc'ed pointer: free(ptr) */
void
hash_free_malloc(void *ptr, void *farg);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HASH_H_INCLUDED

/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

