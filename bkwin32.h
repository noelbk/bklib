#ifndef BKWIN32_H_INCLUDED
#define BKWIN32_H_INCLUDED

#include "bklib/config.h"

#if OS & OS_WIN32

/* !@#$ Windows strikes again.  Windows.h includes winsock.h not
   winsock2.h.  So including winsock2.h later causes
   multiple-definition errors.  So, I have to make sure to include
   winsock2.h before EVERY include of windows.h */

#if 1 // too many pragma pack errors in windows header files!
#include <winsock2.h>
// !@#$ Windows! Winsock2.h leaves a pragma pack(push, 4) behind!  To see it:
//
//   cl -E /Zi /W3 /MD /nologo -DOS=OS_WIN32 -Ibklib -Ibkxdr -Iethertap -Isqlite p2p_peer_login.c | grep 'pragma pack'
//
// 
#pragma warning(push)
#pragma warning(disable : 4161)
#pragma pack(pop)
#pragma warning(pop)
#endif

#include <windows.h>


#endif

#endif // BKWIN32_H_INCLUDED
