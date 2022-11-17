
#include <stdlib.h>
#include "Mac_Resource.h"

main(int argc, char *argv[])
{
	if ( ! argv[1] ) {
		error("Usage: %s <Mac Resource File>\n", argv[0]);
		exit(1);
	}
	Mac_Resource   *res      = new Mac_Resource(argv[1]);
	int             numtypes = res->get_num_resource_types();
	char          **types = new char* [numtypes];
	int             i, j, num_resources;
	short *id_array;
	
	for ( i=0; i<numtypes; ++i )
		types[i] = new char[5];
	res->get_resource_types(types);

	for ( i=0; i<numtypes; ++i ) {
		num_resources = res->get_num_resources(types[i]);
		id_array = new short[num_resources];
		res->get_resource_ids(types[i], id_array);
		printf("Resource set: type = '%s', contains %hu resources.\n",
						types[i], num_resources);
		for ( j=0; j<num_resources; ++j ) {
			printf("\tResource %hu (ID = %d): \"%s\"\n", j+1,
				id_array[j],
				res->get_resource_name(types[i], id_array[j]));
			if ( argv[2] ) {
				char path[23];
				sprintf(path,"%s/%s:%hu", argv[2],
							types[i], id_array[j]);
				FILE *output;
				struct Mac_ResData D;
            if ( (output=fopen(path, "w")) != NULL ) {
					res->get_resource(types[i], id_array[j], &D);
					fwrite(D.data, D.length, 1,  output);
					fclose(output);               
					delete[] D.data;
            }
#ifdef DEBUG
				else {
            	error("Warning: Couldn't write to %s: ", path);
               perror("");
            }
#endif
			}
		}
		delete[]  id_array;
	}
	exit(0);
}
