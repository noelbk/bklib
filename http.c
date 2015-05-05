#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "http.h"
#include "configbk.h"
#include "debug.h"

typedef enum {
    HTTP_STATE_ERROR=-1
    ,HTTP_STATE_INIT=1
    ,HTTP_STATE_SPACE
    ,HTTP_STATE_OP
    ,HTTP_STATE_RESPONSE_CODE
    ,HTTP_STATE_RESPONSE_MESSAGE
    ,HTTP_STATE_URI
    ,HTTP_STATE_VERSION
    ,HTTP_STATE_HEADER_BOL
    ,HTTP_STATE_HEADER
    ,HTTP_STATE_HEADER_NAME
    ,HTTP_STATE_HEADER_VAL
    ,HTTP_STATE_DONE
} http_state_t;

int
http_header_init(http_header_t *http, pool_t *pool) {
    int err=-1;
    memset(http, 0, sizeof(*http));
    do {
	if( !pool ) {
	    http->free_pool = 1;
	    http->pool = pool_new();
	    assertb(http->pool);
	}
	else {
	    http->free_pool = 0;
	    http->pool = pool;
	}
	err = 0;
    } while(0);
    
    return err;
}

int
http_header_clear(http_header_t *http) {
    pool_t *pool = http->pool; /* save the pool! */
    pool_clear(http->pool);
    memset(http, 0, sizeof(*http));
    http->pool = pool;
    return 0;
}

void
http_header_free(http_header_t *http) {
    http_header_clear(http);
    if( http->pool && http->free_pool ) {
	pool_delete(http->pool);
    }
    http->pool = 0 ;
}

char*
http_keyval_get(http_keyval_t *node, char *key, http_keyval_t **found) {
    for(; node; node=node->next ) {
	if( strcasecmp(node->key, key)==0 ) {
	    if( found ) { *found = node; }
	    return node->val;
	}
    }
    return 0;
}

char*
http_header_get(http_header_t *http, char *key, void **iter) {
    return http_keyval_get(http->headers, key, (http_keyval_t**)iter);
}


char *
http_strdup(http_header_t *http, char *str, int len) {
    char *src, *dst, *p;
    int i;
    char *dup;
	

    /* strip leading and trailing space */
    for(; len>0 && isspace(*str); str++, len--);
    for(p=str+len-1; p>str && isspace(*p); p--);
    if( p>str && !isspace(*p) ) p++;
    len = p - str;

    dup = pool_malloc(http->pool, len+1);
    if( dup ) {
	for(i=0, src=str, dst=dup; i<len; i++, src++) {
	    if( *src == '\r' ) continue;
	    *dst++ = *src;
	}
	*dst = 0;
    }
    return dup;
}

