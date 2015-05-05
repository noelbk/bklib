#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <unistd.h>

#include "../sqlite/sqlbk.h"
#include "ipconntrack.h"
#include "sock_raw.h"
#include "debug.h"

typedef struct {
    IpConnTrack ipconntrack;
    char *iface;

    char *db_path;
    sql_t *db;

    char *log_dir;
    int fd_open;
    int fd_max;

    char *raw_dir;
    int raw_len;
    int raw_max;
    FILE *raw_fd;
    
    mstime_t time;
    mstime_t flush_last;
    mstime_t flush_timeout;
    mstime_t conn_timeout;
} IpTrack;

typedef struct {
    array_t vpkt; /* array of IpTrackConnVargPkt */
    FILE  *fd;
    off_t  off;
    char   fname[1024];
    int    conn_id;
} IpTrackConnVarg;

typedef struct {
    IpConnTcpQueue *queue;
    int end; /* end of this packet in queue->buf */
    mstime_t time;
    int from_server;
    int raw_len;
} IpTrackConnVargPkt;

int
IpTrack_init(IpTrack *self) {
    memset(self, 0, sizeof(*self));
    IpConnTrack_init(&self->ipconntrack, 0, 0);
    self->fd_max = 5;
    self->raw_max = 100000000;
    self->conn_timeout = 30*60; /* expire connections after 30 minutes */
    self->flush_timeout = 5; /* update db every 5 secs */
    return 0;
}

int
IpTrack_createdb(IpTrack *self) {
    char *sql[] = {
	"begin transaction",

	"create table conn"
	" ("
	" conn_id      integer primary key"
	" ,time_first  float not null"
	" ,time_last   float not null"
	" ,open        integer not null default 1"
	" ,proto       varchar(8) not null"
	" ,server_addr integer not null"
	" ,server_port integer not null"
	" ,client_addr integer not null"
	" ,client_port integer not null"
	" ,pkt_count   integer default 0 not null"
	" ,msg_len     integer default 0 not null"
	" ,raw_len     integer default 0 not null"
	" )"
	,

	"create table pkt"
	" ("
	" conn_id      integer not null"
	" ,time        float not null"
	" ,from_server integer not null"
	" ,msg_off     integer not null"
	" ,msg_len     integer not null"
	" ,raw_len     integer not null"
	" )"
	,

	"create trigger conn_delete_pkt after delete on conn"
	" begin"
	"   delete from pkt where conn_id = old.conn_id;"
	" end"
	,

	"create trigger pkt_insert after insert on pkt"
	" begin"
	"   update conn set "
	"     time_last = new.time"
	"     ,pkt_count = pkt_count + 1"
	"     ,msg_len = msg_len + new.msg_len"
	"     ,raw_len = raw_len + new.raw_len"
	"     where conn_id = new.conn_id"
	"     ;"
	" end"
	,
	
	"commit transaction",

	0
    };
    int i;
    
    for(i=0; sql[i]; i++) {
	sql_exec(self->db, 0, 0, sql[i]);
	assertb_sql(self->db);
    }
    return sql[i] ? 0 : -1;
}

// set self->lru to the oldest conn with an open file handle
int
IpTrack_foreach_lru_varg(hash_t *hash, hash_node_t *node, void *arg) {
    IpConn **lru = (IpConn**)arg;
    IpConn *conn = (IpConn*)node->node_val;
    IpTrackConnVarg *varg = (IpTrackConnVarg*)conn->conn_varg;
    
    if( varg->fd
	&& (!*lru || conn->conn_time_last < (*lru)->conn_time_last) ) {
	*lru = conn;
    }
    return 0;
}

