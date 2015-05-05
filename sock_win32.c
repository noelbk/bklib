#include "warn.h"
#include "sock.h"

int sock_init() {
    WORD v = MAKEWORD(2,0);
    WSADATA d;
    int i;

    i = WSAStartup(v, &d);

    return 1;
}

int
sock_fini() {
    WSACleanup();
    return 1;
}

int sock_errno(sock_t sock) {
    return WSAGetLastError();

    // BUG: getsockopt(SO_ERROR) returns 0 when it should return
    // EWOULDBLOCK.  See kmulti.cpp
    //
    //int i=-1, l=sizeof(i);
    //getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&i, &l);
    //return i;
}

int
sock_wouldblock(sock_t sock, int ret) {
    int i = sock_errno(sock);

    //debug(("sock_wouldblock: sock_errno=%d EWOULDBLOCK=%d\n", 
    //i, WSAEWOULDBLOCK));

    return i == WSAEWOULDBLOCK || i == WSAEALREADY;
}

int
sock_isconn(sock_t sock, int ret) {
    int i = sock_errno(sock);

    //debug(("sock_wouldblock: sock_errno=%d EWOULDBLOCK=%d\n", 
    //i, WSAEWOULDBLOCK));

    return i == WSAEISCONN;
}

int
sock_nonblock(sock_t sock, int nonblock) {
    return ioctlsocket(sock, FIONBIO, &nonblock);
}

int
sock_close(sock_t sock) {
    return closesocket(sock);
}

int
sock_pair(sock_t sock[2]) {
    int i, err=-1;
    sock_t s;
    struct sockaddr_in addr;

    do {
	sock[0] = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
	assertb(sock_valid(sock[0]));
	sock[1] = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
	assertb(sock_valid(sock[1]));
	
	// bind and listen sock[0]
	i = iaddr_pack(&addr, INADDR_LOOPBACK, 0);
	i = bind(sock[0], (struct sockaddr*)&addr, i);
	assertb_sockerr(i==0);
	i = listen(sock[0], 1);
	assertb_sockerr(i==0);
	
	// connect sock[1] to sock[0]
	i = sizeof(addr);
	i = getsockname(sock[0], (struct sockaddr*)&addr, &i);
	assertb_sockerr(i==0);
	i = sizeof(addr);
	i = connect(sock[1], (struct sockaddr*)&addr, i);
	assertb_sockerr(i==0);

	// accept the new connection and replace sock[0]
	i = sizeof(addr);
	s = accept(sock[0], (struct sockaddr*)&addr, &i);
	assertb_sockerr(s>=0);

	sock_close(sock[0]);
	sock[0] = s;
	assertb(sock_valid(sock[0]))
	err = 0;
    } while(0);
    
    return err;
}

int
sock_valid(sock_t sock) {
    return sock != INVALID_SOCKET;
}

int
sock_read_avail(sock_t sock) {
    int i, n;
    n = 0;
    i = ioctlsocket(sock, FIONREAD, &n);
    if( i<0 ) {
	warn_sockerr();
	n = i;
    }
    return n;
}

char*
sock_err2str(int err) {
    char *s;

    switch(err) {
    case WSAEINTR: s="WSAEINTR"; break;
    case WSAEBADF: s="WSAEBADF"; break;
    case WSAEACCES: s="WSAEACCES"; break;
    case WSAEFAULT: s="WSAEFAULT"; break;
    case WSAEINVAL: s="WSAEINVAL"; break;
    case WSAEMFILE: s="WSAEMFILE"; break;
    case WSAEWOULDBLOCK: s="WSAEWOULDBLOCK"; break;
    case WSAEINPROGRESS: s="WSAEINPROGRESS"; break;
    case WSAEALREADY: s="WSAEALREADY"; break;
    case WSAENOTSOCK: s="WSAENOTSOCK"; break;
    case WSAEDESTADDRREQ: s="WSAEDESTADDRREQ"; break;
    case WSAEMSGSIZE: s="WSAEMSGSIZE"; break;
    case WSAEPROTOTYPE: s="WSAEPROTOTYPE"; break;
    case WSAENOPROTOOPT: s="WSAENOPROTOOPT"; break;
    case WSAEPROTONOSUPPORT: s="WSAEPROTONOSUPPORT"; break;
    case WSAESOCKTNOSUPPORT: s="WSAESOCKTNOSUPPORT"; break;
    case WSAEOPNOTSUPP: s="WSAEOPNOTSUPP"; break;
    case WSAEPFNOSUPPORT: s="WSAEPFNOSUPPORT"; break;
    case WSAEAFNOSUPPORT: s="WSAEAFNOSUPPORT"; break;
    case WSAEADDRINUSE: s="WSAEADDRINUSE"; break;
    case WSAEADDRNOTAVAIL: s="WSAEADDRNOTAVAIL"; break;
    case WSAENETDOWN: s="WSAENETDOWN"; break;
    case WSAENETUNREACH: s="WSAENETUNREACH"; break;
    case WSAENETRESET: s="WSAENETRESET"; break;
    case WSAECONNABORTED: s="WSAECONNABORTED"; break;
    case WSAECONNRESET: s="WSAECONNRESET"; break;
    case WSAENOBUFS: s="WSAENOBUFS"; break;
    case WSAEISCONN: s="WSAEISCONN"; break;
    case WSAENOTCONN: s="WSAENOTCONN"; break;
    case WSAESHUTDOWN: s="WSAESHUTDOWN"; break;
    case WSAETOOMANYREFS: s="WSAETOOMANYREFS"; break;
    case WSAETIMEDOUT: s="WSAETIMEDOUT"; break;
    case WSAECONNREFUSED: s="WSAECONNREFUSED"; break;
    case WSAELOOP: s="WSAELOOP"; break;
    case WSAENAMETOOLONG: s="WSAENAMETOOLONG"; break;
    case WSAEHOSTDOWN: s="WSAEHOSTDOWN"; break;
    case WSAEHOSTUNREACH: s="WSAEHOSTUNREACH"; break;
    case WSAENOTEMPTY: s="WSAENOTEMPTY"; break;
    case WSAEPROCLIM: s="WSAEPROCLIM"; break;
    case WSAEUSERS: s="WSAEUSERS"; break;
    case WSAEDQUOT: s="WSAEDQUOT"; break;
    case WSAESTALE: s="WSAESTALE"; break;
    case WSAEREMOTE: s="WSAEREMOTE"; break;
    case WSAEDISCON: s="WSAEDISCON"; break;
	//case WSAENOMORE: s="WSAENOMORE"; break;
	//case WSAECANCELLED: s="WSAECANCELLED"; break;
	//case WSAEINVALIDPROCTABLE: s="WSAEINVALIDPROCTABLE"; break;
	//case WSAEINVALIDPROVIDER: s="WSAEINVALIDPROVIDER"; break;
	//case WSAEPROVIDERFAILEDINIT: s="WSAEPROVIDERFAILEDINIT"; break;
	//case WSAEREFUSED: s="WSAEREFUSED"; break;
    default: s = "unknown";
    } 

    return s;
}

void
warn_sockerr() {
    int i;
    char *s;
    i = WSAGetLastError();
    switch(i) {
    case WSAEINTR: s = "WSAEINTR"; break;
    case WSAEBADF: s = "WSAEBADF"; break;
    case WSAEACCES: s = "WSAEACCES"; break;
    case WSAEFAULT: s = "WSAEFAULT"; break;
    case WSAEINVAL: s = "WSAEINVAL"; break;
    case WSAEMFILE: s = "WSAEMFILE"; break;
    case WSAEWOULDBLOCK: s = "WSAEWOULDBLOCK"; break;
    case WSAEINPROGRESS: s = "WSAEINPROGRESS"; break;
    case WSAEALREADY: s = "WSAEALREADY"; break;
    case WSAENOTSOCK: s = "WSAENOTSOCK"; break;
    case WSAEDESTADDRREQ: s = "WSAEDESTADDRREQ"; break;
    case WSAEMSGSIZE: s = "WSAEMSGSIZE"; break;
    case WSAEPROTOTYPE: s = "WSAEPROTOTYPE"; break;
    case WSAENOPROTOOPT: s = "WSAENOPROTOOPT"; break;
    case WSAEPROTONOSUPPORT: s = "WSAEPROTONOSUPPORT"; break;
    case WSAESOCKTNOSUPPORT: s = "WSAESOCKTNOSUPPORT"; break;
    case WSAEOPNOTSUPP: s = "WSAEOPNOTSUPP"; break;
    case WSAEPFNOSUPPORT: s = "WSAEPFNOSUPPORT"; break;
    case WSAEAFNOSUPPORT: s = "WSAEAFNOSUPPORT"; break;
    case WSAEADDRINUSE: s = "WSAEADDRINUSE"; break;
    case WSAEADDRNOTAVAIL: s = "WSAEADDRNOTAVAIL"; break;
    case WSAENETDOWN: s = "WSAENETDOWN"; break;
    case WSAENETUNREACH: s = "WSAENETUNREACH"; break;
    case WSAENETRESET: s = "WSAENETRESET"; break;
    case WSAECONNABORTED: s = "WSAECONNABORTED"; break;
    case WSAECONNRESET: s = "WSAECONNRESET"; break;
    case WSAENOBUFS: s = "WSAENOBUFS"; break;
    case WSAEISCONN: s = "WSAEISCONN"; break;
    case WSAENOTCONN: s = "WSAENOTCONN"; break;
    case WSAESHUTDOWN: s = "WSAESHUTDOWN"; break;
    case WSAETOOMANYREFS: s = "WSAETOOMANYREFS"; break;
    case WSAETIMEDOUT: s = "WSAETIMEDOUT"; break;
    case WSAECONNREFUSED: s = "WSAECONNREFUSED"; break;
    case WSAELOOP: s = "WSAELOOP"; break;
    case WSAENAMETOOLONG: s = "WSAENAMETOOLONG"; break;
    case WSAEHOSTDOWN: s = "WSAEHOSTDOWN"; break;
    case WSAEHOSTUNREACH: s = "WSAEHOSTUNREACH"; break;
    case WSAENOTEMPTY: s = "WSAENOTEMPTY"; break;
    case WSAEPROCLIM: s = "WSAEPROCLIM"; break;
    case WSAEUSERS: s = "WSAEUSERS"; break;
    case WSAEDQUOT: s = "WSAEDQUOT"; break;
    case WSAESTALE: s = "WSAESTALE"; break;
    case WSAEREMOTE: s = "WSAEREMOTE"; break;
    case WSAEDISCON: s = "WSAEDISCON"; break;
    case WSASYSNOTREADY: s = "WSASYSNOTREADY"; break;
    case WSAVERNOTSUPPORTED: s = "WSAVERNOTSUPPORTED"; break;
    case WSANOTINITIALISED: s = "WSANOTINITIALISED"; break;
    default: s="unknown"; break;
    }
    
    warn_v("WSAGetlastError: (0x%08x %d) %s\n", i, i, s);
}

void
warn_hresult(HRESULT hr) {
    warn_v("HRESULT: (0x%08x code=%d)%s\n", 
	   hr, HRESULT_CODE(hr), hresult2str(hr));
}