int
http_header_parse(http_header_t *http, char *buf, size_t len) {
    char *ptr, *end=buf+len, *p, *q, *word;
    int is_space, is_nl;
    int line;
    char *line_ptr;
    http_keyval_t *hdr = 0;
    http_state_t state, state_next;
    
    http_header_clear(http);

    line = 0;
    line_ptr = buf;
    state = HTTP_STATE_INIT;

#define HTTP_HEADER_PARSE_ERROR \
	    http->error_state = state; \
	    http->error_line = line; \
	    http->error_col = ptr-line_ptr; \
	    state = HTTP_STATE_ERROR; \
	    break;

    for(ptr=buf; ptr<end && state!=HTTP_STATE_DONE && state!=HTTP_STATE_ERROR; ptr++) {
	/* ignore '\r' */
	if( *ptr == '\r' ) { 
	    continue; 
	}

	is_nl    = *ptr == '\n';
	is_space = *ptr == ' ' || *ptr == '\t';
	if( is_nl ) {
	    line++;
	    line_ptr = ptr;
	}


	/*
	  debug(DEBUG_INFO, 
	  ("http_header_parse: state=%d line=%d col=%d\n",
	  state, line, ptr-line_ptr));
	*/

	switch(state) {
	case HTTP_STATE_INIT: {
	    /* skip spaces and newlines */
	    if( !is_space && !is_nl ) {
		state = HTTP_STATE_OP;
		word = ptr;
		ptr--;
	    }
	    break;
	}

	case HTTP_STATE_SPACE: {
	    /* skip spaces, not newlines */
	    if( !is_space ) {
		state = state_next;
		word = ptr;
		ptr--;
	    }
	    break;
	}

	case HTTP_STATE_OP: {
	    if( is_nl ) { HTTP_HEADER_PARSE_ERROR; }
	    if( is_space ) {
		http->op = http_strdup(http, word, ptr-word);
		if( strncmp(http->op, "HTTP/", 5)==0 ) {
		    http->http_version = http->op;
		    http->op = 0;
		    http->op_code = HTTP_OP_RESPONSE;
		    state      = HTTP_STATE_SPACE; 
		    state_next = HTTP_STATE_RESPONSE_CODE;
		}
		else {
		    if( strcmp(http->op, "GET")==0 ) { 
			http->op_code = HTTP_OP_GET;
		    }
		    else if( strcmp(http->op, "PUT")==0 ) { 
			http->op_code = HTTP_OP_PUT;
		    }
		    else if( strcmp(http->op, "POST")==0 ) { 
			http->op_code = HTTP_OP_POST;
		    }
		    else {
			http->op_code = HTTP_OP_UNKNOWN;
		    }
		    state      = HTTP_STATE_SPACE; 
		    state_next = HTTP_STATE_URI;
		}
	    }
	    break;
	}

	case HTTP_STATE_RESPONSE_CODE: {
	    if( is_space || is_nl ) {
		http->response_code = strtoul(word, &q, 0);
		if( q<=word ) { HTTP_HEADER_PARSE_ERROR; }
		state      = HTTP_STATE_SPACE; 
		state_next = HTTP_STATE_RESPONSE_MESSAGE;
		if( is_nl ) { 
		    /* no message */
		    state  = HTTP_STATE_HEADER;
		}
	    }
	    break;
	}

	case HTTP_STATE_RESPONSE_MESSAGE: {
	    if( is_nl ) {
		http->response_message = http_strdup(http, word, ptr-word);
		state = HTTP_STATE_HEADER;
	    }
	    break;
	}
	
	case HTTP_STATE_URI: {
	    if( is_space || is_nl ) {
		http->uri = http_strdup(http, word, ptr-word);
		state = HTTP_STATE_SPACE; 
		state_next = HTTP_STATE_VERSION;
	    }
	    if( is_nl ) { 
		state_next = HTTP_STATE_HEADER;
	    }
	    break;
	}

	case HTTP_STATE_VERSION: {
	    if( is_space || is_nl ) {
		http->http_version = http_strdup(http, word, ptr-word);
		if( is_nl ) {
		    state = HTTP_STATE_HEADER;
		}
		else {
		    state = HTTP_STATE_SPACE; 
		    state_next = HTTP_STATE_HEADER_BOL;
		}

	    }
	    break;
	}

	case HTTP_STATE_HEADER_BOL: {
	    if( !is_nl ) { HTTP_HEADER_PARSE_ERROR; }
	    state = HTTP_STATE_HEADER;
	    break;
	}

	case HTTP_STATE_HEADER: {
	    if( is_space && !is_nl ) {
		/* add to previous header */
		state = HTTP_STATE_HEADER_VAL;
	    }
	    else {
		/* finish off the last header */
		if( http->headers_last ) {
		    http->headers_last->val = http_strdup(http, word, ptr-word);
		}
		
		if( is_nl ) {
		    /* done! */
		    state = HTTP_STATE_DONE;
		}
		else {
		    /* start new header */
		    hdr = pool_malloc(http->pool, sizeof(*hdr));
		    if( http->headers_last ) {
			http->headers_last->next = hdr;
		    }
		    else {
			http->headers = hdr;
		    }
		    http->headers_last = hdr;

		    word = ptr;
		    state = HTTP_STATE_HEADER_NAME;
		}
	    }
	    break;
	}

	case HTTP_STATE_HEADER_NAME: {
	    if( !(isalnum(*ptr) || *ptr == '-') ) {
		if( *ptr != ':' ) { HTTP_HEADER_PARSE_ERROR; }

		http->headers_last->key = http_strdup(http, word, ptr-word);
		state      = HTTP_STATE_SPACE; 
		state_next = HTTP_STATE_HEADER_VAL;
	    }
	    break;
	}

	case HTTP_STATE_HEADER_VAL: {
	    if( is_nl ) {
		state = HTTP_STATE_HEADER;
	    }
	    break;
	}

	case HTTP_STATE_DONE: {
	    break;
	}
	    
	default: break;
	}
    }

    if( state == HTTP_STATE_ERROR ) {
	return -http->error_state;
    }

    if( state != HTTP_STATE_DONE ) {
	http_header_clear(http);
	return 0;
    }

    http->header_len = ptr-buf;
    http->total_len  = http->header_len;

    /* done! for convenience, set up content length etc */
    http->content_ptr = ptr;
    p = http_keyval_get(http->headers, "content-length", 0);
    http->content_len = 0;
    if( p ) {
	http->content_len = strtoul(p, &q, 0);
	http->total_len  += http->content_len;
    }

    http->content_type = http_keyval_get(http->headers, "content-type", 0);

    return 1;
}



