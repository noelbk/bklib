#ifndef SERVICE_H_INCLUDED
#define SERVICE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// sets sig_exited when service is told to die
#include "sig.h"
#define service_exited sig_exited


typedef int (*service_main_t)(int argc, char **argv);

int
service_init(int argc, char **argv, service_main_t service_main);

int
service_fini();


typedef enum {
    SERVICE_START_AUTO=1
    ,SERVICE_START_MANUAL
} service_start_t;

int
service_install(char *exe_path, char *display_name, service_start_t start);

typedef enum { 
   SERVICE_CTRL_START=1
   ,SERVICE_CTRL_STOP
   ,SERVICE_CTRL_UNINSTALL
} service_control_t;
   
int
service_control(char *exe_path, service_control_t control);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SERVICE_H_INCLUDED
