
#ifndef _Mac_Resource_h
#define _Mac_Resource_h

/* These are routines to parse a Macintosh Resource Fork file

	-Sam Lantinga			(slouken@devolution.com)

Note: Most of the info in this file came from "Inside Macintosh"
*/

#include <stdio.h>
#include "mydebug.h"

/* A Resource class.  It performs all the resource retrieval for a single
   binary resource file
*/

/* The actual resources in the resource fork */
struct Mac_ResData {
	unsigned long length;
	unsigned char *data;
	};

class Mac_Resource {
public:
	Mac_Resource(char *filename);
	~Mac_Resource();

	int   get_num_resource_types(void);
	void  get_resource_types(char *type_array[]);
	int   get_num_resources(char *res_type);
	int   get_resource_ids(char *res_type, short id_array[]);
	char *get_resource_name(char *res_type, short id);
	int   get_resource(char *res_type, short id, struct Mac_ResData *cup);

private:
   friend int Res_cmp(const void *A, const void *B);
   
	/* Number of the types of resources */
	int num_resource_types;

	/* Offset of the Resource data in the resource fork */
	unsigned long Resource_offset;

	struct Resource {
		char          *name;
		unsigned short id;
		unsigned long  offset;
	};
	struct Resource_List {
		char type[5];			/* Four character type + nil */
		unsigned short num_resources;
		struct Mac_Resource::Resource *res_list;
	} *Resources;

	FILE *filep;				/* The Resource Fork File */
	int   base;				/* The offset of the rsrc */
};
#endif /* _Mac_Resource_h */
