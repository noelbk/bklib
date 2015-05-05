#ifndef SOCK_SEND_FD_H_INCLUDED
#define SOCK_SEND_FD_H_INCLUDED

#ifdef __cplusplus 
extern "C" {
#endif // __cplusplus 

// send file descriptors from one process to another over an AF_LOCAL
// socket

int
sock_send_fd(int sock, char *buf, int len, int *fd, int nfds);

int
sock_recv_fd(int sock, char *buf, int len, int *fd, int *nfds);

#ifdef __cplusplus 
}
#endif // __cplusplus 


#endif // SOCK_SEND_FD_H_INCLUDED
