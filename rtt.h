#ifndef RTT_H_INCLUDED
#define RTT_H_INCLUDED

#include "mstime.h"
#include "netpkt.h"
#include "array.h"
#include "defutil.h"

// round-trip time estimator.  stuff packets through send_seq and
// recv_ack, then check timeout() 
typedef struct rtt {
    // make these public so you can set them
    int alpha, beta, rto_min, rto_max;
    int srtt;
    int rto;
    array rtt_list;
    int retry;
} rtt;

int rtt_init(rtt *self);

int rtt_free(rtt *self);
    
// remember when I sent a seq to be acked
void rtt_send_seq(rtt *self, tcp_seq_t seq_next, mstime_t *now);

// accept an ack and update my rtt estimate
void rtt_recv_ack(rtt *self, tcp_seq_t seq_ack, mstime_t *now);

// the same, but get seqs and acks from a tcp packet
void rtt_send_seq_pkt(rtt *self, struct netpkt *pkt, mstime_t *mst);
void rtt_recv_ack_pkt(rtt *self, struct netpkt *pkt, mstime_t *mst);

// returns milliseconds until timeout.  <0 iff time has lapsed, >0
// for ms until next timeout, 0 if no info
int rtt_timeout(rtt *self, mstime_t *mst);
int rtt_timeout_rto(rtt *self);

// call this after timing out.  I assume you're going to
// retransmit all outstanding sequence numbers.  I reset my
// timeout estimates and return the number of retries since the
// last successful recv_ack;
int rtt_retry(rtt *self);

int rtt_self_test(int argc, char **argv);

#endif // RTT_H_INCLUDED