char *
hresult2str(HRESULT hr) {
    int code = HRESULT_CODE(hr);
    char *s=0;
    char buf[128];

    switch(hr) {
    case CO_E_INIT_TLS: s="CO_E_INIT_TLS"; break;
    case CO_E_INIT_SHARED_ALLOCATOR: s="CO_E_INIT_SHARED_ALLOCATOR"; break;
    case CO_E_INIT_MEMORY_ALLOCATOR: s="CO_E_INIT_MEMORY_ALLOCATOR"; break;
    case CO_E_INIT_CLASS_CACHE: s="CO_E_INIT_CLASS_CACHE"; break;
    case CO_E_INIT_RPC_CHANNEL: s="CO_E_INIT_RPC_CHANNEL"; break;
    case CO_E_INIT_TLS_SET_CHANNEL_CONTROL: s="CO_E_INIT_TLS_SET_CHANNEL_CONTROL"; break;
    case CO_E_INIT_TLS_CHANNEL_CONTROL: s="CO_E_INIT_TLS_CHANNEL_CONTROL"; break;
    case CO_E_INIT_UNACCEPTED_USER_ALLOCATOR: s="CO_E_INIT_UNACCEPTED_USER_ALLOCATOR"; break;
    case CO_E_INIT_SCM_MUTEX_EXISTS: s="CO_E_INIT_SCM_MUTEX_EXISTS"; break;
    case CO_E_INIT_SCM_FILE_MAPPING_EXISTS: s="CO_E_INIT_SCM_FILE_MAPPING_EXISTS"; break;
    case CO_E_INIT_SCM_MAP_VIEW_OF_FILE: s="CO_E_INIT_SCM_MAP_VIEW_OF_FILE"; break;
    case CO_E_INIT_SCM_EXEC_FAILURE: s="CO_E_INIT_SCM_EXEC_FAILURE"; break;
    case CO_E_INIT_ONLY_SINGLE_THREADED: s="CO_E_INIT_ONLY_SINGLE_THREADED"; break;
    case CO_E_CANT_REMOTE: s="CO_E_CANT_REMOTE"; break;
    case CO_E_BAD_SERVER_NAME: s="CO_E_BAD_SERVER_NAME"; break;
    case CO_E_WRONG_SERVER_IDENTITY: s="CO_E_WRONG_SERVER_IDENTITY"; break;
    case CO_E_OLE1DDE_DISABLED: s="CO_E_OLE1DDE_DISABLED"; break;
    case CO_E_RUNAS_SYNTAX: s="CO_E_RUNAS_SYNTAX"; break;
    case CO_E_CREATEPROCESS_FAILURE: s="CO_E_CREATEPROCESS_FAILURE"; break;
    case CO_E_RUNAS_CREATEPROCESS_FAILURE: s="CO_E_RUNAS_CREATEPROCESS_FAILURE"; break;
    case CO_E_RUNAS_LOGON_FAILURE: s="CO_E_RUNAS_LOGON_FAILURE"; break;
    case CO_E_LAUNCH_PERMSSION_DENIED: s="CO_E_LAUNCH_PERMSSION_DENIED"; break;
    case CO_E_START_SERVICE_FAILURE: s="CO_E_START_SERVICE_FAILURE"; break;
    case CO_E_REMOTE_COMMUNICATION_FAILURE: s="CO_E_REMOTE_COMMUNICATION_FAILURE"; break;
    case CO_E_SERVER_START_TIMEOUT: s="CO_E_SERVER_START_TIMEOUT"; break;
    case CO_E_CLSREG_INCONSISTENT: s="CO_E_CLSREG_INCONSISTENT"; break;
    case CO_E_IIDREG_INCONSISTENT: s="CO_E_IIDREG_INCONSISTENT"; break;
    case CO_E_NOT_SUPPORTED: s="CO_E_NOT_SUPPORTED"; break;
    case CO_E_RELOAD_DLL: s="CO_E_RELOAD_DLL"; break;
    case CO_E_MSI_ERROR: s="CO_E_MSI_ERROR"; break;
    case CO_E_NOTINITIALIZED: s="CO_E_NOTINITIALIZED"; break;
    case CO_E_ALREADYINITIALIZED: s="CO_E_ALREADYINITIALIZED"; break;
    case CO_E_CANTDETERMINECLASS: s="CO_E_CANTDETERMINECLASS"; break;
    case CO_E_CLASSSTRING: s="CO_E_CLASSSTRING"; break;
    case CO_E_IIDSTRING: s="CO_E_IIDSTRING"; break;
    case CO_E_APPNOTFOUND: s="CO_E_APPNOTFOUND"; break;
    case CO_E_APPSINGLEUSE: s="CO_E_APPSINGLEUSE"; break;
    case CO_E_ERRORINAPP: s="CO_E_ERRORINAPP"; break;
    case CO_E_DLLNOTFOUND: s="CO_E_DLLNOTFOUND"; break;
    case CO_E_ERRORINDLL: s="CO_E_ERRORINDLL"; break;
    case CO_E_WRONGOSFORAPP: s="CO_E_WRONGOSFORAPP"; break;
    case CO_E_OBJNOTREG: s="CO_E_OBJNOTREG"; break;
    case CO_E_OBJISREG: s="CO_E_OBJISREG"; break;
    case CO_E_OBJNOTCONNECTED: s="CO_E_OBJNOTCONNECTED"; break;
    case CO_E_APPDIDNTREG: s="CO_E_APPDIDNTREG"; break;
    case CO_E_RELEASED: s="CO_E_RELEASED"; break;
    case CO_E_FAILEDTOIMPERSONATE: s="CO_E_FAILEDTOIMPERSONATE"; break;
    case CO_E_FAILEDTOGETSECCTX: s="CO_E_FAILEDTOGETSECCTX"; break;
    case CO_E_FAILEDTOOPENTHREADTOKEN: s="CO_E_FAILEDTOOPENTHREADTOKEN"; break;
    case CO_E_FAILEDTOGETTOKENINFO: s="CO_E_FAILEDTOGETTOKENINFO"; break;
    case CO_E_TRUSTEEDOESNTMATCHCLIENT: s="CO_E_TRUSTEEDOESNTMATCHCLIENT"; break;
    case CO_E_FAILEDTOQUERYCLIENTBLANKET: s="CO_E_FAILEDTOQUERYCLIENTBLANKET"; break;
    case CO_E_FAILEDTOSETDACL: s="CO_E_FAILEDTOSETDACL"; break;
    case CO_E_ACCESSCHECKFAILED: s="CO_E_ACCESSCHECKFAILED"; break;
    case CO_E_NETACCESSAPIFAILED: s="CO_E_NETACCESSAPIFAILED"; break;
    case CO_E_WRONGTRUSTEENAMESYNTAX: s="CO_E_WRONGTRUSTEENAMESYNTAX"; break;
    case CO_E_INVALIDSID: s="CO_E_INVALIDSID"; break;
    case CO_E_CONVERSIONFAILED: s="CO_E_CONVERSIONFAILED"; break;
    case CO_E_NOMATCHINGSIDFOUND: s="CO_E_NOMATCHINGSIDFOUND"; break;
    case CO_E_LOOKUPACCSIDFAILED: s="CO_E_LOOKUPACCSIDFAILED"; break;
    case CO_E_NOMATCHINGNAMEFOUND: s="CO_E_NOMATCHINGNAMEFOUND"; break;
    case CO_E_LOOKUPACCNAMEFAILED: s="CO_E_LOOKUPACCNAMEFAILED"; break;
    case CO_E_SETSERLHNDLFAILED: s="CO_E_SETSERLHNDLFAILED"; break;
    case CO_E_FAILEDTOGETWINDIR: s="CO_E_FAILEDTOGETWINDIR"; break;
    case CO_E_PATHTOOLONG: s="CO_E_PATHTOOLONG"; break;
    case CO_E_FAILEDTOGENUUID: s="CO_E_FAILEDTOGENUUID"; break;
    case CO_E_FAILEDTOCREATEFILE: s="CO_E_FAILEDTOCREATEFILE"; break;
    case CO_E_FAILEDTOCLOSEHANDLE: s="CO_E_FAILEDTOCLOSEHANDLE"; break;
    case CO_E_EXCEEDSYSACLLIMIT: s="CO_E_EXCEEDSYSACLLIMIT"; break;
    case CO_E_ACESINWRONGORDER: s="CO_E_ACESINWRONGORDER"; break;
    case CO_E_INCOMPATIBLESTREAMVERSION: s="CO_E_INCOMPATIBLESTREAMVERSION"; break;
    case CO_E_FAILEDTOOPENPROCESSTOKEN: s="CO_E_FAILEDTOOPENPROCESSTOKEN"; break;
    case CO_E_DECODEFAILED: s="CO_E_DECODEFAILED"; break;
    case CO_E_ACNOTINITIALIZED: s="CO_E_ACNOTINITIALIZED"; break;
    case CO_E_CLASS_CREATE_FAILED: s="CO_E_CLASS_CREATE_FAILED"; break;
    case CO_E_SCM_ERROR: s="CO_E_SCM_ERROR"; break;
    case CO_E_SCM_RPC_FAILURE: s="CO_E_SCM_RPC_FAILURE"; break;
    case CO_E_BAD_PATH: s="CO_E_BAD_PATH"; break;
    case CO_E_SERVER_EXEC_FAILURE: s="CO_E_SERVER_EXEC_FAILURE"; break;
    case CO_E_OBJSRV_RPC_FAILURE: s="CO_E_OBJSRV_RPC_FAILURE"; break;
    case CO_E_SERVER_STOPPING: s="CO_E_SERVER_STOPPING"; break;
    }
    if( s ) {
	return s;
    }

    code = HRESULT_CODE(hr);
    switch(code) {
    case E_NOTIMPL: s = "E_NOTIMPL"; break;
    case E_OUTOFMEMORY: s = "E_OUTOFMEMORY"; break;
    case E_INVALIDARG: s = "E_INVALIDARG"; break;
    case E_NOINTERFACE: s = "E_NOINTERFACE"; break;
    case E_POINTER: s = "E_POINTER"; break;
    case E_HANDLE: s = "E_HANDLE"; break;
    case E_ABORT: s = "E_ABORT"; break;
    case E_FAIL: s = "E_FAIL"; break;
    case E_ACCESSDENIED: s = "E_ACCESSDENIED"; break;
    case E_PENDING: s = "E_PENDING"; break;
    case CO_E_INIT_TLS: s = "CO_E_INIT_TLS"; break;
    case CO_E_INIT_SHARED_ALLOCATOR: s = "CO_E_INIT_SHARED_ALLOCATOR"; break;
    case CO_E_INIT_MEMORY_ALLOCATOR: s = "CO_E_INIT_MEMORY_ALLOCATOR"; break;
    case CO_E_INIT_CLASS_CACHE: s = "CO_E_INIT_CLASS_CACHE"; break;
    case CO_E_INIT_RPC_CHANNEL: s = "CO_E_INIT_RPC_CHANNEL"; break;
    case CO_E_INIT_TLS_SET_CHANNEL_CONTROL: s = "CO_E_INIT_TLS_SET_CHANNEL_CONTROL"; break;
    case CO_E_INIT_TLS_CHANNEL_CONTROL: s = "CO_E_INIT_TLS_CHANNEL_CONTROL"; break;
    case CO_E_INIT_UNACCEPTED_USER_ALLOCATOR: s = "CO_E_INIT_UNACCEPTED_USER_ALLOCATOR"; break;
    case CO_E_INIT_SCM_MUTEX_EXISTS: s = "CO_E_INIT_SCM_MUTEX_EXISTS"; break;
    case CO_E_INIT_SCM_FILE_MAPPING_EXISTS: s = "CO_E_INIT_SCM_FILE_MAPPING_EXISTS"; break;
    case CO_E_INIT_SCM_MAP_VIEW_OF_FILE: s = "CO_E_INIT_SCM_MAP_VIEW_OF_FILE"; break;
    case CO_E_INIT_SCM_EXEC_FAILURE: s = "CO_E_INIT_SCM_EXEC_FAILURE"; break;
    case CO_E_INIT_ONLY_SINGLE_THREADED: s = "CO_E_INIT_ONLY_SINGLE_THREADED"; break;
    case CO_E_CANT_REMOTE: s = "CO_E_CANT_REMOTE"; break;
    case CO_E_BAD_SERVER_NAME: s = "CO_E_BAD_SERVER_NAME"; break;
    case CO_E_WRONG_SERVER_IDENTITY: s = "CO_E_WRONG_SERVER_IDENTITY"; break;
    case CO_E_OLE1DDE_DISABLED: s = "CO_E_OLE1DDE_DISABLED"; break;
    case CO_E_RUNAS_SYNTAX: s = "CO_E_RUNAS_SYNTAX"; break;
    case CO_E_CREATEPROCESS_FAILURE: s = "CO_E_CREATEPROCESS_FAILURE"; break;
    case CO_E_RUNAS_CREATEPROCESS_FAILURE: s = "CO_E_RUNAS_CREATEPROCESS_FAILURE"; break;
    case CO_E_RUNAS_LOGON_FAILURE: s = "CO_E_RUNAS_LOGON_FAILURE"; break;
    case CO_E_LAUNCH_PERMSSION_DENIED: s = "CO_E_LAUNCH_PERMSSION_DENIED"; break;
    case CO_E_START_SERVICE_FAILURE: s = "CO_E_START_SERVICE_FAILURE"; break;
    case CO_E_REMOTE_COMMUNICATION_FAILURE: s = "CO_E_REMOTE_COMMUNICATION_FAILURE"; break;
    case CO_E_SERVER_START_TIMEOUT: s = "CO_E_SERVER_START_TIMEOUT"; break;
    case CO_E_CLSREG_INCONSISTENT: s = "CO_E_CLSREG_INCONSISTENT"; break;
    case CO_E_IIDREG_INCONSISTENT: s = "CO_E_IIDREG_INCONSISTENT"; break;
    case CO_E_NOT_SUPPORTED: s = "CO_E_NOT_SUPPORTED"; break;
    case CO_E_RELOAD_DLL: s = "CO_E_RELOAD_DLL"; break;
    case CO_E_MSI_ERROR: s = "CO_E_MSI_ERROR"; break;
    case OLE_E_OLEVERB: s = "OLE_E_OLEVERB"; break;
    case OLE_E_ADVF: s = "OLE_E_ADVF"; break;
    case OLE_E_ENUM_NOMORE: s = "OLE_E_ENUM_NOMORE"; break;
    case OLE_E_ADVISENOTSUPPORTED: s = "OLE_E_ADVISENOTSUPPORTED"; break;
    case OLE_E_NOCONNECTION: s = "OLE_E_NOCONNECTION"; break;
    case OLE_E_NOTRUNNING: s = "OLE_E_NOTRUNNING"; break;
    case OLE_E_NOCACHE: s = "OLE_E_NOCACHE"; break;
    case OLE_E_BLANK: s = "OLE_E_BLANK"; break;
    case OLE_E_CLASSDIFF: s = "OLE_E_CLASSDIFF"; break;
    case OLE_E_CANT_GETMONIKER: s = "OLE_E_CANT_GETMONIKER"; break;
    case OLE_E_CANT_BINDTOSOURCE: s = "OLE_E_CANT_BINDTOSOURCE"; break;
    case OLE_E_STATIC: s = "OLE_E_STATIC"; break;
    case OLE_E_PROMPTSAVECANCELLED: s = "OLE_E_PROMPTSAVECANCELLED"; break;
    case OLE_E_INVALIDRECT: s = "OLE_E_INVALIDRECT"; break;
    case OLE_E_WRONGCOMPOBJ: s = "OLE_E_WRONGCOMPOBJ"; break;
    case OLE_E_INVALIDHWND: s = "OLE_E_INVALIDHWND"; break;
    case OLE_E_NOT_INPLACEACTIVE: s = "OLE_E_NOT_INPLACEACTIVE"; break;
    case OLE_E_CANTCONVERT: s = "OLE_E_CANTCONVERT"; break;
    case OLE_E_NOSTORAGE: s = "OLE_E_NOSTORAGE"; break;
    case DV_E_FORMATETC: s = "DV_E_FORMATETC"; break;
    case DV_E_DVTARGETDEVICE: s = "DV_E_DVTARGETDEVICE"; break;
    case DV_E_STGMEDIUM: s = "DV_E_STGMEDIUM"; break;
    case DV_E_STATDATA: s = "DV_E_STATDATA"; break;
    case DV_E_LINDEX: s = "DV_E_LINDEX"; break;
    case DV_E_TYMED: s = "DV_E_TYMED"; break;
    case DV_E_CLIPFORMAT: s = "DV_E_CLIPFORMAT"; break;
    case DV_E_DVASPECT: s = "DV_E_DVASPECT"; break;
    case DV_E_DVTARGETDEVICE_SIZE: s = "DV_E_DVTARGETDEVICE_SIZE"; break;
    case DV_E_NOIVIEWOBJECT: s = "DV_E_NOIVIEWOBJECT"; break;
    case DRAGDROP_E_NOTREGISTERED: s = "DRAGDROP_E_NOTREGISTERED"; break;
    case DRAGDROP_E_ALREADYREGISTERED: s = "DRAGDROP_E_ALREADYREGISTERED"; break;
    case DRAGDROP_E_INVALIDHWND: s = "DRAGDROP_E_INVALIDHWND"; break;
    case CLASS_E_NOAGGREGATION: s = "CLASS_E_NOAGGREGATION"; break;
    case CLASS_E_CLASSNOTAVAILABLE: s = "CLASS_E_CLASSNOTAVAILABLE"; break;
    case CLASS_E_NOTLICENSED: s = "CLASS_E_NOTLICENSED"; break;
    case VIEW_E_DRAW: s = "VIEW_E_DRAW"; break;
    case REGDB_E_READREGDB: s = "REGDB_E_READREGDB"; break;
    case REGDB_E_WRITEREGDB: s = "REGDB_E_WRITEREGDB"; break;
    case REGDB_E_KEYMISSING: s = "REGDB_E_KEYMISSING"; break;
    case REGDB_E_INVALIDVALUE: s = "REGDB_E_INVALIDVALUE"; break;
    case REGDB_E_CLASSNOTREG: s = "REGDB_E_CLASSNOTREG"; break;
    case REGDB_E_IIDNOTREG: s = "REGDB_E_IIDNOTREG"; break;
    case CAT_E_CATIDNOEXIST: s = "CAT_E_CATIDNOEXIST"; break;
    case CAT_E_NODESCRIPTION: s = "CAT_E_NODESCRIPTION"; break;
    case CS_E_PACKAGE_NOTFOUND: s = "CS_E_PACKAGE_NOTFOUND"; break;
    case CS_E_NOT_DELETABLE: s = "CS_E_NOT_DELETABLE"; break;
    case CS_E_CLASS_NOTFOUND: s = "CS_E_CLASS_NOTFOUND"; break;
    case CS_E_INVALID_VERSION: s = "CS_E_INVALID_VERSION"; break;
    case CS_E_NO_CLASSSTORE: s = "CS_E_NO_CLASSSTORE"; break;
    case CACHE_E_NOCACHE_UPDATED: s = "CACHE_E_NOCACHE_UPDATED"; break;
    case OLEOBJ_E_NOVERBS: s = "OLEOBJ_E_NOVERBS"; break;
    case OLEOBJ_E_INVALIDVERB: s = "OLEOBJ_E_INVALIDVERB"; break;
    case INPLACE_E_NOTUNDOABLE: s = "INPLACE_E_NOTUNDOABLE"; break;
    case INPLACE_E_NOTOOLSPACE: s = "INPLACE_E_NOTOOLSPACE"; break;
    case CONVERT10_E_OLESTREAM_GET: s = "CONVERT10_E_OLESTREAM_GET"; break;
    case CONVERT10_E_OLESTREAM_PUT: s = "CONVERT10_E_OLESTREAM_PUT"; break;
    case CONVERT10_E_OLESTREAM_FMT: s = "CONVERT10_E_OLESTREAM_FMT"; break;
    case CONVERT10_E_OLESTREAM_BITMAP_TO_DIB: s = "CONVERT10_E_OLESTREAM_BITMAP_TO_DIB"; break;
    case CONVERT10_E_STG_FMT: s = "CONVERT10_E_STG_FMT"; break;
    case CONVERT10_E_STG_NO_STD_STREAM: s = "CONVERT10_E_STG_NO_STD_STREAM"; break;
    case CONVERT10_E_STG_DIB_TO_BITMAP: s = "CONVERT10_E_STG_DIB_TO_BITMAP"; break;
    case CLIPBRD_E_CANT_OPEN: s = "CLIPBRD_E_CANT_OPEN"; break;
    case CLIPBRD_E_CANT_EMPTY: s = "CLIPBRD_E_CANT_EMPTY"; break;
    case CLIPBRD_E_CANT_SET: s = "CLIPBRD_E_CANT_SET"; break;
    case CLIPBRD_E_BAD_DATA: s = "CLIPBRD_E_BAD_DATA"; break;
    case CLIPBRD_E_CANT_CLOSE: s = "CLIPBRD_E_CANT_CLOSE"; break;
    case MK_E_CONNECTMANUALLY: s = "MK_E_CONNECTMANUALLY"; break;
    case MK_E_EXCEEDEDDEADLINE: s = "MK_E_EXCEEDEDDEADLINE"; break;
    case MK_E_NEEDGENERIC: s = "MK_E_NEEDGENERIC"; break;
    case MK_E_UNAVAILABLE: s = "MK_E_UNAVAILABLE"; break;
    case MK_E_SYNTAX: s = "MK_E_SYNTAX"; break;
    case MK_E_NOOBJECT: s = "MK_E_NOOBJECT"; break;
    case MK_E_INVALIDEXTENSION: s = "MK_E_INVALIDEXTENSION"; break;
    case MK_E_INTERMEDIATEINTERFACENOTSUPPORTED: s = "MK_E_INTERMEDIATEINTERFACENOTSUPPORTED"; break;
    case MK_E_NOTBINDABLE: s = "MK_E_NOTBINDABLE"; break;
    case MK_E_NOTBOUND: s = "MK_E_NOTBOUND"; break;
    case MK_E_CANTOPENFILE: s = "MK_E_CANTOPENFILE"; break;
    case MK_E_MUSTBOTHERUSER: s = "MK_E_MUSTBOTHERUSER"; break;
    case MK_E_NOINVERSE: s = "MK_E_NOINVERSE"; break;
    case MK_E_NOSTORAGE: s = "MK_E_NOSTORAGE"; break;
    case MK_E_NOPREFIX: s = "MK_E_NOPREFIX"; break;
    case MK_E_ENUMERATION_FAILED: s = "MK_E_ENUMERATION_FAILED"; break;
    case CO_E_NOTINITIALIZED: s = "CO_E_NOTINITIALIZED"; break;
    case CO_E_ALREADYINITIALIZED: s = "CO_E_ALREADYINITIALIZED"; break;
    case CO_E_CANTDETERMINECLASS: s = "CO_E_CANTDETERMINECLASS"; break;
    case CO_E_CLASSSTRING: s = "CO_E_CLASSSTRING"; break;
    case CO_E_IIDSTRING: s = "CO_E_IIDSTRING"; break;
    case CO_E_APPNOTFOUND: s = "CO_E_APPNOTFOUND"; break;
    case CO_E_APPSINGLEUSE: s = "CO_E_APPSINGLEUSE"; break;
    case CO_E_ERRORINAPP: s = "CO_E_ERRORINAPP"; break;
    case CO_E_DLLNOTFOUND: s = "CO_E_DLLNOTFOUND"; break;
    case CO_E_ERRORINDLL: s = "CO_E_ERRORINDLL"; break;
    case CO_E_WRONGOSFORAPP: s = "CO_E_WRONGOSFORAPP"; break;
    case CO_E_OBJNOTREG: s = "CO_E_OBJNOTREG"; break;
    case CO_E_OBJISREG: s = "CO_E_OBJISREG"; break;
    case CO_E_OBJNOTCONNECTED: s = "CO_E_OBJNOTCONNECTED"; break;
    case CO_E_APPDIDNTREG: s = "CO_E_APPDIDNTREG"; break;
    case CO_E_RELEASED: s = "CO_E_RELEASED"; break;
    case CO_E_FAILEDTOIMPERSONATE: s = "CO_E_FAILEDTOIMPERSONATE"; break;
    case CO_E_FAILEDTOGETSECCTX: s = "CO_E_FAILEDTOGETSECCTX"; break;
    case CO_E_FAILEDTOOPENTHREADTOKEN: s = "CO_E_FAILEDTOOPENTHREADTOKEN"; break;
    case CO_E_FAILEDTOGETTOKENINFO: s = "CO_E_FAILEDTOGETTOKENINFO"; break;
    case CO_E_TRUSTEEDOESNTMATCHCLIENT: s = "CO_E_TRUSTEEDOESNTMATCHCLIENT"; break;
    case CO_E_FAILEDTOQUERYCLIENTBLANKET: s = "CO_E_FAILEDTOQUERYCLIENTBLANKET"; break;
    case CO_E_FAILEDTOSETDACL: s = "CO_E_FAILEDTOSETDACL"; break;
    case CO_E_ACCESSCHECKFAILED: s = "CO_E_ACCESSCHECKFAILED"; break;
    case CO_E_NETACCESSAPIFAILED: s = "CO_E_NETACCESSAPIFAILED"; break;
    case CO_E_WRONGTRUSTEENAMESYNTAX: s = "CO_E_WRONGTRUSTEENAMESYNTAX"; break;
    case CO_E_INVALIDSID: s = "CO_E_INVALIDSID"; break;
    case CO_E_CONVERSIONFAILED: s = "CO_E_CONVERSIONFAILED"; break;
    case CO_E_NOMATCHINGSIDFOUND: s = "CO_E_NOMATCHINGSIDFOUND"; break;
    case CO_E_LOOKUPACCSIDFAILED: s = "CO_E_LOOKUPACCSIDFAILED"; break;
    case CO_E_NOMATCHINGNAMEFOUND: s = "CO_E_NOMATCHINGNAMEFOUND"; break;
    case CO_E_LOOKUPACCNAMEFAILED: s = "CO_E_LOOKUPACCNAMEFAILED"; break;
    case CO_E_SETSERLHNDLFAILED: s = "CO_E_SETSERLHNDLFAILED"; break;
    case CO_E_FAILEDTOGETWINDIR: s = "CO_E_FAILEDTOGETWINDIR"; break;
    case CO_E_PATHTOOLONG: s = "CO_E_PATHTOOLONG"; break;
    case CO_E_FAILEDTOGENUUID: s = "CO_E_FAILEDTOGENUUID"; break;
    case CO_E_FAILEDTOCREATEFILE: s = "CO_E_FAILEDTOCREATEFILE"; break;
    case CO_E_FAILEDTOCLOSEHANDLE: s = "CO_E_FAILEDTOCLOSEHANDLE"; break;
    case CO_E_EXCEEDSYSACLLIMIT: s = "CO_E_EXCEEDSYSACLLIMIT"; break;
    case CO_E_ACESINWRONGORDER: s = "CO_E_ACESINWRONGORDER"; break;
    case CO_E_INCOMPATIBLESTREAMVERSION: s = "CO_E_INCOMPATIBLESTREAMVERSION"; break;
    case CO_E_FAILEDTOOPENPROCESSTOKEN: s = "CO_E_FAILEDTOOPENPROCESSTOKEN"; break;
    case CO_E_DECODEFAILED: s = "CO_E_DECODEFAILED"; break;
    case CO_E_ACNOTINITIALIZED: s = "CO_E_ACNOTINITIALIZED"; break;
    case OLE_S_USEREG: s = "OLE_S_USEREG"; break;
    case OLE_S_STATIC: s = "OLE_S_STATIC"; break;
    case OLE_S_MAC_CLIPFORMAT: s = "OLE_S_MAC_CLIPFORMAT"; break;
    case DRAGDROP_S_DROP: s = "DRAGDROP_S_DROP"; break;
    case DRAGDROP_S_CANCEL: s = "DRAGDROP_S_CANCEL"; break;
    case DRAGDROP_S_USEDEFAULTCURSORS: s = "DRAGDROP_S_USEDEFAULTCURSORS"; break;
    case DATA_S_SAMEFORMATETC: s = "DATA_S_SAMEFORMATETC"; break;
    case VIEW_S_ALREADY_FROZEN: s = "VIEW_S_ALREADY_FROZEN"; break;
    case CACHE_S_FORMATETC_NOTSUPPORTED: s = "CACHE_S_FORMATETC_NOTSUPPORTED"; break;
    case CACHE_S_SAMECACHE: s = "CACHE_S_SAMECACHE"; break;
    case CACHE_S_SOMECACHES_NOTUPDATED: s = "CACHE_S_SOMECACHES_NOTUPDATED"; break;
    case OLEOBJ_S_INVALIDVERB: s = "OLEOBJ_S_INVALIDVERB"; break;
    case OLEOBJ_S_CANNOT_DOVERB_NOW: s = "OLEOBJ_S_CANNOT_DOVERB_NOW"; break;
    case OLEOBJ_S_INVALIDHWND: s = "OLEOBJ_S_INVALIDHWND"; break;
    case INPLACE_S_TRUNCATED: s = "INPLACE_S_TRUNCATED"; break;
    case CONVERT10_S_NO_PRESENTATION: s = "CONVERT10_S_NO_PRESENTATION"; break;
    case MK_S_REDUCED_TO_SELF: s = "MK_S_REDUCED_TO_SELF"; break;
    case MK_S_ME: s = "MK_S_ME"; break;
    case MK_S_HIM: s = "MK_S_HIM"; break;
    case MK_S_US: s = "MK_S_US"; break;
    case MK_S_MONIKERALREADYREGISTERED: s = "MK_S_MONIKERALREADYREGISTERED"; break;
    case CO_E_CLASS_CREATE_FAILED: s = "CO_E_CLASS_CREATE_FAILED"; break;
    case CO_E_SCM_ERROR: s = "CO_E_SCM_ERROR"; break;
    case CO_E_SCM_RPC_FAILURE: s = "CO_E_SCM_RPC_FAILURE"; break;
    case CO_E_BAD_PATH: s = "CO_E_BAD_PATH"; break;
    case CO_E_SERVER_EXEC_FAILURE: s = "CO_E_SERVER_EXEC_FAILURE"; break;
    case CO_E_OBJSRV_RPC_FAILURE: s = "CO_E_OBJSRV_RPC_FAILURE"; break;
    case MK_E_NO_NORMALIZED: s = "MK_E_NO_NORMALIZED"; break;
    case CO_E_SERVER_STOPPING: s = "CO_E_SERVER_STOPPING"; break;
    case MEM_E_INVALID_ROOT: s = "MEM_E_INVALID_ROOT"; break;
    case MEM_E_INVALID_LINK: s = "MEM_E_INVALID_LINK"; break;
    case MEM_E_INVALID_SIZE: s = "MEM_E_INVALID_SIZE"; break;
    case CO_S_NOTALLINTERFACES: s = "CO_S_NOTALLINTERFACES"; break;
    case DISP_E_UNKNOWNINTERFACE: s = "DISP_E_UNKNOWNINTERFACE"; break;
    case DISP_E_MEMBERNOTFOUND: s = "DISP_E_MEMBERNOTFOUND"; break;
    case DISP_E_PARAMNOTFOUND: s = "DISP_E_PARAMNOTFOUND"; break;
    case DISP_E_TYPEMISMATCH: s = "DISP_E_TYPEMISMATCH"; break;
    case DISP_E_UNKNOWNNAME: s = "DISP_E_UNKNOWNNAME"; break;
    case DISP_E_NONAMEDARGS: s = "DISP_E_NONAMEDARGS"; break;
    case DISP_E_BADVARTYPE: s = "DISP_E_BADVARTYPE"; break;
    case DISP_E_EXCEPTION: s = "DISP_E_EXCEPTION"; break;
    case DISP_E_OVERFLOW: s = "DISP_E_OVERFLOW"; break;
    case DISP_E_BADINDEX: s = "DISP_E_BADINDEX"; break;
    case DISP_E_UNKNOWNLCID: s = "DISP_E_UNKNOWNLCID"; break;
    case DISP_E_ARRAYISLOCKED: s = "DISP_E_ARRAYISLOCKED"; break;
    case DISP_E_BADPARAMCOUNT: s = "DISP_E_BADPARAMCOUNT"; break;
    case DISP_E_PARAMNOTOPTIONAL: s = "DISP_E_PARAMNOTOPTIONAL"; break;
    case DISP_E_BADCALLEE: s = "DISP_E_BADCALLEE"; break;
    case DISP_E_NOTACOLLECTION: s = "DISP_E_NOTACOLLECTION"; break;
    case DISP_E_DIVBYZERO: s = "DISP_E_DIVBYZERO"; break;
    case TYPE_E_BUFFERTOOSMALL: s = "TYPE_E_BUFFERTOOSMALL"; break;
    case TYPE_E_FIELDNOTFOUND: s = "TYPE_E_FIELDNOTFOUND"; break;
    case TYPE_E_INVDATAREAD: s = "TYPE_E_INVDATAREAD"; break;
    case TYPE_E_UNSUPFORMAT: s = "TYPE_E_UNSUPFORMAT"; break;
    case TYPE_E_REGISTRYACCESS: s = "TYPE_E_REGISTRYACCESS"; break;
    case TYPE_E_LIBNOTREGISTERED: s = "TYPE_E_LIBNOTREGISTERED"; break;
    case TYPE_E_UNDEFINEDTYPE: s = "TYPE_E_UNDEFINEDTYPE"; break;
    case TYPE_E_QUALIFIEDNAMEDISALLOWED: s = "TYPE_E_QUALIFIEDNAMEDISALLOWED"; break;
    case TYPE_E_INVALIDSTATE: s = "TYPE_E_INVALIDSTATE"; break;
    case TYPE_E_WRONGTYPEKIND: s = "TYPE_E_WRONGTYPEKIND"; break;
    case TYPE_E_ELEMENTNOTFOUND: s = "TYPE_E_ELEMENTNOTFOUND"; break;
    case TYPE_E_AMBIGUOUSNAME: s = "TYPE_E_AMBIGUOUSNAME"; break;
    case TYPE_E_NAMECONFLICT: s = "TYPE_E_NAMECONFLICT"; break;
    case TYPE_E_UNKNOWNLCID: s = "TYPE_E_UNKNOWNLCID"; break;
    case TYPE_E_DLLFUNCTIONNOTFOUND: s = "TYPE_E_DLLFUNCTIONNOTFOUND"; break;
    case TYPE_E_BADMODULEKIND: s = "TYPE_E_BADMODULEKIND"; break;
    case TYPE_E_SIZETOOBIG: s = "TYPE_E_SIZETOOBIG"; break;
    case TYPE_E_DUPLICATEID: s = "TYPE_E_DUPLICATEID"; break;
    case TYPE_E_INVALIDID: s = "TYPE_E_INVALIDID"; break;
    case TYPE_E_TYPEMISMATCH: s = "TYPE_E_TYPEMISMATCH"; break;
    case TYPE_E_OUTOFBOUNDS: s = "TYPE_E_OUTOFBOUNDS"; break;
    case TYPE_E_IOERROR: s = "TYPE_E_IOERROR"; break;
    case TYPE_E_CANTCREATETMPFILE: s = "TYPE_E_CANTCREATETMPFILE"; break;
    case TYPE_E_CANTLOADLIBRARY: s = "TYPE_E_CANTLOADLIBRARY"; break;
    case TYPE_E_INCONSISTENTPROPFUNCS: s = "TYPE_E_INCONSISTENTPROPFUNCS"; break;
    case TYPE_E_CIRCULARTYPE: s = "TYPE_E_CIRCULARTYPE"; break;
    case STG_E_INVALIDFUNCTION: s = "STG_E_INVALIDFUNCTION"; break;
    case STG_E_FILENOTFOUND: s = "STG_E_FILENOTFOUND"; break;
    case STG_E_PATHNOTFOUND: s = "STG_E_PATHNOTFOUND"; break;
    case STG_E_TOOMANYOPENFILES: s = "STG_E_TOOMANYOPENFILES"; break;
    case STG_E_ACCESSDENIED: s = "STG_E_ACCESSDENIED"; break;
    case STG_E_INVALIDHANDLE: s = "STG_E_INVALIDHANDLE"; break;
    case STG_E_INSUFFICIENTMEMORY: s = "STG_E_INSUFFICIENTMEMORY"; break;
    case STG_E_INVALIDPOINTER: s = "STG_E_INVALIDPOINTER"; break;
    case STG_E_NOMOREFILES: s = "STG_E_NOMOREFILES"; break;
    case STG_E_DISKISWRITEPROTECTED: s = "STG_E_DISKISWRITEPROTECTED"; break;
    case STG_E_SEEKERROR: s = "STG_E_SEEKERROR"; break;
    case STG_E_WRITEFAULT: s = "STG_E_WRITEFAULT"; break;
    case STG_E_READFAULT: s = "STG_E_READFAULT"; break;
    case STG_E_SHAREVIOLATION: s = "STG_E_SHAREVIOLATION"; break;
    case STG_E_LOCKVIOLATION: s = "STG_E_LOCKVIOLATION"; break;
    case STG_E_FILEALREADYEXISTS: s = "STG_E_FILEALREADYEXISTS"; break;
    case STG_E_INVALIDPARAMETER: s = "STG_E_INVALIDPARAMETER"; break;
    case STG_E_MEDIUMFULL: s = "STG_E_MEDIUMFULL"; break;
    case STG_E_PROPSETMISMATCHED: s = "STG_E_PROPSETMISMATCHED"; break;
    case STG_E_ABNORMALAPIEXIT: s = "STG_E_ABNORMALAPIEXIT"; break;
    case STG_E_INVALIDHEADER: s = "STG_E_INVALIDHEADER"; break;
    case STG_E_INVALIDNAME: s = "STG_E_INVALIDNAME"; break;
    case STG_E_UNKNOWN: s = "STG_E_UNKNOWN"; break;
    case STG_E_UNIMPLEMENTEDFUNCTION: s = "STG_E_UNIMPLEMENTEDFUNCTION"; break;
    case STG_E_INVALIDFLAG: s = "STG_E_INVALIDFLAG"; break;
    case STG_E_INUSE: s = "STG_E_INUSE"; break;
    case STG_E_NOTCURRENT: s = "STG_E_NOTCURRENT"; break;
    case STG_E_REVERTED: s = "STG_E_REVERTED"; break;
    case STG_E_CANTSAVE: s = "STG_E_CANTSAVE"; break;
    case STG_E_OLDFORMAT: s = "STG_E_OLDFORMAT"; break;
    case STG_E_OLDDLL: s = "STG_E_OLDDLL"; break;
    case STG_E_SHAREREQUIRED: s = "STG_E_SHAREREQUIRED"; break;
    case STG_E_NOTFILEBASEDSTORAGE: s = "STG_E_NOTFILEBASEDSTORAGE"; break;
    case STG_E_EXTANTMARSHALLINGS: s = "STG_E_EXTANTMARSHALLINGS"; break;
    case STG_E_DOCFILECORRUPT: s = "STG_E_DOCFILECORRUPT"; break;
    case STG_E_BADBASEADDRESS: s = "STG_E_BADBASEADDRESS"; break;
    case STG_E_INCOMPLETE: s = "STG_E_INCOMPLETE"; break;
    case STG_E_TERMINATED: s = "STG_E_TERMINATED"; break;
    case STG_S_CONVERTED: s = "STG_S_CONVERTED"; break;
    case STG_S_BLOCK: s = "STG_S_BLOCK"; break;
    case STG_S_RETRYNOW: s = "STG_S_RETRYNOW"; break;
    case STG_S_MONITORING: s = "STG_S_MONITORING"; break;
    case STG_S_MULTIPLEOPENS: s = "STG_S_MULTIPLEOPENS"; break;
    case STG_S_CONSOLIDATIONFAILED: s = "STG_S_CONSOLIDATIONFAILED"; break;
    case STG_S_CANNOTCONSOLIDATE: s = "STG_S_CANNOTCONSOLIDATE"; break;
    case RPC_E_CALL_REJECTED: s = "RPC_E_CALL_REJECTED"; break;
    case RPC_E_CALL_CANCELED: s = "RPC_E_CALL_CANCELED"; break;
    case RPC_E_CANTPOST_INSENDCALL: s = "RPC_E_CANTPOST_INSENDCALL"; break;
    case RPC_E_CANTCALLOUT_INASYNCCALL: s = "RPC_E_CANTCALLOUT_INASYNCCALL"; break;
    case RPC_E_CANTCALLOUT_INEXTERNALCALL: s = "RPC_E_CANTCALLOUT_INEXTERNALCALL"; break;
    case RPC_E_CONNECTION_TERMINATED: s = "RPC_E_CONNECTION_TERMINATED"; break;
    case RPC_E_SERVER_DIED: s = "RPC_E_SERVER_DIED"; break;
    case RPC_E_CLIENT_DIED: s = "RPC_E_CLIENT_DIED"; break;
    case RPC_E_INVALID_DATAPACKET: s = "RPC_E_INVALID_DATAPACKET"; break;
    case RPC_E_CANTTRANSMIT_CALL: s = "RPC_E_CANTTRANSMIT_CALL"; break;
    case RPC_E_CLIENT_CANTMARSHAL_DATA: s = "RPC_E_CLIENT_CANTMARSHAL_DATA"; break;
    case RPC_E_CLIENT_CANTUNMARSHAL_DATA: s = "RPC_E_CLIENT_CANTUNMARSHAL_DATA"; break;
    case RPC_E_SERVER_CANTMARSHAL_DATA: s = "RPC_E_SERVER_CANTMARSHAL_DATA"; break;
    case RPC_E_SERVER_CANTUNMARSHAL_DATA: s = "RPC_E_SERVER_CANTUNMARSHAL_DATA"; break;
    case RPC_E_INVALID_DATA: s = "RPC_E_INVALID_DATA"; break;
    case RPC_E_INVALID_PARAMETER: s = "RPC_E_INVALID_PARAMETER"; break;
    case RPC_E_CANTCALLOUT_AGAIN: s = "RPC_E_CANTCALLOUT_AGAIN"; break;
    case RPC_E_SERVER_DIED_DNE: s = "RPC_E_SERVER_DIED_DNE"; break;
    case RPC_E_SYS_CALL_FAILED: s = "RPC_E_SYS_CALL_FAILED"; break;
    case RPC_E_OUT_OF_RESOURCES: s = "RPC_E_OUT_OF_RESOURCES"; break;
    case RPC_E_ATTEMPTED_MULTITHREAD: s = "RPC_E_ATTEMPTED_MULTITHREAD"; break;
    case RPC_E_NOT_REGISTERED: s = "RPC_E_NOT_REGISTERED"; break;
    case RPC_E_FAULT: s = "RPC_E_FAULT"; break;
    case RPC_E_SERVERFAULT: s = "RPC_E_SERVERFAULT"; break;
    case RPC_E_CHANGED_MODE: s = "RPC_E_CHANGED_MODE"; break;
    case RPC_E_INVALIDMETHOD: s = "RPC_E_INVALIDMETHOD"; break;
    case RPC_E_DISCONNECTED: s = "RPC_E_DISCONNECTED"; break;
    case RPC_E_RETRY: s = "RPC_E_RETRY"; break;
    case RPC_E_SERVERCALL_RETRYLATER: s = "RPC_E_SERVERCALL_RETRYLATER"; break;
    case RPC_E_SERVERCALL_REJECTED: s = "RPC_E_SERVERCALL_REJECTED"; break;
    case RPC_E_INVALID_CALLDATA: s = "RPC_E_INVALID_CALLDATA"; break;
    case RPC_E_CANTCALLOUT_ININPUTSYNCCALL: s = "RPC_E_CANTCALLOUT_ININPUTSYNCCALL"; break;
    case RPC_E_WRONG_THREAD: s = "RPC_E_WRONG_THREAD"; break;
    case RPC_E_THREAD_NOT_INIT: s = "RPC_E_THREAD_NOT_INIT"; break;
    case RPC_E_VERSION_MISMATCH: s = "RPC_E_VERSION_MISMATCH"; break;
    case RPC_E_INVALID_HEADER: s = "RPC_E_INVALID_HEADER"; break;
    case RPC_E_INVALID_EXTENSION: s = "RPC_E_INVALID_EXTENSION"; break;
    case RPC_E_INVALID_IPID: s = "RPC_E_INVALID_IPID"; break;
    case RPC_E_INVALID_OBJECT: s = "RPC_E_INVALID_OBJECT"; break;
    case RPC_S_CALLPENDING: s = "RPC_S_CALLPENDING"; break;
    case RPC_S_WAITONTIMER: s = "RPC_S_WAITONTIMER"; break;
    case RPC_E_CALL_COMPLETE: s = "RPC_E_CALL_COMPLETE"; break;
    case RPC_E_UNSECURE_CALL: s = "RPC_E_UNSECURE_CALL"; break;
    case RPC_E_TOO_LATE: s = "RPC_E_TOO_LATE"; break;
    case RPC_E_NO_GOOD_SECURITY_PACKAGES: s = "RPC_E_NO_GOOD_SECURITY_PACKAGES"; break;
    case RPC_E_ACCESS_DENIED: s = "RPC_E_ACCESS_DENIED"; break;
    case RPC_E_REMOTE_DISABLED: s = "RPC_E_REMOTE_DISABLED"; break;
    case RPC_E_INVALID_OBJREF: s = "RPC_E_INVALID_OBJREF"; break;
    case RPC_E_NO_CONTEXT: s = "RPC_E_NO_CONTEXT"; break;
    case RPC_E_TIMEOUT: s = "RPC_E_TIMEOUT"; break;
    case RPC_E_NO_SYNC: s = "RPC_E_NO_SYNC"; break;
    case RPC_E_UNEXPECTED: s = "RPC_E_UNEXPECTED"; break;
    case NTE_BAD_UID: s = "NTE_BAD_UID"; break;
    case NTE_BAD_HASH: s = "NTE_BAD_HASH"; break;
    case NTE_BAD_KEY: s = "NTE_BAD_KEY"; break;
    case NTE_BAD_LEN: s = "NTE_BAD_LEN"; break;
    case NTE_BAD_DATA: s = "NTE_BAD_DATA"; break;
    case NTE_BAD_SIGNATURE: s = "NTE_BAD_SIGNATURE"; break;
    case NTE_BAD_VER: s = "NTE_BAD_VER"; break;
    case NTE_BAD_ALGID: s = "NTE_BAD_ALGID"; break;
    case NTE_BAD_FLAGS: s = "NTE_BAD_FLAGS"; break;
    case NTE_BAD_TYPE: s = "NTE_BAD_TYPE"; break;
    case NTE_BAD_KEY_STATE: s = "NTE_BAD_KEY_STATE"; break;
    case NTE_BAD_HASH_STATE: s = "NTE_BAD_HASH_STATE"; break;
    case NTE_NO_KEY: s = "NTE_NO_KEY"; break;
    case NTE_NO_MEMORY: s = "NTE_NO_MEMORY"; break;
    case NTE_EXISTS: s = "NTE_EXISTS"; break;
    case NTE_PERM: s = "NTE_PERM"; break;
    case NTE_NOT_FOUND: s = "NTE_NOT_FOUND"; break;
    case NTE_DOUBLE_ENCRYPT: s = "NTE_DOUBLE_ENCRYPT"; break;
    case NTE_BAD_PROVIDER: s = "NTE_BAD_PROVIDER"; break;
    case NTE_BAD_PROV_TYPE: s = "NTE_BAD_PROV_TYPE"; break;
    case NTE_BAD_PUBLIC_KEY: s = "NTE_BAD_PUBLIC_KEY"; break;
    case NTE_BAD_KEYSET: s = "NTE_BAD_KEYSET"; break;
    case NTE_PROV_TYPE_NOT_DEF: s = "NTE_PROV_TYPE_NOT_DEF"; break;
    case NTE_PROV_TYPE_ENTRY_BAD: s = "NTE_PROV_TYPE_ENTRY_BAD"; break;
    case NTE_KEYSET_NOT_DEF: s = "NTE_KEYSET_NOT_DEF"; break;
    case NTE_KEYSET_ENTRY_BAD: s = "NTE_KEYSET_ENTRY_BAD"; break;
    case NTE_PROV_TYPE_NO_MATCH: s = "NTE_PROV_TYPE_NO_MATCH"; break;
    case NTE_SIGNATURE_FILE_BAD: s = "NTE_SIGNATURE_FILE_BAD"; break;
    case NTE_PROVIDER_DLL_FAIL: s = "NTE_PROVIDER_DLL_FAIL"; break;
    case NTE_PROV_DLL_NOT_FOUND: s = "NTE_PROV_DLL_NOT_FOUND"; break;
    case NTE_BAD_KEYSET_PARAM: s = "NTE_BAD_KEYSET_PARAM"; break;
    case NTE_FAIL: s = "NTE_FAIL"; break;
    case NTE_SYS_ERR: s = "NTE_SYS_ERR"; break;
    case CRYPT_E_MSG_ERROR: s = "CRYPT_E_MSG_ERROR"; break;
    case CRYPT_E_UNKNOWN_ALGO: s = "CRYPT_E_UNKNOWN_ALGO"; break;
    case CRYPT_E_OID_FORMAT: s = "CRYPT_E_OID_FORMAT"; break;
    case CRYPT_E_INVALID_MSG_TYPE: s = "CRYPT_E_INVALID_MSG_TYPE"; break;
    case CRYPT_E_UNEXPECTED_ENCODING: s = "CRYPT_E_UNEXPECTED_ENCODING"; break;
    case CRYPT_E_AUTH_ATTR_MISSING: s = "CRYPT_E_AUTH_ATTR_MISSING"; break;
    case CRYPT_E_HASH_VALUE: s = "CRYPT_E_HASH_VALUE"; break;
    case CRYPT_E_INVALID_INDEX: s = "CRYPT_E_INVALID_INDEX"; break;
    case CRYPT_E_ALREADY_DECRYPTED: s = "CRYPT_E_ALREADY_DECRYPTED"; break;
    case CRYPT_E_NOT_DECRYPTED: s = "CRYPT_E_NOT_DECRYPTED"; break;
    case CRYPT_E_RECIPIENT_NOT_FOUND: s = "CRYPT_E_RECIPIENT_NOT_FOUND"; break;
    case CRYPT_E_CONTROL_TYPE: s = "CRYPT_E_CONTROL_TYPE"; break;
    case CRYPT_E_ISSUER_SERIALNUMBER: s = "CRYPT_E_ISSUER_SERIALNUMBER"; break;
    case CRYPT_E_SIGNER_NOT_FOUND: s = "CRYPT_E_SIGNER_NOT_FOUND"; break;
    case CRYPT_E_ATTRIBUTES_MISSING: s = "CRYPT_E_ATTRIBUTES_MISSING"; break;
    case CRYPT_E_STREAM_MSG_NOT_READY: s = "CRYPT_E_STREAM_MSG_NOT_READY"; break;
    case CRYPT_E_STREAM_INSUFFICIENT_DATA: s = "CRYPT_E_STREAM_INSUFFICIENT_DATA"; break;
    case CRYPT_E_BAD_LEN: s = "CRYPT_E_BAD_LEN"; break;
    case CRYPT_E_BAD_ENCODE: s = "CRYPT_E_BAD_ENCODE"; break;
    case CRYPT_E_FILE_ERROR: s = "CRYPT_E_FILE_ERROR"; break;
    case CRYPT_E_NOT_FOUND: s = "CRYPT_E_NOT_FOUND"; break;
    case CRYPT_E_EXISTS: s = "CRYPT_E_EXISTS"; break;
    case CRYPT_E_NO_PROVIDER: s = "CRYPT_E_NO_PROVIDER"; break;
    case CRYPT_E_SELF_SIGNED: s = "CRYPT_E_SELF_SIGNED"; break;
    case CRYPT_E_DELETED_PREV: s = "CRYPT_E_DELETED_PREV"; break;
    case CRYPT_E_NO_MATCH: s = "CRYPT_E_NO_MATCH"; break;
    case CRYPT_E_UNEXPECTED_MSG_TYPE: s = "CRYPT_E_UNEXPECTED_MSG_TYPE"; break;
    case CRYPT_E_NO_KEY_PROPERTY: s = "CRYPT_E_NO_KEY_PROPERTY"; break;
    case CRYPT_E_NO_DECRYPT_CERT: s = "CRYPT_E_NO_DECRYPT_CERT"; break;
    case CRYPT_E_BAD_MSG: s = "CRYPT_E_BAD_MSG"; break;
    case CRYPT_E_NO_SIGNER: s = "CRYPT_E_NO_SIGNER"; break;
    case CRYPT_E_PENDING_CLOSE: s = "CRYPT_E_PENDING_CLOSE"; break;
    case CRYPT_E_REVOKED: s = "CRYPT_E_REVOKED"; break;
    case CRYPT_E_NO_REVOCATION_DLL: s = "CRYPT_E_NO_REVOCATION_DLL"; break;
    case CRYPT_E_NO_REVOCATION_CHECK: s = "CRYPT_E_NO_REVOCATION_CHECK"; break;
    case CRYPT_E_REVOCATION_OFFLINE: s = "CRYPT_E_REVOCATION_OFFLINE"; break;
    case CRYPT_E_NOT_IN_REVOCATION_DATABASE: s = "CRYPT_E_NOT_IN_REVOCATION_DATABASE"; break;
    case CRYPT_E_INVALID_NUMERIC_STRING: s = "CRYPT_E_INVALID_NUMERIC_STRING"; break;
    case CRYPT_E_INVALID_PRINTABLE_STRING: s = "CRYPT_E_INVALID_PRINTABLE_STRING"; break;
    case CRYPT_E_INVALID_IA5_STRING: s = "CRYPT_E_INVALID_IA5_STRING"; break;
    case CRYPT_E_INVALID_X500_STRING: s = "CRYPT_E_INVALID_X500_STRING"; break;
    case CRYPT_E_NOT_CHAR_STRING: s = "CRYPT_E_NOT_CHAR_STRING"; break;
    case CRYPT_E_FILERESIZED: s = "CRYPT_E_FILERESIZED"; break;
    case CRYPT_E_SECURITY_SETTINGS: s = "CRYPT_E_SECURITY_SETTINGS"; break;
    case CRYPT_E_NO_VERIFY_USAGE_DLL: s = "CRYPT_E_NO_VERIFY_USAGE_DLL"; break;
    case CRYPT_E_NO_VERIFY_USAGE_CHECK: s = "CRYPT_E_NO_VERIFY_USAGE_CHECK"; break;
    case CRYPT_E_VERIFY_USAGE_OFFLINE: s = "CRYPT_E_VERIFY_USAGE_OFFLINE"; break;
    case CRYPT_E_NOT_IN_CTL: s = "CRYPT_E_NOT_IN_CTL"; break;
    case CRYPT_E_NO_TRUSTED_SIGNER: s = "CRYPT_E_NO_TRUSTED_SIGNER"; break;
    case CRYPT_E_OSS_ERROR: s = "CRYPT_E_OSS_ERROR"; break;
    case CERTSRV_E_BAD_REQUESTSUBJECT: s = "CERTSRV_E_BAD_REQUESTSUBJECT"; break;
    case CERTSRV_E_NO_REQUEST: s = "CERTSRV_E_NO_REQUEST"; break;
    case CERTSRV_E_BAD_REQUESTSTATUS: s = "CERTSRV_E_BAD_REQUESTSTATUS"; break;
    case CERTSRV_E_PROPERTY_EMPTY: s = "CERTSRV_E_PROPERTY_EMPTY"; break;
    case TRUST_E_SYSTEM_ERROR: s = "TRUST_E_SYSTEM_ERROR"; break;
    case TRUST_E_NO_SIGNER_CERT: s = "TRUST_E_NO_SIGNER_CERT"; break;
    case TRUST_E_COUNTER_SIGNER: s = "TRUST_E_COUNTER_SIGNER"; break;
    case TRUST_E_CERT_SIGNATURE: s = "TRUST_E_CERT_SIGNATURE"; break;
    case TRUST_E_TIME_STAMP: s = "TRUST_E_TIME_STAMP"; break;
    case TRUST_E_BAD_DIGEST: s = "TRUST_E_BAD_DIGEST"; break;
    case TRUST_E_BASIC_CONSTRAINTS: s = "TRUST_E_BASIC_CONSTRAINTS"; break;
    case TRUST_E_FINANCIAL_CRITERIA: s = "TRUST_E_FINANCIAL_CRITERIA"; break;
    case TRUST_E_PROVIDER_UNKNOWN: s = "TRUST_E_PROVIDER_UNKNOWN"; break;
    case TRUST_E_ACTION_UNKNOWN: s = "TRUST_E_ACTION_UNKNOWN"; break;
    case TRUST_E_SUBJECT_FORM_UNKNOWN: s = "TRUST_E_SUBJECT_FORM_UNKNOWN"; break;
    case TRUST_E_SUBJECT_NOT_TRUSTED: s = "TRUST_E_SUBJECT_NOT_TRUSTED"; break;
    case DIGSIG_E_ENCODE: s = "DIGSIG_E_ENCODE"; break;
    case DIGSIG_E_DECODE: s = "DIGSIG_E_DECODE"; break;
    case DIGSIG_E_EXTENSIBILITY: s = "DIGSIG_E_EXTENSIBILITY"; break;
    case DIGSIG_E_CRYPTO: s = "DIGSIG_E_CRYPTO"; break;
    case PERSIST_E_SIZEDEFINITE: s = "PERSIST_E_SIZEDEFINITE"; break;
    case PERSIST_E_SIZEINDEFINITE: s = "PERSIST_E_SIZEINDEFINITE"; break;
    case PERSIST_E_NOTSELFSIZING: s = "PERSIST_E_NOTSELFSIZING"; break;
    case TRUST_E_NOSIGNATURE: s = "TRUST_E_NOSIGNATURE"; break;
    case CERT_E_EXPIRED: s = "CERT_E_EXPIRED"; break;
    case CERT_E_VALIDITYPERIODNESTING: s = "CERT_E_VALIDITYPERIODNESTING"; break;
    case CERT_E_ROLE: s = "CERT_E_ROLE"; break;
    case CERT_E_PATHLENCONST: s = "CERT_E_PATHLENCONST"; break;
    case CERT_E_CRITICAL: s = "CERT_E_CRITICAL"; break;
    case CERT_E_PURPOSE: s = "CERT_E_PURPOSE"; break;
    case CERT_E_ISSUERCHAINING: s = "CERT_E_ISSUERCHAINING"; break;
    case CERT_E_MALFORMED: s = "CERT_E_MALFORMED"; break;
    case CERT_E_UNTRUSTEDROOT: s = "CERT_E_UNTRUSTEDROOT"; break;
    case CERT_E_CHAINING: s = "CERT_E_CHAINING"; break;
    case TRUST_E_FAIL: s = "TRUST_E_FAIL"; break;
    case CERT_E_REVOKED: s = "CERT_E_REVOKED"; break;
    case CERT_E_UNTRUSTEDTESTROOT: s = "CERT_E_UNTRUSTEDTESTROOT"; break;
    case CERT_E_REVOCATION_FAILURE: s = "CERT_E_REVOCATION_FAILURE"; break;
    case CERT_E_CN_NO_MATCH: s = "CERT_E_CN_NO_MATCH"; break;
    case CERT_E_WRONG_USAGE: s = "CERT_E_WRONG_USAGE"; break;
    case SPAPI_E_EXPECTED_SECTION_NAME: s = "SPAPI_E_EXPECTED_SECTION_NAME"; break;
    case SPAPI_E_BAD_SECTION_NAME_LINE: s = "SPAPI_E_BAD_SECTION_NAME_LINE"; break;
    case SPAPI_E_SECTION_NAME_TOO_LONG: s = "SPAPI_E_SECTION_NAME_TOO_LONG"; break;
    case SPAPI_E_GENERAL_SYNTAX: s = "SPAPI_E_GENERAL_SYNTAX"; break;
    case SPAPI_E_WRONG_INF_STYLE: s = "SPAPI_E_WRONG_INF_STYLE"; break;
    case SPAPI_E_SECTION_NOT_FOUND: s = "SPAPI_E_SECTION_NOT_FOUND"; break;
    case SPAPI_E_LINE_NOT_FOUND: s = "SPAPI_E_LINE_NOT_FOUND"; break;
    case SPAPI_E_NO_ASSOCIATED_CLASS: s = "SPAPI_E_NO_ASSOCIATED_CLASS"; break;
    case SPAPI_E_CLASS_MISMATCH: s = "SPAPI_E_CLASS_MISMATCH"; break;
    case SPAPI_E_DUPLICATE_FOUND: s = "SPAPI_E_DUPLICATE_FOUND"; break;
    case SPAPI_E_NO_DRIVER_SELECTED: s = "SPAPI_E_NO_DRIVER_SELECTED"; break;
    case SPAPI_E_KEY_DOES_NOT_EXIST: s = "SPAPI_E_KEY_DOES_NOT_EXIST"; break;
    case SPAPI_E_INVALID_DEVINST_NAME: s = "SPAPI_E_INVALID_DEVINST_NAME"; break;
    case SPAPI_E_INVALID_CLASS: s = "SPAPI_E_INVALID_CLASS"; break;
    case SPAPI_E_DEVINST_ALREADY_EXISTS: s = "SPAPI_E_DEVINST_ALREADY_EXISTS"; break;
    case SPAPI_E_DEVINFO_NOT_REGISTERED: s = "SPAPI_E_DEVINFO_NOT_REGISTERED"; break;
    case SPAPI_E_INVALID_REG_PROPERTY: s = "SPAPI_E_INVALID_REG_PROPERTY"; break;
    case SPAPI_E_NO_INF: s = "SPAPI_E_NO_INF"; break;
    case SPAPI_E_NO_SUCH_DEVINST: s = "SPAPI_E_NO_SUCH_DEVINST"; break;
    case SPAPI_E_CANT_LOAD_CLASS_ICON: s = "SPAPI_E_CANT_LOAD_CLASS_ICON"; break;
    case SPAPI_E_INVALID_CLASS_INSTALLER: s = "SPAPI_E_INVALID_CLASS_INSTALLER"; break;
    case SPAPI_E_DI_DO_DEFAULT: s = "SPAPI_E_DI_DO_DEFAULT"; break;
    case SPAPI_E_DI_NOFILECOPY: s = "SPAPI_E_DI_NOFILECOPY"; break;
    case SPAPI_E_INVALID_HWPROFILE: s = "SPAPI_E_INVALID_HWPROFILE"; break;
    case SPAPI_E_NO_DEVICE_SELECTED: s = "SPAPI_E_NO_DEVICE_SELECTED"; break;
    case SPAPI_E_DEVINFO_LIST_LOCKED: s = "SPAPI_E_DEVINFO_LIST_LOCKED"; break;
    case SPAPI_E_DEVINFO_DATA_LOCKED: s = "SPAPI_E_DEVINFO_DATA_LOCKED"; break;
    case SPAPI_E_DI_BAD_PATH: s = "SPAPI_E_DI_BAD_PATH"; break;
    case SPAPI_E_NO_CLASSINSTALL_PARAMS: s = "SPAPI_E_NO_CLASSINSTALL_PARAMS"; break;
    case SPAPI_E_FILEQUEUE_LOCKED: s = "SPAPI_E_FILEQUEUE_LOCKED"; break;
    case SPAPI_E_BAD_SERVICE_INSTALLSECT: s = "SPAPI_E_BAD_SERVICE_INSTALLSECT"; break;
    case SPAPI_E_NO_CLASS_DRIVER_LIST: s = "SPAPI_E_NO_CLASS_DRIVER_LIST"; break;
    case SPAPI_E_NO_ASSOCIATED_SERVICE: s = "SPAPI_E_NO_ASSOCIATED_SERVICE"; break;
    case SPAPI_E_NO_DEFAULT_DEVICE_INTERFACE: s = "SPAPI_E_NO_DEFAULT_DEVICE_INTERFACE"; break;
    case SPAPI_E_DEVICE_INTERFACE_ACTIVE: s = "SPAPI_E_DEVICE_INTERFACE_ACTIVE"; break;
    case SPAPI_E_DEVICE_INTERFACE_REMOVED: s = "SPAPI_E_DEVICE_INTERFACE_REMOVED"; break;
    case SPAPI_E_BAD_INTERFACE_INSTALLSECT: s = "SPAPI_E_BAD_INTERFACE_INSTALLSECT"; break;
    case SPAPI_E_NO_SUCH_INTERFACE_CLASS: s = "SPAPI_E_NO_SUCH_INTERFACE_CLASS"; break;
    case SPAPI_E_INVALID_REFERENCE_STRING: s = "SPAPI_E_INVALID_REFERENCE_STRING"; break;
    case SPAPI_E_INVALID_MACHINENAME: s = "SPAPI_E_INVALID_MACHINENAME"; break;
    case SPAPI_E_REMOTE_COMM_FAILURE: s = "SPAPI_E_REMOTE_COMM_FAILURE"; break;
    case SPAPI_E_MACHINE_UNAVAILABLE: s = "SPAPI_E_MACHINE_UNAVAILABLE"; break;
    case SPAPI_E_NO_CONFIGMGR_SERVICES: s = "SPAPI_E_NO_CONFIGMGR_SERVICES"; break;
    case SPAPI_E_INVALID_PROPPAGE_PROVIDER: s = "SPAPI_E_INVALID_PROPPAGE_PROVIDER"; break;
    case SPAPI_E_NO_SUCH_DEVICE_INTERFACE: s = "SPAPI_E_NO_SUCH_DEVICE_INTERFACE"; break;
    case SPAPI_E_DI_POSTPROCESSING_REQUIRED: s = "SPAPI_E_DI_POSTPROCESSING_REQUIRED"; break;
    case SPAPI_E_INVALID_COINSTALLER: s = "SPAPI_E_INVALID_COINSTALLER"; break;
    case SPAPI_E_NO_COMPAT_DRIVERS: s = "SPAPI_E_NO_COMPAT_DRIVERS"; break;
    case SPAPI_E_NO_DEVICE_ICON: s = "SPAPI_E_NO_DEVICE_ICON"; break;
    case SPAPI_E_INVALID_INF_LOGCONFIG: s = "SPAPI_E_INVALID_INF_LOGCONFIG"; break;
    case SPAPI_E_DI_DONT_INSTALL: s = "SPAPI_E_DI_DONT_INSTALL"; break;
    case SPAPI_E_INVALID_FILTER_DRIVER: s = "SPAPI_E_INVALID_FILTER_DRIVER"; break;
    case SPAPI_E_ERROR_NOT_INSTALLED: s = "SPAPI_E_ERROR_NOT_INSTALLED"; break;
    }
    if( !s ) {
	s = buf;
	snprintf(buf, sizeof(buf), "hr=0x%x code=%d", hr, code);
    }

    return s;
}

