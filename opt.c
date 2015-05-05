/*
    REVISION HISTORY:

    2002/02/27 D.Chan - opt_verify_word accepted alphanumeric and "_.,-". This is
                        is too restrictive (e.g. when the word is an email address)
			so I've expanded the set to be "@-_.,;".
*/

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "opt.h"
#include "configbk.h"

#ifndef WHITESPACE
#define WHITESPACE " \t\r\n"
#endif

int
opt_verify_word(const char *buf) {
    char *p;
    int i;
    
    for(p=(char*)buf, i=OPT_WORD_MAX; *p; p++, i--) {
	if( !i || (!isalnum(*p) && !strchr("@-_.,;", *p)) ) {
	    return 0;
	}
    }
    return 1;
}

// encode a bunch of args with a trailing into a buffer, teriminated by "\n\0"
// call opt_quoted_enc on 
#define OPT_CAT_END ((char*)-1)
#define OPT_CAT_INT ((char*)-2)
int
opt_cat(char *buf, int len, ...) {
    va_list ap;
    char *arg;
    int i;
    char *pout=buf;
    int nout=len;

    va_start(ap, len);
    while(1) {
	arg = va_arg(ap, char*);
	if( arg == OPT_CAT_END ) {
	    break;
	}
	if( !arg ) {
	    continue;
	}
	if( nout < 2 ) {
	    return -1;
	}
	if( pout > buf ) {
	    *pout = ' ';
	    pout++;
	    nout--;
	}
	if( arg == OPT_CAT_INT ) {
	    i = va_arg(ap, int);
	    i = sprintf(pout, "%d", i);
	}
	else {
	    if( opt_needs_quote(arg) ) {
		i = opt_quoted_enc(arg, -1, pout, nout);
	    }
	    else {
		i = sprintf(pout, "%.*s", nout, arg);
	    }
	}
	if( i < 0 ) {
	    return -1;
	}
	pout += i;
	nout -= i;
    }
    if( nout < 2 ) {
	return -1;
    }
    i = sprintf(pout, "\n");
    if( i < 0 ) {
	return -1;
    }
    pout += i;
    nout -= i;

    return pout - buf;
}

int
opt_needs_quote(char *str) {
    return !str || (strpbrk(str, OPT_QUOTED_CHARS) != 0);
}

int
opt_quoted_enc(char *src, int srclen, char *dst, int dstlen) {
    return opt_quoted_enc_style(src, srclen, dst, dstlen, OPT_QUOTED_ENC_C);
}

int
opt_quoted_enc_style(char *src, int srclen, char *dst, int dstlen, opt_quoted_enc_style_t style) {
    char *dst_in = dst, *end = dst + dstlen - 3;
    char *srcend;
    char quote;

    if( style == OPT_QUOTED_ENC_C ) {
	quote = '"';
    }
    else if ( style == OPT_QUOTED_ENC_PERL  ) {
	quote = '\'';
    }
    else {
	return -2;
    }

    if( !src ) {
      src = "";
      srclen = 0;
    }
    if( srclen < 0 ) {
      srclen = strlen(src);
    }
    srcend = src+srclen;

    *dst++ = quote;
    while(src < srcend && dst<end) {
	char *esc = 0;
	switch(*src) {
	case '\n': esc = "\\n"; break;
	case '\r': esc = "\\r"; break;
	case '\t': esc = "\\t"; break;
	case '\\': esc = "\\\\"; break;
	case '\0': esc = "\\0"; break;
	}
	if( !esc ) {
	    if( style == OPT_QUOTED_ENC_C ) {
		switch(*src) {
		case '\"': esc = "\\\""; break;
		}
	    }
	    else if( style == OPT_QUOTED_ENC_PERL ) {
		switch(*src) {
		case '\'': esc = "\\'"; break;
		}
	    }
	}
	if( esc ) {
	    dst += snprintf(dst, end-dst, "%s", esc);
	}
	else {
	    *dst++ = *src; 
	}
	src++;
    }
    if( dst<end ) {
	*dst++ = quote;
	*dst = 0;
	return dst-dst_in;
    }
    return -1;
}

