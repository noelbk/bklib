// included by warn.c
#include "bkwin32.h"
#include <winerror.h>
#include <vfw.h>
#include <mmsystem.h>

int
syserr() {
    return GetLastError();
}

char*
syserr2str(int syserr, char *buf, int len) {
    *buf = 0;
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		  NULL,
		  syserr,
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
		  buf,
		  len,
		  NULL 
		  );
    return buf;
}

static DWORD syserr_saved;

void
syserr_save() {
    syserr_saved = GetLastError();
}

void
syserr_restore() {
    SetLastError(syserr_saved);
}


char *
winmsg2str(int msg) {
    char *s = 0;
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
    case WM_KEYFIRST: s="WM_KEYFIRST"; break;
    case WM_KEYUP: s="WM_KEYUP"; break;
    case WM_CHAR: s="WM_CHAR"; break;
    case WM_DEADCHAR: s="WM_DEADCHAR"; break;
    case WM_SYSKEYDOWN: s="WM_SYSKEYDOWN"; break;
    case WM_SYSKEYUP: s="WM_SYSKEYUP"; break;
    case WM_SYSCHAR: s="WM_SYSCHAR"; break;
    case WM_SYSDEADCHAR: s="WM_SYSDEADCHAR"; break;
    case WM_KEYLAST: s="WM_KEYLAST"; break;
    case WM_IME_STARTCOMPOSITION: s="WM_IME_STARTCOMPOSITION"; break;
    case WM_IME_ENDCOMPOSITION: s="WM_IME_ENDCOMPOSITION"; break;
    case WM_IME_COMPOSITION: s="WM_IME_COMPOSITION"; break;
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
    case WM_CTLCOLORMSGBOX: s="WM_CTLCOLORMSGBOX"; break;
    case WM_CTLCOLOREDIT: s="WM_CTLCOLOREDIT"; break;
    case WM_CTLCOLORLISTBOX: s="WM_CTLCOLORLISTBOX"; break;
    case WM_CTLCOLORBTN: s="WM_CTLCOLORBTN"; break;
    case WM_CTLCOLORDLG: s="WM_CTLCOLORDLG"; break;
    case WM_CTLCOLORSCROLLBAR: s="WM_CTLCOLORSCROLLBAR"; break;
    case WM_CTLCOLORSTATIC: s="WM_CTLCOLORSTATIC"; break;
    case WM_MOUSEFIRST: s="WM_MOUSEFIRST"; break;
    case WM_LBUTTONDOWN: s="WM_LBUTTONDOWN"; break;
    case WM_LBUTTONUP: s="WM_LBUTTONUP"; break;
    case WM_LBUTTONDBLCLK: s="WM_LBUTTONDBLCLK"; break;
    case WM_RBUTTONDOWN: s="WM_RBUTTONDOWN"; break;
    case WM_RBUTTONUP: s="WM_RBUTTONUP"; break;
    case WM_RBUTTONDBLCLK: s="WM_RBUTTONDBLCLK"; break;
    case WM_MBUTTONDOWN: s="WM_MBUTTONDOWN"; break;
    case WM_MBUTTONUP: s="WM_MBUTTONUP"; break;
    case WM_MBUTTONDBLCLK: s="WM_MBUTTONDBLCLK"; break;
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
    case WM_HANDHELDLAST: s="WM_HANDHELDLAST"; break;
    case WM_AFXFIRST: s="WM_AFXFIRST"; break;
    case WM_AFXLAST: s="WM_AFXLAST"; break;
    case WM_PENWINFIRST: s="WM_PENWINFIRST"; break;
    case WM_PENWINLAST: s="WM_PENWINLAST"; break;
    case WM_APP: s="WM_APP"; break;
    case WM_USER: s="WM_USER"; break;
    }
    return s;
}
