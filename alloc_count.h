#ifndef ALLOC_COUNT_H_INCLUDED
#define ALLOC_COUNT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#include "hash.h"
#include "debug.h"

#define ALLOC_COUNT_ADD(ptr, len) alloc_count_add(ptr, len, __FILE__, __LINE__);
#define ALLOC_COUNT_FREE(ptr) alloc_count_free(ptr, __FILE__, __LINE__);

int
alloc_count_init();

int
alloc_count_fini();

int
alloc_count_add(void *ptr, size_t len, char *file, int line);

int
alloc_count_free(void *ptr, char *file, int line);

int
alloc_count_dump_foreach(hash_t *hash, hash_node_t *node, void *arg);

int
alloc_count_dump();

#ifdef __cplusplus
}
#endif

#endif /* ALLOC_COUNT_H_INCLUDED */
