#include <stdio.h>
#include <string.h>

#include "configbk.h"
#include "opt.h"
#include "opt_print.h"

int
opt_print_format(opt_print_t *opt, char *format) {
    if( strcasecmp(format, "perl")==0 ) {
	opt->format = OPT_PRINT_FORMAT_PERL;
    }
    else  if( strcasecmp(format, "xml")==0 ) {
	opt->format = OPT_PRINT_FORMAT_XML;
    } 
    else if( strcasecmp(format, "text")==0 ) {
	opt->format = OPT_PRINT_FORMAT_TEXT;
    }
    else {
	return -1;
    }
    return 0;
}

int
opt_print_begin(opt_print_t *opt) {
    switch(opt->format) {
    case OPT_PRINT_FORMAT_TEXT:
	break;

    case OPT_PRINT_FORMAT_PERL:
	printf("{\n");
	opt->print_indent=1;
	break;

    case OPT_PRINT_FORMAT_XML:
	printf("<root>\n");
	opt->print_indent=1;
	break;
    }
    return 0;
}

int
opt_print_end(opt_print_t *opt) {
    switch(opt->format) {
    case OPT_PRINT_FORMAT_TEXT:
	break;

    case OPT_PRINT_FORMAT_PERL:
	printf("}\n");
	break;

    case OPT_PRINT_FORMAT_XML:
	printf("</root>\n");
	break;
    }
    return 0;
}

int
opt_print_block_begin(opt_print_t *opt) {
    int i;

    for(i=0; i<opt->print_indent; i++) printf("  ");

    opt->print_indent++;

    switch(opt->format) {
    case OPT_PRINT_FORMAT_TEXT:
	printf("begin\n");
	break;

    case OPT_PRINT_FORMAT_PERL:
	printf("{\n");
	break;

    case OPT_PRINT_FORMAT_XML:
	printf("<block>\n");
	break;
    }
    return 0;
}

int
opt_print_block_end(opt_print_t *opt) {
    int i;

    opt->print_indent--;

    for(i=0; i<opt->print_indent; i++) printf("  ");

    switch(opt->format) {
    case OPT_PRINT_FORMAT_TEXT:
	printf("end\n");
	break;

    case OPT_PRINT_FORMAT_PERL:
	printf("},\n");
	break;

    case OPT_PRINT_FORMAT_XML:
	printf("</block>\n");
	break;
    }

    return 0;
}

int
opt_print_array_begin(opt_print_t *opt, char *label) {
    int i;

    for(i=0; i<opt->print_indent; i++) printf("  ");

    opt->print_indent++;

    switch(opt->format) {
    case OPT_PRINT_FORMAT_TEXT:
	printf("begin %s\n", label);
	break;

    case OPT_PRINT_FORMAT_PERL:
	printf("\"%s\", [\n", label);
	break;

    case OPT_PRINT_FORMAT_XML:
	printf("<%s>\n", label);
	break;
    }
    return 0;
}

int
opt_print_array_end(opt_print_t *opt, char *label) {
    int i;

    opt->print_indent--;

    for(i=0; i<opt->print_indent; i++) printf("  ");

    switch(opt->format) {
    case OPT_PRINT_FORMAT_TEXT:
	printf("end %s\n", label);
	break;

    case OPT_PRINT_FORMAT_PERL:
	printf("],\n");
	break;

    case OPT_PRINT_FORMAT_XML:
	printf("</%s>\n", label);
	break;
    }
    return 0;
}

int
opt_print_kv(opt_print_t *opt, char *k, char *v) {
    char kbuf[4096], vbuf[4096];
    int i;

    for(i=0; i<opt->print_indent; i++) {
	printf("  ");
    }

    switch(opt->format) {
    case OPT_PRINT_FORMAT_TEXT:
	printf("%s: %s\n", k, v);
	break;

    case OPT_PRINT_FORMAT_PERL:
	opt_quoted_enc_style(k, -1, kbuf, sizeof(kbuf), OPT_QUOTED_ENC_PERL);
	opt_quoted_enc_style(v, -1, vbuf, sizeof(vbuf), OPT_QUOTED_ENC_PERL);
	printf("%s,  %s,\n", kbuf, vbuf);
	break;

    case OPT_PRINT_FORMAT_XML:
	printf("<%s>%s</%s>\n", k, v, k);
	break;
    }

    return 0;
}

