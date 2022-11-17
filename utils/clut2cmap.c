
/* Converts a macintosh "clut" resource into a C header colormap */

#include <stdio.h>

#include "../bytesex.h"

struct clut_header {
	unsigned long  id;
	unsigned short flags;
	unsigned short size;
	};

struct clut_color {
	unsigned short pixel;
	unsigned short red;
	unsigned short green;
	unsigned short blue;
	};

main()
{
	struct clut_header clut;
	struct clut_color color;
	int i;

	/* Read in the colortable header */
	fread(&clut, sizeof(clut), 1, stdin);
	bytesexl(clut.id);
	bytesexs(clut.flags);
	bytesexs(clut.size);

	/* Print out the corresponding C header */
	printf("\n\
struct {\n\
	unsigned short red;\n\
	unsigned short green;\n\
	unsigned short blue;\n\
	} color_map[%d] = {\n\
", clut.size+1);

	/* Spew the contents */
	for ( i=0; i<(clut.size+1); ++i ) {
		if ( ! fread(&color, sizeof(color), 1, stdin) ) {
			fprintf(stderr, "Short colortable!\n");
			break;
		}
		bytesexs(color.pixel);
		bytesexs(color.red);
		bytesexs(color.green);
		bytesexs(color.blue);

		printf("\t\t{ 0x%.4x, 0x%.4x, 0x%.4x },  /* Pixel %u */\n",
			color.red, color.green, color.blue, color.pixel);
	}
	printf("};\n");
}
