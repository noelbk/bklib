#include <stdio.h>
#include <string.h>

#include "http.h"
#include "debug.h"

char *test_url[] = {
    "http://host.com:1/path/a/b/c?k1=v1&k2=v2&k3&k4"
    ,"http://host.com:1/path/a/b/c"
    ,"http://host.com:1"
    ,"http"
    ,"file:c:/path/a/b/c?k1=v1&k2=v2&k3&k4"
    ,0
};

char *test_cgi[] = {
    "%20%01%ab=%ffsodij"
    ,"a=b&c=d"
    ,"asdf=ghj&"
    ,"&slkdjf"
    ,"iweuh&al;skdjf"
    ,0
};

char *test_reqs[] = {
    "GET /foo.cgi HTTP/1.0\r\n"
    "Host: Frazzle.org\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"

    ,
    "GET /foo.cgi HTTP/1.0\r\n"
    "Host:\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"

    ,
    "POST /foo.cgi HTTP/1.0\r\n"
    "Host: Frazzle.org\r\n"
    "Connection: keep-alive\r\n"
    "Content-Length: 26\r\n"
    "\r\n"
    "abcdefghijklmnopqrstuvwxyz"

    ,
    "HTTP/1.0 200 OK\r\n"
    "Pragma: no-cache\r\n"
    "Cache-Control: no-cache\r\n"
    "Content-Length: 26\r\n"
    "\r\n"
    "abcdefghijklmnopqrstuvwxyz"

    ,0
};

void
http_print(http_header_t *http, FILE *f) {
    http_keyval_t *node;
    fprintf(f,
	    "op=%s"
	    " uri=%s"
	    " version=%s"
	    " code=%d"
	    "\n"
	    ,http->op
	    ,http->uri
	    ,http->http_version
	    ,http->response_code
	   );
    for(node = http->headers; node; node=node->next) {
	fprintf(f, "%s: %s\n", node->key, node->val);
    }
    if( http->content_ptr ) {
	fprintf(f, "content:\n{");
	fwrite(http->content_ptr, 1, http->content_len, f);
	fprintf(f, "}\n");
    }
    fprintf(f, "\n");

}


int
main() {
    http_header_t http;
    char **req, *p;
    int i;
    http_keyval_t *node;
    
    debug_init(DEBUG_INFO, 0, 0);

    for(req=test_url; req && *req; req++) {
	url_t url;
	url_parse(&url, *req);
	printf("url: %s\n", url.url);
	printf("proto=%s\n", url.proto);
	printf("user=%s\n", url.user);
	printf("host=%s\n", url.host);
	printf("port=%d\n", url.port);
	printf("path_args=%s\n", url.path_args);
	printf("path=%s\n", url.path);
	printf("args=%s\n", url.args_str);
	for(node=url.args; node; node=node->next) {
	    printf("arg: %s=%s\n", node->key, node->val);
	}
	printf("\n");
	url_free(&url);
    }

    http_header_init(&http, 0);

    for(req=test_reqs; req && *req; req++) {
	debug(DEBUG_INFO, ("test_req=%s\n", *req));

	for(p=*req; *p; p++) {
	    //p = *req + strlen(*req);

	    i = http_header_parse(&http, *req, p - *req);
	    if( i == 0 ) {
		continue;
	    }
	    else if( i < 0 ) {
		printf("http error\n");
	    }
	    else {
		http_print(&http, stdout);
	    }
	    http_header_clear(&http);
	}
    }

    http_header_free(&http);
    return 0;
}

