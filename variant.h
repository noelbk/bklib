#ifndef VARIANT_H_INCLUDED
#define VARIANT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "bkwin32.h"
#include <oleauto.h>

char*
variant_fmt(VARIANT *v, char *buf, int len);

int
variant_array_bstrs(VARIANT *v, BSTR *str, int n);

VARIANT*
variant_bstr(VARIANT *v, const char *s);

VARIANT*
variant_i4(VARIANT *v, long l);

BSTR
cstr2bstr(const char *cstr);

#ifdef __cplusplus
}
#endif

#endif /* VARIANT_H_INCLUDED */
