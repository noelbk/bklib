#include <openssl/ssl.h>
#include <openssl/err.h>
#include "ssl_util.h"
#include "warn.h"

char *
ssl_err_to_str(long ssl_err) {
    static char *s=0, buf[128];
    switch(ssl_err) {
    case SSL_ERROR_NONE: s = "SSL_ERROR_NONE"; break;
    case SSL_ERROR_SSL: s = "SSL_ERROR_SSL"; break;
    case SSL_ERROR_WANT_READ: s = "SSL_ERROR_WANT_READ"; break;
    case SSL_ERROR_WANT_WRITE: s = "SSL_ERROR_WANT_WRITE"; break;
    case SSL_ERROR_WANT_X509_LOOKUP: s = "SSL_ERROR_WANT_X509_LOOKUP"; break;
    case SSL_ERROR_SYSCALL: s = "SSL_ERROR_SYSCALL"; break;
    case SSL_ERROR_ZERO_RETURN: s = "SSL_ERROR_ZERO_RETURN"; break;
    case SSL_ERROR_WANT_CONNECT: s = "SSL_ERROR_WANT_CONNECT"; break;
    }
    if( !s ) {
	sprintf(buf, "unknown SSL_ERROR=%ld (0x%lx)", 
		ssl_err, ssl_err);
	s = buf;
    }
    return s;
}



void
warn_ssl_ret(SSL *ssl, int err) {
    warn(("SSL_get_error: %s\n",
	  ssl_err_to_str(SSL_get_error(ssl, err))));
    warn_ssl();
}

void
warn_ssl() {
    char buf[4096];
    long i;
    char *s;

    i = ERR_get_error();
    s = ERR_error_string(i, buf);
    warn(("ERR_get_error: (0x%08x %d) %s\n", i, i, s));
    //ERR_print_errors_fp(warn_get_fp());
}


int
ssl_init() {
    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();
    return 1;
}


int
ssl_fini() {
    return 1;
}

static
int
ssl_ctx_either(SSL_CTX **pctx,
	       char *cert_pem, 
	       char *key_pem, char *key_pass, 
	       char *ca_pem) {
    int i, err=-1;

    do {
	assertb(*pctx);
	SSL_CTX_set_options(*pctx, SSL_OP_ALL);
	SSL_CTX_set_mode(*pctx, SSL_MODE_AUTO_RETRY);
	if( cert_pem ) {
	    i = SSL_CTX_use_certificate_file(*pctx, cert_pem, SSL_FILETYPE_PEM);
	    assertb_ssl(i);
	}
	if( key_pass ) {
	    SSL_CTX_set_default_passwd_cb_userdata(*pctx, key_pass);
	    assertb_ssl(i);
	}
	if( key_pem ) {
	    i = SSL_CTX_use_PrivateKey_file(*pctx, key_pem, SSL_FILETYPE_PEM);
	    assertb_ssl(i);
	    i = SSL_CTX_check_private_key(*pctx);
	    assertb_ssl(i);
	}
	if( key_pass ) {
	    SSL_CTX_set_default_passwd_cb_userdata(*pctx, "");
	}
	i = SSL_CTX_set_cipher_list(*pctx, "DEFAULT");
	assertb_ssl(i);
	// SSL_CTX_set_verify(*pctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
	if( ca_pem ) {
	    i = SSL_CTX_load_verify_locations(*pctx, ca_pem, NULL);
	    assertb_ssl(i);
	}
	err = 0;
    } while(0);

    if( err ) {
	if( *pctx ) {
	    SSL_CTX_free(*pctx);
	    *pctx = 0;
	}
    }
    return err;
}

int
ssl_ctx(SSL_CTX **pctx,
	char *cert_pem, 
	char *key_pem, char *key_pass, 
	char *ca_pem) {
    *pctx = SSL_CTX_new(SSLv23_method());
    return ssl_ctx_either(pctx, cert_pem, key_pem, key_pass, ca_pem);
}

int
ssl_ctx_client(SSL_CTX **pctx,
	       char *cert_pem, 
	       char *key_pem, char *key_pass, 
	       char *ca_pem) {
    *pctx = SSL_CTX_new(SSLv23_client_method());
    return ssl_ctx_either(pctx, cert_pem, key_pem, key_pass, ca_pem);
}

int
ssl_ctx_server(SSL_CTX **pctx,
	       char *cert_pem, 
	       char *key_pem, char *key_pass,
	       char *ca_pem) {
    int i, err=-1;

    do {
	*pctx = SSL_CTX_new(SSLv23_server_method());
	i = ssl_ctx_either(pctx, cert_pem, key_pem, key_pass, ca_pem);
	assertb_ssl(!i);
	if( ca_pem ) {
	    SSL_CTX_set_client_CA_list(*pctx, SSL_load_client_CA_file(ca_pem));
	}
	err = 0;
    } while(0);

    if( err ) {
	if( *pctx ) {
	    SSL_CTX_free(*pctx);
	    *pctx = 0;
	}
    }
    return err;
}
