#include "bkwin32.h"

#include "debug.h"

struct thread_t {
    int refcount;
    HANDLE hthread;
    DWORD tid;
    DWORD ret;
    thread_func_t func;
    void *farg;
};

static
void
thread_unref(thread_t *th) {
    if( --th->refcount == 0 ) {
	free(th);
    }
}

static
DWORD WINAPI 
thread_proc(LPVOID farg) {
    DWORD dw;
    thread_t *th = (thread_t *)farg;
    th->ret = (DWORD)th->func(th->farg);
    dw = th->ret;
    thread_unref(th);
    return dw;
}


thread_t*
thread_new(thread_func_t func, void *farg) {
    thread_t *th;
    int err=-1;

    do {
	th = calloc(1, sizeof(*th));
	assertb(th);
	th->refcount = 1;
	th->func = func;
	th->farg = farg;
	th->hthread = CreateThread(0, 0, thread_proc, th, 0, &th->tid);
	assertb_syserr(th->hthread);
	th->refcount++;
	err = 0;
    } while(0);
    if( err ) {
	thread_unref(th);
    }
    return err ? 0 : th;
}

int
thread_set_priority(thread_t *th, int priority) {
    int i, err=-1;
    do {
	i = SetThreadPriority(th->hthread, priority);
	assertb_syserr(i);
	err = 0;
    } while(0);
    return err;
}

int
thread_detach(thread_t *th) {
    CloseHandle(th->hthread);
    thread_unref(th);
    return 0;
}

int
thread_join(thread_t *th, void **ret) {
    WaitForSingleObject(th->hthread, INFINITE);
    GetExitCodeThread(th->hthread, &th->ret);
    *ret = (void*)th->ret;
    return 0;
}

struct mutex_t {
    HANDLE h;
};

mutex_t*
mutex_new() {
    mutex_t *m;
    m = calloc(1, sizeof(*m));
    m->h = CreateMutex(0, 0, 0);
    return m;
}

int
mutex_delete(mutex_t *m) {
    ReleaseMutex(m->h);
    CloseHandle(m->h);
    free(m);
    return 0;
}

int
mutex_lock(mutex_t *m, int timeout) {
    return WaitForSingleObject(m->h, timeout);
}

int
mutex_unlock(mutex_t *m) {
    return ReleaseMutex(m->h);
}

