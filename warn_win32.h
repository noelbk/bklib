#include "bkwin32.h"

//typedef unsigned long HRESULT;

#define assertb_hresult(hr)	\
    if( !SUCCEEDED(hr) ) { \
        debug_hdr(DEBUG_ASSERT, __FILE__, __LINE__);  \
        warn_hresult(hr); \
        break; \
    }

void warn_hresult(HRESULT hr);
char *warn_win32_msg2str(int msg);
char *hresult2str(HRESULT hr);

extern int warn_OutputDebugString; /* flag to enable OutputDebugString echo in warn_v() */

char*
fourcc2str(int fourcc, char *buf, int len);

char *
bmi2str(BITMAPINFO *lpbiOut, char *buf, int len);

char*
driver_msg2str(int msg);

char *
icerr2str(DWORD rc, char *buf, int len);

char* winmsg2str(int msg);
