#include <stdio.h>
#include <stdlib.h>

#include "ipconntrack.h"
#include "netpkt.h"
#include "sock_raw.h"
#include "warn.h"
#include "memutil.h"
#include "sock.h"
#include "defutil.h"
#include "debug.h"

typedef struct {
    IpConnTrack m_conntrack;
    hash_t m_port_ignore;
} IpTrack;

int
IpTrack_init(IpTrack *self) {
    int i, err=-1;
    do {
	i = IpConnTrack_init(&self->m_conntrack, 0, 0);
	assertb(!i);
	i = hash_init(&self->m_port_ignore, hash_hash_int, hash_cmp_int, 0, 0, 0);
	assertb(!i);
	err = 0;
    } while(0);
    return err;
}


int
IpTrack_poll_buf(IpTrack *self, char *pktbuf, int pktlen) {
    int i, err=-1;
    IpConn *conn=0;
    IpConnTcpQueue *queue=0;
    struct netpkt pkt;
    char buf[4096], *ptr, *p;
    int len;

    do {
	// read and parse a packet from my raw device
	netpkt_init(&pkt, pktbuf, pktlen);
	i = netpkt_parse_ether(&pkt);
	assertb(!i);

	// ignore connections for port 22
	if( pkt.pkt_tcp
	    && ( hash_get(&self->m_port_ignore, (void*)(long)ntohs(pkt.pkt_tcp->source))
		 || hash_get(&self->m_port_ignore, (void*)(long)ntohs(pkt.pkt_tcp->dest)))
	    ) {
	    err = 0;
	    break;
	}

	// not ip? Ick!
	if( !pkt.pkt_ip ) {
	    netpkt_fmt(&pkt, buf, sizeof(buf));
	    debug(DEBUG_INFO, ("netpkt not ip: %s\n", buf));

	    err = 0;
	    break;
	}
	
	// find the IP connection
	conn = IpConnTrack_track_pkt(&self->m_conntrack, &pkt, IPCONN_TRACK_BUFFER);
	
	if( !conn ) {
	    err = 0;
	    break;
	}
	
	// print the packet and connection data
	ptr = buf;
	len = sizeof(buf);

	i = snprintf(ptr, len, "connection: ");
	BUF_ADD(ptr, len, i);

	if( conn->conn_pkt_flags & CONN_PKT_FROM_CLIENT ) {
	    queue = &conn->queue_client;
	    i = snprintf(ptr, len, "client: ");
	    BUF_ADD(ptr, len, i);
	}
	else if( conn->conn_pkt_flags & CONN_PKT_FROM_SERVER ) {
	    queue = &conn->queue_server;
	    i = snprintf(ptr, len, "server: ");
	    BUF_ADD(ptr, len, i);
	}
	
	p = IpConn_fmt(conn, ptr, len);
	i = p ? strlen(p) : -1;
	BUF_ADD(ptr, len, i);

	i = snprintf(ptr, len, " pkt: ");
	BUF_ADD(ptr, len, i);
	netpkt_fmt(&pkt, ptr, len);
	i = strlen(ptr);
	BUF_ADD(ptr, len, i);

	i = array_count(&queue->buf);
	if( i ) {
	    i = snprintf(ptr, len, "\n");
	    BUF_ADD(ptr, len, i);

	    i = array_count(&queue->buf);
	    assertb(i<len);
	    memcpy(ptr, array_get(&queue->buf, 0), i);
	    BUF_ADD(ptr, len, i);
	    array_remove_idx(&queue->buf, 0, i);
	}
	i = snprintf(ptr, len, "\n");	
	BUF_ADD(ptr, len, i);

	fwrite(buf, ptr-buf, 1, stderr);

	if( conn->conn_closed ) {
	    IpConnTrack_conn_delete(&self->m_conntrack, conn);
	    conn = 0;
	}

	err = 0;
    } while(0);
    
    return err;
}

int
IpTrack_main(IpTrack *self,  int argc, char **argv) {
    sock_t sock;
    int i, j;
    char *p, buf[4096];

    do {
	if( argc < 2 ) {
	    warn(("usage: %s iface\n", argv[0]));
	    break;
	}

	// open a raw socket
	sock = sock_openraw(argv[1], 1);
	assertb_sockerr(sock>=0);

	for(i=2; i<argc; i++) {
	    j = strtoul(argv[i], &p, 0);
	    assertb(j>0 && p>argv[i]);
	    hash_put(&self->m_port_ignore, (void*)j, (void*)1);

	    debug(DEBUG_INFO, ("ignoring port %d\n", j));
	}

	// poll forever
	while(1) {
	    i = sock_raw_read(sock, buf, sizeof(buf));
	    if( i > 0 ) {
		IpTrack_poll_buf(self, buf, i);	    
	    }
	}
    } while(0);
    return 0;
}

int
main(int argc, char **argv) {
    IpTrack self;

    debug_init(DEBUG_INFO, 0, 0);
    debug(DEBUG_INFO, ("ipconntrack_t\n"));

    IpTrack_init(&self);

    return IpTrack_main(&self, argc, argv);
}
/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

