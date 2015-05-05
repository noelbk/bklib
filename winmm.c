#include "bklib/bkwin32.h"
#include "bklib/winmm.h"

char*
mmsyserr2str(int err) {
    char *s = "unknown MMSYSERR";
    switch(err) {
    case MMSYSERR_ALLOCATED: s="MMSYSERR_ALLOCATED"; break;
    case MMSYSERR_BADDEVICEID: s="MMSYSERR_BADDEVICEID"; break;
    case MMSYSERR_NODRIVER: s="MMSYSERR_NODRIVER"; break;
    case MMSYSERR_NOMEM: s="MMSYSERR_NOMEM"; break;
    case WAVERR_BADFORMAT: s="WAVERR_BADFORMAT"; break;
    case WAVERR_SYNC: s="WAVERR_SYNC"; break;
    }
    return s;
}

