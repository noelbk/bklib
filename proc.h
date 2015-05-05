#ifndef PROC_H_INCLUDED
#define PROC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "configbk.h"

#if OS == OS_UNIX
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#endif

#if OS == OS_WIN32
    enum signal_t{
	SIGKILL=9
	,SIGTERM=15
	,SIGQUIT=6
    };
#endif

typedef unsigned long procid_t;

int
proc_kill(procid_t pid, int sig);

// set SIGCHLD to call waitpid() on all children
int
proc_init_reap();

// start a process running.  argv0 and env can be null. returns the
// process id or 0
procid_t
proc_start(char *argv0, char *cmdline, char *env);

// wait for a process to complete and returns its exit code.
// returns:
//    0 ok.  *status gets the exit code
//   -1 no such pid
//   -2 timed out
//   process exit code 
// Declare Function proc_wait Lib "proc.dll" (ByVal procid As Long, status As Integer, ByVal timeout As Integer) As Integer
int
proc_wait(procid_t pid, int *status, int timeout);

// shorthand for proc_wait(proc_start()).  returns exit status
int
proc_run(char *argv0, char *cmdline, char *env);

// returns true iff pid refers to a running process
int
proc_running(procid_t pid);

procid_t
proc_getpid();

// bring all windows owned by this process to the top
int
proc_foreground(procid_t pid);

// Declare Function proc_run_stdbuf Lib "proc.dll" (ByVal cmdline As String, status As Integer, ByVal timeout As Integer, ByVal stdin_buf As String, ByVal stdin_len As Integer, ByVal stdout_buf As String, ByVal stdout_len As Integer) As Integer
int
proc_run_stdbuf(char *cmdline, int *status, int timeout,
		char *stdin_buf, int stdin_len, 
		char *stdout_buf, int stdout_len);
    
typedef int proc_stdpipe_t;

// Declare Function proc_start_stdpipe_write Lib "proc.dll" (ByVal s As String, stdin_write As Long, stdout_read As Long) As Long
procid_t
proc_start_stdpipe(char *argv0,
		   char *cmdline,
		   char *envp,
		   proc_stdpipe_t *stdin_write,
		   proc_stdpipe_t *stdout_read
		   );

// Declare Function proc_stdpipe_read Lib "proc.dll" (ByVal stdout_read as Long, ByVal buf as String, buflen As Integer) As Integer
int
proc_stdpipe_read(proc_stdpipe_t stdout_read, char *buf, int);

// Declare Function proc_stdpipe_write Lib "proc.dll" (ByVal stdin_write as Long, buf as String, buflen As Integer) As Integer
int
proc_stdpipe_write(proc_stdpipe_t stdin_write, char *buf, int n);

// Declare Function proc_stdpipe_close Lib "proc.dll" (ByVal stdpipe as Long) As Integer
int
proc_stdpipe_close(proc_stdpipe_t stdin_write);

typedef unsigned long threadid_t;

threadid_t
proc_getthreadid();

/* Use ShellExecuteEx on Win32 and return bogus pid, or call proc_start on unix */
procid_t
proc_shellex(char *argv0, char *cmdline, char *env);


#ifdef __cplusplus
}
#endif

#endif // PROC_H_INCLUDED


