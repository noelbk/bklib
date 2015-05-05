#ifndef HTTP_REQ_H_INCLUDED
#define HTTP_REQ_H_INCLUDED

#include "http.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum http_req_state_t {
    HTTP_REQ_RESOLVE
    ,HTTP_REQ_CONNECT
    ,HTTP_REQ_SEND
    ,HTTP_REQ_HEAD
    ,HTTP_REQ_BODY
    ,HTTP_REQ_EOF
    ,HTTP_REQ_ERROR
} http_req_state_t;

typedef enum http_req_error_t {
    HTTP_REQ_ERROR_NONE             = 0
    ,HTTP_REQ_ERROR_BAD_URL         = -1
    ,HTTP_REQ_ERROR_CONNECT         = -2
    ,HTTP_REQ_ERROR_BAD_RESPONSE    = -3
    ,HTTP_REQ_ERROR_INCOMPLETE      = -4
    ,HTTP_REQ_ERROR_FILE_NOT_FOUND  = -5
} http_req_error_t;

typedef struct http_req_t {
    url_t      *req_url;
    http_req_state_t req_state;
    http_req_error_t req_error;
    
    http_header_t *reply_hdr;
    int         reply_max;
    int         reply_len;
} http_req_t;

typedef int (*http_req_func_t)(http_req_t *req, char *buf, int len, void *arg);

/* does an HTTP GET request to url, and calls func until func returns nonzero. requires sock_init(). */
int
http_get(char *url, http_req_func_t func, void *farg);

char*
http_req_state_fmt(http_req_state_t state, char *buf, int len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HTTP_REQ_H_INCLUDED
