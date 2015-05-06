#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#include "sock.h"
#include "sig.h"
#include "fdselect.h"
#include "http.h"
#include "pool.h"
#include "debug.h"
#include "dir.h"

typedef struct {
    sock_t  accept_sock;
    int     accept_port;
    fdselect_t *fdselect;
    pool_t  *pool;
    char    *root_dir;
} http_server_t;

#define HTTP_SERVER_CLIENT_BUF_MAX 4096
typedef struct {
    http_server_t *server;
    sock_t sock;
    struct sockaddr_in addr;

    char   recv_buf[HTTP_SERVER_CLIENT_BUF_MAX+1];
    int    recv_len;
    int    recv_max;

    int    send_fd;
    off_t  send_off;
    off_t  send_end;
    
    http_header_t http_req;
    pool_t *pool;
} http_server_client_t;

int
http_server_init(http_server_t *server, 
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
    return err;
}

int
http_server_free(http_server_t *server) {
    if(server->pool) pool_delete(server->pool);
    memset(server, 0, sizeof(*server));
    return 0;
}

int
http_server_client_init(http_server_t *server, http_server_client_t *client) {
    int err=-1;
    do {
	memset(client, 0, sizeof(*client));
	client->server = server;
	client->pool = pool_new();
	http_header_init(&client->http_req, client->pool);
	client->recv_max = HTTP_SERVER_CLIENT_BUF_MAX;
	err = 0;
    } while(0);
    return err;
}

int
http_server_client_free(http_server_client_t *client) {
    http_server_t *server = client->server;

    http_header_clear(&client->http_req);
    if( client->send_fd >= 0 ) {
	close(client->send_fd);
    }
    if( client->sock ) {
	fdselect_unset(server->fdselect, client->sock, FDSELECT_WRITE);
	fdselect_unset(server->fdselect, client->sock, FDSELECT_READ);
	sock_close(client->sock);
    }
    if( client->pool ) {
	pool_delete(client->pool);
    }
    memset(client, 0, sizeof(*client));
    return 0;
}

void
http_server_client_select_write(fdselect_t *sel, fd_t fd, int state, void *arg) ;
void
http_server_client_select_read(fdselect_t *sel, fd_t fd, int state, void *arg);


void
http_server_select_accept(fdselect_t *sel, fd_t fd, int state, void *arg) {
    http_server_t *server = (http_server_t *)arg;
    http_server_client_t *client;
    struct sockaddr_in addr;
    sockaddrlen_t addrlen;
    sock_t sock;
    int i; //, err=-1;
    char buf1[1024];

    do {
	addrlen = sizeof(addr);
	sock = accept(server->accept_sock, (struct sockaddr*)&addr, &addrlen);
	assertb(sock>=0 || sock_wouldblock(server->accept_sock, sock));
	
	debug(DEBUG_INFO,
	      ("http_server_select_accept: accepted from %s\n"
	       ,iaddr_fmt(&addr, buf1, sizeof(buf1)))
	      );

	client = pool_malloc(server->pool, sizeof(*client));
	assertb(client);
	
	i = http_server_client_init(server, client);
	assertb(i==0);
	client->sock = sock;
	client->addr = addr;
	i = fdselect_set(server->fdselect, client->sock, FDSELECT_READ, 
			 http_server_client_select_read, client);
	assertb(i>=0);
	//err = 0;
    } while(0);
}
    
