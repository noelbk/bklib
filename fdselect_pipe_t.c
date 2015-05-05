#include <stdlib.h>

#include "bklib/config.h"
#include "bklib/fdselect.h"
#include "bklib/fdselect_pipe.h"
#include "bklib/sock.h"
#include "bklib/debug.h"

int ACCEPT_PORT = 9000;
char *CONNECT_HOST = "localhost";
int CONNECT_PORT = 80;

typedef struct http_router_connection_t http_router_connection_t;
int http_router_connection_index = 0;

struct http_router_connection_t {
    fdselect_t          *fdselect;
    fdselect_pipe_t     pipe;

    sock_t              server_sock;
    struct sockaddr_in  server_sockaddr;
    sockaddrlen_t       server_sockaddrlen;
    char                server_buf[4096];
    int                 server_max;

    sock_t              client_sock;
    struct sockaddr_in  client_sockaddr;
    sockaddrlen_t       client_sockaddrlen;
    char                client_buf[4096];
    int                 client_max;
    
    int                 index;
};

int
http_router_connection_init(http_router_connection_t *conn) {
    int err=-1;

    do {
	memset(conn, 0, sizeof(*conn));
	conn->server_sock = (sock_t)-1;
	conn->server_max = sizeof(conn->server_buf)-1;
	conn->client_sock = (sock_t)-1;
	conn->client_max = sizeof(conn->client_buf)-1;
	err = 0;
    } while(0);
    return err;
}

int
http_router_connection_delete(http_router_connection_t *conn) {
    int err=-1;

    do {
	fdselect_pipe_close(&conn->pipe, 1);
	conn->server_sock = (sock_t)-1;
	conn->client_sock = (sock_t)-1;
	err = 0;
    } while(0);
    return err;
}

http_router_connection_t*
http_router_connection_new() {
    http_router_connection_t *conn = 0;
    int r, err=-1;

    do {
	conn = (http_router_connection_t *)malloc(sizeof(*conn));
	assertb(conn);
	r = http_router_connection_init(conn);
	assertb(r >= 0);
	err = 0;
    } while(0);
    if( err ) {
	if( conn ) {
	    http_router_connection_delete(conn);
	    conn = 0;
	}
    }
    return conn;
}

int
http_router_connection_pipe_callback(fdselect_pipe_t *pipe
				     ,fdselect_pipe_sock_t *sock
				     ,fdselect_pipe_event_t event
				     ,char *buf, int len, void *arg) {
    http_router_connection_t *conn = (http_router_connection_t*)arg;
    int err=-1;

    do {
	switch(event) {
	case FDSELECT_PIPE_CLOSED:
	    //debug(DEBUG_INFO, ("pipe_callback conn=%d closed\n", conn->index));

	    http_router_connection_delete(conn);
	    conn = 0;
	    break;

	case FDSELECT_PIPE_READ:
	    if( len > 0 ) {
		buf[len] = 0;
		//debug(DEBUG_INFO, 
		//      ("pipe_callback recv: conn=%d %s: len=%d\n",
		//       conn->index, (char*)sock->extra, len));
	    }
	    else {
		//debug(DEBUG_INFO, 
		//      ("pipe_callback recv: conn=%d %s: len=%d\n",
		//       conn->index, (char*)sock->extra, len));
	    }
	    break;

	case FDSELECT_PIPE_WRITE:
	    break;
	}
	err = 0;
    } while(0);
    return err;
}

int
http_router_connection_pipe(http_router_connection_t *conn) {
    int r, err=-1;
    char buf[1024];

    do {
	debug(DEBUG_INFO, 
	      ("http_router_connection_pipe: conn=%d sock=%d connected to server at: %s\n"
	       ,conn->index, conn->server_sock
	       ,iaddr_fmt(&conn->server_sockaddr, buf, sizeof(buf))));

	r = fdselect_pipe_init(&conn->pipe
			       ,conn->fdselect
			       ,http_router_connection_pipe_callback
			       ,conn
			       ,conn->server_sock
			       ,conn->server_buf, conn->server_max
			       ,conn->client_sock
			       ,conn->client_buf, conn->client_max
			       );
	assertb(r>=0);
	conn->pipe.sock[0].extra = "server";
	conn->pipe.sock[1].extra = "client";
	err = 0;
    } while(0);
    return err;
}

void
http_router_select_connect(fdselect_t *fdselect, fd_t fd, int events, void *arg) {
    http_router_connection_t *conn = (http_router_connection_t*)arg;
    int r; //, err = -1;
	
    do {
	r = connect(conn->server_sock, (struct sockaddr*)&conn->server_sockaddr, 
		    conn->server_sockaddrlen);

	debug(DEBUG_INFO, 
	      ("http_router_select_connect: conn=%d sock=%d connect=%d\n", 
	       conn->index, conn->server_sock, r));

	if( r == 0 || sock_isconn(conn->server_sock, r) ) {
	    fdselect_unset(fdselect, conn->server_sock, FDSELECT_ALL);
	    http_router_connection_pipe(conn);
	}
	
	assertb_sockerr(sock_wouldblock(conn->server_sock, r));
	
	//err = 0;
    } while(0);
}


