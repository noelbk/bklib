/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "hash.h"
#include "sock.h"
#include "netpkt.h"
#include "warn.h"
#include "ipconntrack.h"
#include "defutil.h"
#include "config.h"

//---------------------------------------------------------------------
int
IpConnTcpQueue_init(IpConnTcpQueue *self) {
    memset(self, 0, sizeof(*self));
    array_init(&self->buf, 1, 0);
    array_init(&self->pkt, sizeof(void*), 0);
    return 0;
}

int
IpConnTcpQueue_clear(IpConnTcpQueue *self) {
    memset(self, 0, sizeof(*self));
    return 0;
}


int
IpConn_init(IpConn *self) {
    memset(self, 0, sizeof(*self));
    IpConnTcpQueue_init(&self->queue_server);
    IpConnTcpQueue_init(&self->queue_client);
    IpConnStats_init(&self->conn_stats_client);
    IpConnStats_init(&self->conn_stats_server);
    return 0;
};

int
IpConn_clear(IpConn *self) {
    IpConnTcpQueue_clear(&self->queue_server);
    IpConnTcpQueue_clear(&self->queue_client);
    IpConnStats_clear(&self->conn_stats_client);
    IpConnStats_clear(&self->conn_stats_server);
    memset(self, 0, sizeof(*self));
    return 0;
}

hash_val_t
IpConn_hash(const void *key) {
    return hash_str_elf((char*)key, sizeof(IpConnAddr), 0);
}

int
IpConn_cmp(const void *va, const void *vb) {
    IpConnAddr *a=(IpConnAddr*)va, *b=(IpConnAddr*)vb;
    int i = memcmp(a, b, sizeof(IpConnAddr));
    
#if 0
    debug(("IpConn_cmp=%d\n"
	   "a->proto=%d          b->proto=%d\n"
	   "a->client=%08x:%d      b->client=%08x:%d\n"
	   "a->server=%08x:%d      b->server=%08x:%d\n"
	   , i,
	   a->proto,   b->proto,
	   ntohl(a->client_addr.s_addr),   a->client_port,   
	   ntohl(b->client_addr.s_addr),   b->client_port,
	   ntohl(a->server_addr.s_addr),   a->server_port,   
	   ntohl(b->server_addr.s_addr),   b->server_port
	   ));
#endif
    return i;
}

//---------------------------------------------------------------------
int
IpConnTrack_init(IpConnTrack *self, u32 *local_addr, int naddr) {
    memset(self, 0, sizeof(*self));

    hash_init(&self->m_conn_hash, &IpConn_hash, &IpConn_cmp, 0, 0, 0);
    array_init(&self->m_local_addrs, sizeof(u32), 0);

    if( local_addr ) {
	u32 *addr;
	addr = (u32 *)array_add(&self->m_local_addrs, naddr);
	memcpy(addr, local_addr, naddr*sizeof(*local_addr));
    }
    return 0;
}


int
IpConnTrack_foreach_clear(hash_t *hash, hash_node_t *node, void *farg) {
    IpConn_clear((IpConn*)node->node_val);
    return 0;
}


int
IpConnTrack_clear(IpConnTrack *self) {
    hash_foreach(&self->m_conn_hash, IpConnTrack_foreach_clear, 0);
    hash_clear(&self->m_conn_hash);
    array_clear(&self->m_local_addrs);
    memset(self, 0, sizeof(*self));
    return 0;
}

// find or create a connection record for a packet.  Connections are
// from a client (the side which initiates the connection) to a server
// (the side which accepts the connection).  *froself->m_client is true iff
// this packet was sent from the client.  Otherwise, it's from a
// server.
//

