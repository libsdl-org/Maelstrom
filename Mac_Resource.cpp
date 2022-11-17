
/* These are routines to parse a Macintosh Resource Fork file

	-Sam Lantinga			(slouken@devolution.com)

Note: Most of the info in this file came from "Inside Macintosh"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Mac_Resource.h"
#include "bitesex.h"

#ifndef SEEK_SET
#define SEEK_SET	0
#endif

/* The format for AppleDouble files -- in a header file */
#define APPLEDOUBLE_MAGIC	0x00051607
#include "applefile.h"

/* These are the data structures that make up the Macintosh Resource Fork */
struct Resource_Header {
	unsigned long	res_offset;	/* Offset of resources in file */
	unsigned long	map_offset;	/* Offset of resource map in file */
	unsigned long	res_length;	/* Length of the resource data */
	unsigned long	map_length;	/* Length of the resource map */
};

struct Resource_Data {
	unsigned long	Data_length;	/* Length of the resources data */
#ifdef SHOW_VARLENGTH_FIELDS
	unsigned char	Data[0];	/* The Resource Data */
#endif
	};

struct Type_entry {
	char		Res_type[4];	/* Resource type */
	unsigned short	Num_rez;	/* # this type resources in map - 1 */
	unsigned short	Ref_offset;	/* Offset from type list, of reference
					   list for this type */
	};

struct Ref_entry {
	unsigned short	Res_id;		/* The ID for this resource */
	unsigned short	Name_offset;	/* Offset in name list of resource
					   name, or -1 if no name */
	unsigned char	Res_attrs;	/* Resource attributes */
	unsigned char	Res_offset[3];	/* 3-byte offset from Resource data */
	unsigned long	Reserved;	/* Reserved for use in-core */
	};

struct Name_entry {
	unsigned char	Name_len;	/* Length of the following name */
#ifdef SHOW_VARLENGTH_FIELDS
	unsigned char	name[0];	/* Variable length resource name */
#endif
	};

struct Resource_Map {
	unsigned char	Reserved[22];	/* Reserved for use in-core */
	unsigned short	Map_attrs;	/* Map attributes */
	unsigned short	types_offset;	/* Offset of resource type list */
	unsigned short  names_offset;	/* Offset of resource name list */
	unsigned short	num_types;	/* # of types in map - 1 */
#ifdef SHOW_VARLENGTH_FIELDS
	struct Type_entry  types[0];	 /* Variable length types list */
	struct Ref_entry   refs[0];	 /* Variable length reference list */
	struct Name_entry  names[0];	 /* Variable length name list */
#endif
	};

int Res_cmp(const void *A, const void *B)
{
	struct Mac_Resource::Resource *a, *b;

	a=(struct Mac_Resource::Resource *)A;
	b=(struct Mac_Resource::Resource *)B;

	if ( a->id > b->id )
		return(1);
	else if ( a->id < b->id )
		return(-1);
	else /* They are equal?? */
		return(0);
}