int
IpTrack_foreach_flush(hash_t *hash, hash_node_t *node, void *arg) {
    IpTrack *self = (IpTrack*)arg;
    IpConn *conn = (IpConn*)node->node_val;
    IpTrackConnVarg *varg = (IpTrackConnVarg*)conn->conn_varg;
    IpTrackConnVargPkt *vpkt;
    int i, vpkt_count, idx, off;
    char buf[4096];

    do {
	debug(DEBUG_INFO, 
	      ("flush connection: %s\n",
	       IpConn_fmt(conn, buf, sizeof(buf))));

	assertb(varg);
	vpkt_count = array_count(&varg->vpkt)-1;
	vpkt = (IpTrackConnVargPkt*)array_get(&varg->vpkt, 0);
	for(idx=0; idx<vpkt_count; idx++) {
	    off = 0;
	    for(i=idx-1; i>=0; i--) {
		if( vpkt[i].queue == vpkt[idx].queue ) {
		    off = vpkt[i].end;
		    break;
		}
	    }
	    i = vpkt[idx].end - off;
	
	    sql_exec(self->db, 0, 0, 
		     "insert into pkt"
		     " (conn_id"
		     " ,time"
		     " ,from_server"
		     " ,msg_off"
		     " ,msg_len"
		     " ,raw_len"
		     " ) values"
		     " (%u, %f, %u, %u, %u, %u)"
		     ,varg->conn_id
		     ,vpkt[idx].time, vpkt[idx].from_server
		     ,varg->off, i, vpkt->raw_len
		     );
	    assertb_sql(self->db);
	    
	    if( i > 0 ) {
		if( !varg->fd ) {
		    if( self->fd_open > self->fd_max ) {
			IpConn *lru=0;
		
			IpConnTrack_conn_foreach(&self->ipconntrack, 
						 IpTrack_foreach_lru_varg, &lru);
			assertb(lru && lru->conn_varg);

			fclose(((IpTrackConnVarg*)lru->conn_varg)->fd);
			((IpTrackConnVarg*)lru->conn_varg)->fd = 0;
			self->fd_open--;
		    }

		    varg->fd = fopen(varg->fname, "a");
		    assertb_syserr(varg->fd);
		    self->fd_open++;
		    fseek(varg->fd, 0, SEEK_END);
		    varg->off = ftell(varg->fd);
		}

		fwrite(array_get(&vpkt[idx].queue->buf, off), 1, i, varg->fd);
		varg->off += i;
	    }
	}
	array_clear(&varg->vpkt);

	array_clear(&conn->queue_server.buf);
	array_clear(&conn->queue_client.buf);
	
	if( conn->conn_closed 
	    || (self->time - conn->conn_time_last > self->conn_timeout)
	    ) {

	    debug(DEBUG_INFO, 
		  ("closing connection: %s\n",
		   IpConn_fmt(conn, buf, sizeof(buf))));

	    sql_exec(self->db, 0, 0, 
		     "update conn set open=0 where conn_id=%u",
		     varg->conn_id);
	    assertb_sql(self->db);

	    if( varg->fd ) {
		fclose(varg->fd);
		varg->fd = 0;
	    }
	    self->fd_open--;
	    free(varg);
	    conn->conn_varg = 0;
		
	    IpConnTrack_conn_delete(&self->ipconntrack, conn);
	    conn = 0;
	}
    } while(0);
    return 0;
}