IpConn*
IpConnTrack_track_pkt(IpConnTrack *self, struct netpkt *pkt, int track_flags) {
    IpConnAddr key;
    IpConn *conn=0;
    IpConnTcpQueue *queue=0;
    int tmp_port;
    //int err=-1;
    int i;
    int flags=0, local=0;
    hash_node_t *node=0;
    u32 tmp_addr;
    u32 *addr;
    netpkt_ip      *ip=0;
    netpkt_tcp  *tcp=0;
    netpkt_udp  *udp=0;
    //netpkt_icmp *icmp=0;
    
    ip   = pkt->pkt_ip;
    tcp  = pkt->pkt_tcp;
    udp  = pkt->pkt_udp;
    //icmp = pkt->pkt_icmp;

    if( !ip || (!tcp && !udp) ) {
	return 0;
    }

    do {
	/* check if this matches one of my local addresses */
	local = 0;
	for(i=array_count(&self->m_local_addrs), 
		addr=(u32*)array_get(&self->m_local_addrs, 0);
	    i>0; i--, addr++) {
	    if( ip->src == *addr ) {
		flags |= CONN_PKT_LOCAL_SRC;
		local = 1;
	    }
	    if( ip->dst == *addr ) {
		flags |= CONN_PKT_LOCAL_DST;
		local = 1;
	    }
	}

	// ignore non-local packets
	if( (track_flags & IPCONN_TRACK_LOCAL) && !local ) {
	    break;
	}
	
	/* The canonical hash key in self->conn_hash is
           client_ip/server_ip/client_port/server_port */
	memset(&key, 0, sizeof(key));

	/* see if this packet is from server to client.  */
	flags |= CONN_PKT_FROM_SERVER;
	key.proto    = ip->p;
	key.client_addr = ip->dst;
	key.server_addr = ip->src;
	if( tcp  ) {
	    key.client_port = ntohs(tcp->dest);
	    key.server_port = ntohs(tcp->source);
	}
	else if( udp ) {
	    key.client_port = ntohs(udp->dest);
	    key.server_port = ntohs(udp->source);
	}

	node = hash_get(&self->m_conn_hash, &key);
	if( node ) {
	    //debug(("track_pkt: packet from server conn=%p\n", node->hash_val));
	    break;
	}
	
	/* otherwise, this packet is from client to server */
	flags &= ~CONN_PKT_FROM_SERVER;
	flags |= CONN_PKT_FROM_CLIENT;
	SWAP(key.client_addr, key.server_addr, tmp_addr);
	SWAP(key.client_port, key.server_port, tmp_port);
	node = hash_get(&self->m_conn_hash, &key);
	if( node ) {
	    //debug(("track_pkt: packet from client conn=%p\n", node->hash_val));
	    break;
	}
	
	// Only start connections on a pure TCP SYN, not a SYN,ACK
	//if( tcp && (!tcp_syn(tcp) || tcp_ack(tcp)) ) {
	//    break;
	//}
	    
	/* doesn't match anything, start a new connection */
	flags |= CONN_PKT_FIRST;

	conn = (IpConn*)malloc(sizeof(*conn));
	assertb(conn);
	IpConn_init(conn);

	conn->conn_addr = key;	
	conn->conn_state = CONN_STATE_SYN;
	conn->conn_time_first = mstime();

	if( flags & CONN_PKT_LOCAL_DST ) {
	    conn->conn_flags |= CONN_LOCAL_SERVER;
	}
	if( flags & CONN_PKT_LOCAL_SRC ) {
	    conn->conn_flags |= CONN_LOCAL_CLIENT;
	}
	
	//debug(("track_pkt: new connection conn=%p\n", conn));

	node = hash_put(&self->m_conn_hash, conn, conn);
	
	//err = 0;
    } while(0);
    if( !node ) {
	return 0;
    }

    conn = (IpConn*)node->node_val;
    conn->conn_pkt_flags = flags;
    IpConnTrack_track_state(self, conn, pkt);

    // track the stream itself
    if( conn->conn_pkt_flags & CONN_PKT_FROM_SERVER ) {
	queue = &conn->queue_server;
    }
    else if( conn->conn_pkt_flags & CONN_PKT_FROM_CLIENT ) {
	queue = &conn->queue_client;
    }
    if( queue ) {
	IpConnTrack_track_stream(self, conn, queue, pkt, 
				 track_flags & IPCONN_TRACK_BUFFER);
    }
    
    return conn;
}