int
opt_quoted_dec(char **psrc, char *dst, int len, int noslash) {
    char *src, *dst_in, *end = dst+len;
    int done;

    src = *psrc;
    dst_in = dst;
    end = dst+len-1;

    // skip leading whitespace
    src += strspn(src, WHITESPACE);

    done = 0;
    while(*src && dst<end && !done) {
	switch(*src) {
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	    // skip trailing whitespace
	    src += strspn(src, WHITESPACE);
	    done=1;
	    break;
	    
	case '"': {
	    // parse quoted string
	    int instr = 1;
	    src++;
	    while(instr && *src && dst<end) {
		switch(*src) {
		case '"':
		    instr=0;
		    src++; 
		    break;
		    
		case '\\':
		    if( noslash ) {
			*(dst++) = *(src++); 
		    }
		    else {
			src++;
			switch(*src) {
			case 'n': *dst++ = '\n'; break;
			case 'r': *dst++ = '\r'; break;
			case 't': *dst++ = '\t'; break;
			case '\\': *dst++ = '\\'; break;
			case '\0': *dst++ = '\0'; break;
			case '\"': *dst++ = '\"'; break;
			default: *(dst++) = *src; break;
			}
			src++;
		    }
		    break;

		default: 
		    *(dst++) = *(src++); 
		    break;
		}
	    }
	}
	    break;

	default:
	    *(dst++) = *(src++);
	    break;
	}
    }
    if( dst<=end ) {
	*dst = 0;
    }
    *psrc = src;
    return dst<=end ? dst - dst_in : -1;
}

int
opt_str2argv(char **pstr, int argn, char **argv) {
    char *str=*pstr, *end, *dst;
    int argc = 0;

    memset(argv, 0, argn*sizeof(*argv));
    end = str + strlen(str)+1;
    dst = str;
    while(*str && str<end) {
	argv[argc++] = dst;
	dst += opt_quoted_dec(&str, dst, end-str, 0)+1;
    }
    *dst = 0;
    *pstr = str;
    return argc;
}


int
opt_namev2optv(char **namev, int optn, OptSet *optv) {
    char *p, *q;
    int optc;
    memset(optv, 0, optn*sizeof(*optv));
    for(optc=0; *namev && optc<optn; namev++, optv++, optc++) {
	p = *namev;
	if( p[0] == '!' ) {
	    optv->required = 1;
	    p++;
	}
	if( p[1] == '=' ) {
	    optv->argtype = *p;
	    p += 2;
	}
	optv->names = p;
	q = strstr(p, "default:");
	if( q ) {
	    optv->defval = q+8;
	}
    }
    return optc;
}

OptSet*
opt_get(OptSet *optv, char *name) {
    int namelen, n;
    char *p;

    namelen = strlen(name);
    for(; optv->names; optv++) {
	p = optv->names;
	while(1) {
	    n = strcspn(p, "|" WHITESPACE);
	    if( n <= 0 ) break;
	    if( (namelen == n) && (strncmp(p, name, n) == 0) ) {
		// found it!
		break;
	    }
	    p += n;
	    if( *p != '|' || !*++p ) {
		n = 0;
		break;
	    }
	}
	if( n > 0 ) {
	    return optv;
	}
    }

    return 0;
}

int
opt_setval(OptSet *opt, int *argidx, int argc, char **argv) {
    int err = 0;
    char *p;

    do {
	if( *argidx >= argc ) {
	    err = OPT_ERR_OPTREQ;
	    break;
	}
	opt->argidx = *argidx;
	opt->argval = argv[*argidx];
	if( !opt->argtype ) {
	    // no required arg
	    opt->val.i = 1;
	    err = 0;
	    break;
	}
	// this option has a required arg.  We are pointing to the arg
	// right now unless we're pointing to the switch, which starts
	// with '-'
	if( opt->argval[0] == '-' ) {
	    (*argidx)++;
	}
	if( *argidx >= argc ) {
	    // error - missing required arg
	    err = OPT_ERR_ARGREQ;
	    break;
	}
	opt->argval = argv[*argidx];
	err = 0;
	switch(opt->argtype) {
	case 'f':
	    opt->val.f = strtod(opt->argval, &p);
	    if( p == opt->argval ) {
		err = OPT_ERR_ARG;
	    }
	    break;
	    
	case 'i':
	    opt->val.i = strtoul(opt->argval, &p, 0);
	    if( p == opt->argval ) {
		err = OPT_ERR_ARG;
	    }
	    break;

	case 's':
	    opt->val.s = opt->argval;
	    break;

	case 'w':
	    if( !opt_verify_word(opt->argval) ) {
		err = OPT_ERR_ARG;
	    }
	    else {
		opt->val.s = opt->argval;
	    }
	    break;

	default:
	    err = OPT_ERR_TYPE;
	    break;
	}
    } while(0);
    opt->err = err;
    return err;
}


