/*
*/
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <errno.h>
#include "bkwin32.h"

#include "opt.h"
#include "proc.h"
#include "warn.h"
#include "opt.h"

int
proc_kill(procid_t pid, int sig) {
    return 0;
}

// start a process running.  argv0 and env can be null. returns the
// process id or 0
procid_t
proc_start(char *argv0, char *cmdline, char *env) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    int i, err=-1;

    do {
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	memset(&pi, 0, sizeof(pi));
	i = CreateProcess((argv0 && *argv0) ? argv0 : 0, // lpApplicationName        
			  (cmdline && *cmdline) ? cmdline : 0, // lpCommandLine       
			  0, 		       // lpProcessAttributes 
			  0, 		       // lpThreadAttributes  
			  1,		       // bInheritHandles     
			  NORMAL_PRIORITY_CLASS,   // dwCreationFlags     
			  env,		       // lpEnvironment       
			  0,		       // lpCurrentDirectory  
			  &si,		       // lpStartupInfo       
			  &pi);		       // lpProcessInformation
	assertb(i);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	debug(DEBUG_INFO, ("proc_start cmdline=[%s] pid=%lx\n", cmdline, pi.dwProcessId));
	err = 0;
    } while(0);
    return err ? err : pi.dwProcessId;
}

// wait for a process to complete and returns its exit code.
// returns:
//    0 ok.  *status gets the exit code
//   -1 no such pid
//   -2 timed out
//   process exit code 
int
proc_wait(procid_t pid, int *status, int timeout) {
    HANDLE h;
    DWORD dw;
    int i, err=-1;

    do {
	err = -1;
	h = OpenProcess(SYNCHRONIZE, 0, pid);
	if( !h ) {
	    //warn(("proc_wait: OpenProcess pid=%lx\n", pid));
	    //warn_syserr();
	    break;
	}
	
	err = -2;
	if(timeout<0 ) {
	    timeout = INFINITE;
	}
	//warn(("wait for pid=%lx, timeout=%d\n", pid, timeout));
	dw = WaitForSingleObject(h, timeout);
	if( dw != WAIT_OBJECT_0 ) {
	    warn(("proc_wait: unexpected result 0x%lx\n",dw));
	    if (dw == WAIT_FAILED) warn_syserr();
	    break;
	}

	err = -3;
	i = GetExitCodeProcess(h, &dw);
	*status = (int)dw;
	if( !i ) break;

	err = 0;
    } while(0);
    if( h ) {
	CloseHandle(h);
    }
    return err;
}

// returns true iff pid refers to a running process
procid_t
proc_getpid() {
    return GetCurrentProcessId();
}

threadid_t
proc_getthreadid() {
    return GetCurrentThreadId();
}

// returns true iff pid refers to a running process
int
proc_running(procid_t pid) {
    HANDLE h;
    DWORD dw;
    int i;

    h = OpenProcess(PROCESS_QUERY_INFORMATION, 0, pid);
    if( !h ) {
	return 0;
    }
    i = GetExitCodeProcess(h, &dw);
    CloseHandle(h);

    if( !i || dw != STILL_ACTIVE ) {
	return 0;
    }
    
    return 1;
}

#if 0 // NO PROC_FOREGROUND 

static
BOOL CALLBACK
proc_foreground_enum(HWND hwnd,      // handle to parent window
		     LPARAM lparam   // application-defined value
		     ) {
    DWORD pid;
    if( GetWindowThreadProcessId(hwnd, &pid) && (pid == (DWORD)lparam) 
	&& IsWindowVisible(hwnd) ) {

	// debug
	//debug(("proc_foreground: hwnd=0x%x\n", hwnd));

	ShowWindow(hwnd, SW_SHOWNORMAL);
	SetForegroundWindow(hwnd);
	SetActiveWindow(hwnd);
    }
    return TRUE;
}

// bring all windows owned by this process to the top
int
proc_foreground(procid_t pid) {
    return EnumWindows(proc_foreground_enum, pid);
}


#endif // NO PROC_FOREGROUND 

