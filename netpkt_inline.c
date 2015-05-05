#include "configbk.h"

static inline 
u16
netpkt_adjust_cksum(u16 oldsum, u32 old, u32 new)
{
	u32 diff[2] = { ~old, new };
	int sum;

	sum = oldsum ^ 0xFFFF;
	sum += netpkt_cksum_in(diff, sizeof(diff));
        sum = (sum & 0xffff) + (sum >> 16);
        sum = (sum & 0xffff) + (sum >> 16);
        return ~(sum);
}

/* rewrite the src and dst addr in an ip packet, and incrementally
   update the checksums without recomputing */
static inline
void
netpkt_adjust_ip_src_dst(struct netpkt *pkt, u32 src, u32 dst)
{
	u16 *checkp=0;

	if( pkt->pkt_tcp ) {
	    checkp = &pkt->pkt_tcp->check;
	}
	else if( pkt->pkt_udp ) {
	    checkp = &pkt->pkt_udp->check;
	}
	if( checkp ) {
	    *checkp = netpkt_adjust_cksum(*checkp, pkt->pkt_ip->src, src);
	    *checkp = netpkt_adjust_cksum(*checkp, pkt->pkt_ip->dst, dst);
	    if( !*checkp && pkt->pkt_udp ) *checkp = 0xFFFF;
	}
	pkt->pkt_ip->src = src;
	pkt->pkt_ip->dst = dst;
	pkt->pkt_ip->sum = netpkt_cksum_ip(pkt);

}
