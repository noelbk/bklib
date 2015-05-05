#include <stdio.h>
#include <stdint.h>

#include "thread.h"
#include "debug.h"
#include "mstime.h"

#define N 10

void*
thfunc(void *arg) {
    int i;
    for(i=0; i<N; i++) {
	printf("thfunc: arg=%p i=%d\n", arg, i);
	mssleep(.5);
    }
    return arg;
}

int main() {
    thread_t *th[N];
    int i;
    void *ret;

    for(i=0; i<N; i++) {
	th[i] = thread_new(thfunc, (void*)(intptr_t)i);
    }

    for(i=0; i<N; i++) {
	thread_join(th[i], &ret);
	printf("joined thread i=%d ret=%p\n", i, ret);
	thread_detach(th[i]);
    }

    return 0;
}
