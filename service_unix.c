int
service_init(int argc, char **argv, service_main_t service_main) {
    // TODO - handle start/stop/restart args
    return service_main ? service_main(argc, argv) : -1;
}

int
service_fini() {
    return 0;
}


int
service_install(char *exe_path, char *display_name, service_start_t startup) {
    // TODO - install in /etc/rc.d/appname
    
    return -1;
}

int
service_control(char *exe_path, service_control_t control) {
    int err=-1;
    do {
	switch( control ) {
	case SERVICE_CTRL_START:
	    break;
	case SERVICE_CTRL_STOP:
	    break;
	case SERVICE_CTRL_UNINSTALL:
	    break;
	}

	err = 0;
    } while(0);
    return err;
}
