
#ifndef _newmem_h
#define _newmem_h

/* This header contains custom new and delete operators for memory debugging */

#ifdef __WIN95__
#include <new.h>

#define malloc(n)	LocalAlloc(LMEM_FIXED, n)
#define free(p)		LocalFree(p)
#endif


void *operator new(size_t size)
{
	void *ptr;

	if ( (ptr=malloc(size)) == NULL ) {
		error("Out of Memory!\n");
		return(NULL);
	}
	return(ptr);
}

void *operator new[] (size_t size)
{
	void *ptr;

	if ( (ptr=malloc(size)) == NULL ) {
		error("Out of Memory!\n");
		return(NULL);
	}
	return(ptr);
}

void operator delete(void *ptr)
{
	free(ptr);
}

void operator delete[] (void *ptr)
{
	free(ptr);
}

#endif /* _newmem_h */