/* returns the number of chars in dst */
int
http_cgi_dec(char *src, int src_len, char *dst, int dst_len) {
    char hex[3], *p;
    char *src_end = src+src_len;
    char *dst_orig=dst, *dst_end = dst+dst_len;
    
    for(; src<src_end && dst<dst_end; src++) {
	if( *src == '%' ) {
	    src++;
	    assertb(src_end-src>=2);
	    hex[0] = src[0];
	    hex[1] = src[1];
	    hex[2] = 0;
	    *dst = (char)(strtoul(hex, &p, 16) & 0xff);
	    dst += 1;
	    src += 2-1;
	}
	else {
	    *dst = *src;
	    dst += 1;
	}
    }
    if( dst >= dst_end ) {
	return 0;
    }
    *dst = 0;
    return dst - dst_orig;
}

char*
http_cgi_enc(char *src, int src_len, char *dst, int dst_len) {
    char *src_end = src+src_len;
    char *dst_orig=dst, *dst_end = dst+dst_len;
    for(; src<src_end && dst>dst_end; src++) {
	if( isalnum(*src) || *src == '-' || *src == '_' ) {
	    *dst++ = *src;
	}
	else {
	    assertb(dst_end-dst>=3);
	    snprintf(dst, 3, "%%%02x", (int)*src);
	    dst += 3;
	}
    }
    if( dst >= dst_end ) { return 0; }
    return dst_orig;
}

void
http_keyval_free(pool_t *pool, http_keyval_t *node) {
    http_keyval_t *next; 

    for(; node; node = next ) {
	next = node->next;
	pool_free(pool, node->key);
	pool_free(pool, node->val);
	pool_free(pool, node);
    }
}

http_keyval_t*
http_cgi_args(pool_t *pool, char *src, int src_len) {
    char **pkv=0;
    char *src_end;
    char *buf = 0;
    char *word;
    http_keyval_t *node=0, *node_first=0; 
    int i;
    
    do {
	if( src_len <= 0 ) {
	    src_len = strlen(src);
	}
	src_end = src+src_len;

	buf = pool_malloc(pool, src_len);
	assertb(buf);

	word = src;
	node = pool_malloc(pool, sizeof(*node));
	assertb(node);
	node_first = node;
	pkv = &node->key;

	for(; src<src_end; src++) {
	    if( !(*src == '&' || *src == '=') ) {
		/* no delimiter yet */
		continue;
	    }

	    /* decode key or val */
	    i = http_cgi_dec(word, src-word, buf, src_len);
	    assertb(i>=0);
	    *pkv = pool_malloc(pool, i+1);
	    memcpy(*pkv, buf, i);
	    (*pkv)[i] = 0;

	    if( *src == '&' ) {
		/* start new key */
		assertb(node);
		node->next = pool_malloc(pool, sizeof(*node));
		node = node->next;
		pkv = &node->key;
	    }
	    else if ( *src == '=' ) {
		/* new val for current key */
		assertb(node && !node->val);
		pkv = &node->val;
	    }
	    src++;
	    word = src;
	}
    } while(0);

    if( src < src_end ) {
	/* error! */
	http_keyval_free(pool, node_first);
	pool_free(pool, buf);
	return 0;
    }

    /* decode key or val */
    if( src > word ) {
	i = http_cgi_dec(word, src-word, buf, src_len);
	*pkv = pool_malloc(pool, i+1);
	memcpy(*pkv, buf, i);
	(*pkv)[i] = 0;
    }

    pool_free(pool, buf);
    return node_first;
}

int
url_parse_char(char **strpp, char *c) {
    char *strp = *strpp;
    char buf[3], *p=buf;
    int err = 0;

    if( *strp == '%' ) {
	strp += 1;
	buf[0] = strp[0];
	buf[1] = strp[1];
	buf[3] = 0;
	strp  += 2;
	*c = (char)strtoul(buf, &p, 16);
	if( !*c && p <= buf ) {
	    err  = 1;
	}
    }
    else {
	*c = *strp;
	strp += 1;
    }
    *strpp = strp;
    return err;
}

void
url_parse_err(url_t *url, url_parse_state_t state, url_parse_err_t code, off_t pos) {
    url->err_state = state;
    url->err_code = code;
    url->err_pos = pos;
}

#define URL_PARSE_CHAR_ERR \
		url_parse_err(url, state, URL_PARSE_ERR_CHAR, strp-str);

