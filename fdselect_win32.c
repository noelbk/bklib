/*-------------------------------------------------------------------*/

void
fdselect_free_os_event(fdselect_fd *f) {
    WSAEventSelect(f->fd, (HANDLE)f->os_event, 0);
    CloseHandle((HANDLE)f->os_event);
}

int
fdselect_set_handle(fdselect_t *sel, 
		    HANDLE h,
		    int events, 
		    fdselect_fd_func func,
		    void *arg
		    ) {
    int err=-1;
    fdselect_fd *f;

    do {
	err = fdselect_set(sel, (fd_t)h, events, func, arg);
	assertb(!err);
	err = -1;
	f = fdselect_get(sel, (fd_t)h, events);
	assertb(f);
	f->os_event = (unsigned long)h;
	f->not_sock = 1;
	err = 0;
    } while(0);
    return err;
}

#define FD_READ_EVENTS (FD_READ | FD_CLOSE | FD_CONNECT | FD_ACCEPT | FD_OOB)
#define FD_WRITE_EVENTS (FD_WRITE | FD_CLOSE | FD_CONNECT | FD_ACCEPT )

int
fdselect_select_os(fdselect_t *sel, int timeout) {
    HANDLE h[FDSELECT_SET_MAX];
    int i, j, n;
    fdselect_fd *f;
    int err = -1;
    DWORD dw;

    if( timeout == FDSELECT_INFINITE ) {
	timeout = INFINITE;
    }

    do {
	f = (fdselect_fd *)array_get(&sel->fd_array, 0);
	n = array_count(&sel->fd_array);
	assertb(n<FDSELECT_SET_MAX);

	for(i=0; i<n; i++) {
	    long e = 0;
	    assertb(f[i].events);
	    if( !f[i].os_event ) {
		f[i].os_event = (unsigned long)CreateEvent(0,0,0,0);
		assertb_syserr(f[i].os_event);
		f[i].free_os_event = 1;

		e = 0;
		if( f[i].events & FDSELECT_READ ) {
		    e |= FD_READ_EVENTS;
		}
		if( f[i].events & FDSELECT_WRITE ) {
		    e |= FD_WRITE_EVENTS;
		}
		if( f[i].events & FDSELECT_ADDRESS_LIST_CHANGE ) {
		    e |= FD_ADDRESS_LIST_CHANGE;
		    dw = 0;
		    /* socket must be nonblocking */
		    sock_nonblock(f[i].fd, 1); 
		    j = WSAIoctl(f[i].fd, SIO_ADDRESS_LIST_CHANGE, 
				 0, 0, 0, 0, &dw, 0, 0);
		    assert_continue(sock_wouldblock(f[i].fd, j));
		}

		e = WSAEventSelect(f[i].fd, (HANDLE)f[i].os_event, e);
		assertb_sockerr(e==0);

		if( f[i].events & FDSELECT_WRITE ) {

		    // !@#$ windows strikes again!  WSAEventSelect
		    // does not set the event even if the socket is
		    // writable!  It only sets the event afterits
		    // writable after a first blocked call.  So, I
		    // always set the event and let the app handle the
		    // EWOULDBLOCK

		    // This is an attempt to not set the writable
		    // event if the last operation blocked.  However
		    // !@#$ windows loses the EWOULDBLOCK error code
		    // somewhere.  x always = 0 here.

		    int x=-1, l=sizeof(x);
		    l = getsockopt(f[i].fd, SOL_SOCKET, SO_ERROR, (char*)&x, &l);
		    if( x != WSAEWOULDBLOCK ) {
			SetEvent((HANDLE)f[i].os_event);
		    }
		}
	    }
	    h[i] = (HANDLE)f[i].os_event;
	}
	assertb(i==n);

	//debug_if(DEBUG_INFO) {
	//    debugf("fdselect_win32 h=[");
	//    for(i=0; i<n; i++) {
	//	debugf(" %p", h[i]);
	//    }
	//    debugf("]\n");
	//}

	if( n == 0 ) {
	    if( sel->enable_win_loop ) {
		MSG msg;
		GetMessage(&msg, 0, 0, 0);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	    }
	}
	else {
	    i = WaitForMultipleObjects(n, h, FALSE, timeout);
	    assertb_syserr(i != WAIT_FAILED);

	    if( i == WAIT_FAILED ) {
		assertb_sockerr(0);
	    }
	    else if( i == WAIT_TIMEOUT ) {
		err = 0;
		break;
	    }
	    else if( i >= (int)WAIT_OBJECT_0 && i < (int)WAIT_OBJECT_0+n ) {
		i -= WAIT_OBJECT_0;
	    }
	    else if ( i >= (int)WAIT_ABANDONED_0 && i < (int)WAIT_ABANDONED_0+n ) {
		i -= WAIT_ABANDONED_0;
	    }
	    else {
		assertb_sockerr(0);
	    }
	
	    // handle selects of non-sockets
	    if( f[i].not_sock ) {
		if( (f[i].events & FDSELECT_READ) && f[i].read_func ) {
		    f[i].read_func(sel, f[i].fd, FDSELECT_READ, f[i].read_farg);
		}
		if( (f[i].events & FDSELECT_WRITE) && f[i].write_func ) {
		    f[i].write_func(sel, f[i].fd, FDSELECT_WRITE, f[i].write_farg);
		}
		i++;
	    }

	    /* check for network events on all sockets */
	    for(; !sel->select_break && i<n; i++) {
		WSANETWORKEVENTS e;
		int fdevents = 0;
	    
		if( f[i].not_sock  || f[i].fd <= 0 ) {
		    continue;
		}

		j = WSAEnumNetworkEvents(f[i].fd, (HANDLE)f[i].os_event, &e);
		if( j != 0 ) {
		    continue;
		}
	    
		if( e.lNetworkEvents & FD_READ_EVENTS ) {
		    fdevents |= FDSELECT_READ;
		}
		if( e.lNetworkEvents & FD_WRITE_EVENTS ) {
		    fdevents |= FDSELECT_WRITE;
		}
		if( e.lNetworkEvents & FD_ADDRESS_LIST_CHANGE ) {
		    fdevents |= FDSELECT_ADDRESS_LIST_CHANGE;
		    dw = 0;
		    j  = WSAIoctl(f[i].fd, SIO_ADDRESS_LIST_CHANGE, 
				  0, 0, 0, 0, &dw, 0, 0);
		    assert_continue(sock_wouldblock(f[i].fd, j));
		}

		if( (f[i].events & (FDSELECT_READ | FDSELECT_ADDRESS_LIST_CHANGE))
		    && (f[i].events & fdevents)
		    && f[i].read_func 
		    ) {
		    f[i].read_func(sel, f[i].fd, fdevents, f[i].read_farg);
		}
		if( sel->select_break ) break;

		if( (f[i].events & FDSELECT_WRITE) && f[i].write_func 
		    && (1 || (e.lNetworkEvents & FD_WRITE_EVENTS)) ) {
		    // kludge - always call FDSELECT_WRITE functions
		    f[i].write_func(sel, f[i].fd, FDSELECT_WRITE, f[i].write_farg);
		}
		if( sel->select_break ) break;
	    }
	    if( !sel->select_break && sel->enable_win_loop ) {
		MSG msg;
		while( PeekMessage(&msg, 0, 0, 0, PM_REMOVE) ) {
		    TranslateMessage(&msg);
		    DispatchMessage(&msg);
		}
	    }
	}

	err = 0;
    } while(0);

    return err;
}

