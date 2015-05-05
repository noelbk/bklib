#include <stdio.h>
#include "llist.h"

struct t {
    int i;
    lnode_t asc;
    lnode_t desc;
};

#define N 16

void
llist_print(llist_t *list, int struct_offset) {
    int i;
    struct t *pt;
    lnode_t *node;

    printf("-> ");
    for(i=0, node = llist_head(list); node; node=node->next) {
	pt = (struct t*)(((char*)node) - struct_offset);
	printf("%s%d", (i++ ? ", " : ""), pt->i);
    }
    printf("\n");
    printf("<- ");
    for(i=0, node = llist_tail(list); node; node=node->prev) {
	pt = (struct t*)(((char*)node) - struct_offset);
	printf("%s%d", (i++ ? ", " : ""), pt->i);
    }
    printf("\n");
}

int 
main() {
    struct t t[N];
    llist_t asc, desc;
    int i;

    llist_init(&asc);
    llist_init(&desc);

    for(i=0; i<N; i++) {
	t[i].i = i;
	llist_append(&asc, &t[i].asc);
	llist_insert(&desc, &t[i].desc);
    }
    llist_print(&asc, OFFSETOF(struct t, asc));
    llist_print(&desc, OFFSETOF(struct t, desc));

    for(i=0; i<N; i+=2) {
	llist_remove(&asc, &t[i].asc);
	llist_remove(&desc, &t[i].desc);
    }
    llist_print(&asc, OFFSETOF(struct t, asc));
    llist_print(&desc, OFFSETOF(struct t, desc));

    for(i=1; i<N; i+=2) {
	llist_remove(&asc, &t[i].asc);
	llist_remove(&desc, &t[i].desc);
    }
    llist_print(&asc, OFFSETOF(struct t, asc));
    llist_print(&desc, OFFSETOF(struct t, desc));
}


