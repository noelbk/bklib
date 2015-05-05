#include "rtt.h"
#include "warn.h"

#define RTT_DEBUG __FILE__

typedef struct rtt_time {
    tcp_seq_t seq; // the next expected ack
    mstime_t time; // the time of receipt
} rtt_time;

int
rtt_init(rtt *rtt) {
    memset(rtt, 0, sizeof(*rtt));
    array_init(&rtt->rtt_list, sizeof(rtt_time), 0);
    rtt->alpha = 800;
    rtt->beta = 2000;
    rtt->rto_min = 1000;
    rtt->rto_max = 60000;
    
    rtt->srtt = 0;
    rtt->rto = 0;
    rtt->retry = 0;
    return 0;
}

int
rtt_free(rtt *rtt) {
    array_clear(&rtt->rtt_list);
    return 0;
}

void
rtt_send_seq(rtt *self, tcp_seq_t seq_next, mstime_t *mst) {
    rtt_time *rtt;
    do {
	rtt = (rtt_time*)array_get(&self->rtt_list, -1);

	// ignore unless this comes after the list of seqs I'm waiting to ack
	if( rtt && tcp_seq_diff(rtt->seq, seq_next)>=0 ) {
	    break;
	}

	rtt = (rtt_time*)array_add(&self->rtt_list, 1);
	rtt->time = *mst;
	rtt->seq = seq_next;

	// debug
	debug(DEBUG_INFO, ("send_seq: rtt=%p seq=%lu\n", rtt, seq_next));

    } while(0);
}

// track round-trip times.  pkt contains a seq which I expect to be acked
void
rtt_send_seq_pkt(rtt *self, struct netpkt *pkt, mstime_t *mst) {
    netpkt_tcp *tcp = pkt->pkt_tcp;
    tcp_seq_t seq, seq_next;
    int seq_next_diff;

    do {
	if( !tcp ) break;

	seq = ntohl(tcp->seq);
	seq_next = tcp_seq_next(pkt);
	seq_next_diff = tcp_seq_diff(seq_next, seq);
	// ignore if this contains no new seqence numbers to ack
	if( seq_next_diff<=0 ) {
	    break;
	}

	rtt_send_seq(self, seq_next, mst);
    } while(0);
}

void
rtt_recv_ack(rtt *self, tcp_seq_t ack, mstime_t *mst) {
    int idx, count;
    rtt_time *rtt=0, *rtt_next;
    int ms;
    
    do {
	// find the latest seq which is acked
	count = array_count(&self->rtt_list);
	rtt_next = (rtt_time*)array_get(&self->rtt_list, 0);
	for(idx=0; idx<count; idx++, rtt_next++) {
	    if( tcp_seq_diff(rtt_next->seq, ack) > 0 ) {
		break;
	    }
	    rtt = rtt_next;
	}
	if( !rtt || idx == 0 ) {
	    break;
	}

	self->retry = 0;
	
	ms = mstime_sub(mst, &self->time);

	// debug
	debug(DEBUG_INFO, 
	      ("recv_ack: ack=%lu idx=%d rtt=%p self->seq=%lu self->time=%lu mst=%lu ms=%d\n", 
	       ack, idx, rtt, rtt->seq, rtt->time, mst, ms));

	// forget about the seqs just acked
	array_remove_idx(&self->rtt_list, 0, idx);
	
	// update the smoothed round-trip time and timeout
	if( !self->srtt ) {
	    self->srtt = ms;
	}
	else {
	    self->srtt = ((self->alpha * self->srtt) 
		      + (1000-self->alpha) * ms) / 1000;
	    ms = MAX(self->rto_min, self->beta * self->srtt / 1000);
	    ms = MIN(ms, self->rto_max);
	    self->rto = ms;
	}
    } while(0);
}


// track round-trip times.  pkt contains an ack for the last send_seq
void
rtt_recv_ack_pkt(rtt *self, struct netpkt *pkt, mstime_t *mst) {
    if( pkt->pkt_tcp ) {
	rtt_recv_ack(self, htonl(pkt->pkt_tcp->ack_seq), mst);
    }
}

// returns <0 iff I timed out waiting for an ack
int
rtt_timeout(rtt *self, mstime_t *mst) {
    rtt_time *rtt;
    rtt = (rtt_time *)array_get(&self->rtt_list, 0);

    debug(DEBUG_INFO, ("rtt::timeout: count=%d mst=%lu ", array_count(&self->rtt_list), mst));

    if( !rtt ) {
	return 0;
    }
    return mstime_sub(&rtt->time, mst) + rtt_timeout_rto(self);
}

int
rtt_retry(rtt *self) {
    array_clear(&self->rtt_list);
    return self->retry++;
}

int
rtt_timeout_rto(rtt *self) {
    return self->rto ? self->rto : self->rto_max;
}

int
rtt_self_test(int argc, char **argv) {
    rtt rtt_self;
    rtt *self;
    mstime_t mst;
    int i;
    tcp_seq_t seq;    

    
    rtt_init(&rtt_self);
    self = &rtt_self;

    mstime_set(&mst, 0, 0);

    for(i=0; i<10; i++) {
	mstime_add_ms(&mst, 0, 1000);
	seq = i*5;
	rtt_send_seq(self, seq, &mst);
	printf("send_seq(%d) at %lu srtt=%d rto=%d timeout=%d\n", 
	       seq, mst, self->srtt, self->rto, rtt_timeout(self, &mst));
    }
    
    for(i=0; i<8; i++) {
	mstime_add_ms(&mst, 0, 1000);
	seq = i*5;
	rtt_recv_ack(self, seq, &mst);
	printf("recv_ack(%d) at %lu srtt=%d rto=%d timeout=%d\n", 
	       seq, mst, rtt.self->srtt, rtt.self->rto, rtt.timeout(&mst));
    }

    for(i=0; i<10; i++) {
	mstime_add_ms(&mst, 0, 1000);
	printf("at %lu srtt=%d rto=%d timeout=%d\n", 
	       mst, self->srtt, self->rto, rtt_timeout(self, &mst));
    }


    return 0;
}


