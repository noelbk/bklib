#include <errno.h>
#include <string.h>

int
syserr() {
    return errno;
}

char *
syserr2str(int syserr, char *buf, int len) {
    *buf = 0;
    strncpy(buf, strerror(syserr), len);
    return buf;
}

void
warn_sockerr() {
    warn_syserr();
}

char*
warn_err() {
    return strerror(errno);
}

static int syserr_saved;

void
syserr_save() {
    syserr_saved = errno;
}

void
syserr_restore() {
    errno = syserr_saved;
}

