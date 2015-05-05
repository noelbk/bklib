#ifndef HTTP_H_INCLUDED
#define HTTP_H_INCLUDED

#include "pool.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


typedef enum {
    HTTP_OP_UNKNOWN
    ,HTTP_OP_RESPONSE
    ,HTTP_OP_GET
    ,HTTP_OP_PUT
    ,HTTP_OP_POST
} http_op_t;

typedef struct http_keyval_s {
    struct http_keyval_s *next;
    char *key;
    char *val;
} http_keyval_t;

typedef struct {
    char    *op;
    int      op_code;
    int      response_code;
    char    *response_message;
    char    *uri;
    char    *http_version;
    float    version;

    http_keyval_t *headers, *headers_last;
    size_t   header_len;  /* number of bytes from header to content */
    size_t   total_len;   /* header_len + content_len */

    char    *content_ptr; /* points to the first byte of content in
			     buf after the header if
			     http_header_parse(hdr, buf) returned 1 */
    size_t   content_len; /* Content-Length: field from header */
    char    *content_type;

    int      error_line;
    int      error_state;
    int      error_col;
    
    pool_t  *pool;
    int      free_pool;
} http_header_t;

int
http_header_init(http_header_t *http, pool_t *pool);

int
http_header_clear(http_header_t *http);

void
http_header_free(http_header_t *http);

void
http_keyval_free(pool_t *pool, http_keyval_t *root);

char*
http_keyval_get(http_keyval_t *node, char *key, http_keyval_t **found);

/* return successive values of key in header.  iter can be null.
   Equivaluent to http_keyval_get(http->headers, key, found) */
char*
http_header_get(http_header_t *http, char *key, void **iter);

char *
http_strdup(http_header_t *http, char *str, int len);

/* parses an HTTP client request or server response in buf.  Returns
   >0 if complete header, 0 if incomplete, or <0 on error.  */
int
http_header_parse(http_header_t *http, char *buf, size_t len);


/* returns the number of chars in dst */
int
http_cgi_dec(char *src, int src_len, char *dst, int dst_len);

char*
http_cgi_enc(char *src, int src_len, char *dst, int dst_len);

http_keyval_t*
http_cgi_args(pool_t *pool, char *src, int src_len);

typedef enum url_parse_err_t {
    URL_PARSE_ERR_OK=0
    ,URL_PARSE_ERR_CHAR
    ,URL_PARSE_ERR_OVER
} url_parse_err_t;

typedef enum url_parse_state_t {
    URL_PARSE_PROTO
    ,URL_PARSE_HOST
    ,URL_PARSE_PORT
    ,URL_PARSE_PATH_ARGS
    ,URL_PARSE_PATH
    ,URL_PARSE_ARGS
} url_parse_state_t;


typedef struct url_t {
    char *url;
    char *proto;
    char *user;
    char *host;
    int port;
    char *path_args;
    char *path;
    char *args_str;
    http_keyval_t *args;

    url_parse_err_t err_code;
    url_parse_state_t err_state;
    off_t err_pos;
    
    pool_t *pool;
} url_t;

int
url_parse(url_t *url, char *str);

void
url_free(url_t *url);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HTTP_H_INCLUDED