void
http_router_select_accept(fdselect_t *fdselect, fd_t accept_sock, int events, void *arg) {
    int r; //, err = -1;
    sock_t sock;
    struct sockaddr_in sockaddr;
    sockaddrlen_t sockaddrlen;
    char buf[4096];
    http_router_connection_t *conn=0;
    long connect_ip;

    do {
	sockaddrlen = sizeof(sockaddr);
	sock = accept(accept_sock, (struct sockaddr*)&sockaddr, &sockaddrlen);
	assertb_sockerr(sock >= 0);
	
	r = sock_nonblock(sock, 1);
	assertb_sockerr(r == 0);

	conn = http_router_connection_new();
	assertb(conn);
	conn->index = http_router_connection_index++;
	
	conn->fdselect = fdselect;
	conn->client_sock = sock;
	conn->client_sockaddr = sockaddr;
	conn->client_sockaddrlen = sockaddrlen;

	debug(DEBUG_INFO, 
	      ("select_accept: conn=%d accepted sock=%d client from: %s\n"
	       ,conn->index, conn->client_sock
	       ,iaddr_fmt(&conn->client_sockaddr, buf, sizeof(buf))));
	
	conn->server_sock = socket(AF_INET, SOCK_STREAM, 0);
	assertb_sockerr(conn->server_sock >= 0);
	r = sock_nonblock(conn->server_sock, 1);
	assertb_sockerr(r == 0);
	
	connect_ip = inet_resolve(CONNECT_HOST);
	assertb(connect_ip != INADDR_NONE);
	conn->server_sockaddrlen = 
	    iaddr_pack(&conn->server_sockaddr, connect_ip, CONNECT_PORT);
	r = connect(conn->server_sock, 
		    (struct sockaddr*)&conn->server_sockaddr, 
		    conn->server_sockaddrlen);

	debug(DEBUG_INFO, 
	      ("select_accept: conn=%d connect(sock=%d sockaddr=%s)=%d sock=%d\n"
	       ,conn->index, conn->server_sock
	       ,iaddr_fmt(&conn->server_sockaddr, buf, sizeof(buf))
	       ,r
	       ,conn->server_sock
	       ));
	
	if( r == 0 ) {
	    http_router_connection_pipe(conn);
	    //err = 0;
	    break;
	}
	
	assertb_sockerr( sock_wouldblock(conn->server_sock, r) || sock_errno(conn->server_sock) == 0);

	r = fdselect_set(fdselect, conn->server_sock, FDSELECT_READ | FDSELECT_WRITE, 
			 http_router_select_connect, conn);
	assertb( r >= 0);
	
	debug(DEBUG_INFO, 
	      ("select_accept: conn=%d fdselect_set(sock=%d, FDSELECT_READ | FDSELECT_WRITE)\n"
	       ,conn->index
	       ,conn->server_sock
	       ));
	

	//err = 0;
    } while(0);
}


int
main(int argc, char **argv) {
    int r = -1;
    fdselect_t fdselect;
    sock_t accept_sock;
    struct sockaddr_in accept_sockaddr;
    sockaddrlen_t accept_sockaddrlen;
    int argi;
    char *p;

    do {
	debug_init(DEBUG_INFO, 0, 0);

	argi=1;
	if( argi < argc ) {
	    ACCEPT_PORT = strtoul(argv[argi], &p, 0);
	    assertb(p>argv[argi]);
	    argi++;
	}

	if( argi < argc ) {
	    CONNECT_HOST = argv[argi];
	    argi++;
	}
	
	if( argi < argc ) {
	    CONNECT_PORT = strtoul(argv[argi], &p, 0);
	    assertb(p>argv[argi]);
	    argi++;
	}

	r = sock_init();
	assertb_sockerr(r >= 0);
	
	r = fdselect_init(&fdselect);
	assertb(r >= 0);

	accept_sock = socket(AF_INET, SOCK_STREAM, 0);
	assertb_sockerr(accept_sock >= 0);
	accept_sockaddrlen = iaddr_pack(&accept_sockaddr, 0, ACCEPT_PORT);
	r = bind(accept_sock, (struct sockaddr*)&accept_sockaddr, 
		 accept_sockaddrlen);
	assertb_sockerr(r == 0);
	r = listen(accept_sock, 5);
	fdselect_set(&fdselect, accept_sock, FDSELECT_READ, 
		     http_router_select_accept, 0);

	while(1) {
	    fdselect_select(&fdselect, 10000);
	}
    } while(0);
    return 0;
}
