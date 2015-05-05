#ifndef POOL_H_INCLUDED
#define POOL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <sys/types.h>

typedef struct pool_s pool_t;

void
pool_free_close(pool_t *pool, void *ptr, void *arg);

pool_t*
pool_new();

void
pool_delete(pool_t *pool);

void
pool_clear(pool_t *pool);

void*
pool_malloc(pool_t *pool, size_t len);

void*
pool_realloc(pool_t *pool, void *ptr, size_t len);

char *
pool_strdup(pool_t *pool, const char *str);

char *
pool_memdup(pool_t *pool, const char *str, int len);

// does pool_free(*dst), then *dst = strdup(str)
char*
pool_copy_str(pool_t *pool, char **dst, const char *str);

// does pool_free(*dst), then *dst = malloc(len)
void*
pool_copy_buf(pool_t *pool, void **dst, const void *buf, size_t len);

typedef void (*pool_free_func_t)(void *ptr, void *arg);

    /* if free_func is nonzero, calls free_func(ptr, arg), otherwise free(ptr). */
void*
pool_add(pool_t *pool, void *ptr, pool_free_func_t free, void *arg);

void
pool_remove(pool_t *pool, void *ptr);

int
pool_addref(pool_t *pool, void *ptr);

void
pool_free(pool_t *pool, void *ptr);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // POOL_H_INCLUDED