char*
fourcc2str(int fourcc, char *buf, int len) {
    int i;
    char *orig=buf;

    do {
	i = snprintf(buf, len, "{%x ", fourcc);
	assertb(i>0);
	buf += i;
	len -= i;
	
	for(i=0; i<4; i++) {
	    if( !isprint(((char*)&fourcc)[i]) ) break;
	}
	if( i >= 4 ) {
	    assertb(len >= 4);
	    memcpy(buf, &fourcc, 4);
	    buf += 4;
	    len -= 4;
	}
	else {
	    char *s = 0;
	    switch(fourcc) {
	    case BI_RGB: s="BI_RGB"; break;
	    case BI_RLE8: s="BI_RLE8"; break;
	    case BI_RLE4: s="BI_RLE4"; break;
	    case BI_BITFIELDS: s="BI_BITFIELDS"; break;
	    default: s = 0; break;
	    }
	    if( s ) {
		i = snprintf(buf, len, "%s", s);
		assertb(i>0);
		buf += i;
		len -= i;
	    }
	}
	assertb(len>2);
	i = snprintf(buf, len, "}");
    } while(0);
    return orig;
}

char *
bmi2str(BITMAPINFO *lpbiOut, char *buf, int len) {
    char buf1[1024];
    if( !lpbiOut ) {
	snprintf(buf, len, "NULL");
    }
    else {
	snprintf(buf, len, 
		 "{biCompression=%s"
		 " biSize=%d"
		 " biPlanes=%d"
		 " biBitCount=%d"
		 " biClrUsed=%d"
		 " biWidth=%d"
		 " biHeight=%d}"
		 ,fourcc2str(lpbiOut->bmiHeader.biCompression, 
			     buf1, sizeof(buf1))
		 ,(int)lpbiOut->bmiHeader.biSize
		 ,(int)lpbiOut->bmiHeader.biPlanes
		 ,(int)lpbiOut->bmiHeader.biBitCount
		 ,(int)lpbiOut->bmiHeader.biClrUsed
		 ,(int)lpbiOut->bmiHeader.biWidth
		 ,(int)lpbiOut->bmiHeader.biHeight
		 );
    }
    return buf;
}


