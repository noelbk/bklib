#ifndef DEFUTIL_H_INCLUDED
#define DEFUTIL_H_INCLUDED

#define FREE_ZERO(ptr) if( ptr ) { free(ptr); ptr = 0; }

#ifndef ABS
#define ABS(a) (((a)<0) ? -(a) : (a))
#endif // ABS

#ifndef SIGN
#define SIGN(a) (((a)<0) ? -1 : 1)
#endif // SIGN

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif // MIN

#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))
#endif // MAX

#ifndef OFFSETOF
#define OFFSETOF(struct, memb) ((int)(&(((struct*)0)->memb)))
#endif // OFFSETOF

#define NELTS(x) (sizeof(x)/sizeof(*(x)))

#define SWAP(a, b, tmp) ((tmp)=(a), (a)=(b), (b)=(tmp))

#define BUF_ADD(buf, len, off) \
    assertb((off)>=0);	       \
    (buf) += (off);	       \
    (len) -= (off);

#define CMP(a, b) (((a) > (b)) ? 1 : ((a) < (b)) ? -1 : 0)

// return ceil(n/d)
#define DIV_ROUND_UP(n, d) ((int)((n + (d-1))/d))

#define FLAG_FMT(buf, len, i, flags, FLAG) \
if( (flags) & (FLAG) ) { \
    (flags) &= ~(FLAG); \
    (i) = snprintf((buf), (len), "%s%s", #FLAG, ((flags) ? " | " : "")); \
    BUF_ADD((buf), (len), (i)); \
}

#define FLAG_FMT_END(buf, len, i, flags) \
if( (flags) ) { \
    (i) = snprintf((buf), (len), " unknown_flags=%d", (flags)); \
    BUF_ADD((buf), (len), (i)); \
}

#endif // DEFUTIL_H_INCLUDED