int
IpConnTrack_track_state(IpConnTrack *self, IpConn *conn, struct netpkt *pkt) {
    netpkt_tcp *tcp = pkt->pkt_tcp;
    IpConnStats *st=0;
    
    if( conn->conn_pkt_flags & CONN_PKT_FROM_CLIENT ) {
	st = &conn->conn_stats_client;
	if( conn->conn_flags & CONN_LOCAL_CLIENT ) {
	    conn->conn_pkt_flags |= CONN_PKT_LOCAL_SRC;
	}
	if( conn->conn_flags & CONN_LOCAL_SERVER ) {
	    conn->conn_pkt_flags |= CONN_PKT_LOCAL_DST;
	}
    }
    else if( conn->conn_pkt_flags & CONN_PKT_FROM_SERVER ) {
	st = &conn->conn_stats_server;
	if( conn->conn_flags & CONN_LOCAL_CLIENT ) {
	    conn->conn_pkt_flags |= CONN_PKT_LOCAL_DST;
	}
	if( conn->conn_flags & CONN_LOCAL_SERVER ) {
	    conn->conn_pkt_flags |= CONN_PKT_LOCAL_SRC;
	}
    }

    if( st ) {
	st->packets++;
	st->bytes += pkt->pkt_len;

	// track the next expected sequence numbers to mark duplicate
	// packets.  I won't complain if retries are different.
	pkt->pkt_tcp_seq_diff = 0;
	if( tcp ) {
	    u32 seq;

	    seq = ntohl(tcp->seq);
	    if( !st->tcp_seq_next_ok ||
		seq == st->tcp_seq_next ) {
		if( tcp_syn(tcp) ) {
		    st->tcp_seq_next = seq + 1;
		}
		else {
		    st->tcp_seq_next = seq + pkt->pkt_len;
		}
		st->tcp_seq_next_ok = 1;
	    }
	    else {
		pkt->pkt_tcp_seq_diff = 
		    tcp_seq_diff(seq, st->tcp_seq_next);
	    }
	}
    }

    conn->conn_time_prev = conn->conn_time_last;
    conn->conn_time_last = mstime();

    if( tcp ) {
	if( tcp_fin(tcp) || tcp_rst(tcp) ) {
	    conn->conn_state = CONN_STATE_FIN;
	}
    }
    
    return 0;
}

char*
IpConnAddr_fmt(IpConnAddr *addr, char *buf, int n) {
    char *buf_orig = buf;
    int i, err=-1; 

    do {
	netpkt_proto_fmt(addr->proto, buf, n);
	i = strlen(buf);
	BUF_ADD(buf, n, i);

	i = snprintf(buf, n, " server=%s:%d",
		     netpkt_ntoa(addr->server_addr, 0), 
		     addr->server_port);
	BUF_ADD(buf, n, i);
	i = snprintf(buf, n, " client=%s:%d", 
		     netpkt_ntoa(addr->client_addr, 0), 
		     addr->client_port);
	BUF_ADD(buf, n, i);
	err = 0;
    } while(0);
    return err ? 0 : buf_orig;
}


