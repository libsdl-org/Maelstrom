#ifndef _bitesex_h
#define _bitesex_h

#include "SDL_endian.h"

#define bytesex32(x)	(x = SDL_SwapBE32(x))
#define bytesex16(x)	(x = SDL_SwapBE16(x))

/* Swap bytes from big-endian to this machine's type.
   The input data is assumed to be always in big-endian format.
*/
static inline void
byteswap(Uint16 *array, int nshorts)
{
	for (; nshorts-- > 0; array++)
		bytesex16(*array);
}

#endif /* _bitesex_h */
