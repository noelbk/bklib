#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "sock.h"
#include "proc.h"
#include "debug.h"

char *USAGE =
    "USAGE: server ip:port argv...\n"
    "accept connections on ip:port and fork/exec argv\n"
    ;

int
main(int argc, char **argv) {
    int e, err = -1;
    char buf[4096];
    sock_t sock;
    struct sockaddr_in addr;
    sockaddrlen_t addrlen;
    
    do {
	if( argc < 3 ) {
	    fputs(USAGE, stderr);
	    exit(1);
	    break;
	}

	debug_init(DEBUG_INFO, 0, 0);
	sock_init();
	// don't forget to reap my children
	proc_init_reap();

	// one argument: ip:port
	addrlen = iaddr_parse(&addr, argv[1]);
	assertb(addrlen>0);
	argv += 2;
	argc -= 2;

	// listen
	sock = socket(AF_INET, SOCK_STREAM, 0);
	assertb_sockerr(sock >= 0);
	e = bind(sock, (struct sockaddr*)&addr, addrlen);
	assertb_sockerr(!e);
	e = listen(sock, 5);
	assertb_sockerr(!e);
	e = fcntl(sock, F_GETFD);
	assertb_sockerr(e>=0);
	e = fcntl(sock, F_SETFD, e | FD_CLOEXEC);
	assertb_sockerr(!e);
	debug(DEBUG_INFO, ("listening on %s\n",
			   iaddr_fmt(&addr, buf, sizeof(buf))));

	// accept, fork, exec
	while(1) {
	    sock_t client_sock;
	    int pid;
	    
	    addrlen = sizeof(addr);
	    client_sock = accept(sock, (struct sockaddr*)&addr, &addrlen);
	    assertb_sockerr(sock >= 0);
	    debug(DEBUG_INFO, 
		  ("accepted connection from %s\n",
		   iaddr_fmt(&addr, buf, sizeof(buf))));

	    pid = fork();
	    assertb_syserr(pid >= 0);
	    if(  pid > 0 ) {
		int i, n;
		char *p = buf, *end = p+sizeof(buf)-1;
		for(i=0; p<end && i<argc; i++) {
		    n = snprintf(p, end-p, "%s%s", (i==0 ? "" : " "), argv[i]);
		    p += n;
		}
		debug(DEBUG_INFO, 
		      ("forked %d, starting %s\n", pid, buf));
		
		e = close(client_sock);
		assertb_syserr(!e);
	    }
	    else {
		e = close(sock);
		assertb_syserr(!e);
		e = dup2(client_sock, 0);
		assertb_syserr(e>=0);
		e = dup2(client_sock, 1);
		assertb_syserr(e>=0);
		e = execvp(argv[0], argv);
		assertb_syserr(!e);
	    }
	}
    } while(0);
    return err;
}
