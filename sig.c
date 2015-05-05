#include "sig.h"
#include "configbk.h"

#if OS & OS_UNIX 
#include <signal.h>
#endif

int sig_exited=0;

void
sig_exit(int sig) {
    sig_exited = sig;
}

int
sig_init() {
    int err=-1;
    
    do {
#if OS & OS_UNIX 
	signal(SIGKILL, sig_exit);
	signal(SIGQUIT, sig_exit);
	signal(SIGINT,  sig_exit);
	signal(SIGTERM, sig_exit);
	signal(SIGPIPE, SIG_IGN);
#endif
    err = 0;
    } while(0);
    return err;
}