/* Here's an iterator to find heuristically (I've always wanted to use that
   word :-) a macintosh resource fork from a general mac name.

   This function may be overkill, but I want to be able to find any Macintosh
   resource fork, darn it! :)
*/
static FILE *Open_MacRes(char **original, int *resbase)
{
	char *filename, *dirname, *basename, *ptr, *newname;
	FILE *resfile=NULL;

	/* Search and replace characters */
	const int N_SNRS = 2;
	struct searchnreplace {
		char search;
		char replace;
	} snr[N_SNRS] = {
		{ '\0',	'\0' },
		{ ' ',	'_' },
	};
	int iterations=0;

	/* Separate the Mac name from a UNIX path */
	filename = new char[strlen(*original)+1];
	strcpy(filename, *original);
	if ( (basename=strrchr(filename, '/')) != NULL ) {
		dirname = filename;
		*(basename++) = '\0';
	} else {
		dirname = "";
		basename = filename;
	}

	for ( iterations=0; iterations < N_SNRS; ++iterations ) {
		/* Translate ' ' into '_', etc */
		/* Note that this translation is irreversible */
		for ( ptr = basename; *ptr; ++ptr ) {
			if ( *ptr == snr[iterations].search )
				*ptr = snr[iterations].replace;
		}

		/* First look for Executor (tm) resource forks */
		newname = new char[strlen(dirname)+2+1+strlen(basename)+1];
		sprintf(newname, "%s%s%%%s", dirname, (*dirname ? "/" : ""),
								basename);
		if ( (resfile=fopen(newname, "rb")) != NULL ) {
			break;
		}
		delete[] newname;

		/* Look for raw resource fork.. */
		newname = new char[strlen(dirname)+2+strlen(basename)+1];
		sprintf(newname, "%s%s%s", dirname, (*dirname ? "/" : ""),
								basename);
		if ( (resfile=fopen(newname, "rb")) != NULL ) {
			break;
		}
	}
	/* Did we find anything? */
	if ( iterations != N_SNRS ) {
		*original = newname;

		/* Look for AppleDouble format header */
		*resbase = 0;
		ASHeader header;
		if (fread(&header.magicNum,sizeof(header.magicNum),1,resfile)&&
		 	(bytesexl(header.magicNum) == APPLEDOUBLE_MAGIC) ) {
			fread(&header.versionNum,
					sizeof(header.versionNum), 1, resfile);
			bytesexl(header.versionNum);
			fread(&header.filler,
					sizeof(header.filler), 1, resfile);
			fread(&header.numEntries,
					sizeof(header.numEntries), 1, resfile);
			bytesexs(header.numEntries);
#ifdef APPLEDOUBLE_DEBUG
mesg("Header magic: 0x%.8x, version 0x%.8x\n",
				header.magicNum, header.versionNum);
#endif

			ASEntry entry;
#ifdef APPLEDOUBLE_DEBUG
mesg("Number of entries: %d, sizeof(entry) = %d\n",
				header.numEntries, sizeof(entry));
#endif
			for ( int i = 0; i<header.numEntries; ++ i ) {
				if (! fread(&entry, sizeof(entry), 1, resfile))
					break;
				bytesexl(entry.entryID);
				bytesexl(entry.entryOffset);
				bytesexl(entry.entryLength);
#ifdef APPLEDOUBLE_DEBUG
mesg("Entry (%d): ID = 0x%.8x, Offset = %d, Length = %d\n",
		i+1, entry.entryID, entry.entryOffset, entry.entryLength);
#endif
				if ( entry.entryID == AS_RESOURCE ) {
					*resbase = entry.entryOffset;
					break;
				}
			}
		}
		(void) fseek(resfile, 0, SEEK_SET);
	}
#ifdef APPLEDOUBLE_DEBUG
mesg("Resfile base = %d\n", *resbase);
#endif
	delete[] filename;
	return(resfile);
}

Mac_Resource:: Mac_Resource(char *filename)
{
	struct Resource_Header Header;
	struct Resource_Map    Map;
	struct Type_entry      type_ent;
	struct Ref_entry       ref_ent;
	unsigned short        *ref_offsets;
	unsigned char          name_len;
	unsigned long          cur_offset;
	int i, n;

	/* Try to open the Macintosh resource fork */
	if ( (filep=Open_MacRes(&filename, &base)) == NULL ) {
		char errbuf[BUFSIZ];
		sprintf(errbuf, "Couldn't open resource fork '%s'", filename);
		perror(errbuf);
		exit(253);
	} else {
		/* Open_MacRes() passes back the real name of resource file */
		delete[] filename;
	}
	fseek(filep, base, SEEK_SET);

	if ( ! fread(&Header, sizeof(Header), 1, filep) ) {
		error("Mac_Resource: Couldn't read resource info!\n");
		fclose(filep);
		exit(253);
	}
	bytesexl(Header.res_length);
	bytesexl(Header.res_offset);
	Resource_offset = Header.res_offset;
	bytesexl(Header.map_length);
	bytesexl(Header.map_offset);

	fseek(filep, base+Header.map_offset, SEEK_SET);
	if ( ! fread(&Map, sizeof(Map), 1, filep) ) {
		error("Mac_Resource: Couldn't read resource info!\n");
		fclose(filep);
		exit(253);
	}
	bytesexs(Map.types_offset);
	bytesexs(Map.names_offset);
	bytesexs(Map.num_types);
	Map.num_types += 1;	/* Value in fork is one short */

	/* Fill in our class members */
	num_resource_types = Map.num_types;
	Resources = new struct Resource_List[num_resource_types];

	ref_offsets = new unsigned short[num_resource_types];
	fseek(filep, base+Header.map_offset+Map.types_offset+2, SEEK_SET);
	for ( i=0; i<num_resource_types; ++i ) {
		if ( ! fread(&type_ent, sizeof(type_ent), 1, filep) ) {
			error("Mac_Resource: Couldn't read resource info!\n");
			fclose(filep);
			exit(253);
		}
		bytesexs(type_ent.Num_rez);
		bytesexs(type_ent.Ref_offset);
		type_ent.Num_rez += 1;	/* Value in fork is one short */

		ref_offsets[i] = type_ent.Ref_offset;
		strncpy(Resources[i].type, type_ent.Res_type, 4);
		Resources[i].type[4] = '\0';
		Resources[i].num_resources = type_ent.Num_rez;
		Resources[i].res_list = 
			new struct Resource[Resources[i].num_resources];
	}

	for ( i=0; i<num_resource_types; ++i ) {
		fseek(filep, 
		base+Header.map_offset+Map.types_offset+ref_offsets[i],
								SEEK_SET);
		for ( n=0; n<Resources[i].num_resources; ++n ) {
			if ( ! fread(&ref_ent, sizeof(ref_ent), 1, filep) ) {
				error(
				"Mac_Resource: Couldn't read resource info!\n");
				fclose(filep);
				exit(253);
			}
			bytesexs(ref_ent.Res_id);
			bytesexs(ref_ent.Name_offset);
			Resources[i].res_list[n].offset = 
					((ref_ent.Res_offset[0]<<16) |
                                         (ref_ent.Res_offset[1]<<8) |
                                         (ref_ent.Res_offset[2]));
			Resources[i].res_list[n].id = ref_ent.Res_id;

			/* Grab the name, while we're here... */
			if ( ref_ent.Name_offset == 0xFFFF ) {
				Resources[i].res_list[n].name = new char[1];
				Resources[i].res_list[n].name[0] = '\0';
			} else {
				cur_offset=ftell(filep);
				fseek(filep, 
		base+Header.map_offset+Map.names_offset+ref_ent.Name_offset,
								SEEK_SET);
				fread(&name_len, 1, 1, filep);
				Resources[i].res_list[n].name = 
							new char[name_len+1];
				fread(Resources[i].res_list[n].name,
							1, name_len, filep);
				Resources[i].res_list[n].name[name_len] = '\0';
				fseek(filep, cur_offset, SEEK_SET);
			}
		}
		/* Sort the resources in ascending order. :) */
		qsort(Resources[i].res_list,Resources[i].num_resources,
					sizeof(struct Resource), Res_cmp);
	}
	delete[] ref_offsets;
}

