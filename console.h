/*
  console.h
  Noel Burton-Krahn <noel@burton-krahn.com>
  Dec 29, 2007

  Function definitions for selecting and reading from the console.
*/

#ifndef CONSOLE_H_INCLUDED
#define CONSOLE_H_INCLUDED

#include "bklib/fdselect.h"

typedef struct console_s* console_t;

int console_close(console_t e);

console_t console_open();

// returns 0 iff read not complete, >0 bytes read, <0 on error
int console_read(console_t console, char *buf, int n);

/* add console to fdselect */
int console_select(console_t console,
		   fdselect_t *sel,
		   int events, 
		   fdselect_fd_func func,
		   void *arg);

#endif /* CONSOLE_H_INCLUDED */
