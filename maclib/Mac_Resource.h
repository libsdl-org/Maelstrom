/*
  maclib:  A companion library to SDL for working with Macintosh (tm) data
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#ifndef _Mac_Resource_h
#define _Mac_Resource_h

/* These are routines to parse a Macintosh Resource Fork file

   Note: Most of the info in this file came from "Inside Macintosh"
*/

#include <stdio.h>
#include <stdarg.h>

/* The actual resources in the resource fork */
typedef struct {
	Uint32 length;
	Uint8   *data;
} Mac_ResData;


class Mac_Resource {

public:
	Mac_Resource(const char *filename);
	~Mac_Resource();

	/* Create a NULL terminated list of resource types in this file */
	char  **Types(void);

	/* Return the number of resources of the given type */
	Uint16  NumResources(const char *res_type);

	/* Create a 0xFFFF terminated list of resource ids for a type */
	Uint16 *ResourceIDs(const char *res_type);

	/* Return a resource of a certain type and id.  These resources
	   are deleted automatically when Mac_Resource object is deleted.
	 */
	char   *ResourceName(const char *res_type, Uint16 id);
	Mac_ResData *Resource(const char *res_type, Uint16 id);
	Mac_ResData *Resource(const char *res_type, const char *name);

	/* This returns a more detailed error message, or NULL */
	char *Error(void) {
		return(errstr);
	}

protected:
	friend int Res_cmp(const void *A, const void *B);

	/* Offset of Resource data in resource fork */
	Uint32 res_offset;
	Uint16 num_types;		/* Number of types of resources */

	struct resource {
		char  *name;
		Uint16 id;
		Uint32 offset;
		Mac_ResData *data;
	};

	struct resource_list {
		char   type[5];			/* Four character type + nul */
		Uint16 count;
		struct resource *list;
	} *Resources;

	FILE   *filep;				/* The Resource Fork File */
	Uint32  base;				/* The offset of the rsrc */

	/* Useful for getting error feedback */
	void error(const char *fmt, ...) {
		va_list ap;

		va_start(ap, fmt);
		SDL_vsnprintf(errbuf, sizeof(errbuf), fmt, ap);
		va_end(ap);
		errstr = errbuf;
	}
	char *errstr;
	char  errbuf[BUFSIZ];
};

#endif /* _Mac_Resource_h */
