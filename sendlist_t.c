/* 
 * USAGE
 *   ./sendlist_t -clients 5 -bytes 10000000 -readbuf $((65536*4)) 2>out; tail out
 *     sent 20Mb through 5 clients in 0.110757 secs. read_buflen=262144. 903Mb/s
 *
 *   ./sendlist_t -clients 5 -bytes 10000000 -readbuf $((65536*4)) -threaded 2>out; tail out
 *     sent 20Mb through 5 clients in 0.0417428 secs. read_buflen=262144. 2.4Gb/s
 *
 */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include "debug.h"
#include "mmap.h"
#include "mstime.h"
#include "sendlist.h"
#include "sock.h"
#include "alloc_count.h"
#include "readable.h"
#include "thread.h"

size_t read_buflen = 4096*16;

typedef struct client_s client_t;
struct client_s {
    int idx;
    thread_t *thread;
    sendlist_t sendlist;
    client_t *next;
    sock_t send_fd;
    sock_t recv_fd;
    int sending;
    size_t send_len;
    size_t send_sum;
    size_t recv_len;
    size_t recv_sum;
    int recv_eof;
};

size_t
sum(size_t *sum, size_t *len, void *buf, size_t buflen) {
    *len += buflen;
    while(buflen-- > 0 ) {
	*sum += *((unsigned char*)buf++);
    }
    return *sum;
}

int
client_close(client_t *client) {
    if( client->next ) {
	sendlist_close(&client->sendlist);
    }
    client->recv_eof = 1;
    client->sending = 0;
    return 0;
}

void
client_sendbuf_free(sendbuf_t *sendbuf, void *arg) {
    client_t *client = (client_t *)arg;
    client->sending--;
    if( sendbuf->buf ) {
	ALLOC_COUNT_FREE(sendbuf->buf);
	free(sendbuf->buf);
    }
}

void
client_select_read(fdselect_t *fdsel, fd_t fd, int state, void *arg) {
    client_t *client = (client_t*)arg;
    int e, err=-1;
    size_t i, len;
    size_t buflen = read_buflen;
    void *buf = 0;
    
    do {
	debugf("client_select_read client=%d\n", client->idx);
	buf = malloc(buflen);
	assertb(buf);
	ALLOC_COUNT_ADD(buf, buflen);
	len = read(fd, buf, buflen);
	if( len <= 0 ) {
	    // read eof
	    e = fdselect_unset(fdsel, fd, FDSELECT_READ);
	    assertb(!e);
	    close(fd);
	    assertb_syserr(!e);
	    fd = client->recv_fd = -1;
	    err = 0;
	    client->recv_eof++;
	    if( client->next ) {
		e = sendlist_shutdown(&client->sendlist, client_sendbuf_free, client);
		assertb(e>=0);
		client->sending++;
	    }
	    break;
	}
	assertb(len>0);
	client->recv_len += len;
	for(i=0; i<len; i++) {
	    client->recv_sum += ((unsigned char*)buf)[i];
	}
	if( client->next ) {
	    for(i=0; i<len; i++) {
		client->send_sum += ((unsigned char*)buf)[i];
	    }
	    e = sendlist_send_buf(&client->sendlist, buf, len, client_sendbuf_free, client);
	    assertb(e>=0);
	    buf = 0;
	    client->sending++;
	    client->send_len += len;
	}
	err = 0;
    } while(0);
    
    if( buf ) {
	free(buf);
	ALLOC_COUNT_FREE(buf);
    }
    
    if( err ) {
	if( fd >= 0 ) {
	    fdselect_unset(fdsel, fd, FDSELECT_READ);
	    close(fd);
	}
	client->recv_eof = 1;
	client->sending = 0;
    }
}

int
client_sendlist_flushed_func(sendlist_t *sendlist, int err, void *arg) {
    client_t *client = (client_t*)arg;
    if( err == 0 ) {
	// sendbuf empty, start reading recv_fd
	if( client->recv_fd >= 0 ) {
	    fdselect_set(sendlist->fdsel, client->recv_fd, FDSELECT_READ, client_select_read, client);
	}
    }
    else {
	// error or sendbuf full, stop reading recv_fd
	if( client->recv_fd >= 0 ) {
	    fdselect_unset(sendlist->fdsel, client->recv_fd, FDSELECT_READ);
	}
	
	if( err < 0 ) {
	    // error, close
	    debugf("ERROR: client[%d] closed.  err=%d\n", client->idx, err);
	    client_close(client);
	}
    }
    return 0;
}

void
client0_sendbuf_free(sendbuf_t *sendbuf, void *arg) {
    client_t *client = (client_t*)arg;
    client->sending--;
    if( !sendbuf->buf && sendbuf->fd<0 ) {
	client->recv_eof=1;
    }
}

