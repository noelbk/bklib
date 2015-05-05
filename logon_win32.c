#include "bkwin32.h"
#include <string.h>
#include "debug.h"

int
logon_checkpass(char *user, char *pass, char *domain, 
		char *errbuf, int errlen) {
    HANDLE h=0;
    int i, err=-1;
    char *p;

    do {
	*errbuf = 0;

	p = strchr(user, '@');
	if( !p ) {
	    domain = ".";
	}

	i = LogonUser(user, domain, pass, 
		      LOGON32_LOGON_INTERACTIVE,
		      LOGON32_PROVIDER_DEFAULT, 
		      &h);

	if( !i ) {
	    syserr2str(syserr(), errbuf, errlen);
	}

	debug(DEBUG_INFO, 
	      ("LogonUser user=%s pass=%s domain=%s i=%d err=%s\n",
	       user, pass, domain, i, errbuf));


	err = i == 1 ? 0 : -1;
    } while(0);
    if( h ) CloseHandle(h);
    return err;
}

