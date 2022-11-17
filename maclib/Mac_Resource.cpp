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

/* These are routines to parse a Macintosh Resource Fork file

	-Sam Lantinga			(slouken@devolution.com)

Note: Most of the info in this file came from "Inside Macintosh"
*/

#include <stdlib.h>
#include <string.h>

#include "SDL_types.h"
#include "bitesex.h"
#include "Mac_Resource.h"

#ifndef SEEK_SET
#define SEEK_SET	0
#endif

/* The format for AppleDouble files -- in a header file */
#define APPLEDOUBLE_MAGIC	0x00051607
#include "applefile.h"

/* The format for MacBinary files -- in a header file */
#define MACBINARY_MASK		0xFCFF
#define MACBINARY_MAGIC		0x8081
#include "macbinary.h"

/* These are the data structures that make up the Macintosh Resource Fork */
struct Resource_Header {
	Uint32	res_offset;	/* Offset of resources in file */
	Uint32	map_offset;	/* Offset of resource map in file */
	Uint32	res_length;	/* Length of the resource data */
	Uint32	map_length;	/* Length of the resource map */
};

struct Resource_Data {
	Uint32	Data_length;	/* Length of the resources data */
#ifdef SHOW_VARLENGTH_FIELDS
	Uint8	Data[0];	/* The Resource Data */
#endif
};

struct Type_entry {
	char	Res_type[4];	/* Resource type */
	Uint16	Num_rez;	/* # this type resources in map - 1 */
	Uint16	Ref_offset;	/* Offset from type list, of reference
				   list for this type */
};

struct Ref_entry {
	Uint16	Res_id;		/* The ID for this resource */
	Uint16	Name_offset;	/* Offset in name list of resource
				   name, or -1 if no name */
	Uint8	Res_attrs;	/* Resource attributes */
	Uint8	Res_offset[3];	/* 3-byte offset from Resource data */
	Uint32	Reserved;	/* Reserved for use in-core */
};

struct Name_entry {
	Uint8	Name_len;	/* Length of the following name */
#ifdef SHOW_VARLENGTH_FIELDS
	Uint8	name[0];	/* Variable length resource name */
#endif
	};

struct Resource_Map {
	Uint8	Reserved[22];	/* Reserved for use in-core */
	Uint16	Map_attrs;	/* Map attributes */
	Uint16	types_offset;	/* Offset of resource type list */
	Uint16  names_offset;	/* Offset of resource name list */
	Uint16	num_types;	/* # of types in map - 1 */
#ifdef SHOW_VARLENGTH_FIELDS
	struct Type_entry  types[0];	 /* Variable length types list */
	struct Ref_entry   refs[0];	 /* Variable length reference list */
	struct Name_entry  names[0];	 /* Variable length name list */
#endif
	};

