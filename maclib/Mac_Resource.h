/*
    MACLIB:  A companion library to SDL for working with Macintosh (tm) data
    Copyright (C) 1997-2021 Sam Lantinga <slouken@libsdl.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _Mac_Resource_h
#define _Mac_Resource_h

/* These are routines to parse a Macintosh Resource Fork file

	-Sam Lantinga			(slouken@devolution.com)

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
