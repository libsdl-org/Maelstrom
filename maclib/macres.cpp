/*
    MACRES:  A program to extract data from Macintosh (tm) resource forks
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

/* Test program to list and extract Macintosh resources from a resource fork */

#include <stdlib.h>

#include "SDL.h"
#include "Mac_Resource.h"

int main(int argc, char *argv[])
{
	Mac_Resource *res;
	char      **types;
	Uint16     *ids;
	int         i, j;

	if ( ! argv[1] ) {
		fprintf(stderr, "Usage: %s <Mac Resource File>\n", argv[0]);
		exit(1);
	}

	res = new Mac_Resource(argv[1]);
	if ( res->Error() ) {
		fprintf(stderr, "Mac_Resource: %s\n", res->Error());
		delete res;
		exit(2);
	}
	
	types = res->Types();
	for ( i=0; types[i]; ++i ) {
		ids = res->ResourceIDs(types[i]);
		printf("Resource set: type = '%s', contains %hd resources\n",
					types[i], res->NumResources(types[i]));
		for ( j=0; ids[j] < 0xFFFF; ++j ) {
			printf("\tResource %d (ID = %d): \"%s\"\n", j+1,
				ids[j], res->ResourceName(types[i], ids[j]));
			if ( argv[2] ) {
				char path[23];
				sprintf(path,"%s/%s:%hu", argv[2],
							types[i], ids[j]);
				FILE *output;
				Mac_ResData *D;
            			if ( (output=fopen(path, "w")) != NULL ) {
					D = res->Resource(types[i], ids[j]);
					fwrite(D->data, D->length, 1,  output);
					fclose(output);               
            			}
			}
		}
		delete[]  ids;
	}
	delete[] types;
	delete res;
	exit(0);
}
