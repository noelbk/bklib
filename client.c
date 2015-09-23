#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "sock.h"
#include "proc.h"
#include "debug.h"

char *USAGE =
    "USAGE: client host:port argv...\n"
    "connect to host:port and exec argv\n"
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

	// one argument: host:port
	addrlen = iaddr_parse(&addr, argv[1]);
	assertb(addrlen>0);
	argv += 2;
	argc -= 2;

	// listen
	sock = socket(AF_INET, SOCK_STREAM, 0);
	assertb_sockerr(sock >= 0);
	e = connect(sock, (struct sockaddr*)&addr, addrlen);
	assertb_sockerr(!e);
	debug(DEBUG_INFO, ("connected to %s\n",
			   iaddr_fmt(&addr, buf, sizeof(buf))));

	e = dup2(sock, 0);
	assertb_syserr(e>=0);
	e = dup2(sock, 1);
	assertb_syserr(e>=0);
	e = execvp(argv[0], argv);
	assertb_syserr(!e);
    } while(0);
    return err;
}
