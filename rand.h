//--------------------------------------------------------------------
// rand.h - random numbers
// Noel Burton-Krahn
// Oct 25, 2003
//
// Copyright 2002 Burton-Krahn, Inc.
//
//
#ifndef RAND_H_INCLUDED
#define RAND_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int
rand_init(unsigned long seed);

unsigned long
rand_u32(unsigned long max);

double
rand_d(double max);

int
rand_bytes(char *buf, int len);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* RAND_H_INCLUDED */



