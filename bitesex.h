
#ifndef _bitesex_h
#define _bitesex_h

/* Grab the appropriate BYTE_ORDER definition */
#ifdef linux
#include <endian.h>
#else
#ifdef sparc
#ifdef __SVR4	/* Solaris */
#include <sys/byteorder.h>
#else		/* SunOS */
#define BYTE_ORDER	BIG_ENDIAN
#endif /* __SVR4 */
#else
#ifdef _SGI_SOURCE
#include <sys/endian.h>
#else
#ifdef _INCLUDE_HPUX_SOURCE
#include <machine/param.h>
#else
#ifdef __mips
#include <sex.h>
#else
#ifdef _AIX
#include <sys/machine.h>
#else
#ifdef __FreeBSD__
#include <machine/endian.h>
#else
#if defined(__svr4__)
#include <sys/byteorder.h>
#else
#ifdef __WIN95__
#define LITTLEENDIAN
#else
#error	Compiling for an unknown architecture!
#endif /* Win95 */
#endif /* SVR4 */
#endif /* FreeBSD */
#endif /* AIX */
#endif /* DEC */
#endif /* HPUX */
#endif /* SGI */
#endif /* sparc */
#endif /* linux */

/* Okay, try double underscore */
#ifdef __BYTE_ORDER__
#define BYTE_ORDER	__BYTE_ORDER__
#define LITTLE_ENDIAN	__LITTLE_ENDIAN__
#define BIG_ENDIAN	__BIG_ENDIAN__
#else
/* Um, try double leading underscore */
#ifdef __BYTE_ORDER
#define BYTE_ORDER	__BYTE_ORDER
#define LITTLE_ENDIAN	__LITTLE_ENDIAN
#define BIG_ENDIAN	__BIG_ENDIAN
#endif /* __BYTE_ORDER */
#endif /* __BYTE_ORDER__ */

/* If we haven't gotten BYTE_ORDER by now, try to kludge it out */
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN	1234
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN	4321
#endif

#ifndef BYTE_ORDER
/* I don't remember which system this covers */
#ifdef _BIG_ENDIAN
#define BYTE_ORDER	BIG_ENDIAN
#endif
/* DEC -- Little Endian */
#ifdef LITTLEENDIAN
#define BYTE_ORDER LITTLE_ENDIAN
#endif
#endif /* No BYTE_ORDER yet */

/* Macros to access the high and low byte of a short int */
#define HIBYTE(word)	(((word)>>8)&0xff)
#define LOBYTE(word)	((word)&0xff)

#if BYTE_ORDER == LITTLE_ENDIAN

/* Swap bytes from big-endian to this machine's type.
   The input data is assumed to be always in big-endian format.
*/
static inline void
byteswap(unsigned short *array, int nshorts)
{
	for (; nshorts-- > 0; array++)
		*array= HIBYTE(*array) | (LOBYTE(*array) << 8);
}

/* Macros to change the byte order of input values */
#define bytesexl(x)	(x = \
        ((x << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00) | (x >> 24)))
#define bytesexs(x)	(x = (x << 8) | (x >> 8))

#else
static inline void
byteswap(unsigned short *, int)
{ return; }

#define bytesexl(x)	(x)
#define bytesexs(x)	(x)

#endif /* Endianness */

#ifndef BYTE_ORDER
#error I dont have any byte sex!
#endif
#endif /* _bitesex_H */