void
usage() {
    fprintf(stderr,
	    "USAGE: sendlist_t\n"
	    "\n"
	    "  -clients N      Number of clients to create (default 5)\n"
	    "  -bytes N        total bytes to send (default )\n"
	    "  -readbuf N      size of client's read buffers (default 4096)\n"
	    "  -sendlist       test one thread total, using sendlist (default)\n"
	    "  -threaded       test one thread per client\n"
	    );
}

void*
client_thread(void *arg) {
    client_t *client = (client_t*)arg;
    void *buf=0;
    size_t off, len;
    ssize_t n;
    int e, err=-1;
    
    do {
	if( client->recv_fd < 0 ) {
	    err = 0;
	    break;
	}
	
	buf = malloc(read_buflen);
	assertb(buf);
	e = sock_nonblock(client->recv_fd, 0);
	assertb_sockerr(e>=0);
	if( client->send_fd >= 0 ) {
	    e = sock_nonblock(client->send_fd, 0);
	    assertb_sockerr(e>=0);
	}
	while(1) {
	    n = recv(client->recv_fd, buf, read_buflen, 0);
	    debugf("client recv client=%d n=%ld\n", client->idx, n);
	    if( n == 0 ) {
		err = 0;
		break;
	    }
	    assertb(n>=0);
	    len = n;
	    sum(&client->recv_sum, &client->recv_len, buf, n);
	    off = 0;
	    if( client->send_fd >= 0 ) {
		while(off < len) {
		    n = send(client->send_fd, buf+off, len-off, 0);
		    debugf("client send client=%d n=%lu\n", client->idx, n);
		    assertb(n>=0);
		    sum(&client->send_sum, &client->send_len, buf+off, n);
		    off += n;
		}
		assertb(off==len);
	    }
	}
	err = 0;
    } while(0);
    if( buf ) {
	free(buf);
    } 
    if( client->send_fd >= 0 ) {
	close(client->send_fd);
	client->send_fd = -1;
    }
    if( client->recv_fd >= 0 ) {
	close(client->recv_fd);
	client->recv_fd = -1;
    }
    return (void*)(intptr_t)err;
}

typedef enum {
    TEST_SENDLIST
    ,TEST_THREADS
} test_type_t;

int
client_dump(client_t *clients, int num_clients) {
    int eof = 1;
    int idx;
    for(idx=0; idx<num_clients; idx++) {
	client_t *c = clients+idx;
	debugf("client[%d]: %10lu %10lu %10d %10d %10lu %10lu\n"
	       ,c->idx
	       ,c->recv_len
	       ,c->recv_sum
	       ,c->recv_eof
	       ,c->sending
	       ,c->send_len
	       ,c->send_sum
	       );
	if( !c->recv_eof || c->sending>0 ) {
	    eof = 0;
	}
    }
    debugf("eof=%d\n\n", eof);
    return eof;
}

