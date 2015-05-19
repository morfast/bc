#ifndef __INTF_SYS_H__
#define __INTF_SYS_H__

#include <sys/types.h>

typedef unsigned char uchar;
/*
 * 大端字节序
 */
static  inline ushort getshort_inline(register void const *ptr)
{
    uchar   datah, datal;

    datah = *(uchar *) ptr;
    datal = *((uchar *) ptr + 1);
    return (ushort) ((datah << 8) | datal);
}

static  inline void putshort_inline(register void *ptr, register ushort value)
{
    *(uchar *) ptr = (uchar) (value >> 8);
    *((uchar *) ptr + 1) = (uchar) value;
}

static  inline u_int32_t getlong_inline(register void const *ptr)
{
    uchar   value0, value1, value2, value3;

    value0 = *(uchar *) ptr;
    value1 = *((uchar *) ptr + 1);
    value2 = *((uchar *) ptr + 2);
    value3 = *((uchar *) ptr + 3);
    return (u_int32_t) ((value0 << 24) | (value1 << 16) | (value2 << 8) | value3);
}

static  inline void putlong_inline(register void *ptr, register u_int32_t value)
{
    *(uchar *) ptr = (uchar) (value >> 24);
    *((uchar *) ptr + 1) = (uchar) (value >> 16);
    *((uchar *) ptr + 2) = (uchar) (value >> 8);
    *((uchar *) ptr + 3) = (uchar) value;
}

#define GETSHORT(ptr)           getshort_inline (ptr)
#define PUTSHORT(ptr, val)      putshort_inline (ptr, val)
#define GETLONG(ptr)            getlong_inline(ptr)
#define PUTLONG(ptr, val)       putlong_inline(ptr, val)

#define GET_INT_N_LE(ptr,n,out) do {\
    (out) = 0;\
    int i; \
    for (i = 0; i < n; i++) { \
        (out) += (*((ptr) + i) << (8 * i)); \
    }\
} while(0)\

#define GET_INT_N_BE(ptr,n,out) do {\
    (out) = 0;\
    int i; \
    for (i = 0; i < n; i++) { \
        (out) += (*((ptr) + i) << (8 * (n - 1 - i))); \
    }\
} while(0)\



#endif

