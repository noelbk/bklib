#include "alloc_count.h"

#include "bklib/hash.h"
#include "bklib/debug.h"

typedef struct alloc_count_info_t {
    void *ptr;
    size_t len;
    char *file;
    int line;
    int count;
} alloc_count_info_t;

hash_t alloc_count_hash;

int
alloc_count_init() {
    return hash_init(&alloc_count_hash, hash_hash_int, hash_cmp_int, 0, sizeof(alloc_count_info_t), 0);
}

int
alloc_count_fini() {
    return hash_clear(&alloc_count_hash);
}

int
alloc_count_add(void *ptr, size_t len, char *file, int line) {
    hash_node_t *node;
    int err=-1;
    node = hash_get(&alloc_count_hash, ptr);
    if( node ) {
	alloc_count_info_t *pinfo = (alloc_count_info_t*)node->node_val;

	debug(DEBUG_WARN,
	      ("alloc_count_add: ptr=%p len=%d alloced twice at %s:%d and %s:%d\n",
	       ptr, len, pinfo->file, pinfo->line, file, line));

	pinfo->count++;
    }
    else {
	alloc_count_info_t info;

	info.ptr = ptr;
	info.len = len;
	info.file = file;
	info.line = line;
	info.count = 1;

	hash_put(&alloc_count_hash, ptr, &info);
	err = 0;
    }
    return err;
}

int
alloc_count_free(void *ptr, char *file, int line) {
    hash_node_t *node;
    int err=-1;

    node = hash_get(&alloc_count_hash, ptr);
    if( !node ) {
	debug(DEBUG_WARN,
	      ("alloc_count_free: ptr=%p freed without alloc at %s:%d\n",
	       ptr, file, line));
    }
    else {
	hash_remove(&alloc_count_hash, ptr);
	err = 0;
    }
    return err;
}

int
alloc_count_dump_foreach(hash_t *hash, hash_node_t *node, void *arg) {
    alloc_count_info_t *info = (alloc_count_info_t*)node->node_val;
    debugf("alloc_count_dump: ptr=%p len=%d at %s:%d\n",
	   info->ptr, info->len, info->file, info->line);
    return 0;
}


int
alloc_count_dump() {
    return hash_foreach(&alloc_count_hash, alloc_count_dump_foreach, 0);
}


