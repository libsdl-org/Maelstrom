
#define MATCH(A, B)	(A == B)

#include "colortable.h"

#include "newcolors.h"

main()
{
	int i, c;

	for ( i=0; i<256; ++i ) {
		for ( c=0; c<256; ++c ) {
			if ( MATCH(new_colors[i].red, (full_colors[c].red&0xFF)) &&
			   MATCH(new_colors[i].green, (full_colors[c].green&0xFF)) &&
			   MATCH(new_colors[i].blue, (full_colors[c].blue&0xFF)) )
				break;
		}
		if ( c == 256 ) {
			printf("Unmatched color: { 0x%x, 0x%x, 0x%x },\n",
			new_colors[i].red, new_colors[i].green, new_colors[i].blue);
		}
	}
}