char*
driver_msg2str(int msg) {
    char *s=0;
    static char buf[128];

    switch(msg) {
    case DRV_LOAD: s="DRV_LOAD"; break;
    case DRV_ENABLE: s="DRV_ENABLE"; break;
    case DRV_OPEN: s="DRV_OPEN"; break;
    case DRV_CLOSE: s="DRV_CLOSE"; break;
    case DRV_DISABLE: s="DRV_DISABLE"; break;
    case DRV_FREE: s="DRV_FREE"; break;
    case DRV_CONFIGURE: s="DRV_CONFIGURE"; break;
    case DRV_QUERYCONFIGURE: s="DRV_QUERYCONFIGURE"; break;
    case DRV_INSTALL: s="DRV_INSTALL"; break;
    case DRV_REMOVE: s="DRV_REMOVE"; break;
    case DRV_EXITSESSION: s="DRV_EXITSESSION"; break;
    case DRV_POWER: s="DRV_POWER"; break;
    case DRV_CANCEL: s="DRV_CANCEL"; break;
	//case DRV_OK: s="DRV_OK"; break;
	//case DRV_RESTART: s="DRV_RESTART"; break;

	/*
    case ICM_USER: s="ICM_USER"; break;
	//case ICM_RESERVED: s="ICM_RESERVED"; break;
	//case ICM_RESERVED_LOW: s="ICM_RESERVED_LOW"; break;
	//case ICM_RESERVED_HIGH: s="ICM_RESERVED_HIGH"; break;
    case ICM_GETSTATE: s="ICM_GETSTATE"; break;
    case ICM_SETSTATE: s="ICM_SETSTATE"; break;
    case ICM_GETINFO: s="ICM_GETINFO"; break;
    case ICM_CONFIGURE: s="ICM_CONFIGURE"; break;
    case ICM_ABOUT: s="ICM_ABOUT"; break;
    case ICM_GETDEFAULTQUALITY: s="ICM_GETDEFAULTQUALITY"; break;
    case ICM_GETQUALITY: s="ICM_GETQUALITY"; break;
    case ICM_SETQUALITY: s="ICM_SETQUALITY"; break;
    case ICM_SET: s="ICM_SET"; break;
    case ICM_GET: s="ICM_GET"; break;
    case ICM_FRAMERATE: s="ICM_FRAMERATE"; break;
    case ICM_KEYFRAMERATE: s="ICM_KEYFRAMERATE"; break;
    case ICM_COMPRESS_GET_FORMAT: s="ICM_COMPRESS_GET_FORMAT"; break;
    case ICM_COMPRESS_GET_SIZE: s="ICM_COMPRESS_GET_SIZE"; break;
    case ICM_COMPRESS_QUERY: s="ICM_COMPRESS_QUERY"; break;
    case ICM_COMPRESS_BEGIN: s="ICM_COMPRESS_BEGIN"; break;
    case ICM_COMPRESS: s="ICM_COMPRESS"; break;
    case ICM_COMPRESS_END: s="ICM_COMPRESS_END"; break;
    case ICM_DECOMPRESS_GET_FORMAT: s="ICM_DECOMPRESS_GET_FORMAT"; break;
    case ICM_DECOMPRESS_QUERY: s="ICM_DECOMPRESS_QUERY"; break;
    case ICM_DECOMPRESS_BEGIN: s="ICM_DECOMPRESS_BEGIN"; break;
    case ICM_DECOMPRESS: s="ICM_DECOMPRESS"; break;
    case ICM_DECOMPRESS_END: s="ICM_DECOMPRESS_END"; break;
    case ICM_DECOMPRESS_SET_PALETTE: s="ICM_DECOMPRESS_SET_PALETTE"; break;
    case ICM_DECOMPRESS_GET_PALETTE: s="ICM_DECOMPRESS_GET_PALETTE"; break;
    case ICM_DRAW_QUERY: s="ICM_DRAW_QUERY"; break;
    case ICM_DRAW_BEGIN: s="ICM_DRAW_BEGIN"; break;
    case ICM_DRAW_GET_PALETTE: s="ICM_DRAW_GET_PALETTE"; break;
    case ICM_DRAW_START: s="ICM_DRAW_START"; break;
    case ICM_DRAW_STOP: s="ICM_DRAW_STOP"; break;
    case ICM_DRAW_END: s="ICM_DRAW_END"; break;
    case ICM_DRAW_GETTIME: s="ICM_DRAW_GETTIME"; break;
    case ICM_DRAW: s="ICM_DRAW"; break;
    case ICM_DRAW_WINDOW: s="ICM_DRAW_WINDOW"; break;
    case ICM_DRAW_SETTIME: s="ICM_DRAW_SETTIME"; break;
    case ICM_DRAW_REALIZE: s="ICM_DRAW_REALIZE"; break;
    case ICM_DRAW_FLUSH: s="ICM_DRAW_FLUSH"; break;
    case ICM_DRAW_RENDERBUFFER: s="ICM_DRAW_RENDERBUFFER"; break;
    case ICM_DRAW_START_PLAY: s="ICM_DRAW_START_PLAY"; break;
    case ICM_DRAW_STOP_PLAY: s="ICM_DRAW_STOP_PLAY"; break;
    case ICM_DRAW_SUGGESTFORMAT: s="ICM_DRAW_SUGGESTFORMAT"; break;
    case ICM_DRAW_CHANGEPALETTE: s="ICM_DRAW_CHANGEPALETTE"; break;
    case ICM_GETBUFFERSWANTED: s="ICM_GETBUFFERSWANTED"; break;
    case ICM_GETDEFAULTKEYFRAMERATE: s="ICM_GETDEFAULTKEYFRAMERATE"; break;
    case ICM_DECOMPRESSEX_BEGIN: s="ICM_DECOMPRESSEX_BEGIN"; break;
    case ICM_DECOMPRESSEX_QUERY: s="ICM_DECOMPRESSEX_QUERY"; break;
    case ICM_DECOMPRESSEX: s="ICM_DECOMPRESSEX"; break;
    case ICM_DECOMPRESSEX_END: s="ICM_DECOMPRESSEX_END"; break;
    case ICM_COMPRESS_FRAMES_INFO: s="ICM_COMPRESS_FRAMES_INFO"; break;
    case ICM_SET_STATUS_PROC: s="ICM_SET_STATUS_PROC"; break;
	*/
    }
    if( !s ) {
	snprintf(buf, sizeof(buf), "%d", msg);
	s = buf;
    }
    return s;
}

