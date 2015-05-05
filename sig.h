#ifndef SIG_H_INCLUDED
#define SIG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern int sig_exited;

void sig_exit(int sig);

int sig_init();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SIG_H_INCLUDED */
