#ifndef LLIST_H_INCLUDED
#define LLIST_H_INCLUDED

#include <stdlib.h>
#include <string.h>
#include "defutil.h"
#include "configbk.h"

typedef struct lnode_t lnode_t;
struct lnode_t {
    lnode_t *next;
    lnode_t *prev;
};

typedef struct llist_t llist_t;
struct llist_t {
    lnode_t head;
    lnode_t *tail;
};

#define NODE_TO_STRUCT(struct, member, node) \
  ((struct*)(((char*)node) - OFFSETOF(struct, member)))

static inline
void
llist_init(llist_t *list) {
    memset(list, 0, sizeof(*list));
}

static inline
lnode_t*
llist_head(llist_t *list) {
    return list->head.next;
}

static inline
lnode_t*
llist_tail(llist_t *list) {
    return list->tail;
}

static inline
void
llist_add_after(llist_t *list, lnode_t *prev, lnode_t *node) {
    if( !prev ) {
	prev = &list->head;
    }
    if( !list->tail || list->tail == prev ) {
	list->tail = node;
    }
    node->prev = prev;
    node->next = prev->next;
    if( prev->next ) {
	prev->next->prev = node;
    }
    prev->next = node;

    if( node->prev == &list->head ) {
	/* break link to sentinel */
	node->prev = 0;
    }
}

static inline
void
llist_insert(llist_t *list, lnode_t *node) {
    llist_add_after(list, &list->head, node);
}

static inline
void
llist_append(llist_t *list, lnode_t *node) {
    llist_add_after(list, list->tail, node);
}

static inline
void
llist_remove(llist_t *list, lnode_t *node) {
    if( node == list->tail ) {
	list->tail = node->prev;
    }
    if( list->head.next == node ) {
	/* rejoin link to sentinel */
	node->prev = &list->head;
    }
    if( node->prev ) node->prev->next = node->next;
    if( node->next ) node->next->prev = node->prev;
    if( list->head.next ) {
	/* break link to sentinel */
	list->head.next->prev = 0; 
    }
}

static inline
void
llist_to_tail(llist_t *list, lnode_t *node) {
    llist_remove(list, node);
    llist_append(list, node);
}

#endif // LLIST_H_INCLUDED
