/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

//--------------------------------------------------------------------
// mstime.h - return double seconds
// Noel Burton-Krahn
// Jan 24, 2002
//
// Copyright 2002 Burton-Krahn, Inc.
//
//
#ifndef MSTIME_H_INCLUDED
#define MSTIME_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef double mstime_t;

mstime_t mstime();

char *mstime_fmt(mstime_t t, char *buf, int len);

char *mstime_fmt2(mstime_t t, const char *fmt, char *buf, int len);

#define MSTIME_INFINITE FP_INFINITE

mstime_t
mstime_gmt_offset(mstime_t mst);

void mssleep(double secs);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif // MSTIME_H_INCLUDED
/*
  Noel Burton-Krahn <noel@burton-krahn.com>
  Nov 14, 2002
  Copyright 2002, Burton-Krahn, Inc

  This code is provided for evaluation purposes only, and no portion
  of it may be used, reproduced, or redistributed without the express
  written consent of Burton-Krahn, Inc.

  This code has no warranty of any kind.
*/

