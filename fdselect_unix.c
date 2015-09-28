void
fdselect_free_os_event(fdselect_fd *f) {
}

int
fdselect_select_os(fdselect_t *sel, int timeout) {
    fd_set rfds, wfds;
    int i, nfds;
    fd_t fd_max;
    fdselect_fd *f;
    struct timeval tv, *ptv;
    int err=-1;
    do {
	sel->select_break = 0;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	fd_max = 0;
    
	f = (fdselect_fd*)array_get(&sel->fd_array, -1);
	for(i=array_count(&sel->fd_array)-1; i>=0; i--, f--) {
	    if( f->fd > fd_max ) fd_max = f->fd;
	    if( f->events & FDSELECT_READ ) {
		//debug(DEBUG_INFO, ("fdselect_select_os: fd=%d FDSELECT_READ\n", f->fd));
		FD_SET(f->fd, &rfds);
	    }
	    if( f->events & FDSELECT_WRITE ) {
		//debug(DEBUG_INFO, ("fdselect_select_os: fd=%d FDSELECT_WRITE\n", f->fd));
		FD_SET(f->fd, &wfds);
	    }
	}
	if( timeout >= 0 ) {
	    tv.tv_sec = timeout / 1000;
	    tv.tv_usec = (timeout % 1000) * 1000;
	    ptv = &tv;
	}
	else {
	    ptv = 0;
	}

	sel->select_break = 0;
	nfds = select(fd_max+1, &rfds, &wfds, 0, ptv);
	assertb_sockerr(nfds>=0);

	f = (fdselect_fd*)array_get(&sel->fd_array, -1);
	for(i=array_count(&sel->fd_array)-1; nfds>0 && i>=0; i--, f--) {
	    if( !f->events ) continue;
	    if( FD_ISSET(f->fd, &rfds) && f->read_func ) {
		f->read_func(sel, f->fd, FDSELECT_READ, f->read_farg);
	    }
	    if( sel->select_break ) break;

	    if( FD_ISSET(f->fd, &wfds) && f->write_func ) {
		f->write_func(sel, f->fd, FDSELECT_WRITE, f->write_farg);
	    }
	    if( sel->select_break ) break;
	}
	err = 0;
    } while(0);
    return err;
}
