#include "bklib/bkwin32.h"
#include "bklib/opt.h"
#include "bklib/debug.h"

/* call ShellEx instead of CreateProcess.  More flexible process
   opening, but doesn't retrurn a valid process id */
procid_t
proc_shellex(char *argv0, char *cmdline, char *env) {
    char cmd[4096];
    SHELLEXECUTEINFO se;
    int i, err=-1;
    procid_t pid=0;

    do {
	if( !argv0 ) {
	    opt_quoted_dec(&cmdline, cmd, sizeof(cmd), 1);
	    argv0 = cmd;
	}

	memset(&se, 0, sizeof(se));
	se.cbSize = sizeof(se);
	se.fMask = SEE_MASK_DOENVSUBST 
	    | SEE_MASK_FLAG_DDEWAIT 
	    | SEE_MASK_NOCLOSEPROCESS
	    ;
	se.lpFile = cmd;
	se.lpParameters = cmdline;
	se.nShow = SW_SHOW;
	i = ShellExecuteEx(&se);
	assertb_syserr(i!=0);

	/* todo - call GetProcessID, but that may not always work */
	pid = 1;

	CloseHandle(se.hProcess);
	err = 0;
    } while(0);

    if( err ) {
	pid = 0;
    }

    return pid;
}


