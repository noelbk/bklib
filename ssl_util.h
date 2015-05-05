#ifndef SSL_UTIL_H_INCLUDED
#define SSL_UTIL_H_INCLUDED

#include <openssl/ssl.h>

#ifdef __cplusplus
extern "C" {
#endif

int ssl_init();

int ssl_fini();


int
ssl_ctx_client(SSL_CTX** ctx,
	       char *cert_pem, 
	       char *priv_pem, char *priv_pass, 
	       char *ca_pem);

int
ssl_ctx_server(SSL_CTX **ctx,
	       char *cert_pem,
	       char *priv_pem, char *priv_pass,
	       char *ca_pem);

int
ssl_ctx(SSL_CTX **pctx,
	char *cert_pem, 
	char *key_pem, char *key_pass, 
	char *ca_pem);

#include "warn.h"

void warn_ssl_ret(SSL *ssl, int ret);
void warn_ssl();
#define assertb_ssl(cond) \
    if( !(cond) ) { warn(("assert_ssl: ")); warn_ssl(); break; }

char* ssl_err_to_str(long ssl_err);

#ifdef __cplusplus
}
#endif

#endif // SSL_UTIL_H_INCLUDED