void
http_server_client_select_read(fdselect_t *sel, fd_t fd, int state, void *arg) {
    http_server_client_t *client = (http_server_client_t *)arg;
    http_server_t *server = client->server;
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
	    if( i == 0 ) {
		err = -1;
		break;
	    }
	    assertb_sockerr(i);
	}
	client->recv_len += i;
	client->recv_buf[client->recv_len] = 0;

	debug(DEBUG_INFO,
	      ("http_server_client_select_read: addr=%s recv=%d\n"
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
	    err = -i;
	    break;
	}

	i = snprintf(buf, sizeof(buf), "%s%s", 
		     server->root_dir, client->http_req.uri);
	err = 404; /* file not found */
	i = stat(buf, &st);
	assertb(i==0);
	client->send_fd = open(buf, O_RDONLY);
	assertb_syserr(client->send_fd >= 0);
	i = fstat(client->send_fd, &st);
	assertb_syserr(i >= 0);
	client->send_off = 0;
	client->send_end = st.st_size;

	debug(DEBUG_INFO,
	      ("http_server_client_select_read: sending file=%s\n", buf));

	i = snprintf(buf, sizeof(buf),
		     "HTTP/1.0 200 Ok\r\n"
		     "Pragma: no-cache\r\n"
		     "Cache-Control: no-cache\r\n"
		     "Connection: keep-alive\r\n"
		     "Content-Length: %lu\r\n"
		     "Content-Type: text/plain\r\n"
		     "\r\n"
		     ,st.st_size);
	n = i;
	i = send(client->sock, buf, n, 0);
	assertb(i==n);
	
	/* stop reading and start writing */
	fdselect_unset(server->fdselect, client->sock, FDSELECT_READ);
	fdselect_set(server->fdselect, client->sock, FDSELECT_WRITE, 
		     http_server_client_select_write, client);

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
	http_server_client_free(client);
    }
}

/* eat the request body, then get a new request */
int
http_server_client_end_req(http_server_client_t *client) {
    http_server_t *server = client->server;
    int err=-1;

    do {
	/* normal EOF, go back to reading */
	if( client->send_fd >= 0 ) {
	    close(client->send_fd);
	}
	client->send_fd = -1;
	fdselect_unset(server->fdselect, client->sock, FDSELECT_WRITE);
	fdselect_set(server->fdselect, client->sock, FDSELECT_READ, 
		     http_server_client_select_read, client);
	client->recv_len = 0;
	http_header_clear(&client->http_req);
	err = 0;
	break;
    } while(0);
    return err;
}

void
http_server_client_select_write(fdselect_t *sel, fd_t fd, int state, void *arg) {
    http_server_client_t *client = (http_server_client_t*)arg;
    int i, err=-1, closed=0;
    char buf1[1024];
    
    do {
	if( client->send_off >= client->send_end ) {
	    err = 0;
	    closed = 1;
	    break;
	}

	i = sendfile(client->sock, client->send_fd, &client->send_off,
		     client->send_end-client->send_off);
	debug(DEBUG_INFO,
	      ("http_server_client_select_write: addr=%s sendfile=%d\n"
	       ,iaddr_fmt(&client->addr, buf1, sizeof(buf1))
	       ,i
	       ));
	if( i<0 && sock_wouldblock(client->sock, i) ) {
	    err = 0;
	    closed = 1;
	    break;
	}
	assertb_syserr(i>0);

	
	err = 0;
    } while(0);
    if( err || closed ) {
	http_server_client_free(client);
    }
}


int
main(int argc, char **argv) {
    fdselect_t fdselect;
    http_server_t server_s, *server=&server_s;
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
	http_server_init(server, &fdselect, buf);

	server->accept_sock = socket(AF_INET, SOCK_STREAM, 0);
	assertb_sockerr(server->accept_sock>=0);
	i = 1;
	i = setsockopt(server->accept_sock, SOL_SOCKET, SO_REUSEADDR, 
		       (const char*)&i, sizeof(i));
	assertb_sockerr(i==0);
	addrlen = iaddr_pack(&addr, INADDR_ANY, 8000);
	i = bind(server->accept_sock, (struct sockaddr*)&addr, addrlen);
	assertb_sockerr(i==0);
	i = listen(server->accept_sock, 5);
	assertb_sockerr(i==0);
	
	i = fdselect_set(server->fdselect, server->accept_sock, FDSELECT_READ,
			 http_server_select_accept, server);
	assertb(i>=0);

	while(!sig_exited) {
	    fdselect_select(&fdselect, 1);
	}
	err = 0;
    } while(0);
    return err;
}
 
