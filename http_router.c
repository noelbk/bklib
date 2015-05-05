/* http_router.c - accept incoming HTTP requests and proxy them to
   other http servers

   Author: Noel Burton-Krahn <noel@burton-krahn.com>
   Created: Sep 18, 2006
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bklib/sock.h"
#include "bklib/sig.h"
#include "bklib/fdselect.h"
#include "bklib/http.h"
#include "bklib/pool.h"
#include "bklib/debug.h"
#include "bklib/dir.h"

typedef struct {
    sock_t  accept_sock;
    int     accept_port;
    fdselect_t *fdselect;
    pool_t  *pool;
    char    *root_dir;
} http_router_t;

#define HTTP_ROUTER_CLIENT_BUF_MAX 4096
typedef struct {
    http_router_t *server;
    sock_t sock;
    struct sockaddr_in addr;

    char   recv_buf[HTTP_ROUTER_CLIENT_BUF_MAX+1];
    int    recv_len;
    int    recv_max;

    FILE  *send_file;
    char   send_buf[HTTP_ROUTER_CLIENT_BUF_MAX+1];
    char  *send_ptr;
    int    send_len;
    
    http_header_t http_req;
    pool_t *pool;
} http_router_client_t;

int
http_router_init(http_router_t *server, 
		 fdselect_t *fdselect,
		 char *root_dir) {
    int err;
    do {
	memset(server, 0, sizeof(*server));
	server->fdselect = fdselect;
	server->pool = pool_new();
	assertb(server->pool);
	server->root_dir = pool_strdup(server->pool, root_dir);
	err = 0;
    } while(0);
    return 0;
}

int
http_router_free(http_router_t *server) {
    if(server->pool) pool_delete(server->pool);
    memset(server, 0, sizeof(*server));
    return 0;
}

int
http_router_client_init(http_router_t *server, http_router_client_t *client) {
    int err=-1;
    do {
	memset(client, 0, sizeof(*client));
	client->server = server;
	client->pool = pool_new();
	http_header_init(&client->http_req, client->pool);
	client->recv_max = HTTP_ROUTER_CLIENT_BUF_MAX;
	err = 0;
    } while(0);
    return err;
}

int
http_router_client_free(http_router_client_t *client) {
    http_router_t *server = client->server;

    http_header_clear(&client->http_req);
    if( client->send_file ) {
	fclose(client->send_file);
    }
    if( client->sock ) {
	sock_close(client->sock);
	fdselect_unset(server->fdselect, client->sock, FDSELECT_WRITE);
	fdselect_unset(server->fdselect, client->sock, FDSELECT_READ);
    }
    if( client->pool ) {
	pool_delete(client->pool);
    }
    memset(client, 0, sizeof(*client));
    return 0;
}

void
http_router_client_select_write(fdselect_t *sel, fd_t fd, int state, void *arg) ;


void
http_router_client_select_read_header(fdselect_t *sel, fd_t fd, int state, void *arg);

/* after reading the header and figuring out where to redirect, select
   while connecting to redirected server */
void
http_router_client_select_connect(fdselect_t *sel, fd_t fd, int state, void *arg);


void
http_router_select_accept(fdselect_t *sel, fd_t fd, int state, void *arg) {
    http_router_t *server = (http_router_t *)arg;
    http_router_client_t *client;
    struct sockaddr_in addr;
    sockaddrlen_t addrlen;
    sock_t sock;
    int i, err=-1;
    char buf1[1024];

    do {
	addrlen = sizeof(addr);
	sock = accept(server->accept_sock, (struct sockaddr*)&addr, &addrlen);
	assertb(sock>=0 || sock_wouldblock(server->accept_sock, sock));
	
	debug(DEBUG_INFO,
	      ("http_router_select_accept: accepted from %s\n"
	       ,iaddr_fmt(&addr, buf1, sizeof(buf1)))
	      );

	client = pool_malloc(server->pool, sizeof(*client));
	assertb(client);
	
	i = http_router_client_init(server, client);
	assertb(i==0);
	client->sock = sock;
	client->addr = addr;
	i = fdselect_set(server->fdselect, client->sock, FDSELECT_READ, 
			 http_router_client_select_read_header, client);
	assertb(i>=0);
	err = 0;
    } while(0);
}
    
