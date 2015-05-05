/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

/* 
   ipconntrack.h
   Noel Burton-Krahn
   Mar 18, 2002

   A class for tracking TCP connections from raw packets.
   
*/

#ifndef IPCONNTRACK_H_INCLUDED
#define IPCONNTRACK_H_INCLUDED

#include "hash.h"
#include "netpkt.h"
#include "mstime.h"

// conn->conn_state
enum {
    CONN_STATE_SYN = 1,
    CONN_STATE_ESTAB    = 2,
    CONN_STATE_FIN  = 3,
};

// conn->conn_flags
enum {
    CONN_LOCAL_SERVER     =1<<0,
    CONN_LOCAL_CLIENT     =1<<1,
};

// conn->conn_pkt_flags
enum {
    CONN_PKT_FROM_CLIENT  =1<<0,
    CONN_PKT_FROM_SERVER  =1<<1,
    CONN_PKT_FIRST        =1<<2,
    CONN_PKT_LOCAL_SRC    =1<<5,
    CONN_PKT_LOCAL_DST    =1<<6,

    CONN_PKT_TCP_MISSED_SYN = 1<<12,
    CONN_PKT_TCP_FUTURE_SEQ = 1<<13,
    CONN_PKT_TCP_MASK = (CONN_PKT_TCP_MISSED_SYN | CONN_PKT_TCP_FUTURE_SEQ),
    
};


typedef struct _IpConnStats {
    u32 tcp_seq_next_ok;
    u32 tcp_seq_next;
    unsigned long  packets;
    unsigned long  bytes;
} IpConnStats;

int IpConnStats_init(IpConnStats *self);
int IpConnStats_clear(IpConnStats *self);


/* grouped in a struct to make a hash key */
typedef struct _IpConnAddr {
    
    int            proto;
    u32            client_addr;
    u32            server_addr;
    int            client_port;
    int            server_port;
} IpConnAddr;

int IpConnAddr_new(IpConnAddr *self);
char *IpConnAddr_fmt(IpConnAddr *self, char *buf, int len);

typedef struct _IpConnTcpQueue {
    int       seq_ok;
    tcp_seq_t ack;     // the last ack I sent
    tcp_seq_t seq;     // the next expected seq iff seq_ok
    tcp_seq_t seq_syn; // the first seq iff seq_ok
    int       win;     // the last window
    array_t     buf;
    array_t     pkt;
    int       seq_fin; // the seq at FIN
    int       state; // SYN, CONNECTED, CLOSED, etc
} IpConnTcpQueue;

int IpConnTcpQueue_init(IpConnTcpQueue *self);
int IpConnTcpQueue_clear(IpConnTcpQueue *self);

typedef struct _IpConn {
    IpConnAddr conn_addr;
    
    /* connection state and flags */
    int            conn_closed;
    int            conn_state;
    int            conn_flags;

    /* flags for the last packet */
    int            conn_pkt_flags;

    /* stats */
    mstime_t conn_time_first;
    mstime_t conn_time_last;
    mstime_t conn_time_prev;
    IpConnStats conn_stats_client;
    IpConnStats conn_stats_server;

    /* queues for collecting and comparing packets between master and
       backup in hotswap.cpp */

    /* TCP state at server and client */
    IpConnTcpQueue queue_server;
    IpConnTcpQueue queue_client;

    /* a place to stash extra data.  You must free it */
    void *conn_userdata;

} IpConn; 

int IpConn_new(IpConn *self);
int IpConn_clear(IpConn *self);
char *IpConn_fmt(IpConn *self, char *buf, int len);


hash_val_t IpConn_hash(const void *a);
int    IpConn_cmp(const void *a, const void *b);


enum {
    IPCONN_TRACK_LOCAL = 1,  // only track local packets
    IPCONN_TRACK_BUFFER = 2, // save stream buffers
};

typedef struct _IpConnTrack {
    hash_t   m_conn_hash;
    array_t  m_local_addrs;
} IpConnTrack;

int IpConnTrack_new(IpConnTrack *self);
int IpConnTrack_clear(IpConnTrack *self);
int IpConnTrack_init(IpConnTrack *self, u32 *local_addr, int naddr);

IpConn *IpConnTrack_track_pkt(IpConnTrack *self, struct netpkt *pkt, int track_flags);
int IpConnTrack_track_state(IpConnTrack *self, IpConn *conn, struct netpkt *pkt);
int IpConnTrack_track_stream(IpConnTrack *self, IpConn *conn, IpConnTcpQueue *queue, 
			 struct netpkt *pkt, int buffer_stream);

int IpConnTrack_conn_delete(IpConnTrack *self, IpConn *conn);
int IpConnTrack_conn_foreach(IpConnTrack *self, hash_foreach_func func, void *arg);
int IpConnTrack_conn_count(IpConnTrack *self);
IpConn *IpConnTrack_conn_get(IpConnTrack *self, IpConnAddr *addr);

#endif // IPCONNTRACK_H_INCLUDED
/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

