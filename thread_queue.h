#ifndef THREAD_QUEUE_H_INCLUDED
#define  THREAD_QUEUE_H_INCLUDED

#include "bklib/fdselect.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct thread_queue_s *thread_queue_t;

thread_queue_t thread_queue_new(int maxlen, void *arg);

int thread_queue_delete(thread_queue_t queue);

typedef void (*thread_queue_free_func_t)(void *data);

int thread_queue_enqueue(thread_queue_t queue, void *data, thread_queue_free_func_t free_func);

thread_queue_elt_t thread_queue_dequeue(thread_queue_t queue);

int thread_queue_free(thread_queue_elt_t elt);

typedef void (*thread_queue_select_func_t)(thread_queue_t queue, int events, void *arg);

int thread_queue_select(thread_queue_t queue, fdselect_t *sel, int events, thread_queue_select_func_t func, void *arg);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // THREAD_QUEUE_H_INCLUDED