#define URL_PARSE_CHAR_BUF \
		err = url_parse_char(&strp, &c); \
		if( err ) { \
		    url_parse_err(url, state, URL_PARSE_ERR_CHAR, strp-str); \
		} \
		if( bufp - buf >= sizeof(buf)-1 ) { \
		    url_parse_err(url, state, URL_PARSE_ERR_OVER, strp-str); \
		    break; \
		} \
		*bufp = c; \
                bufp += 1; \
		*bufp = 0;

#define URL_PARSE_STATE(state_new) \
 		state = state_new; \
		bufp = buf; \
		*buf = 0;

int
url_parse(url_t *url, char *str) {
    url_parse_state_t state;
    char *strp = str;
    char buf[16834];
    char *bufp = buf;
    char c;
    int err;
    int done=0;

    memset(url, 0, sizeof(*url));
    url->pool = pool_new();
    pool_copy_str(url->pool, &url->url, str);

    URL_PARSE_STATE(URL_PARSE_PROTO);

    do {
	switch(state) {
	case URL_PARSE_PROTO:
	    if( !*strp || *strp == ':' ) {
		pool_copy_str(url->pool, &url->proto, buf);
	    }

	    if( !*strp ) {
		done = 1;
	    }
	    else if( *strp == ':' ) {
		strp += 1;
		URL_PARSE_STATE(URL_PARSE_PATH_ARGS);
		if( strncmp(strp, "//", 2) == 0 ) {
		    strp  += 2;
		    URL_PARSE_STATE(URL_PARSE_HOST);
		}
	    }
	    else if( isalnum(*strp) || strchr("%_-.", *strp) ) {
		URL_PARSE_CHAR_BUF;
	    }
	    else {
		URL_PARSE_CHAR_ERR;
	    }
	    break;
	    
	case URL_PARSE_HOST:
	    if( !*strp || strchr(":/", *strp) ) {
		pool_copy_str(url->pool, &url->host, buf);
	    }

	    if( !*strp ) {
		done = 1;
	    }
	    else if( *strp == '@' ) {
		strp += 1;
		pool_copy_str(url->pool, &url->user, buf);
		URL_PARSE_STATE(URL_PARSE_HOST);
	    }
	    else if( *strp == '/' ) {
		URL_PARSE_STATE(URL_PARSE_PATH_ARGS);
	    }
	    else if( *strp == ':' ) {
		strp += 1;
		URL_PARSE_STATE(URL_PARSE_PORT);
	    }
	    else if( isalnum(*strp) || strchr("%_-.", *strp) ) {
		URL_PARSE_CHAR_BUF;
	    }
	    else {
		URL_PARSE_CHAR_ERR;
	    }
	    break;

	case URL_PARSE_PORT:
	    if( !*strp || *strp == '/' ) {
		url->port = strtoul(buf, &bufp, 0);
		if( bufp<=buf ) {
		    URL_PARSE_CHAR_ERR;
		}
	    }

	    if( !*strp ) {
		done = 1;
	    }
	    else if( *strp == '/' ) {
		URL_PARSE_STATE(URL_PARSE_PATH_ARGS);
	    }
	    else if( isdigit(*strp) || *strp == '%' ) {
		URL_PARSE_CHAR_BUF;
	    }
	    else {
		URL_PARSE_CHAR_ERR;
	    }
	    break;

	case URL_PARSE_PATH_ARGS:
	    /* copy the full path and args, then parse path and args separately */
	    pool_copy_str(url->pool, &url->path_args, strp);
	    URL_PARSE_STATE(URL_PARSE_PATH);
	    break;

	case URL_PARSE_PATH:
	    if( !*strp || *strp == '?' ) {
		pool_copy_str(url->pool, &url->path, buf);
	    }

	    if( !*strp ) {
		done = 1;
	    }
	    else if( *strp == '?' ) {	
		strp += 1;
		URL_PARSE_STATE(URL_PARSE_ARGS);
	    }
	    else {
		URL_PARSE_CHAR_BUF;
	    }
	    break;

	case URL_PARSE_ARGS:
	    pool_copy_str(url->pool, &url->args_str, strp);
	    url->args = http_cgi_args(url->pool, url->args_str, strlen(url->args_str));
	    done = 1;
	    break;
	}
    } while( !done && !url->err_code );

    // default port
    if( !url->err_code ) {
	if( !url->port ) {
	    if( strcasecmp(url->proto, "http")==0 ) {
		url->port = 80;
	    }
	    else if( strcasecmp(url->proto, "https")==0 ) {
		url->port = 443;
	    }
	    else if( strcasecmp(url->proto, "ftp")==0 ) {
		url->port = 21;
	    }
	}
    }

    return url->err_code;
}

void
url_free(url_t *url) {
    pool_delete(url->pool);
    memset(url, 0, sizeof(*url));
}