char *
icerr2str(DWORD rc, char *buf, int len) {
    char *s=0;
    switch(rc) {
	/*
    case ICERR_OK: s="ICERR_OK"; break;
    case ICERR_DONTDRAW: s="ICERR_DONTDRAW"; break;
    case ICERR_NEWPALETTE: s="ICERR_NEWPALETTE"; break;
    case ICERR_GOTOKEYFRAME: s="ICERR_GOTOKEYFRAME"; break;
    case ICERR_STOPDRAWING: s="ICERR_STOPDRAWING"; break;
    case ICERR_UNSUPPORTED: s="ICERR_UNSUPPORTED"; break;
    case ICERR_BADFORMAT: s="ICERR_BADFORMAT"; break;
    case ICERR_MEMORY: s="ICERR_MEMORY"; break;
    case ICERR_INTERNAL: s="ICERR_INTERNAL"; break;
    case ICERR_BADFLAGS: s="ICERR_BADFLAGS"; break;
    case ICERR_BADPARAM: s="ICERR_BADPARAM"; break;
    case ICERR_BADSIZE: s="ICERR_BADSIZE"; break;
    case ICERR_BADHANDLE: s="ICERR_BADHANDLE"; break;
    case ICERR_CANTUPDATE: s="ICERR_CANTUPDATE"; break;
    case ICERR_ABORT: s="ICERR_ABORT"; break;
    case ICERR_ERROR: s="ICERR_ERROR"; break;
    case ICERR_BADBITDEPTH: s="ICERR_BADBITDEPTH"; break;
    case ICERR_BADIMAGESIZE: s="ICERR_BADIMAGESIZE"; break;
    case ICERR_CUSTOM: s="ICERR_CUSTOM"; break;
	*/
    default:
	break;
    }
    if( s ) {
	snprintf(buf, len, "%s", s);
    }
    else {
	snprintf(buf, len, "%lu", (unsigned long)rc);
    }
    return buf;
}