Mac_Resource:: ~Mac_Resource()
{
	for ( int i=0; i<num_resource_types; ++i ) {
		for ( int n=0; n<Resources[i].num_resources; ++n )
			delete[] Resources[i].res_list[n].name;
		delete[] Resources[i].res_list;
	}
	delete[] Resources;
	fclose(filep);
}

int
Mac_Resource:: get_num_resource_types(void)
{
	return(num_resource_types);
}

void
Mac_Resource:: get_resource_types(char *type_array[])
{
	int i;

	for ( i=0; i<num_resource_types; ++i )
		strcpy(type_array[i], Resources[i].type);
}

int
Mac_Resource:: get_num_resources(char *res_type)
{
	int i;

	for ( i=0; i<num_resource_types; ++i ) {
		if ( strncmp(res_type, Resources[i].type, 4) == 0 )
			return(Resources[i].num_resources);
	}
	return(-1);
}

int
Mac_Resource:: get_resource_ids(char *res_type, short id_array[])
{
	int i, n;

	for ( i=0; i<num_resource_types; ++i ) {
		if ( strncmp(res_type, Resources[i].type, 4) == 0 ) {
			for ( n=0; n<Resources[i].num_resources; ++n )
				id_array[n] = Resources[i].res_list[n].id;
			return(0);
		}
	}
	return(-1);
}

char *
Mac_Resource:: get_resource_name(char *res_type, short id)
{
	int i, n;

	for ( i=0; i<num_resource_types; ++i ) {
		if ( strncmp(res_type, Resources[i].type, 4) == 0 ) {
			for ( n=0; n<Resources[i].num_resources; ++n ) {
				if ( id == Resources[i].res_list[n].id )
					return(Resources[i].res_list[n].name);
			}
		}
	}
	return(NULL);
}

int
Mac_Resource:: get_resource(char *res_type, short id, struct Mac_ResData *cup)
{
	int i, n;

	for ( i=0; i<num_resource_types; ++i ) {
		if ( strncmp(res_type, Resources[i].type, 4) == 0 ) {
			for ( n=0; n<Resources[i].num_resources; ++n ) {
				if ( id == Resources[i].res_list[n].id ) {
					fseek(filep, base+Resource_offset+Resources[i].res_list[n].offset, SEEK_SET);
					fread(&cup->length, 4, 1, filep);
					bytesexl(cup->length);
					cup->data = new unsigned char[cup->length];
					if ( ! fread(cup->data, cup->length, 1, filep) )
						return(-1);
					return(0);
				}
			}
		}
	}
	return(-1);
}

