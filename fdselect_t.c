#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "config.h"


#include "sock.h"
#include "fdselect.h"
#include "warn.h"
#include "rand.h"

#if OS & OS_WIN32
#include "bkwin32.h"

typedef DWORD WINAPI thread_func_t(LPVOID lpParameter);

#define THREAD_FUNC(name, arg) DWORD WINAPI name(LPVOID arg)

tid_t
thread_tid() {
    return GetCurrentThreadId();
}

tid_t
thread_start(thread_func_t func, void *arg) {
    DWORD dw;
    HANDLE h;
    h = CreateThread(0, 0, func, arg, 0, &dw);
    CloseHandle(h);
    return (tid_t)dw;
}

#endif // OS_WIN32

#define CHILD_FORK_MAX     10
#define CHILD_BUF_MAX 32768
#define CHILD_SLEEP_RAND 100
#define CHILD_CALL_MAX 20

typedef unsigned long tid_t;


typedef struct {
    sock_t sock;
    int id;
} pipe_child_arg_t;


/* just keep sending and receiving for a while */
THREAD_FUNC(pipe_child, varg) {
    pipe_child_arg_t *arg = (pipe_child_arg_t*)varg;
    int id = arg->id;
    sock_t fd = arg->sock;
    int i, n;
    char buf[CHILD_BUF_MAX];
    tid_t tid;
    tid = thread_tid();
    for(i=0; i<CHILD_CALL_MAX; i++) {
	n = rand_u32(CHILD_BUF_MAX);
	n = send(fd, buf, n, 0);
	printf("pipe_child[tid=%d]: send n=%d\n", tid, n);
	if( n <= 0 ) break;

	n = recv(fd, buf, sizeof(buf), 0);
	printf("pipe_child[tid=%d]: recv n=%d\n", tid, n);
	if( n <= 0 ) break;
	
	//usleep((double)rand() * CHILD_SLEEP_RAND / RAND_MAX);
    }
    sock_close(fd);
    free(arg);

    return 0;
}

void
pipe_select_read(fdselect_t *sel, int fd, int event, void *arg);

void
pipe_select_write(fdselect_t *sel, int fd, int event, void *arg);


void
pipe_select_read(fdselect_t *sel, int fd, int event, void *arg) {
    int n;
    char buf[CHILD_BUF_MAX];
    tid_t tid = (tid_t)arg;

    n = recv(fd, buf, sizeof(buf), 0);
    printf("pipe_select_read[tid=%d]: recv n=%d\n", tid, n);
    if( sock_wouldblock(fd, n) ) {
	return;
    }
    
    fdselect_unset(sel, fd, FDSELECT_READ);
    if( n>0 ) {
	fdselect_set(sel, fd, FDSELECT_WRITE, pipe_select_write, arg);
    }
}

void 
pipe_select_write(fdselect_t *sel, int fd, int event, void *arg) {
    int n;
    char buf[CHILD_BUF_MAX];
    tid_t tid = (tid_t)arg;

    n = rand_u32(CHILD_BUF_MAX);
    n = send(fd, buf, n, 0);
    printf("pipe_select_write[tid=%d]: send n=%d\n", tid, n);
    if( sock_wouldblock(fd, n) ) {
	return;
    }

    fdselect_unset(sel, fd, FDSELECT_WRITE);
    if( n>0 ) {
	fdselect_set(sel, fd, FDSELECT_READ, pipe_select_read, arg);
    }
}

int
main() {
    fdselect_t sel;
    sock_t pipe[2];
    int i, n;
    tid_t tid;
    
    debug_init(DEBUG_INFO, 0, 0);
    sock_init();
    fdselect_init(&sel);
    
    for(i=0; i<CHILD_FORK_MAX; i++) {
	pipe_child_arg_t *a;

	n = sock_pair(pipe);
	assertb_sockerr(n>=0);
	n = sock_nonblock(pipe[0], 1);
	assertb_sockerr(n>=0);

	a = calloc(1, sizeof(*a));
	a->sock = pipe[1];
	a->id    = i;
	tid = thread_start(pipe_child, a);
	fdselect_set(&sel, pipe[0], FDSELECT_READ, pipe_select_read, (void*)tid);
    }

    while(1) {
	i = fdselect_count(&sel);
	printf("fdselect_select: count=%d\n", i);
	if( i<=0 ) break;
	fdselect_select(&sel, 1000);
    }
    return 0;
}
