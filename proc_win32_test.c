#include <stdio.h> 
#include "bkwin32.h" 
 
#define BUFSIZE 4096 
 
HANDLE hChildStdinRd, hChildStdinWr, hChildStdinWrDup, 
    hChildStdoutRd, hChildStdoutWr, hChildStdoutRdDup, 
    hInputFile, hSaveStdin, hSaveStdout; 
 
BOOL CreateChildProcess(VOID); 
VOID WriteToPipe(VOID); 
VOID ReadFromPipe(VOID); 
VOID ErrorExit(LPTSTR); 
VOID ErrMsg(LPTSTR, BOOL); 
 
pid_t
proc_create(char **argv, file_t *stdin, file_t *stdout) { 
    int i, err=-1;

    do {

 
	if (! CreateChildProcess()) 
	    ErrorExit("Create process failed"); 
 
	// After process creation, restore the saved STDIN and STDOUT. 
 
	if (! SetStdHandle(STD_INPUT_HANDLE, hSaveStdin)) 
	    ErrorExit("Re-redirecting Stdin failed\n"); 
 
	if (! SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout)) 
	    ErrorExit("Re-redirecting Stdout failed\n"); 
 
	// Get a handle to the parent's input file. 
 
	if (argc > 1) 
	    hInputFile = CreateFile(argv[1], GENERIC_READ, 0, NULL, 
				    OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL); 
	else 
	    hInputFile = hSaveStdin; 
 
	if (hInputFile == INVALID_HANDLE_VALUE) 
	    ErrorExit("no input file\n"); 
 
	// Write to pipe that is the standard input for a child process. 
 
	WriteToPipe(); 
 
	// Read from pipe that is the standard output for child process. 
 
	ReadFromPipe(); 
 
	return 0; 
    } while(0);

} 
 
pid_t
proc_create(char **argv, file_t *stdin, file_t *stdout) { 
{ 
    PROCESS_INFORMATION pi; 
    STARTUPINFO si; 
    SECURITY_ATTRIBUTES sa; 
 
    do {
	// Set the bInheritHandle flag so pipe handles are inherited. 
	memset(&sa, 0, sizeof(sa));
	sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
	sa.bInheritHandle = TRUE; 
	sa.lpSecurityDescriptor = NULL; 
 
	// The steps for redirecting child process's STDOUT: 
	//     1. Save current STDOUT, to be restored later. 
	//     2. Create anonymous pipe to be STDOUT for child process. 
	//     3. Set STDOUT of the parent process to be write handle to 
	//        the pipe, so it is inherited by the child process. 
	//     4. Create a noninheritable duplicate of the read handle and
	//        close the inheritable read handle. 
	hSaveStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
 
	r = CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &sa, 0);
	assertb_syserr(r);
	
	// Set a write handle to the pipe to be STDOUT. 
	r = SetStdHandle(STD_OUTPUT_HANDLE, hChildStdoutWr)
	assertb_syserr(r);
 
	// Create noninheritable read handle and close the inheritable read 
	// handle. 
	r = DuplicateHandle(GetCurrentProcess(), hChildStdoutRd,
			    GetCurrentProcess(), &hChildStdoutRdDup , 0,
			    FALSE,
			    DUPLICATE_SAME_ACCESS);
	assertb_syserr(r);
	CloseHandle(hChildStdoutRd);

	// The steps for redirecting child process's STDIN: 
	//     1.  Save current STDIN, to be restored later. 
	//     2.  Create anonymous pipe to be STDIN for child process. 
	//     3.  Set STDIN of the parent to be the read handle to the 
	//         pipe, so it is inherited by the child process. 
	//     4.  Create a noninheritable duplicate of the write handle, 
	//         and close the inheritable write handle. 
 
	// Save the handle to the current STDIN. 
	hSaveStdin = GetStdHandle(STD_INPUT_HANDLE); 
 
	// Create a pipe for the child process's STDIN. 
	r = CreatePipe(&hChildStdinRd, &hChildStdinWr, &sa, 0);
	assertb(!r);
	
	// Set a read handle to the pipe to be STDIN. 	
	r = SetStdHandle(STD_INPUT_HANDLE, hChildStdinRd)
	assertb(!r);
 
	// Duplicate the write handle to the pipe so it is not inherited. 
	r = DuplicateHandle(GetCurrentProcess(), hChildStdinWr, 
			    GetCurrentProcess(), &hChildStdinWrDup, 0, 
			    FALSE,                  // not inherited 
			    DUPLICATE_SAME_ACCESS); 
	assertb(r);
 
	CloseHandle(hChildStdinWr); 
 
	// create the child process. 
	memset(&pi, 0, sizeof(pi));
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(STARTUPINFO); 
	
	// Create the child process. 
	return CreateProcess(NULL, 
			     cmdline,       // command line 
			     NULL,          // process security attributes 
			     NULL,          // primary thread security attributes 
			     TRUE,          // handles are inherited 
			     0,             // creation flags 
			     NULL,          // use parent's environment 
			     NULL,          // use parent's current directory 
			     &si,  // STARTUPINFO pointer 
			     &pi);  // receives PROCESS_INFORMATION 

    } while(0);
    r = SetStdHandle(STD_INPUT_HANDLE, hSaveStdin);
    r = SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
}
 