// returns optind: the index of the first non-option, or -(optind+1)
// the index of the first bad option
// unknown option
// missing required arg
int
opt_argv2optv(OptSet *optv, int argc, char **argv) {
    int optind;
    char *parg;
    OptSet *opt;
    int i, err=-1;

    do {
	// try all options
	err = 0;
	for(optind=0; optind<argc; optind++) {
	    parg = argv[optind];
	    if( !strcmp(parg, "--") ) {
		optind++;
		break;
	    }
	    if( *parg != '-' ) {
		break;
	    }
	    parg += strspn(parg, "-");

	    // try all aliases for this opt
	    opt = opt_get(optv, parg);
	    if( !opt ) {
		err = OPT_ERR_OPT;
		break;
	    }
	    err = opt_setval(opt, &optind, argc, argv);
	    if( err ) { break; }
	}
	if( err ) break;

	// now find required args
	for(i=0, opt=optv; opt->names; i++, opt++) {
	    if( !opt->required || opt->argval ) {
		// this arg is either optional or already complete
		continue;
	    }
	    err = opt_setval(opt, &optind, argc, argv);
	    if( err ) break;
	}
	
	err = 0;
    } while(0);
    return err ? -(optind+1) : optind;
}


/* extract one argument from an argv list starting at opt->argi.  sets
   fields in opt, and increments opt->argi. returns OPT_ERROR_END at
   end, <0 otherwise
*/
int
opt_get_argv(opt_set_t *opts, int argc, char **argv, opt_iter_t *opt) {
    char *p, *arg;
    do {
	opt->error = OPT_ERROR_END;
	opt->id = 0;

	/* check if no more arguments */
	if( opt->argi >= argc ) {
	    break;
	}
	arg = argv[opt->argi];
	opt->arg = arg;

	/* check if not an option */
	if( !strchr("-", *arg) ) break; /* "-" is an argument itself */
	if( !strcmp(arg, "--") ) {      /* "--" means end of arguments */
	    opt->argi++;
	    break;
	}
	
	p = arg+1;
	if(*p == '-') p++; /* allow --arg */

	/* find matching option */
	for(; opts->name && strcmp(p, opts->name); opts++);
	if( !opts->name ) {
	    /* error: unknown option */
	    opt->error = OPT_ERROR_UNKNOWN;
	    break;
	}
	opt->argi++;
	opt->name = opts->name;
	opt->id   = opts->id;
	opt->type = opts->type;
	
	if( opt->type == OPT_TYPE_FLAG ) {
	    /* no argument, just toggle the flag */
	    opt->val.i = 1;
	    opt->error = 0;
	    break;
	}

	if( opt->argi >= argc ) {
	    /* error: missing arg */
	    opt->error = OPT_ERROR_MISSING;
	    break;
	}
	arg = argv[opt->argi];
	opt->argi++;
	
	p = arg;
	switch(opt->type) {
	case OPT_TYPE_STR:
	    opt->val.s = arg; 
	    p = arg+1;
	    break;
	case OPT_TYPE_INT:
	    opt->val.i = strtol(arg, &p, 0);
	    break;
	case OPT_TYPE_UINT:
	    opt->val.u = strtoul(arg, &p, 0);
	    break;
	case OPT_TYPE_REAL:
	    opt->val.d = strtod(arg, &p);
	    break;
	default:
	    break;
	}
	if( p <= arg ) {
	    /* error: not a number */
	    opt->error = OPT_ERROR_BAD;
	    opt->val.s = arg;
	}	
	opt->error = 0;
    } while(0);
    return opt->error ? opt->error : opt->id;
}

int
opt_perror(opt_iter_t *opt, FILE *f) {
    switch(opt->error) {
    case OPT_ERROR_UNKNOWN:
	fprintf(f, "unknown option %s\n", opt->arg);
	break;
    case OPT_ERROR_MISSING:
	fprintf(f, "unknown option %s\n", opt->arg);
	break;
    case OPT_ERROR_BAD: 
	fprintf(f, "bad argument for option %s: %s\n", 
		opt->arg, opt->val.s); 
	break;
    default:
	break;
    }
    return 0;
}
