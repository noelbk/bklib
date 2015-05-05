#include <string.h>
#include <stdio.h>

#include "config.h"
#include "variant.h"
#include "warn.h"

char*
variant_fmt(VARIANT *v, char *buf, int len)
{
    DWORD iNeed = 0;
    DWORD iVSize = 0;
    DWORD iCurBufSize = 0;
    HRESULT hr;
    int i, n, err=-1;
    char *orig = buf;

    static char sbuf[4096];
    if( !buf ) {       
	buf = sbuf;
	len = sizeof(sbuf);
    }

    switch (v->vt) {
    case VT_NULL: i = snprintf(buf, len, "(NULL)"); break;
    case VT_BOOL: i = snprintf(buf, len, "%d", (int)v->boolVal != FALSE); break;
    case VT_UI1:  i = snprintf(buf, len, "%d", (int)v->bVal); break;
    case VT_I2:   i = snprintf(buf, len, "%d", (int)v->iVal); break;
    case VT_I4:   i = snprintf(buf, len, "%ld", (long)v->lVal); break;
    case VT_R4:   i = snprintf(buf, len, "%g", v->fltVal); break;
    case VT_R8:   i = snprintf(buf, len, "%g", v->dblVal); break;
    case VT_BSTR: i = snprintf(buf, len, "%wS", v->bstrVal); break;
			  
    case VT_BSTR|VT_ARRAY: {
	BSTR HUGEP *parray;
	long l, u;
	hr = SafeArrayGetUBound(v->parray, 1, &u);
	assertb_hresult(hr);
	hr = SafeArrayGetLBound(v->parray, 1, &l);
	assertb_hresult(hr);
	n = u-l+1;
	hr = SafeArrayAccessData(v->parray, (void **)&parray);
	assertb_hresult(hr);
	for(i=0; len > 0 && i<n; i++) {
	    err = -1;
	    if( i > 0 ) {
		l = snprintf(buf, len, " "); 
		assertb(l>=0);
		buf += l;
		len -= l;
	    }
	    l = snprintf(buf, len, "%wS", parray[i]); 
	    assertb(l>=0);
	    buf += l;
	    len -= l;
	    err = 0;
	}
	hr = SafeArrayUnaccessData(v->parray);
	i = err ? -1 : buf-orig;
    }
	break;

    case VT_DISPATCH:  // Currently only used for embedded objects
    case VT_BOOL|VT_ARRAY: 
    case VT_UI1|VT_ARRAY: 
    case VT_I2|VT_ARRAY: 
    case VT_I4|VT_ARRAY: 
    case VT_R4|VT_ARRAY: 
    case VT_R8|VT_ARRAY: 
	
    case VT_DISPATCH | VT_ARRAY: 
	i = snprintf(buf, len, "(complex type)");
	break;

    default:
	i = snprintf(buf, len, "(unknown type)");
    }
    
    return i>=0 ? orig : 0;
}

int
variant_array_bstrs(VARIANT *v, BSTR *str, int n)
{
    BSTR *arrayContents;
    int err=-1;
    HRESULT hr;
    int i;

    do {
	v->vt = VT_BSTR|VT_ARRAY;
	v->parray = SafeArrayCreateVector(VT_BSTR, 0, n);
	assertb_syserr(v->parray);
	hr = SafeArrayAccessData(v->parray, (void **)&arrayContents);
	assertb_hresult(hr);
	for(i=0; i<n; i++) {
	    arrayContents[i] = str[i];
	    str[i] = 0;
	}
	SafeArrayUnaccessData(v->parray);

	err = 0;
    } while(0);
    return err;
}

BSTR
cstr2bstr(const char *s) {
    OLECHAR *o=0;
    int err=-1;
    size_t l, n;
    BSTR b=0;

    do {
	assertb(s);
	l = strlen(s)+1; // add 1 for terminating null
	o = malloc(l*sizeof(*o));
	assertb_syserr(o);
	n = mbstowcs(o, s, l);
	assertb(n==l-1);
	b = SysAllocString(o);
	assertb(b);
	err = 0;
    } while(0);
    if( o ) {
	free(o);
    }
    if( err ) {
	if( b ) { 
	    SysFreeString(b); 
	    b = 0;
	}
    }
    return err ? 0 : b;
}

VARIANT*
variant_bstr(VARIANT *v, const char *s) {
    int err=-1;

    VariantInit(v);
    do {
	v->vt = VT_BSTR;
	v->bstrVal = cstr2bstr(s);
	assertb(v->bstrVal);
	err = 0;
    } while(0);
    if( err ) {
	if( v->bstrVal ) { 
	    SysFreeString(v->bstrVal); 
	    v->bstrVal = 0;
	}
	VariantInit(v);
    }
    return err ? 0 : v;
}


VARIANT*
variant_i4(VARIANT *v, long l) {
    int err=-1;
    VariantInit(v);
    do {
	v->vt = VT_I4;
	v->lVal = l;
	err = 0;
    } while(0);
    if( err ) {
	VariantInit(v);
    }
    return err ? 0 : v;
}


