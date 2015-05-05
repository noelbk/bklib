#include <stdio.h>
#include <string.h>

#include "bklib/opt.h"
#include "bklib/configbk.h"

char *tests[] = {
    "arg1    arg2    arg3 \"\" that_was_empty  \" spaced out string\n\"  ",
    "arg1 arg2 quoted_\"\r\n\t\\\"    ",
    0,
};


int main() {
    char *src, *p, buf[1024], **t, srcbuf[1024];
    int n;

    snprintf(srcbuf, sizeof(srcbuf), "Noel Burton-Krahn");
    opt_quoted_enc_style(srcbuf, -1, buf, sizeof(buf), OPT_QUOTED_ENC_PERL);
    snprintf(srcbuf, sizeof(srcbuf), "Noel's Home Laptop");
    opt_quoted_enc_style(srcbuf, -1, buf, sizeof(buf), OPT_QUOTED_ENC_PERL);

    for(t=tests; *t; t++) {
	printf("test: [%s]\n", *t);
	strncpy(buf, *t, sizeof(buf));
	src = buf;
	while(*src) {
	    p = src;
	    n = opt_quoted_dec(&src, src, strlen(src), 0);
	    printf("arg: len=%d [%s]\n", n, p);
	}
	printf("end\n\n");
    }

    {
	char *user = "noel\nburton\001krahn";
	char *desk = "My Desktop";
	int ver = 1234;
	opt_cat(buf, sizeof(buf), 
		    "login", 
		    "-user", user, 
		    "-desk", desk, 
		    "-ver", OPT_CAT_INT, ver, 
		    OPT_CAT_END);
	printf("opt_cat: buf=%s\n", buf);
    }

    return 0;
}