procid_t
proc_start_stdpipe(char *argv0, char *cmdline, char *envp,
		   proc_stdpipe_t *stdin_write,
		   proc_stdpipe_t *stdout_read
		   ) {

    int stdin_pipe[2], stdin_bak=-1,
	stdout_pipe[2], stdout_bak=-1;
    char *p, *argv[1024];
    int pid;
    int i, err=-1;

#define READ 0
#define WRITE 1
#define STDIN _fileno(stdin)	
#define STDOUT _fileno(stdout)

    do {
	// convert cmdline to argv
	p = cmdline;
	i = opt_str2argv(&p, sizeof(argv)/sizeof(*argv)-1, argv);
	assertb(i>0);
	argv[i] = 0;

	*stdin_write = *stdout_read = 0;
	
	i = _pipe(stdin_pipe, 256, _O_BINARY | O_NOINHERIT);
	assertb_syserr(i>=0);
	*stdin_write = stdin_pipe[WRITE];

	i = _pipe(stdout_pipe, 256, _O_BINARY | O_NOINHERIT);
	assertb_syserr(i>=0);
	*stdout_read = stdout_pipe[READ];

	stdin_bak = _dup(STDIN);
	stdout_bak = _dup(STDOUT);

	_dup2(stdin_pipe[READ], STDIN);
	_close(stdin_pipe[READ]);

	_dup2(stdout_pipe[WRITE], STDOUT);
	_close(stdout_pipe[WRITE]);
	
	pid = _spawnvp(_P_NOWAIT, argv[0], argv);

	return pid;
	err = 0;
    } while(0);

    if( stdin_bak >= 0 ) {
	_dup2(stdin_bak, STDIN);
	_close(stdin_bak);
    }
    if( stdout_bak >= 0 ) {
	_dup2(stdout_bak, STDOUT);
	_close(stdout_bak);
    }

    if( err ) {
	if( *stdin_write ) {
	    _close(*stdin_write);
	    *stdin_write = 0;
	}
	if( *stdout_read ) {
	    _close(*stdout_read);
	    *stdout_read = 0;
	}
	pid = 0;
    }
    
    return pid;
}

#if 0
int
proc_run_stdbuf(char *cmdline, int *status, int timeout,
		char *stdin_buf, int stdin_len, 
		char *stdout_buf, int stdout_len) {
    proc_stdpipe_t stdin_h, stdout_h;
    procid_t pid;
    long l, i;
    
    do {
	l = -1;
	pid = proc_start_stdpipe(0, cmdline, 0, &stdin_h, &stdout_h);
	assertb(pid);
        if( stdin_buf && stdin_len > 0 ) {
            l = proc_stdpipe_write(stdin_h, stdin_buf, stdin_len);
        }
	proc_stdpipe_close(stdin_h);

        l = 0;
        while(1) {
            if( stdout_len-l-1 <= 0 ) {
                break;
            }
            if( !proc_running(pid) ) {
                break;
            }
            i = proc_stdpipe_read(stdout_h, stdout_buf+l, stdout_len-l-1);
            if( i <= 0 ) {
                break;
            }
            l += i;
        }
        if( l > 0 && l < stdout_len) {
            stdout_buf[l] = 0;
        }
	proc_stdpipe_close(stdout_h);
	proc_wait(pid, status, timeout);
    } while(0);
    return l;
}
#endif

int
proc_stdpipe_write(proc_stdpipe_t stdin_write, char *buf, int n) {
    if( n == 0 ) { n = strlen(buf); }
    return _write(stdin_write, buf, n);
}

int
proc_stdpipe_read(proc_stdpipe_t stdout_read, char *buf, int n) {
    return _read(stdout_read, buf, n);
}

int
proc_stdpipe_close(proc_stdpipe_t stdin_write) {
    return _close(stdin_write);
}

#include <lmcons.h>
#include <userenv.h>

