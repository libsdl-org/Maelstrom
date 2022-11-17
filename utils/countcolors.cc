
#include "../Mac_Resource.h"
#include "../bitesex.h"
#include "colortable.h"

unsigned short refcount[256];

main(int argc, char *argv[])
{
	Mac_Resource   *res      = new Mac_Resource("Maelstrom Sprites");
	int             numtypes = res->get_num_resource_types();
	char          **types = new char* [numtypes]; 
	int             i, j, k, num_resources;
	short *id_array;
	struct Mac_ResData D;
	
	for ( i=0; i<numtypes; ++i )
		types[i] = new char[5];
	res->get_resource_types(types);

	for ( i=0; i<numtypes; ++i ) {
		num_resources = res->get_num_resources(types[i]);
		id_array = new short[num_resources];
		res->get_resource_ids(types[i], id_array);
		/* Only extract the icon resources */
		if ( strcmp(types[i], "ics8") && strcmp(types[i], "icl8") )
			continue;
		fprintf(stderr, "Opening icon resource: %s\n", types[i]);
		for ( j=0; j<num_resources; ++j ) {
			struct Mac_ResData D;
			res->get_resource(types[i], id_array[j], &D);
			for ( int l=0; l<D.length; ++l )
				++refcount[D.data[l]];
			delete[] D.data;
		}
		delete[]  id_array;
	}
	delete   res;
	delete[] types;

	/* Load the icon info */
	for ( int a=1; argv[a]; ++a ) {
		FILE *f = fopen(argv[a], "r");
		unsigned short width, height;
		fread(&width, 2, 1, f);
		bytesexs(width);
		fread(&height, 2, 1, f);
		bytesexs(height);
		int len = width*height;
		unsigned char *buffer = new unsigned char[len];
		int newlen = fread(buffer, 1, len, f);
		if ( len != newlen ) {
			fprintf(stderr, "Corrupt icon file! (%s)\n", argv[a]);
			continue;
		}
		fprintf(stderr, "Opening icon file: %s\n", argv[a]);
		for ( k=0; k<len; ++k )
			++refcount[buffer[k]];
		fclose(f);
	}

	/* Print out the information */
	int numcolors=0;
	for ( k=0; k<256; ++k ) {
		if ( ! refcount[k] ) {
			printf("Pixel %d unused\n", k);
		} else
			++numcolors;
	} 
	printf("%d total colors used.\n", numcolors);
}
