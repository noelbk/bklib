/*
  console_win32.c
  Noel Burton-Krahn <noel@burton-krahn.com>
  Dec 29, 2007

  Functions for selecting and reading from the console for win32.
*/

#include "bklib/bkwin32.h"

#include <stdio.h>

#include "bklib/debug.h"
#include "bklib/defutil.h"

struct console_s {
    HANDLE read_handle;
    int   read_pending;
    DWORD read_result;
    HANDLE read_event;
    OVERLAPPED read_overlap;
    char read_buf[4096];
};


int
console_close(console_t e) {
    struct console_s *console = (struct console_s *)e;
    if( console->read_handle && console->read_handle != INVALID_HANDLE_VALUE ) {
	CloseHandle(console->read_handle);
	console->read_handle = 0;
    }
    if( console->read_event ) {
	CloseHandle(console->read_event);
	console->read_event = 0;
    }
    free(console);
    return 0;
}

console_t
console_open() {
    console_t console = 0;
    int err=-1;
    char buf[4096];
    int reboot=0;

    do {
	console = (console_t)malloc(sizeof(*console));
	assertb(console);
	memset(console, 0, sizeof(*console));
	
	if( 1 ) {
	    console->read_handle = GetStdHandle(STD_INPUT_HANDLE);
	}
	else {
	    snprintf(buf, sizeof(buf), "CONIN$");

	    debug(DEBUG_INFO,  
		  ("console_open: CreateFile(path=%s)\n", buf));
 
	    console->read_handle = 
		CreateFile(buf,
		   GENERIC_READ | GENERIC_WRITE,
		   0,
		   0,
		   OPEN_EXISTING,
		   FILE_FLAG_OVERLAPPED | FILE_ATTRIBUTE_SYSTEM,
		   0
		   );
	}
	assertb_syserr(console->read_handle != INVALID_HANDLE_VALUE);

	console->read_pending = 0;
	console->read_event = CreateEvent(0,0,0,0);
	assertb(console->read_event);

	err = 0;
	break;
	
    } while(0);

    if( err ) {
	console_close(console);
	console = 0;
    }

    return console;
}

// returns 0 iff read not complete, >0 bytes read, <0 on error
int
console_read(console_t console, char *buf, int buflen) {
    int i, err;
    DWORD dw=0;
    int lastError;
    int n = 0;

    do {
	// finish the previous overlapped read
	assertb( console->read_pending );
	if( console->read_result ) {
	    dw = console->read_result;
	}
	else {
	    i = GetOverlappedResult(console->read_handle, &console->read_overlap, 
				    &dw, FALSE);
	    if( !i && GetLastError() == ERROR_IO_INCOMPLETE ) {
		n = 0;
		err = 0;
		break;
	    }
	    assertb_syserr(i);
	    console->read_pending = 0;
	}

	n = 0;
	while(1) {
	    i = MIN((int)dw, buflen - n);
	    memcpy(buf + n, console->read_buf, i);
	    if( i < dw ) {
		i = dw - i;
		console->read_result = 5;
		memcpy(console->read_buf, console->read_buf + i, i);
		SetEvent(console->read_event);
		break;
	    }

	    // start a new overlapped read
	    memset(&console->read_overlap, 0, sizeof(console->read_overlap));
	    ResetEvent(console->read_event);
	    console->read_overlap.hEvent = console->read_event;
	    console->read_result = 0;

	    dw = sizeof(console->read_buf);
	    debug(DEBUG_INFO,
		  ("console_read reading dw=%ld\n", dw));
	    i = ReadFile(console->read_handle, console->read_buf, sizeof(console->read_buf), 
			 &dw, &console->read_overlap);
	    lastError = GetLastError();

	    debug(DEBUG_INFO,
		  ("console_read"
		   " i=%d"
		   " dw=%ld"
		   " GetLastError()=%d"
		   " ERROR_IO_PENDING=%d"
		   " IsSet(read_event)=%d"
		   "\n"
		   , i, (long)dw, lastError, ERROR_IO_PENDING
		   , (int)(WaitForSingleObject(console->read_overlap.hEvent, 0) == WAIT_OBJECT_0)
		   ));
	    if( i ) {
		continue;
	    }
	    else if( lastError == ERROR_IO_PENDING ) {
		console->read_pending = 1;
		console->read_result = 0;
		break;
	    }
	    else {
		assertb_syserr(0);
	    }
	}
	
	err = 0;
    } while(0);

    return err ? err : n;
}

int
console_select(console_t console,
	       fdselect_t *sel,
	       int events, 
	       fdselect_fd_func func,
	       void *arg) {
    int i, err=-1;
    DWORD dw;
    int lastError;

    do {
	// start a new overlapped read
	console->read_pending = 0;
	memset(&console->read_overlap, 0, sizeof(console->read_overlap));
	console->read_overlap.hEvent = console->read_event;
	i = ReadFile(console->read_handle, console->read_buf, sizeof(console->read_buf), 
		     &dw, &console->read_overlap);
	lastError = GetLastError();

	debug(DEBUG_INFO,
	      ("console_select"
	       " i=%d"
	       " dw=%ld"
	       " GetLastError()=%d"
	       " ERROR_IO_PENDING=%d"
	       " IsSet(read_event)=%d"
	       "\n"
	       , i, (long)dw, lastError, ERROR_IO_PENDING
	       , (int)(WaitForSingleObject(console->read_overlap.hEvent, 0) == WAIT_OBJECT_0)
	       ));

	assertb_syserr(i || (!i && lastError == ERROR_IO_PENDING));
	console->read_pending = 1;
	
	err = fdselect_set_handle(sel, console->read_event, events, func, arg);
    } while(0);

    return err;
}

// int
// console_thread() {
//     while(1) {
// 	i = ReadFile(console->read_handle, console->read_buf, sizeof(console->read_buf), 
// 		     &dw, &console->read_overlap);
// 	console->read_len = dw;
// 	i = thread_queue_enqueue(console->read_queue, 0, 0);
// 	if( i == 0 ) {
// 	}
//     }
// }
// 
// int
// console_read() {
//     elt = thread_queue_dequeue(console->read_queue);
//     len = MIN(buflen, console->read_len - console->read_off);
//     memcpy(buf, console->read_buf + console->read_off, len);
//     console->read_off += len;
//     if( console->read_off >= console->read_len) {
// 	elt = thread_queue_dequeue(console->read_queue, THREAD_QUEUE_REMOVE);
//     }
//     return len;
// }
// 
