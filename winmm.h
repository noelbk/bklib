#ifndef WINMM_H_INCLUDED
#define WINMM_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "bklib/bkwin32.h"
#include "bklib/debug.h"

#include <mmsystem.h>

#define assertb_winmm(ret) \
    assertbf((ret == MMSYSERR_NOERROR), \
        ("mmsyserr=%s\n", mmsyserr2str(ret)))

char*
mmsyserr2str(int err);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // WINMM_H_INCLUDED
