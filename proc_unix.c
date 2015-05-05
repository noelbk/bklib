#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "sock.h"
#include "opt.h"
#include "debug.h"
#include "defutil.h"
#include "str.h"

int
proc_wait(procid_t pid, int *status, int timeout) {
    int i;
    i = waitpid(pid, status, 0);
    if( pid < 0 ) {
	return i;
    }
    if( i == pid ) {
	return 0;
    }
    return -1;
}

int
proc_kill(procid_t pid, int sig) {
    return kill(pid, sig);
}

int
proc_running(procid_t pid) {
    return kill(pid, 0)==0 || errno != ESRCH;
}

procid_t
proc_getpid() {
    return getpid();
}

void
proc_sigchld_reap(int sig) {
    int status;
    pid_t pid;

    while( 1 ) {
	pid = waitpid(-1, &status, WNOHANG);
	if( pid <= 0 ) break;
	debug(DEBUG_INFO, 
	      ("proc_sigchld_reap: pid=%d status=%d\n", pid, status));
    }
}

int
proc_init_reap() {
    signal(SIGCHLD,  proc_sigchld_reap);
    return 0;
}

procid_t
proc_shellex(char *argv0, char *cmdline, char *env) {
    char buf[8192];
    int i;

    if( (i=strspn(cmdline, CHARS_CWORD))>0 && cmdline[i] == ':' ) {
	/* cmdline =~ /^\w+:/ => url */
	snprintf(buf, sizeof(buf), "konqueror \"%s\"", cmdline);
	cmdline = buf;
    }    
    else if( strchr("\\/", cmdline[0]) && strchr("\\/", cmdline[1]) ) {
	/* cmdline =~ /^\\\\/ => samba share */
	snprintf(buf, sizeof(buf), "konqueror \"smb:%s\"", cmdline);
	cmdline = buf;
    }
    return proc_start_stdpipe(argv0, cmdline, env, 0, 0);
}

procid_t
proc_start(char *argv0, char *cmdline, char *env) {
    return proc_start_stdpipe(argv0, cmdline, env, 0, 0); 
}

procid_t
proc_start_stdpipe(char *argv0,
		   char *cmdline,
		   char *env,
		   proc_stdpipe_t *stdin_write,
		   proc_stdpipe_t *stdout_read
		   ) {
    sock_t pipe_stdin[2];
    sock_t pipe_stdout[2];
    procid_t pid;
    char *argv[2048];
    int argc;
    int i, err=-1;
    char buf[4096], *p;

    do {
	/* copy the cmdline and break it into argv */
	assertb(strlen(cmdline) < sizeof(buf)-1);
	strncpy(buf, cmdline, sizeof(buf));
	p = buf;
	argc = opt_str2argv(&p, NELTS(argv)-1, argv);
	assertb(argc>0);
	argv[argc] = 0;
	if( !argv0 ) argv0 = argv[0];

	if( stdin_write ) {
	    i = sock_pair(pipe_stdin);
	    assertb_sockerr(i==0);
	}

	if( stdout_read ) {
	    i = sock_pair(pipe_stdout);
	    assertb_sockerr(i==0);
	}

	pid = fork();
	assertb_syserr(pid>=0);

	if( pid == 0 ) {
	    if( stdin_write ) {
		dup2(pipe_stdin[1], 0);
		sock_close(pipe_stdin[1]);
		sock_close(pipe_stdin[0]);
	    }

	    if( stdout_read ) {
		dup2(pipe_stdout[1], 1);
		sock_close(pipe_stdout[1]);
		sock_close(pipe_stdout[0]);
	    }

	    debug(DEBUG_INFO, ("proc_start_pipe: execvp(%s)\n", argv[0]));

	    i = execvp(argv0, argv);
	    assertb_syserr(0);
	    exit(i);
	}
	
	if( stdin_write ) {
	    *stdin_write = pipe_stdin[0];
	    sock_close(pipe_stdin[1]);
	}

	if( stdout_read ) {
	    *stdout_read = pipe_stdout[0];
	    sock_close(pipe_stdout[1]);
	}
    
	err = pid;
    } while(0);

    return err;
}

int
proc_stdpipe_read(proc_stdpipe_t s, char *buf, int len) {
    return recv(s, buf, len, 0);
}


int
proc_stdpipe_write(proc_stdpipe_t s, char *buf, int len) {
    return send(s, buf, len, 0);
}


int
proc_stdpipe_close(s) {
    return close(s);
}


