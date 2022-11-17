
/* Generic glue code for debugging among all source files */

#ifndef _mydebug_h
#define _mydebug_h

#include "myerror.h"

#ifndef DEBUG
#define NDEBUG				/* Turns off assert() calls */
#endif
#include <assert.h>

#if defined(MEM_DEBUG) && defined(HEAPAGNT)
#include <../heapagnt/include/heapagnt.h>
#endif

#endif /* _mydebug_h */