int
main(int argc, char **argv) {
    int e, err=-1;
    fdselect_t fdsel;
    sock_t sock[2];
    int idx;
    int eof;
    fd_t rand_fd = -1;
    fd_t tmp_fd = -1;
    char tmp_path[1024];
    ssize_t i;
    size_t tmp_len=100000;
    int off;
    int num_clients = 5;
    int argi;
    unsigned char *tmp_buf;
    mmap_t tmp_mm;
    char buf1[1024], buf2[1024], *q;
    mstime_t t0, dt;
    client_t *clients = 0;
    test_type_t test = TEST_SENDLIST;
    void *retp;
    
    debug_init(DEBUG_INFO, 0, 0);
    alloc_count_init();

    do {
	for(argi=1; argi<argc; argi++) {
	    if( !strcmp(argv[argi], "-clients") ) {
		argi++;
		num_clients = strtoul(argv[argi], &q, 0);
		assertb(q > argv[argi]);
	    }
	    else if( !strcmp(argv[argi], "-bytes") ) {
		argi++;
		tmp_len = strtoul(argv[argi], &q, 0);
		assertb(q > argv[argi]);
	    }
	    else if( !strcmp(argv[argi], "-readbuf") ) {
		argi++;
		read_buflen = strtoul(argv[argi], &q, 0);
		assertb(q > argv[argi]);
	    }
	    else if( !strcmp(argv[argi], "-threaded") ) {
		test = TEST_THREADS;
	    }
	    else if( !strcmp(argv[argi], "-sendlist") ) {
		test = TEST_SENDLIST;
	    }
	    else {
		break;
	    }
	}
	if( argi < argc ) {
	    fprintf(stderr, "unknown argument: %s\n", argv[argi]);
	    usage();
	    err = 1;
	    break;
	}
	
	clients = (client_t*)calloc(num_clients*sizeof(*clients), 1);
	assertb(clients);
	
	e = fdselect_init(&fdsel);
	assertb(!e);
	    
	for(idx=0; idx<num_clients; idx++) {
	    client_t *client = &clients[idx];

	    memset(client, 0, sizeof(*client));
	    client->idx = idx;
	    client->recv_fd = -1;
	    client->send_fd = -1;
	}
	
	for(idx=0; idx<num_clients-1; idx++) {
	    client_t *client = &clients[idx];
	    
	    client->next = &clients[idx+1];
	    e = sock_pair(sock);
	    assertb(!e);
	    e = sock_nonblock(sock[0], 1);
	    assertb(!e);
	    e = sock_nonblock(sock[1], 1);
	    assertb(!e);
	    client->send_fd = sock[0];
	    e = sendlist_init(&client->sendlist, &fdsel, client->send_fd, client_sendlist_flushed_func, client);
	    assertb(!e);
	    client->next->recv_fd = sock[1];
	    e = fdselect_set(&fdsel, client->next->recv_fd, FDSELECT_READ, client_select_read, client->next);
	    assertb(!e);
	}

	// send from client 0
	rand_fd = open("/dev/urandom", O_RDONLY);
	assertb_syserr(rand_fd>=0);

	snprintf(tmp_path, sizeof(tmp_path), "/tmp/randXXXXXX");
	tmp_fd = mkstemp(tmp_path);
	assertb_syserr(tmp_fd>=0);
	unlink(tmp_path);
	e = mmap_fd(&tmp_mm, tmp_fd, MMAP_MODE_WRITE, tmp_len);
	assertb(!e);
	tmp_buf = mmap_write(&tmp_mm, 0, tmp_len);
	off = 0;
	while(off<tmp_len) {
	    i = read(rand_fd, tmp_buf+off, tmp_len-off);
	    assertb_syserr(i>0);
	    off += i;
	}
	assertb_syserr(off==tmp_len);
	close(rand_fd);
	rand_fd = -1;
	lseek(tmp_fd, 0, SEEK_SET);
		   
	t0 = mstime();
	
	e = sendlist_send_fd(&clients[0].sendlist, tmp_fd, 0, tmp_len, client0_sendbuf_free, &clients[0]);
	assertb(e>=0);
	clients[0].sending++;
	sum(&clients[0].send_sum, &clients[0].send_len, tmp_buf, tmp_len);
	
	e = sendlist_send_buf(&clients[0].sendlist, tmp_buf, tmp_len, client0_sendbuf_free, &clients[0]);
	assertb(e>=0);
	clients[0].sending++;
	sum(&clients[0].send_sum, &clients[0].send_len, tmp_buf, tmp_len);
	
	e = sendlist_shutdown(&clients[0].sendlist, client0_sendbuf_free, &clients[0]);
	assertb(e>=0);
	clients[0].sending++;

	debugf("client:    %10s %10s %10s %10s %10s %10s\n"
	       ,"recv_len"
	       ,"recv_sum"
	       ,"recv_eof"
	       ,"sending"
	       ,"send_len"
	       ,"send_sum"
	       );

	if( test == TEST_THREADS ) {
	    for(idx=0; idx<num_clients; idx++) {
		client_t *client = clients + idx;
		client->thread = thread_new(client_thread, client);
		assertb(client->thread);
	    }
	    while(clients[0].sending) {
		e = fdselect_select(&fdsel, FDSELECT_INFINITE);
		assertb(!e);
	    }
	    for(idx=0; idx<num_clients; idx++) {
		client_t *client = clients + idx;
		thread_join(client->thread, &retp);
		thread_detach(client->thread);
	    }
	    client_dump(clients, num_clients);
	}
	else {
	    eof = 0;
	    while(!eof) {
		e = fdselect_select(&fdsel, FDSELECT_INFINITE);
		assertb(!e);

		debugf("fdselect_select\n");
		eof = client_dump(clients, num_clients);
	    }
	}
	dt = mstime() - t0;
	debugf("sent %s through %d clients in %lg secs. read_buflen=%lu. %s/s\n"
	       ,readable_bytes(clients[0].send_len, buf1, sizeof(buf1))
	       ,num_clients
	       ,dt
	       ,read_buflen
	       ,readable_bytes(clients[0].send_len * num_clients / dt, buf2, sizeof(buf2))
	       );
	
	err = 0;
    } while(0);

    alloc_count_dump();
    alloc_count_fini();
    
    return err;
    
}


