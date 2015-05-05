#include "bklib/alloc_count.h"
#include "bklib/debug.h"

#include <stdlib.h>

int main() {
    void *p;
    
    debug_init(DEBUG_INFO, 0, 0);
    alloc_count_init();
    
    p = malloc(10);
    ALLOC_COUNT_ADD(p, 10);
    free(p);
    ALLOC_COUNT_FREE(p);

    p = malloc(10);
    ALLOC_COUNT_ADD(p, 10);

    p = malloc(10);
    ALLOC_COUNT_ADD(p, 10);
    ALLOC_COUNT_ADD(p, 10);

    p = (void*)1;
    ALLOC_COUNT_FREE(p);

    alloc_count_dump();
    alloc_count_fini();

    return 0;
}