void
http_router_client_select_read_header(fdselect_t *sel, fd_t fd, int state, void *arg) {
    http_router_client_t *client = (http_router_client_t *)arg;
    http_router_t *server = client->server;
    struct stat st;
    int i, n, err=-1;
    char buf[4096];
    char buf1[1024];

    do {
	i = client->recv_max - client->recv_len;
	if( i <= 0 ) {
	    err = 405; /* request too big */
	    break;
	}

	i = recv(client->sock, client->recv_buf+client->recv_len, i, 0);
	if( i <= 0 ) {
	    if( sock_wouldblock(client->sock, i) ) {
		/* ignore spurious blocking reads */
		err = 0;
		break;
	    }
	    assertb_sockerr(i);
	}
	client->recv_len += i;
	client->recv_buf[client->recv_len] = 0;

	debug(DEBUG_INFO,
	      ("http_router_client_select_read_header: addr=%s recv=%d\n"
	       "recv_buf=\n%s\n"
	       ,iaddr_fmt(&client->addr, buf1, sizeof(buf1))
	       ,i
	       ,client->recv_buf
	       ));

	i = http_header_parse(&client->http_req, client->recv_buf, client->recv_len);
	if( i == 0 ) {
	    err = 0;
	    break;
	}
	else if( i < 0 ) {
	    err = i;
	    break;
	}

	/* finally found out what the client was after, figure out
	   where to proxy it to */
	i = http_router_find_proxy(server, &client->http_req, &proxy_uri);
	if( i < 0 ) {
	    err = i;
	    break;
	}
	
	/* start connecting to remote host */
	client->send_sock = sock_new();
	inet_resolve();
	sock_connect(client->send_sock);
	fdselect_set(server->fdselect, client->send_sock, FDSELECT_READ, 
		     http_router_client_select_connect, client);
	
	/* stop reading until I connect */
	fdselect_unset(server->fdselect, client->sock, FDSELECT_READ);

	err = 0;

    } while(0);

    if( err > 0 ) {
	i = snprintf(buf, sizeof(buf),
		     "HTTP/1.0 %d\r\n"
		     "Content-Type: text/html; charset=iso-8859-1\r\n"
		     "\r\n"
		     "<HTML><HEAD>\r\n"
		     "<TITLE>HTTP Error %d</TITLE>\r\n"
		     "</HEAD><BODY>\r\n"
		     "<H1>HTTP Error %d</H1>\r\n"
		     "%s<br>\r\n"
		     "<HR>\r\n"
		     "</BODY></HTML>\r\n"
		     
		     , err, err, err
		     , client->http_req.uri);
	send(client->sock, buf, i, 0);
    }
    if( err ) {
	http_router_client_free(client);
    }
}

/* eat the request body, then get a new request */
int
http_router_client_end_req(http_router_client_t *client) {
    http_router_t *server = client->server;
    int err=-1;

    do {
	/* normal EOF, go back to reading */
	fclose(client->send_file);
	client->send_file = 0;
	fdselect_unset(server->fdselect, client->sock, FDSELECT_WRITE);
	fdselect_set(server->fdselect, client->sock, FDSELECT_READ, 
		     http_router_client_select_read_header, client);
	client->recv_len = 0;
	http_header_clear(&client->http_req);
	err = 0;
	break;
    } while(0);
    return err;
}

void
http_router_client_select_write(fdselect_t *sel, fd_t fd, int state, void *arg) {
    http_router_client_t *client = (http_router_client_t*)arg;
    int i, err=-1;
    char buf1[1024];
    
    do {
	if( client->send_len <= 0 ) {
	    i = fread(client->send_buf, 1, sizeof(client->send_buf), 
		      client->send_file);
	    if( i == 0 ) {
		i = http_router_client_end_req(client);
		assertb(i==0);
	    }
	    assertb(i>0);
	    client->send_len = i;
	    client->send_ptr = client->send_buf;
	}

	i = send(client->sock, client->send_ptr, client->send_len, 0);
	assertb(i>0);

	client->send_len -= i;
	client->send_ptr += i;

	debug(DEBUG_INFO,
	      ("http_router_client_select_write: addr=%s send=%d\n"
	       ,iaddr_fmt(&client->addr, buf1, sizeof(buf1))
	       ,i
	       ));


	err = 0;
    } while(0);
    if( err ) {
	http_router_client_free(client);
    }
}


int
main(int argc, char **argv) {
    fdselect_t fdselect;
    http_router_t server_s, *server=&server_s;
    struct sockaddr_in addr;
    sockaddrlen_t addrlen;
    int i, err=-1;
    char buf[4096];

    do {
	debug_init(DEBUG_INFO, 0, 0);
	sock_init();
	sig_init();

	fdselect_init(&fdselect);
	getcwd(buf, sizeof(buf));
	http_router_init(server, &fdselect, buf);

	server->accept_sock = socket(AF_INET, SOCK_STREAM, 0);
	assertb_sockerr(server->accept_sock>=0);
	i = 1;
	setsockopt(server->accept_sock, SOL_SOCKET, SO_REUSEADDR, 
		   (const char*)&i, sizeof(i));
	addrlen = iaddr_pack(&addr, INADDR_ANY, 8080);
	i = bind(server->accept_sock, (struct sockaddr*)&addr, addrlen);
	assertb_sockerr(i==0);
	i = listen(server->accept_sock, 5);
	assertb_sockerr(i==0);
	
	i = fdselect_set(server->fdselect, server->accept_sock, FDSELECT_READ,
			 http_router_select_accept, server);
	assertb(i>=0);

	while(!sig_exited) {
	    fdselect_select(&fdselect, 1);
	}
	err = 0;
    } while(0);
    return err;
}
 
