#include <sys/types.h>
#include <security/pam_appl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"


typedef struct {
    char *user;
    char *pass;
    char *errbuf;
    int   errlen;
    int  err;
} pass_conv_arg_t;


static int
pass_conv(int num_msg, const struct pam_message **msg,
	  struct pam_response **resp, void *varg) {
    int i, err=PAM_CONV_ERR;
    struct pam_response *reply=0;
    pass_conv_arg_t *arg = (pass_conv_arg_t*)varg;

    do {	
	arg->err = 0;
	*arg->errbuf = 0;
	*resp = 0;

	assertb(num_msg>0);
	reply = (struct pam_response*)calloc(num_msg, sizeof(*reply));
	assertb(reply);

	for (i=0; i<num_msg; i++) {
	    char *p=0;

	    switch (msg[i]->msg_style) {
	    case PAM_PROMPT_ECHO_OFF:
		p = arg->pass;
		break;
	    case PAM_PROMPT_ECHO_ON:
		p = arg->user;
		break;
	    case PAM_ERROR_MSG:
		arg->err = PAM_ERROR_MSG;
		snprintf(arg->errbuf, arg->errlen, "%s", msg[i]->msg);
		break;
	    case PAM_TEXT_INFO:
		break;
	    case PAM_BINARY_PROMPT:
		break;
	    default:
		break;
	    }
	    reply[i].resp = p ? strdup(p) : 0;
	    reply[i].resp_retcode = 0;
	}
	
	*resp = reply;
	err = PAM_SUCCESS;
    } while(0);

    if( err != PAM_SUCCESS ) {
	if( reply ) {
	    free(reply);
	}
    }
    if( !arg->err ) {
	arg->err = err;
    }
    return err;
}

int
logon_checkpass(char *user, char *pass, char *domain,
		char *errbuf, int errlen) {
    int i, err=-1;

    pam_handle_t *pamh=0;
    pass_conv_arg_t arg;
    struct pam_conv conv;
    
    do {
	memset(&arg, 0, sizeof(arg));
	arg.user = user;
	arg.pass = pass;
	arg.errbuf = errbuf;
	arg.errlen = errlen;

	memset(&conv, 0, sizeof(conv));
	conv.conv = pass_conv;
	conv.appdata_ptr = &arg;

	if( !domain ) { domain = "other"; }
	i = pam_start(domain, 0, &conv, &pamh);
        assertb(i == PAM_SUCCESS);

        i = pam_authenticate(pamh, 0);
	if( i != PAM_SUCCESS ) break;

	err = 0;
    } while(0);

    if( pamh ) {
	pam_end(pamh,i);
    }

    return err;
}