int
IpTrack_pktbuf(IpTrack *self, char *pktbuf, int pktlen) {
    int i, err=-1;
    IpConn *conn=0;
    IpConnTcpQueue *queue=0;
    struct netpkt pkt;
    char buf[4096], *p;
    IpTrackConnVarg *varg;
    IpTrackConnVargPkt *vpkt;
    int from_server;

    mstime_t t;
    double ti, tf;

    do {
	t = mstime();

	// save raw packets
	if( self->raw_dir ) {
	    if( !self->raw_len >= self->raw_max ) {
		fclose(self->raw_fd);
		self->raw_fd = 0;
	    }
	    if( !self->raw_fd ) {
		snprintf(buf, sizeof(buf), "%s/%09.5f.raw", self->raw_dir, t);
		self->raw_fd = fopen(buf, "w");
		assertb_syserr(self->raw_fd);
		self->raw_len = 0;
	    }
	    // total length
	    i = pktlen + 12;
	    self->raw_len += i;
	    fwrite(&i, 1, 4, self->raw_fd);
	    
	    // time
	    i = (int)t;
	    fwrite(&i, 1, 4, self->raw_fd);
	    tf = modf(t, &ti);
	    i = (int)(tf * 1000000);
	    fwrite(&i, 1, 4, self->raw_fd);

	    // packet
	    fwrite(pktbuf, 1, pktlen, self->raw_fd);
	}

	// read and parse a packet
	netpkt_init(&pkt, pktbuf, pktlen);
	i = netpkt_parse_ether(&pkt);
	if( i ) {
	    debug(DEBUG_WARN, 
		  ("bad checksum: %s\n"
		   ,netpkt_fmt(&pkt, buf, sizeof(buf))));
	    break;
	}
	
	// not ip? Ick!
	if( !pkt.pkt_ip ) {
	    netpkt_fmt(&pkt, buf, sizeof(buf));
	    debug(DEBUG_INFO, ("not ip: %s\n", buf));

	    err = 0;
	    break;
	}
	
	// find the IP connection
	conn = IpConnTrack_track_pkt(&self->ipconntrack, &pkt, 
				     IPCONN_TRACK_BUFFER);
	if( !conn ) {
	    netpkt_fmt(&pkt, buf, sizeof(buf));
	    debug(DEBUG_WARN, ("no connection: %s\n", buf));
	    break;
	}
	
	// from server or client?
	if( conn->conn_pkt_flags & CONN_PKT_FROM_SERVER ) {
	    from_server = 1;
	    queue = &conn->queue_server;
	}
	else if( conn->conn_pkt_flags & CONN_PKT_FROM_CLIENT ) {
	    from_server = 0;
	    queue = &conn->queue_client;
	}
	else {
	    from_server = -1;
	}

	// create new connection if first
	if( conn->conn_pkt_flags & CONN_PKT_FIRST ) {
	    char b1[40], b2[40];
	    char proto[40];

	    // the protocol label
	    switch(conn->conn_addr.proto) {
	    case IPPROTO_TCP: p = "tcp"; break;
	    case IPPROTO_UDP: p = "udp"; break;
	    default: p = 0; break;
	    }
	    if( p ) {
		i = snprintf(proto, sizeof(proto), "%s", p);
	    }
	    else {
		i = snprintf(proto, sizeof(proto), "ip-%d", conn->conn_addr.proto);
	    }

	    sql_exec(self->db, 0, 0, 
		     "insert into conn"
		     " (time_first"
		     " ,time_last"
		     " ,proto"
		     " ,server_addr"
		     " ,server_port"
		     " ,client_addr"
		     " ,client_port"
		     " ) values"
		     " (%f, %f, '%q', %u, %u, %u, %u)"
		     ,conn->conn_time_last
		     ,conn->conn_time_last
		     ,proto
		     ,conn->conn_addr.server_addr
		     ,conn->conn_addr.server_port
		     ,conn->conn_addr.client_addr
		     ,conn->conn_addr.client_port
		     );
	    assertb_sql(self->db);

	    varg = (IpTrackConnVarg*)calloc(1, sizeof(*varg));
	    assertb(varg);
	    i = array_init(&varg->vpkt, sizeof(IpTrackConnVargPkt), 0);
	    assertb(i>=0);
	    varg->conn_id = sql_last_insert_rowid(self->db);
	    assertb(varg->conn_id >= 0);
	    
	    i = snprintf(varg->fname, sizeof(varg->fname),
			 "%s/%s-%s:%u-%s:%u-%u.raw",
			 self->log_dir
			 ,proto
			 ,netpkt_ntoa(conn->conn_addr.server_addr, b1)
			 ,conn->conn_addr.server_port
			 ,netpkt_ntoa(conn->conn_addr.client_addr, b2)
			 ,conn->conn_addr.client_port
			 ,varg->conn_id
			 );
	    assertb(i>0);

	    conn->conn_varg = varg;
	}
	else {
	    varg = (IpTrackConnVarg*)conn->conn_varg;
	    assertb(varg);
	}

	/* record this packet and dump it in the database later */
	vpkt = (IpTrackConnVargPkt*)array_add(&varg->vpkt, 1);
	assertb(vpkt);
	vpkt->time = conn->conn_time_last;
	vpkt->from_server = from_server;
	vpkt->queue = queue;
	vpkt->end = array_count(&queue->buf);
	vpkt->raw_len = pkt.pkt_rawlen;

	err = 0;
    } while(0);

    return err;
}

int
main(int argc, char **argv) {
    IpTrack self_buf, *self = &self_buf;
    int i, err=-1;
    struct stat st;
    char buf[4096];
    sock_t sock;

    do {
	debug_init(DEBUG_INFO, 0, 0);

	IpTrack_init(self);

	i = 1;
	if( i+4 > argc ) {
	    fprintf(stderr, "usage: %s iface db_path log_dir raw_dir\n", argv[0]);
	    exit(1);
	}
	self->iface = argv[i++];
	self->db_path = argv[i++];
	self->log_dir = argv[i++];
	self->raw_dir = argv[i++];
    
	i = stat(self->db_path, &st);
	self->db = sql_open(self->db_path, 0);
	assertb(self->db);
	if( !S_ISREG(st.st_mode) ) {
	    IpTrack_createdb(self);
	}

	// open a raw socket
	sock = sock_openraw(self->iface, 1);
	assertb_sockerr(sock>=0);

	while(1) {
	    i = sock_raw_read(sock, buf, sizeof(buf));
	    self->time = mstime();
	    if( i > 0 ) {
		IpTrack_pktbuf(self, buf, i);
	    }
	    if( self->time - self->flush_last > self->flush_timeout ) {
		IpConnTrack_conn_foreach(&self->ipconntrack, 
					 IpTrack_foreach_flush, self);
		self->flush_last = self->time;
	    }
	}
    } while(0);
    return err;
}