int Res_cmp(const void *A, const void *B)
{
	struct Mac_Resource::resource *a, *b;

	a=(struct Mac_Resource::resource *)A;
	b=(struct Mac_Resource::resource *)B;

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
static void CheckAppleFile(FILE *resfile, Uint32 *resbase)
{
	ASHeader header;
	if (fread(&header.magicNum,sizeof(header.magicNum),1,resfile)&&
		(bytesex32(header.magicNum) == APPLEDOUBLE_MAGIC) ) {
		fread(&header.versionNum,
				sizeof(header.versionNum), 1, resfile);
		bytesex32(header.versionNum);
		fread(&header.filler,
				sizeof(header.filler), 1, resfile);
		fread(&header.numEntries,
				sizeof(header.numEntries), 1, resfile);
		bytesex16(header.numEntries);
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
			bytesex32(entry.entryID);
			bytesex32(entry.entryOffset);
			bytesex32(entry.entryLength);
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
	fseek(resfile, 0, SEEK_SET);
}
static void CheckMacBinary(FILE *resfile, Uint32 *resbase)
{
	MBHeader header;
	if (fread(header.data,sizeof(header.data),1,resfile)&&
		((header.Version()&MACBINARY_MASK) == MACBINARY_MAGIC) ) {
		*resbase = sizeof(header.data) + header.DataLength();
	}
	fseek(resfile, 0, SEEK_SET);
}
static FILE *Open_MacRes(char **original, Uint32 *resbase)
{
	const char *dirname;
	char *filename, *basename, *ptr, *newname;
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
	size_t len = strlen(*original)+1;
	filename = new char[len];
	SDL_strlcpy(filename, *original, len);
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
		size_t len = strlen(dirname)+2+1+strlen(basename)+1;
		newname = new char[len];
		SDL_snprintf(newname, len, "%s%s%%%s", dirname, (*dirname ? "/" : ""),
								basename);
		if ( (resfile=fopen(newname, "rb")) != NULL ) {
			break;
		}
		delete[] newname;

		/* Look for MacBinary files */
		len = strlen(dirname)+2+strlen(basename)+4+1;
		newname = new char[len];
		SDL_snprintf(newname, len, "%s%s%s.bin", dirname, (*dirname ? "/" : ""),
								basename);
		if ( (resfile=fopen(newname, "rb")) != NULL ) {
			break;
		}
		delete[] newname;

		/* Look for raw resource fork.. */
		len = strlen(dirname)+2+strlen(basename)+1;
		newname = new char[len];
		SDL_snprintf(newname, len, "%s%s%s", dirname, (*dirname ? "/" : ""),
								basename);
		if ( (resfile=fopen(newname, "rb")) != NULL ) {
			break;
		}

#ifdef __MACOSX__
		newname = new char[strlen(dirname)+strlen("/../Resources/")+strlen(basename)+1];
		sprintf(newname, "%s/../Resources/%s", dirname, basename);
		if ( (resfile=fopen(newname, "rb")) != NULL ) {
			break;
		}
		delete[] newname;
#endif
	}
	/* Did we find anything? */
	if ( iterations != N_SNRS ) {
		*original = newname;
		*resbase = 0;

		/* Look for AppleDouble format header */
		CheckAppleFile(resfile, resbase);

		/* Look for MacBinary format header */
		CheckMacBinary(resfile, resbase);
	}
#ifdef APPLEDOUBLE_DEBUG
mesg("Resfile base = %d\n", *resbase);
#endif
	delete[] filename;
	return(resfile);
}

Mac_Resource:: Mac_Resource(const char *file)
{
	char *filename = (char *)file;
	struct Resource_Header Header;
	struct Resource_Map    Map;
	struct Type_entry      type_ent;
	struct Ref_entry       ref_ent;
	Uint16                *ref_offsets;
	Uint8                  name_len;
	unsigned long          cur_offset;
	int i, n;

	/* Clear out any variables */
	Resources = NULL;

	/* Try to open the Macintosh resource fork */
	errstr = NULL;
	if ( (filep=Open_MacRes(&filename, &base)) == NULL ) {
		error("Couldn't open resource file '%s'", filename);
		return;
	} else {
		/* Open_MacRes() passes back the real name of resource file */
		delete[] filename;
	}
	fseek(filep, base, SEEK_SET);

	if ( ! fread(&Header, sizeof(Header), 1, filep) ) {
		error("Couldn't read resource info from '%s'", filename);
		return;
	}
	bytesex32(Header.res_length);
	bytesex32(Header.res_offset);
	res_offset = Header.res_offset;
	bytesex32(Header.map_length);
	bytesex32(Header.map_offset);

	fseek(filep, base+Header.map_offset, SEEK_SET);
	if ( ! fread(&Map, sizeof(Map), 1, filep) ) {
		error("Couldn't read resource info from '%s'", filename);
		return;
	}
	bytesex16(Map.types_offset);
	bytesex16(Map.names_offset);
	bytesex16(Map.num_types);
	Map.num_types += 1;	/* Value in fork is one short */

	/* Fill in our class members */
	num_types = Map.num_types;
	Resources = new struct resource_list[num_types];
	for ( i=0; i<num_types; ++i )
		Resources[i].list = NULL;

	ref_offsets = new Uint16[num_types];
	fseek(filep, base+Header.map_offset+Map.types_offset+2, SEEK_SET);
	for ( i=0; i<num_types; ++i ) {
		if ( ! fread(&type_ent, sizeof(type_ent), 1, filep) ) {
			error("Couldn't read resource info from '%s'",
								filename);
			delete[] ref_offsets;
			return;
		}
		bytesex16(type_ent.Num_rez);
		bytesex16(type_ent.Ref_offset);
		type_ent.Num_rez += 1;	/* Value in fork is one short */

		SDL_memcpy(Resources[i].type, type_ent.Res_type, 4);
		Resources[i].type[4] = '\0';
		Resources[i].count   = type_ent.Num_rez;
		Resources[i].list = new struct resource[Resources[i].count];
		for ( n=0; n<Resources[i].count; ++n ) {
			Resources[i].list[n].name = NULL;
			Resources[i].list[n].data = NULL;
		}
		ref_offsets[i] = type_ent.Ref_offset;
	}

	for ( i=0; i<num_types; ++i ) {
		fseek(filep, 
		base+Header.map_offset+Map.types_offset+ref_offsets[i],
								SEEK_SET);
		for ( n=0; n<Resources[i].count; ++n ) {
			if ( ! fread(&ref_ent, sizeof(ref_ent), 1, filep) ) {
				error("Couldn't read resource info from '%s'",
								filename);
				delete[] ref_offsets;
				return;
			}
			bytesex16(ref_ent.Res_id);
			bytesex16(ref_ent.Name_offset);
			Resources[i].list[n].offset = 
					((ref_ent.Res_offset[0]<<16) |
                                         (ref_ent.Res_offset[1]<<8) |
                                         (ref_ent.Res_offset[2]));
			Resources[i].list[n].id = ref_ent.Res_id;

			/* Grab the name, while we're here... */
			if ( ref_ent.Name_offset == 0xFFFF ) {
				Resources[i].list[n].name = new char[1];
				Resources[i].list[n].name[0] = '\0';
			} else {
				cur_offset = ftell(filep);
				fseek(filep, 
		base+Header.map_offset+Map.names_offset+ref_ent.Name_offset,
								SEEK_SET);
				fread(&name_len, 1, 1, filep);
				Resources[i].list[n].name=new char[name_len+1];
				fread(Resources[i].list[n].name,
							1, name_len, filep);
				Resources[i].list[n].name[name_len] = '\0';
				fseek(filep, cur_offset, SEEK_SET);
			}
		}
#ifndef macintosh
		/* Sort the resources in ascending order. :) */
		qsort(Resources[i].list,Resources[i].count,
					sizeof(struct resource), Res_cmp);
#endif
	}
	delete[] ref_offsets;
}

Mac_Resource:: ~Mac_Resource()
{
	if ( filep )
		fclose(filep);

	if ( ! Resources )
		return;
	for ( int i=0; i<num_types; ++i ) {
		if ( ! Resources[i].list )
			continue;
		for ( int n=0; n<Resources[i].count; ++n ) {
			if ( Resources[i].list[n].name )
				delete[] Resources[i].list[n].name;
			if ( Resources[i].list[n].data ) {
				delete[] Resources[i].list[n].data->data;
				delete Resources[i].list[n].data;
			}
		}
		delete[] Resources[i].list;
	}
	delete[] Resources;
}

char **
Mac_Resource:: Types(void)
{
	int i;
	char **types;

	types = new char *[num_types+1];
	for ( i=0; i<num_types; ++i )
		types[i] = Resources[i].type;
	types[i] = NULL;
	return(types);
}

Uint16
Mac_Resource:: NumResources(const char *res_type)
{
	int i;

	for ( i=0; i<num_types; ++i ) {
		if ( strncmp(res_type, Resources[i].type, 4) == 0 )
			return(Resources[i].count);
	}
	return(0);
}

Uint16 *
Mac_Resource:: ResourceIDs(const char *res_type)
{
	int i, n;
	Uint16 *ids;

	for ( i=0; i<num_types; ++i ) {
		if ( strncmp(res_type, Resources[i].type, 4) == 0 ) {
			ids = new Uint16[Resources[i].count+1];
			for ( n=0; n<Resources[i].count; ++n )
				ids[n] = Resources[i].list[n].id;
			ids[n] = 0xFFFF;
			return(ids);
		}
	}
	error("Couldn't find resources of type '%s'", res_type);
	return(NULL);
}

char *
Mac_Resource:: ResourceName(const char *res_type, Uint16 id)
{
	int i, n;

	for ( i=0; i<num_types; ++i ) {
		if ( strncmp(res_type, Resources[i].type, 4) == 0 ) {
			for ( n=0; n<Resources[i].count; ++n ) {
				if ( id == Resources[i].list[n].id )
					return(Resources[i].list[n].name);
			}
		}
	}
	error("Couldn't find resource of type '%s', id %hu", res_type, id);
	return(NULL);
}

Mac_ResData *
Mac_Resource:: Resource(const char *res_type, Uint16 id)
{
	int i, n;
	Mac_ResData *d;

	for ( i=0; i<num_types; ++i ) {
		if ( strncmp(res_type, Resources[i].type, 4) == 0 ) {
			for ( n=0; n<Resources[i].count; ++n ) {
				if ( id == Resources[i].list[n].id ) {
					/* Is it already loaded? */
					d = Resources[i].list[n].data;
					if ( d )
						return(d);

					/* Load it */
					d = new Mac_ResData;
					fseek(filep, base+res_offset+Resources[i].list[n].offset, SEEK_SET);
					fread(&d->length, 4, 1, filep);
					bytesex32(d->length);
					d->data = new Uint8[d->length];
					if (!fread(d->data,d->length,1,filep)) {
						delete[] d->data;
						error("Couldn't read %d bytes", d->length);
						delete d;
						return(NULL);
					}
					Resources[i].list[n].data = d;
					return(d);
				}
			}
		}
	}
	error("Couldn't find resource of type '%s', id %hu", res_type, id);
	return(NULL);
}

Mac_ResData *
Mac_Resource:: Resource(const char *res_type, const char *name)
{
	int i, n;
	Mac_ResData *d;

	for ( i=0; i<num_types; ++i ) {
		if ( strncmp(res_type, Resources[i].type, 4) == 0 ) {
			for ( n=0; n<Resources[i].count; ++n ) {
				if (strcmp(name, Resources[i].list[n].name)==0){
					/* Is it already loaded? */
					d = Resources[i].list[n].data;
					if ( d )
						return(d);

					/* Load it */
					d = new Mac_ResData;
					fseek(filep, base+res_offset+Resources[i].list[n].offset, SEEK_SET);
					fread(&d->length, 4, 1, filep);
					bytesex32(d->length);
					d->data = new Uint8[d->length];
					if (!fread(d->data,d->length,1,filep)) {
						delete[] d->data;
						error("Couldn't read %d bytes", d->length);
						delete d;
						return(NULL);
					}
					Resources[i].list[n].data = d;
					return(d);
				}
			}
		}
	}
	error("Couldn't find resource of type '%s', name %s", res_type, name);
	return(NULL);
}

