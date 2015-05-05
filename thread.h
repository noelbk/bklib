#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct thread_t;
typedef struct thread_t thread_t;

typedef void * (*thread_func_t)(void *);

thread_t*
thread_new(thread_func_t func, void *arg);

int
thread_set_priority(thread_t *th, int priority);

int
thread_detach(thread_t*);

int
thread_join(thread_t*, void **ret);

struct mutex_t;
typedef struct mutex_t mutex_t;

mutex_t* mutex_new();

int mutex_delete(mutex_t*);

int mutex_lock(mutex_t*, int ms_timeout);

int mutex_unlock(mutex_t*);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // THREAD_H_INCLUDED
