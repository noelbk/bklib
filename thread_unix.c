#include <pthread.h>
#include <stdlib.h>

#include "bklib/debug.h"
#include "bklib/proc.h"

struct thread_t {
    pthread_t pth;
    void *ret;
};


thread_t*
thread_new(thread_func_t func, void *farg) {
    thread_t *th;
    int i, err=-1;

    do {
	th = calloc(1, sizeof(*th));
	i = pthread_create(&th->pth, 0, func, farg);
	assertb_syserr(i==0);
	err = 0;
    } while(0);
    return err ? 0 : th;
}

int
thread_detach(thread_t *th) {
    pthread_detach(th->pth);
    free(th);
    return 0;
}

int
thread_join(thread_t *th, void **ret) {
    pthread_join(th->pth, &th->ret);
    *ret = th->ret;
    return 0;
}

threadid_t
proc_getthreadid() {
    return pthread_self();
}