char*
warn_win32_msg2str(int msg) {
    char *s=0;
    static char buf[128];

    switch(msg) {	
    case WM_NULL: s="WM_NULL"; break;
    case WM_CREATE: s="WM_CREATE"; break;
    case WM_DESTROY: s="WM_DESTROY"; break;
    case WM_MOVE: s="WM_MOVE"; break;
    case WM_SIZE: s="WM_SIZE"; break;
    case WM_ACTIVATE: s="WM_ACTIVATE"; break;
    case WM_SETFOCUS: s="WM_SETFOCUS"; break;
    case WM_KILLFOCUS: s="WM_KILLFOCUS"; break;
    case WM_ENABLE: s="WM_ENABLE"; break;
    case WM_SETREDRAW: s="WM_SETREDRAW"; break;
    case WM_SETTEXT: s="WM_SETTEXT"; break;
    case WM_GETTEXT: s="WM_GETTEXT"; break;
    case WM_GETTEXTLENGTH: s="WM_GETTEXTLENGTH"; break;
    case WM_PAINT: s="WM_PAINT"; break;
    case WM_CLOSE: s="WM_CLOSE"; break;
    case WM_QUERYENDSESSION: s="WM_QUERYENDSESSION"; break;
    case WM_QUERYOPEN: s="WM_QUERYOPEN"; break;
    case WM_ENDSESSION: s="WM_ENDSESSION"; break;
    case WM_QUIT: s="WM_QUIT"; break;
    case WM_ERASEBKGND: s="WM_ERASEBKGND"; break;
    case WM_SYSCOLORCHANGE: s="WM_SYSCOLORCHANGE"; break;
    case WM_SHOWWINDOW: s="WM_SHOWWINDOW"; break;
    case WM_WININICHANGE: s="WM_WININICHANGE"; break;
	//case WM_SETTINGCHANGE: s="WM_SETTINGCHANGE"; break;
    case WM_DEVMODECHANGE: s="WM_DEVMODECHANGE"; break;
    case WM_ACTIVATEAPP: s="WM_ACTIVATEAPP"; break;
    case WM_FONTCHANGE: s="WM_FONTCHANGE"; break;
    case WM_TIMECHANGE: s="WM_TIMECHANGE"; break;
    case WM_CANCELMODE: s="WM_CANCELMODE"; break;
    case WM_SETCURSOR: s="WM_SETCURSOR"; break;
    case WM_MOUSEACTIVATE: s="WM_MOUSEACTIVATE"; break;
    case WM_CHILDACTIVATE: s="WM_CHILDACTIVATE"; break;
    case WM_QUEUESYNC: s="WM_QUEUESYNC"; break;
    case WM_GETMINMAXINFO: s="WM_GETMINMAXINFO"; break;
    case WM_PAINTICON: s="WM_PAINTICON"; break;
    case WM_ICONERASEBKGND: s="WM_ICONERASEBKGND"; break;
    case WM_NEXTDLGCTL: s="WM_NEXTDLGCTL"; break;
    case WM_SPOOLERSTATUS: s="WM_SPOOLERSTATUS"; break;
    case WM_DRAWITEM: s="WM_DRAWITEM"; break;
    case WM_MEASUREITEM: s="WM_MEASUREITEM"; break;
    case WM_DELETEITEM: s="WM_DELETEITEM"; break;
    case WM_VKEYTOITEM: s="WM_VKEYTOITEM"; break;
    case WM_CHARTOITEM: s="WM_CHARTOITEM"; break;
    case WM_SETFONT: s="WM_SETFONT"; break;
    case WM_GETFONT: s="WM_GETFONT"; break;
    case WM_SETHOTKEY: s="WM_SETHOTKEY"; break;
    case WM_GETHOTKEY: s="WM_GETHOTKEY"; break;
    case WM_QUERYDRAGICON: s="WM_QUERYDRAGICON"; break;
    case WM_COMPAREITEM: s="WM_COMPAREITEM"; break;
    case WM_GETOBJECT: s="WM_GETOBJECT"; break;
    case WM_COMPACTING: s="WM_COMPACTING"; break;
    case WM_COMMNOTIFY: s="WM_COMMNOTIFY"; break;
    case WM_WINDOWPOSCHANGING: s="WM_WINDOWPOSCHANGING"; break;
    case WM_WINDOWPOSCHANGED: s="WM_WINDOWPOSCHANGED"; break;
    case WM_POWER: s="WM_POWER"; break;
    case WM_COPYDATA: s="WM_COPYDATA"; break;
    case WM_CANCELJOURNAL: s="WM_CANCELJOURNAL"; break;
    case WM_NOTIFY: s="WM_NOTIFY"; break;
    case WM_INPUTLANGCHANGEREQUEST: s="WM_INPUTLANGCHANGEREQUEST"; break;
    case WM_INPUTLANGCHANGE: s="WM_INPUTLANGCHANGE"; break;
    case WM_TCARD: s="WM_TCARD"; break;
    case WM_HELP: s="WM_HELP"; break;
    case WM_USERCHANGED: s="WM_USERCHANGED"; break;
    case WM_NOTIFYFORMAT: s="WM_NOTIFYFORMAT"; break;
    case WM_CONTEXTMENU: s="WM_CONTEXTMENU"; break;
    case WM_STYLECHANGING: s="WM_STYLECHANGING"; break;
    case WM_STYLECHANGED: s="WM_STYLECHANGED"; break;
    case WM_DISPLAYCHANGE: s="WM_DISPLAYCHANGE"; break;
    case WM_GETICON: s="WM_GETICON"; break;
    case WM_SETICON: s="WM_SETICON"; break;
    case WM_NCCREATE: s="WM_NCCREATE"; break;
    case WM_NCDESTROY: s="WM_NCDESTROY"; break;
    case WM_NCCALCSIZE: s="WM_NCCALCSIZE"; break;
    case WM_NCHITTEST: s="WM_NCHITTEST"; break;
    case WM_NCPAINT: s="WM_NCPAINT"; break;
    case WM_NCACTIVATE: s="WM_NCACTIVATE"; break;
    case WM_GETDLGCODE: s="WM_GETDLGCODE"; break;
    case WM_SYNCPAINT: s="WM_SYNCPAINT"; break;
    case WM_NCMOUSEMOVE: s="WM_NCMOUSEMOVE"; break;
    case WM_NCLBUTTONDOWN: s="WM_NCLBUTTONDOWN"; break;
    case WM_NCLBUTTONUP: s="WM_NCLBUTTONUP"; break;
    case WM_NCLBUTTONDBLCLK: s="WM_NCLBUTTONDBLCLK"; break;
    case WM_NCRBUTTONDOWN: s="WM_NCRBUTTONDOWN"; break;
    case WM_NCRBUTTONUP: s="WM_NCRBUTTONUP"; break;
    case WM_NCRBUTTONDBLCLK: s="WM_NCRBUTTONDBLCLK"; break;
    case WM_NCMBUTTONDOWN: s="WM_NCMBUTTONDOWN"; break;
    case WM_NCMBUTTONUP: s="WM_NCMBUTTONUP"; break;
    case WM_NCMBUTTONDBLCLK: s="WM_NCMBUTTONDBLCLK"; break;
	//case WM_NCXBUTTONDOWN: s="WM_NCXBUTTONDOWN"; break;
	//case WM_NCXBUTTONUP: s="WM_NCXBUTTONUP"; break;
	//case WM_NCXBUTTONDBLCLK: s="WM_NCXBUTTONDBLCLK"; break;
	//case WM_INPUT: s="WM_INPUT"; break;
    case WM_KEYFIRST: s="WM_KEYFIRST"; break;
	//case WM_KEYDOWN: s="WM_KEYDOWN"; break;
    case WM_KEYUP: s="WM_KEYUP"; break;
    case WM_CHAR: s="WM_CHAR"; break;
    case WM_DEADCHAR: s="WM_DEADCHAR"; break;
    case WM_SYSKEYDOWN: s="WM_SYSKEYDOWN"; break;
    case WM_SYSKEYUP: s="WM_SYSKEYUP"; break;
    case WM_SYSCHAR: s="WM_SYSCHAR"; break;
    case WM_SYSDEADCHAR: s="WM_SYSDEADCHAR"; break;
	//case WM_UNICHAR: s="WM_UNICHAR"; break;
    case WM_KEYLAST: s="WM_KEYLAST"; break;
	//case WM_KEYLAST: s="WM_KEYLAST"; break;
    case WM_IME_STARTCOMPOSITION: s="WM_IME_STARTCOMPOSITION"; break;
    case WM_IME_ENDCOMPOSITION: s="WM_IME_ENDCOMPOSITION"; break;
    case WM_IME_COMPOSITION: s="WM_IME_COMPOSITION"; break;
	//case WM_IME_KEYLAST: s="WM_IME_KEYLAST"; break;
    case WM_INITDIALOG: s="WM_INITDIALOG"; break;
    case WM_COMMAND: s="WM_COMMAND"; break;
    case WM_SYSCOMMAND: s="WM_SYSCOMMAND"; break;
    case WM_TIMER: s="WM_TIMER"; break;
    case WM_HSCROLL: s="WM_HSCROLL"; break;
    case WM_VSCROLL: s="WM_VSCROLL"; break;
    case WM_INITMENU: s="WM_INITMENU"; break;
    case WM_INITMENUPOPUP: s="WM_INITMENUPOPUP"; break;
    case WM_MENUSELECT: s="WM_MENUSELECT"; break;
    case WM_MENUCHAR: s="WM_MENUCHAR"; break;
    case WM_ENTERIDLE: s="WM_ENTERIDLE"; break;
    case WM_MENURBUTTONUP: s="WM_MENURBUTTONUP"; break;
    case WM_MENUDRAG: s="WM_MENUDRAG"; break;
    case WM_MENUGETOBJECT: s="WM_MENUGETOBJECT"; break;
    case WM_UNINITMENUPOPUP: s="WM_UNINITMENUPOPUP"; break;
    case WM_MENUCOMMAND: s="WM_MENUCOMMAND"; break;
	//case WM_CHANGEUISTATE: s="WM_CHANGEUISTATE"; break;
	//case WM_UPDATEUISTATE: s="WM_UPDATEUISTATE"; break;
	//case WM_QUERYUISTATE: s="WM_QUERYUISTATE"; break;
    case WM_CTLCOLORMSGBOX: s="WM_CTLCOLORMSGBOX"; break;
    case WM_CTLCOLOREDIT: s="WM_CTLCOLOREDIT"; break;
    case WM_CTLCOLORLISTBOX: s="WM_CTLCOLORLISTBOX"; break;
    case WM_CTLCOLORBTN: s="WM_CTLCOLORBTN"; break;
    case WM_CTLCOLORDLG: s="WM_CTLCOLORDLG"; break;
    case WM_CTLCOLORSCROLLBAR: s="WM_CTLCOLORSCROLLBAR"; break;
    case WM_CTLCOLORSTATIC: s="WM_CTLCOLORSTATIC"; break;
    case WM_MOUSEFIRST: s="WM_MOUSEFIRST"; break;
	//case WM_MOUSEMOVE: s="WM_MOUSEMOVE"; break;
    case WM_LBUTTONDOWN: s="WM_LBUTTONDOWN"; break;
    case WM_LBUTTONUP: s="WM_LBUTTONUP"; break;
    case WM_LBUTTONDBLCLK: s="WM_LBUTTONDBLCLK"; break;
    case WM_RBUTTONDOWN: s="WM_RBUTTONDOWN"; break;
    case WM_RBUTTONUP: s="WM_RBUTTONUP"; break;
    case WM_RBUTTONDBLCLK: s="WM_RBUTTONDBLCLK"; break;
    case WM_MBUTTONDOWN: s="WM_MBUTTONDOWN"; break;
    case WM_MBUTTONUP: s="WM_MBUTTONUP"; break;
    case WM_MBUTTONDBLCLK: s="WM_MBUTTONDBLCLK"; break;
	//case WM_MOUSEWHEEL: s="WM_MOUSEWHEEL"; break;
	//case WM_XBUTTONDOWN: s="WM_XBUTTONDOWN"; break;
	//case WM_XBUTTONUP: s="WM_XBUTTONUP"; break;
	//case WM_XBUTTONDBLCLK: s="WM_XBUTTONDBLCLK"; break;
	//case WM_MOUSELAST: s="WM_MOUSELAST"; break;
	//case WM_MOUSELAST: s="WM_MOUSELAST"; break;
	//case WM_MOUSELAST: s="WM_MOUSELAST"; break;
    case WM_PARENTNOTIFY: s="WM_PARENTNOTIFY"; break;
    case WM_ENTERMENULOOP: s="WM_ENTERMENULOOP"; break;
    case WM_EXITMENULOOP: s="WM_EXITMENULOOP"; break;
    case WM_NEXTMENU: s="WM_NEXTMENU"; break;
    case WM_SIZING: s="WM_SIZING"; break;
    case WM_CAPTURECHANGED: s="WM_CAPTURECHANGED"; break;
    case WM_MOVING: s="WM_MOVING"; break;
    case WM_POWERBROADCAST: s="WM_POWERBROADCAST"; break;
    case WM_DEVICECHANGE: s="WM_DEVICECHANGE"; break;
    case WM_MDICREATE: s="WM_MDICREATE"; break;
    case WM_MDIDESTROY: s="WM_MDIDESTROY"; break;
    case WM_MDIACTIVATE: s="WM_MDIACTIVATE"; break;
    case WM_MDIRESTORE: s="WM_MDIRESTORE"; break;
    case WM_MDINEXT: s="WM_MDINEXT"; break;
    case WM_MDIMAXIMIZE: s="WM_MDIMAXIMIZE"; break;
    case WM_MDITILE: s="WM_MDITILE"; break;
    case WM_MDICASCADE: s="WM_MDICASCADE"; break;
    case WM_MDIICONARRANGE: s="WM_MDIICONARRANGE"; break;
    case WM_MDIGETACTIVE: s="WM_MDIGETACTIVE"; break;
    case WM_MDISETMENU: s="WM_MDISETMENU"; break;
    case WM_ENTERSIZEMOVE: s="WM_ENTERSIZEMOVE"; break;
    case WM_EXITSIZEMOVE: s="WM_EXITSIZEMOVE"; break;
    case WM_DROPFILES: s="WM_DROPFILES"; break;
    case WM_MDIREFRESHMENU: s="WM_MDIREFRESHMENU"; break;
    case WM_IME_SETCONTEXT: s="WM_IME_SETCONTEXT"; break;
    case WM_IME_NOTIFY: s="WM_IME_NOTIFY"; break;
    case WM_IME_CONTROL: s="WM_IME_CONTROL"; break;
    case WM_IME_COMPOSITIONFULL: s="WM_IME_COMPOSITIONFULL"; break;
    case WM_IME_SELECT: s="WM_IME_SELECT"; break;
    case WM_IME_CHAR: s="WM_IME_CHAR"; break;
    case WM_IME_REQUEST: s="WM_IME_REQUEST"; break;
    case WM_IME_KEYDOWN: s="WM_IME_KEYDOWN"; break;
    case WM_IME_KEYUP: s="WM_IME_KEYUP"; break;
    case WM_MOUSEHOVER: s="WM_MOUSEHOVER"; break;
    case WM_MOUSELEAVE: s="WM_MOUSELEAVE"; break;
    case WM_NCMOUSEHOVER: s="WM_NCMOUSEHOVER"; break;
    case WM_NCMOUSELEAVE: s="WM_NCMOUSELEAVE"; break;
	//case WM_WTSSESSION_CHANGE: s="WM_WTSSESSION_CHANGE"; break;
	//case WM_TABLET_FIRST: s="WM_TABLET_FIRST"; break;
	//case WM_TABLET_LAST: s="WM_TABLET_LAST"; break;
    case WM_CUT: s="WM_CUT"; break;
    case WM_COPY: s="WM_COPY"; break;
    case WM_PASTE: s="WM_PASTE"; break;
    case WM_CLEAR: s="WM_CLEAR"; break;
    case WM_UNDO: s="WM_UNDO"; break;
    case WM_RENDERFORMAT: s="WM_RENDERFORMAT"; break;
    case WM_RENDERALLFORMATS: s="WM_RENDERALLFORMATS"; break;
    case WM_DESTROYCLIPBOARD: s="WM_DESTROYCLIPBOARD"; break;
    case WM_DRAWCLIPBOARD: s="WM_DRAWCLIPBOARD"; break;
    case WM_PAINTCLIPBOARD: s="WM_PAINTCLIPBOARD"; break;
    case WM_VSCROLLCLIPBOARD: s="WM_VSCROLLCLIPBOARD"; break;
    case WM_SIZECLIPBOARD: s="WM_SIZECLIPBOARD"; break;
    case WM_ASKCBFORMATNAME: s="WM_ASKCBFORMATNAME"; break;
    case WM_CHANGECBCHAIN: s="WM_CHANGECBCHAIN"; break;
    case WM_HSCROLLCLIPBOARD: s="WM_HSCROLLCLIPBOARD"; break;
    case WM_QUERYNEWPALETTE: s="WM_QUERYNEWPALETTE"; break;
    case WM_PALETTEISCHANGING: s="WM_PALETTEISCHANGING"; break;
    case WM_PALETTECHANGED: s="WM_PALETTECHANGED"; break;
    case WM_HOTKEY: s="WM_HOTKEY"; break;
    case WM_PRINT: s="WM_PRINT"; break;
    case WM_PRINTCLIENT: s="WM_PRINTCLIENT"; break;
	//case WM_APPCOMMAND: s="WM_APPCOMMAND"; break;
	//case WM_THEMECHANGED: s="WM_THEMECHANGED"; break;
    case WM_HANDHELDFIRST: s="WM_HANDHELDFIRST"; break;
    case WM_HANDHELDLAST: s="WM_HANDHELDLAST"; break;
    case WM_AFXFIRST: s="WM_AFXFIRST"; break;
    case WM_AFXLAST: s="WM_AFXLAST"; break;
    case WM_PENWINFIRST: s="WM_PENWINFIRST"; break;
    case WM_PENWINLAST: s="WM_PENWINLAST"; break;
    case WM_APP: s="WM_APP"; break;
    case WM_USER: s="WM_USER"; break;
    }
    if( !s ) {
	snprintf(buf, sizeof(buf), "%d (0x%08x)", msg, msg);
	s = buf;
    }
    return s;
}