int
proc_run_stdbuf(char *cmdline, int *status, int timeout,
		char *stdin_buf, int stdin_len, 
		char *stdout_buf, int stdout_len)
{
    HANDLE hToken=0;
    HANDLE hDuplicateToken=0;
    STARTUPINFO si;
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION pi;
    LPVOID pEnvBlock=0;
    HANDLE hStdOutputReadTmp=0, hStdOutputRead=0, hStdOutputWrite=0;
    HANDLE hStdInputRead=0, hStdInputWrite=0, hStdInputWriteTmp=0;
    TCHAR szUserName[UNLEN + 1] = {0}; // UNLEN is defined in LMCONS.H
    DWORD dwUserNameSize = sizeof(szUserName);
    int i, err = -1;
    DWORD dw;
    int total = 0;
    int process_running = 0;

    memset(&sa, 0, sizeof(sa));
    memset(&pi, 0, sizeof(pi));
    do {
	debug(DEBUG_INFO, 
              ("proc_run_stdbuf: cmdline=[%s]\n"
               ,cmdline
               ));
	
	// get the right rights if I'm impersonating a user
	i = OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_DUPLICATE, TRUE, &hToken );
        if( i && hToken ) {
            // obtain TOKEN_DUPLICATE privilege
            i = DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, &hDuplicateToken);
            assertb_syserr(i);

            i = CreateEnvironmentBlock(&pEnvBlock, hDuplicateToken, FALSE);
            assertb_syserr(i);
        }
	
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	i = CreatePipe(&hStdInputRead,  &hStdInputWriteTmp, &sa, 0);
        assertb_syserr(i);
	i = DuplicateHandle(GetCurrentProcess(),
			    hStdInputWriteTmp,
			    GetCurrentProcess(),
			    &hStdInputWrite,
			    0, 
			    FALSE, // Make it uninheritable.
			    DUPLICATE_SAME_ACCESS);
	assertb_syserr(i);
        CloseHandle(hStdInputWriteTmp);
	hStdInputWriteTmp = 0;

	i = CreatePipe(&hStdOutputReadTmp, &hStdOutputWrite, &sa, 0);
        assertb_syserr(i);
	i = DuplicateHandle(GetCurrentProcess(),
			    hStdOutputReadTmp,
			    GetCurrentProcess(),
			    &hStdOutputRead,
			    0, 
			    FALSE, // Make it uninheritable.
			    DUPLICATE_SAME_ACCESS);
	assertb_syserr(i);
        CloseHandle(hStdOutputReadTmp);
	hStdOutputReadTmp = 0;
	   

	memset(&si, 0, sizeof(si));
	si.cb		= sizeof(si);
	si.lpDesktop	= NULL;
	si.dwFlags	= STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow	= SW_SHOW;
	si.hStdInput    = hStdInputRead;
	si.hStdOutput   = hStdOutputWrite;

        if( hDuplicateToken ) {
	    i = GetUserName(szUserName, &dwUserNameSize);
	    assertb_syserr(i);
	    debug(DEBUG_INFO, ("proc_run_stdbuf: Creating process as user (%s) with hDuplicateToken\n", szUserName));

            i = CreateProcessAsUser(hDuplicateToken,
                                    NULL,
                                    (LPTSTR)cmdline, 
                                    NULL,
                                    NULL,
                                    TRUE,
                                    DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT,
                                    pEnvBlock,
                                    NULL,
                                    &si,
                                    &pi);
        }
        else {
            i = CreateProcess(NULL,
                              (LPTSTR)cmdline, 
                              NULL,
                              NULL,
                              TRUE,
                              DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT,
                              pEnvBlock,
                              NULL,
                              &si,
                              &pi);
        }
        assertb_syserr(i);

        CloseHandle(hStdInputRead);
	hStdInputRead = 0;
        CloseHandle(hStdOutputWrite);
	hStdOutputWrite = 0;

        /* use strlen if stdin_len is unspecified */
        if( stdin_buf && stdin_len < 0 ) {
            stdin_len = strlen(stdin_buf);
        }

        /* write everything to stdin */
        if( stdin_buf && stdin_len > 0 ) {
            int total = 0;
            while(total < stdin_len) {
                i = WriteFile(hStdInputWrite, stdin_buf + total, stdin_len - total, &dw, NULL);
                assertb_syserr(i);
                assertb(dw>0);
                total += (int)dw;
            }
        }

        /* read from process until it dies */
        process_running = 1;
        total = 0;
        while(total < stdout_len) {
            HANDLE handles[2];
            int nhandles = 0;
            
	    if( hStdOutputRead ) {
		if( total < stdout_len - 1 ) {
		    handles[nhandles++] = hStdOutputRead;
		}
		else {
		    /* buffer full, stop reading */
		    CloseHandle(hStdOutputRead);
		    hStdOutputRead = 0;
		}
	    }

            if( process_running ) {
                handles[nhandles++] = pi.hProcess;
            }
            
            i = WaitForMultipleObjects(nhandles, handles, FALSE, timeout);

            if( i == WAIT_TIMEOUT ) {
                break;
            }
 
            assertb_syserr(i >= (int)WAIT_OBJECT_0 && i < (int)WAIT_OBJECT_0 + nhandles);
            i -= WAIT_OBJECT_0;

            if( handles[i] == pi.hProcess ) {
                /* process died */
                process_running = 0;
                break;
            }
            else if( hStdOutputRead && handles[i] == hStdOutputRead ) {
                /* can read from stdout */

                i = ReadFile(hStdOutputRead, stdout_buf + total, stdout_len - total - 1, &dw, NULL);

		if( !i || dw == 0 ) {
		    CloseHandle(hStdOutputRead);
		    hStdOutputRead = 0;
		    continue;
		}

                assertb_syserr(i);
                assertb(dw>0);
                total += (int)dw;
                stdout_buf[total] = 0;
            }
        }

	if( process_running ) {
            WaitForSingleObject(pi.hProcess, timeout);
        }
        
        err = 0;

    } while(0);

    if( pi.hThread ) { CloseHandle(pi.hThread); }
    if( pi.hProcess ) { CloseHandle(pi.hProcess); }
    if( hStdOutputReadTmp ) { CloseHandle(hStdOutputReadTmp); }
    if( hStdOutputRead ) { CloseHandle(hStdOutputRead); }
    if( hStdOutputWrite ) { CloseHandle(hStdOutputWrite); }
    if( hStdInputRead ) { CloseHandle(hStdInputRead); }
    if( hStdInputWriteTmp ) { CloseHandle(hStdInputWriteTmp); }
    if( hStdInputWrite ) { CloseHandle(hStdInputWrite); }
    if( hToken ) { CloseHandle(hToken); }
    if( hDuplicateToken ) { CloseHandle(hDuplicateToken); }

    return err ? err : total;
}