char *
IpConn_fmt(IpConn *conn, char *buf, int n) {
    char *buf_orig = buf;
    int i, err=-1;
    char *p;
    
    do {
	i = snprintf(buf, n, "connection=%p ", conn);
	BUF_ADD(buf, n, i);
       
	if( !conn ) { 
	    err = 0;
	    break; 
	}
	
	p = IpConnAddr_fmt(&conn->conn_addr, buf, n);
	i = p ? strlen(buf) : -1;
	BUF_ADD(buf, n, i);

	i = snprintf(buf, n,
		     " flags=%s%s%s"
		     " dt=%.3f"
		     " client(bytes=%ld pkt=%ld buf=%d)"
		     " server(bytes=%ld pkt=%ld buf=%d)"
		     ,(conn->conn_pkt_flags & CONN_PKT_FIRST ? "f" : "-")
		     ,(conn->conn_pkt_flags & CONN_PKT_FROM_CLIENT ? "c" : "-")
		     ,(conn->conn_pkt_flags & CONN_PKT_FROM_SERVER ? "s" : "-")
		     ,mstime() - conn->conn_time_prev 
		     ,conn->conn_stats_client.bytes
		     ,conn->conn_stats_client.packets
		     ,array_count(&conn->queue_client.buf) 
		     ,conn->conn_stats_server.bytes
		     ,conn->conn_stats_server.packets
		     ,array_count(&conn->queue_server.buf) 
		     );
	BUF_ADD(buf, n, i);

	err = 0;
    } while(0);
    return err ? 0 : buf_orig;
}

int
IpConnTrack_conn_foreach(IpConnTrack *self, hash_foreach_func func, void *arg) {
    return hash_foreach(&self->m_conn_hash, func, arg);
}

int
IpConnTrack_conn_count(IpConnTrack *self) {
    return hash_count(&self->m_conn_hash);
}

IpConn*
IpConnTrack_conn_get(IpConnTrack *self, IpConnAddr *addr) {
    hash_node_t *node;
    node = hash_get(&self->m_conn_hash, addr);
    return node ? (IpConn*)node->node_val : 0;
}


// track a tcp stream.  returns the difference between this packet's
// SEQ and the expected next SEQ.
int
IpConnTrack_track_stream(IpConnTrack *self, IpConn *conn, 
			 IpConnTcpQueue *queue, 
			 struct netpkt *pkt, 
			 int buffer_stream) {
    netpkt_tcp *tcp;
    netpkt_udp *udp;
    tcp_seq_t seq;
    char *dst, *src;
    int len, diff=0;
    
    do {
	tcp = pkt->pkt_tcp;
	udp = pkt->pkt_udp;
	src = pkt->pkt_msg;
	len = pkt->pkt_len;
	diff = 0;
	
	if( tcp ) {
	    if( tcp_ack(tcp) ) {
		queue->ack = ntohl(tcp->ack_seq);
	    }
	    queue->win = ntohs(tcp->window);

	    seq = ntohl(tcp->seq);

	    if( !queue->seq_ok ) {
		queue->seq_syn = seq;
		queue->seq =  seq;
		queue->seq_ok = 1;
		if( tcp_syn(tcp) ) {
		    queue->seq++;
		}
		else {
		    conn->conn_pkt_flags |= CONN_PKT_TCP_MISSED_SYN;
		}
	    }
	
	    diff = tcp_seq_diff(seq, queue->seq);

	    if( diff > 1 ) {
		// ignore packets (far) in the future
		conn->conn_pkt_flags |= CONN_PKT_TCP_FUTURE_SEQ;
		break;
	    }

	    src += -diff;
	    len -= -diff;
	    if( len <= 0 ) {
		// ignore past packets
		break;
	    }
	    queue->seq += len;
	}
	else if( udp ) {
	}
	
	if( buffer_stream ) {
	    dst = (char*)array_add(&queue->buf, len);
	    assertb(dst);
	    memcpy(dst, src, len);
	}
    } while(0);
    
    return diff;
}

int
IpConnStats_init(IpConnStats *self) { 
    memset(self, 0, sizeof(*self)); 
    return 0;
}

int
IpConnStats_clear(IpConnStats *self) { 
    memset(self, 0, sizeof(*self)); 
    return 0;
}

int
IpConnAddr_init(IpConnAddr *self) { 
    memset(self, 0, sizeof(*self)); 
    return 0;
}

int
IpConnAddr_clear(IpConnAddr *self) { 
    memset(self, 0, sizeof(*self)); 
    return 0;
}

int
IpConnTrack_conn_delete(IpConnTrack *self, IpConn *conn) {
    hash_remove(&self->m_conn_hash, conn);
    IpConn_clear(conn);
    free(conn);
    return 0;
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