VOID WriteToPipe(VOID) 
{ 
    DWORD dwRead, dwWritten; 
    CHAR chBuf[BUFSIZE]; 
 
    // Read from a file and write its contents to a pipe. 
 
    for (;;) 
	{ 
	    if (! ReadFile(hInputFile, chBuf, BUFSIZE, &dwRead, NULL) || 
		dwRead == 0) break; 
	    if (! WriteFile(hChildStdinWrDup, chBuf, dwRead, 
			    &dwWritten, NULL)) break; 
	} 
 
    // Close the pipe handle so the child process stops reading. 
 
    if (! CloseHandle(hChildStdinWrDup)) 
	ErrorExit("Close pipe failed\n"); 
} 
 
VOID ReadFromPipe(VOID) 
{ 
    DWORD dwRead, dwWritten; 
    CHAR chBuf[BUFSIZE]; 
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 

    // Close the write end of the pipe before reading from the 
    // read end of the pipe. 
 
    if (!CloseHandle(hChildStdoutWr)) 
	ErrorExit("Closing handle failed"); 
 
    // Read output from the child process, and write to parent's STDOUT. 
 
    for (;;) 
	{ 
	    if( !ReadFile( hChildStdoutRdDup, chBuf, BUFSIZE, &dwRead, 
			   NULL) || dwRead == 0) break; 
	    if (! WriteFile(hSaveStdout, chBuf, dwRead, &dwWritten, NULL)) 
		break; 
	} 
} 
 
VOID ErrorExit (LPTSTR lpszMessage) 
{ 
    fprintf(stderr, "%s\n", lpszMessage); 
    ExitProcess(0); 
} 
 
// The code for the child process. 

#include "bkwin32.h" 
#define BUFSIZE 4096 
 
VOID main(VOID) 
{ 
    CHAR chBuf[BUFSIZE]; 
    DWORD dwRead, dwWritten; 
    HANDLE hStdin, hStdout; 
    BOOL r; 
 
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
    hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    if ((hStdout == INVALID_HANDLE_VALUE) || 
	(hStdin == INVALID_HANDLE_VALUE)) 
	ExitProcess(1); 
 
    for (;;) 
	{ 
	    // Read from standard input. 
	    r = ReadFile(hStdin, chBuf, BUFSIZE, &dwRead, NULL); 
	    if (! r || dwRead == 0) 
		break; 
 
	    // Write to standard output. 
	    r = WriteFile(hStdout, chBuf, dwRead, &dwWritten, NULL); 
	    if (! r) 
		break; 
	} 
} 
Platform SDK Release: August 2001  What did you think of this topic?
Let us know.  Order a Platform SDK CD Online
(U.S/Canada)   (International) 

 

