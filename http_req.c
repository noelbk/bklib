#include <stdio.h>
#include <sys/stat.h>

#include "configbk.h"
#include "debug.h"
#include "sock.h"

#include "http_req.h"

char*
http_req_state_fmt(http_req_state_t state, char *buf, int len) {
    char *s;
    switch(state) {
    case HTTP_REQ_RESOLVE: s = "HTTP_REQ_RESOLVE"; break;
    case HTTP_REQ_CONNECT: s = "HTTP_REQ_CONNECT"; break;
    case HTTP_REQ_SEND: s = "HTTP_REQ_SEND"; break;
    case HTTP_REQ_HEAD: s = "HTTP_REQ_HEAD"; break;
    case HTTP_REQ_BODY: s = "HTTP_REQ_BODY"; break;
    case HTTP_REQ_EOF: s = "HTTP_REQ_EOF"; break;
    case HTTP_REQ_ERROR: s = "HTTP_REQ_ERROR"; break;
    }
    return s;
}

int
http_get(char *urlstr, http_req_func_t func, void *farg) {
    url_t url;
    struct sockaddr_in iaddr;
    sockaddrlen_t addrlen;
    int i, err=-1;
    sock_t sock;
    int url_parsed=0;
    char buf[16384], buf1[1024];
    size_t len;
    http_header_t http;
    http_req_t req;
    int got_header = 0;
    http_req_error_t req_err = 0;

    do {
	memset(&req, 0, sizeof(req));
	req.req_url = &url;
	req.reply_hdr = &http;

	req_err = HTTP_REQ_ERROR_BAD_URL;
	i = url_parse(&url, urlstr);
	assertb(i==0);
	url_parsed = 1;

	if( strcasecmp(url.proto, "file") == 0 ) {
	    struct stat st;	
	    FILE *f=0;
	    req_err = HTTP_REQ_ERROR_FILE_NOT_FOUND;
	    do {
		i = stat(url.path, &st);
		assertb(i==0);
		http.content_len = st.st_size;
		req.req_state = HTTP_REQ_BODY;
		f = fopen(url.path, "r");
		assertb(f);
		while(1) {
		    len = fread(buf, 1, sizeof(buf), f);
		    if( len < 0 ) {
			req_err = HTTP_REQ_ERROR_INCOMPLETE;
			break;
		    }
		    if( len <= 0 ) break;
		    err = func(&req, buf, len, farg);
		    if( err ) break;
		}
		req_err = 0;
		err = 0;
	    } while(0);
	    if( f ) {
		fclose(f);
	    }
	    break;
	}
	
	req_err = HTTP_REQ_ERROR_BAD_URL;
	assertb( strcasecmp(url.proto, "http") == 0 );

	req_err = HTTP_REQ_ERROR_CONNECT;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	assertb_sockerr(sock>=0);

	req.req_state = HTTP_REQ_RESOLVE;
	i = snprintf(buf, sizeof(buf), "resolving host %s\n", url.host);
	err = func(&req, buf, i, farg);
	if( err != 0 ) break;

	addrlen = iaddr_pack(&iaddr, inet_resolve(url.host), url.port);

	req.req_state = HTTP_REQ_CONNECT;
	i = snprintf(buf, sizeof(buf), "connecting to host %s at %s\n"
		     ,url.host
		     ,iaddr_fmt(&iaddr, buf1, sizeof(buf1))
		     );
	err = func(&req, buf, i, farg);
	if( err != 0 ) break;

	i = connect(sock, (struct sockaddr*)&iaddr, addrlen);
	assertb_sockerr(i==0);
	
	i = snprintf(buf, sizeof(buf),
		     "GET %s HTTP/1.0\r\n"
		     "Host: %s\r\n"
		     "\r\n"
		     ,url.path_args
		     ,url.host
		     );

	req.req_state = HTTP_REQ_SEND;
	err = func(&req, buf, i, farg);
	if( err != 0 ) break;

	i = sock_send_timeout(sock, buf, i, 5000);
	assertb(i>=0);

	len = 0;
	got_header = 0;
	while(1) {
	    assertb( len < sizeof(buf) );
	    i = recv(sock, buf+len, sizeof(buf)-len, 0);
	    if( i < 0 ) {
		warn_sockerr(sock);
		req_err = HTTP_REQ_ERROR_INCOMPLETE;
		break;
	    }

	    if( i == 0 ) {
		req.req_state = HTTP_REQ_EOF;
		err = func(&req, 0, 0, farg);
		break;
	    }
	    len += i;

	    if( !got_header ) {
		http_header_init(&http, 0);
		i = http_header_parse(&http, buf, len);
		if( i < 0 ) {
		    req_err = HTTP_REQ_ERROR_BAD_RESPONSE;
		    break;
		}
		if( i == 0 ) {
		    continue;
		}
		got_header = 1;

		req.reply_max = http.content_len;
		req.req_state = HTTP_REQ_HEAD;
		err = func(&req, buf, http.header_len, farg);
		if( err != 0 ) {
		    break;
		}

		len -= http.header_len;
		if( len > 0 ) {
		    memmove(buf, buf+http.header_len, len);
		}
	    }

	    if( got_header ) {
		req.reply_len += len;
		req.req_state = HTTP_REQ_BODY;
		err = func(&req, buf, len, farg);
		len = 0;
		if( err ) {
		    break;
		}
	    }
	}
	req_err = 0;
    } while(0);

    if( got_header && http.response_code != 200 ) {
	req_err = HTTP_REQ_ERROR_FILE_NOT_FOUND;
    }

    if( req_err ) {
	req.req_state = HTTP_REQ_ERROR;
	req.req_error = req_err;
	err = func(&req, buf, len, farg);
    }

    if( url_parsed ) {
	url_free(&url);
    }

    if( got_header ) {
	http_header_free(&http);
    }
    return err;
}
