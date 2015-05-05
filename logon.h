#ifndef LOGON_H_INCLUDED
#define LOGON_H_INCLUDED

// check that user and pass are valid for a local account. domain can be null.
// returns 0 iff logon ok
int
logon_checkpass(char *user, char *pass, char *domain, 
		char *errbuf, int errlen);

#endif // LOGON_H_INCLUDED
