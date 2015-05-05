#include "bkwin32.h"
#include <winsvc.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "service.h"
#include "sig.h"
#include "dir.h"

SERVICE_STATUS_HANDLE g_service_handle=0;
SERVICE_STATUS g_service_status;

extern int main(int argc, char **argv);

static service_main_t g_service_main = 0;

static
VOID WINAPI
ServiceHandler(DWORD fdwControl) {
    
    debug(DEBUG_INFO, ("ServiceHandler: fdwControl=%ld\n", fdwControl));

    switch(fdwControl) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
	sig_exited = 1;
	break;
    }

    g_service_status.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_service_handle, &g_service_status);
}

static
VOID WINAPI
ServiceMain(DWORD argc,   // number of arguments
	    LPTSTR *argv  //  array of argument string pointers
	    ) {
    char buf[1024];

    debug(DEBUG_INFO, ("ServiceMain: argc=%d argv[0]=%s\n", argc, argv[0]));

    do {
	memset(&g_service_status, 0, sizeof(g_service_status));
	g_service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_service_status.dwCurrentState = SERVICE_RUNNING;
	g_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	path2name(argv[0], buf, sizeof(buf));
	g_service_handle =
	    RegisterServiceCtrlHandler(buf, ServiceHandler);
	assertb_syserr(g_service_handle);

	g_service_status.dwCurrentState = SERVICE_START_PENDING;
	SetServiceStatus(g_service_handle, &g_service_status);

	g_service_status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(g_service_handle, &g_service_status);

	debug(DEBUG_INFO, ("ServiceMain: calling main\n"));
	g_service_main(argc, argv);
	debug(DEBUG_INFO, ("ServiceMain: done\n"));

	g_service_status.dwCurrentState = SERVICE_STOPPED;
	SetServiceStatus(g_service_handle, &g_service_status);
    } while(0);
}


int
service_init(int argc, char **argv, service_main_t service_main) {
    SERVICE_TABLE_ENTRY ste[2];
    int i;

    g_service_main = service_main;

    memset(ste, 0, sizeof(ste));
    ste[0].lpServiceName = argv[0];
    ste[0].lpServiceProc = ServiceMain;

    debug(DEBUG_INFO, ("service_main: calling StartServiceCtrlDispatcher\n"));
    i = StartServiceCtrlDispatcher(ste);
    debug(DEBUG_INFO, ("service_main: called StartServiceCtrlDispatcher i=%d\n", i));
    
    if( !i ) {
	i = service_main(argc, argv);
    }

    //debug(DEBUG_INFO, ("service_main: done i=%d\n", i));
    return i;
}

int
service_fini() {
    return 0;
}


int
service_install(char *exe_path, char *display_name, service_start_t startup) {
    SC_HANDLE scm=0;
    char short_name[1024];
    SC_HANDLE svc=0;
    int i, err=-1;

    do {
	scm = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	assertb_syserr(scm);

	i = -1;
	switch(startup) {
	case SERVICE_START_AUTO: i = SERVICE_AUTO_START; break;
	case SERVICE_START_MANUAL: i = SERVICE_DEMAND_START; break;
	}
	assertb(i != -1);

	path2name(exe_path, short_name, sizeof(short_name));

	/* change existing service entry or add a new one */
	svc = OpenService(scm
			  ,short_name
			  ,SERVICE_ALL_ACCESS
			  );
	if( svc ) {
	    i = DeleteService(svc);
	    assertb_syserr(i);
	    CloseServiceHandle(svc);
	    svc = 0;
	}
	
	svc = CreateService(scm                        // hSCManager
			    ,short_name		       // lpServiceName
			    ,display_name	       // lpDisplayName
			    ,SERVICE_ALL_ACCESS        // dwDesiredAccess
			    ,SERVICE_WIN32_OWN_PROCESS // dwServiceType
			    | SERVICE_INTERACTIVE_PROCESS
			    ,i			       // dwStartType
			    ,SERVICE_ERROR_NORMAL      // dwErrorControl
			    ,exe_path		       // lpBinaryPathName
			    ,0                         // lpLoadOrderGroup
			    ,0                         // lpdwTagId	
			    ,0                         // lpDependencies
			    ,0                         // lpServiceStartName
			    ,0                         // lpPassword
			    );
	assertb_syserr(svc);
	err = 0;
    } while(0);
    if( svc ) CloseServiceHandle(svc);
    if( scm ) CloseServiceHandle(scm);
    return err;
}

int
service_control(char *exe_path, service_control_t control) {
    SC_HANDLE scm=0;
    char short_name[1024];
    SC_HANDLE svc=0;
    int i, err=-1;
    SERVICE_STATUS status;
    char *argv[100];
    int   argc=0;

    do {
	scm = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
	assertb_syserr(scm);

	path2name(exe_path, short_name, sizeof(short_name));
	svc = OpenService(scm
			  ,short_name
			  ,SERVICE_ALL_ACCESS
			  );
# if 0
	if( !svc ) {
	    if( control == SERVICE_CTRL_START ) {
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);

		/* no such service, try to start the exepath by hand */
		i = CreateProcess(exe_path,              /* lpApplicationName */
				  0, 		     /* lpCommandLine */
				  0, 		     /* lpProcessAttributes */
				  0, 		     /* lpThreadAttributes */
				  FALSE, 		     /* bInheritHandles */
				  DETACHED_PROCESS       /* dwCreationFlags */
				  | NORMAL_PRIORITY_CLASS,
				  0,                     /* lpEnvironment */
				  0, 		     /* lpCurrentDirectory */
				  &si, 		     /* lpStartupInfo */
				  &pi);		     /* lpProcessInformation */
		assertb_syserr(i);
		err = 0;
	    }	
	    if( control == SERVICE_CTRL_STOP ) {
		/* todo - find process and kill it, like this:
		   
		   proc_find(exe_path);
		   proc_kill(pid, SIGTERM);
		   err = 0;
		*/
	    }
	    break;
	}
#endif

	if( !svc && 
	    (control == SERVICE_CTRL_UNINSTALL 
	    || control == SERVICE_CTRL_STOP)  ) {
	    // already uninstalled or stopped
	    err = 0;
	    break;
	}

	assertb(svc);
	switch( control ) {
	case SERVICE_CTRL_START:
	    memset(argv, 0, sizeof(*argv));
	    argv[argc++] = short_name;
	    i = StartService(svc, argc, argv);
	    if( !i && GetLastError() == ERROR_SERVICE_ALREADY_RUNNING ) {
		// no error - already running
		i = 1;
	    }
	    break;
	case SERVICE_CTRL_STOP:
	    i = ControlService(svc, SERVICE_CONTROL_STOP, &status);
	    break;
	case SERVICE_CTRL_UNINSTALL:
	    i = ControlService(svc, SERVICE_CONTROL_STOP, &status);
	    i = DeleteService(svc);
	    if( i==0 && GetLastError() == ERROR_SERVICE_MARKED_FOR_DELETE ) {
		i = 1;
	    }
	    break;
	}
	assertb_syserr(i);

	err = 0;
    } while(0);
    if( svc ) CloseServiceHandle(svc);
    if( scm ) CloseServiceHandle(scm);
    return err;
}
