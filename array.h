/**
  \file array.h
  \brief A contiguous array of fixed-length elements, good for small lists.
  \author Noel Burton-Krahn <noel@burton-krahn.com>
  \date Jan 24, 2002


  \note Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// default amount to gropw the array
#define ARRAY_DEFAULT_BLOCKSZ 10

typedef int    (*array_cmp_func)(const void *key_find, const void *key_hash);

typedef struct array_s array_t;

// called to free the whole array
typedef void (*array_free_func_t)(array_t *array, void *farg);

struct array_s {
    int m_eltsz, m_blocksz;
    char *m_array;
    int m_count, m_max;
    array_free_func_t m_free_func;
    void *m_free_farg;
};

/** Initialize an uninitialized array 
    \param self an uninitialized array
    \param eltsz the size of each element (>0)
    \param blocksz the number of elements to allocate when growing the
           array
    \return 0 if successful
 */
int array_init(array_t *self, int eltsz, int blocksz);

int array_set_free(array_t *self, array_free_func_t func, void *farg);

/* don't free the array.  Usually used with array_import for static arrays */
void
array_free_noop(array_t *array, void *farg);

/** Import an existing C array into an uninitialized array 
    \param self an uninitialized array
    \param eltsz the size of each element (>0)
    \param blocksz the number of elements to allocate when growing the
           array (can be zero)
    \param array a pointer to a C array
    \param nelts number of elements in C array

    \return 0 if successful
    
    If the array is modified (with array_add, etc), then the imported
    C array must have been allocated with malloc(), because it will be
    fed through realloc.
 */
int array_import(array_t *self
		 , void *array, int nelts
		 , int eltsz, int blocksz
		 , array_free_func_t free_func
		 , void *free_farg
		 );

/** Import an existing C array into an uninitialized array 
    \param self an initialized array

    \return the internal C array
    
    Extract the internal C array, and clear self.  Call array_count()
    before this.  The returned array will usually be malloced, unless
    it was imported with array_import.  It is safe to call
    array_clear() after this.
 */
void *array_extract(array_t *self);

/** Free all allocated elements in an array
    \param self an initialized array
    \return void
 */
void array_clear(array_t *self);

/** Make room for len elements at end of array.
    \param self an initialized array
    \param len the number of elements to add
    \return a pointer to the first added element if successful, or 0.
    \note uses realloc()
 */
void *array_add(array_t *self, int len);

/** Append elt to array if it doesn't already exist.  Uses array_find
    \param array an initialized array
    \param cmp a comparison function
    \param elt the element to add if !array_find(array, cmp, elt);
*/
int
array_add_nodup(array_t *array, array_cmp_func cmp, const void *elt);

/** remove some elements starting at idx
    \param self an initialized array
    \param idx the 0 based index to return.  Can be negative to index
      backwards from end.
    \param len the number of elements to remove
    \return void
    \note uses memmove to shift remaining elements right.
 */
void array_remove_idx(array_t *self, int idx, int len);

/** remove some elements starting at ptr
    \param self an initialized array
    \param ptr a pointer to the first element to remove, from array_get().
    \return void
    \note uses memmove to shift remaining elements right.
 */
void array_remove_ptr(array_t *self, void *ptr);

int array_index(array_t *self, void *ptr);

/** return the size of each element in the array
    \param self an initialized array
    \return the size of each element
 */
int array_eltsz(array_t *self);

// to be called by pool_free() calls free(array) iff arg != 0
void pool_free_array(void *array, void *arg);

/* reinitialize dst, and copy src to it */
int array_copy(array_t *dst, array_t *src);

/* insert nelts elements into dst starting at idx */
void *array_insert(array_t *dst, int idx, void *ptr, int nelts);

/* copy nelts elements from ptr to end of dst dst */
void *array_append(array_t *dst, void *ptr, int nelts);

/* copy all elts from src to the end of dst  */
void *array_concat(array_t *dst, array_t *src);

/* return the first element in array for which cmp(needle, elt)==0, or -1 */
int array_find(array_t *array, array_cmp_func cmp, const void *needle);

/* return > 0 iff every element in array exists in superset */
int array_subset(array_t *array, array_t *superset, array_cmp_func cmp);

/* return the index of the last elt in array which is <= cmp_elt.
   array must be sorted */
int array_bsearch(array_t *array, array_cmp_func cmp, void *cmp_elt);

/* qsorts array in place */
int array_qsort(array_t *array, array_cmp_func cmp);

#include "bklib/array_inline.c"

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ARRAY_H_INCLUDED
