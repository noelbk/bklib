#ifndef BITOPS_H_INCLUDED
#define BITOPS_H_INCLUDED

#define bit_get(x, len, off) \
(((x)>>(off)) & ((1<<(len))-1))

#define bit_set(x, len, off, bits) \
(((x) & ~(((1<<(len))-1)<<(off))) | ((bits)<<(off)))

#endif // BITOPS_H_INCLUDED
