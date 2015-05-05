
// who knows why Windows loves the underscore prefix?
#define vsnprintf  _vsnprintf
#define snprintf   _snprintf
#define popen      _popen
#define mkdir      _mkdir
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
//#define timezone   _timezone
#define inline     __inline

#define DLLCALL __stdcall
