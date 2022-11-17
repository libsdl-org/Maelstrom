
#include <stdio.h>
#include <string.h>

#include "colortable.h"		/* The standard Maelstrom colormap */

#define MATCH(A, B)	(A == B)

main(int argc, char *argv[])
{
	char buffer[4096];
	char redb[3], greenb[3], blueb[3];
	int red, green, blue;
	int c2, lc, ld;
	char *ptr;

	while ( gets(buffer) ) {
		red = green = blue = 0;
		ptr=buffer/*+strlen(buffer)-9*/;
		strncpy(redb, ptr, 2);
		redb[2]='\0';
		sscanf(redb, "%x", &red);
		ptr += 2;
		strncpy(greenb, ptr, 2);
		greenb[2]='\0';
		sscanf(greenb, "%x", &green);
		ptr += 2;
		strncpy(blueb, ptr, 2);
		blueb[2]='\0';
		sscanf(blueb, "%x", &blue);

		/* Try to find a matching color */
		ld = 100000; lc=0;
		for ( c2=0; c2<256; ++c2 ) {
			int d, rd, gd, bd;

			if ( MATCH(red, (full_colors[c2].red&0xFF)) &&
			     MATCH(green, (full_colors[c2].green&0xFF)) &&
			     MATCH(blue, (full_colors[c2].blue&0xFF)) ) {
				break;
			}
			rd = ((full_colors[c2].red&0xFF)-red);
			rd = rd*rd;
			gd = ((full_colors[c2].green&0xFF)-green);
			gd = gd*gd;
			bd = ((full_colors[c2].blue&0xFF)-blue);
			bd = bd*bd;
			d = rd+gd+bd;
			if ( d < ld ) {
				ld = d;
				lc = c2;
			}
		}
		if ( c2 == 256 )
			c2=lc;

		printf("Try color: #%0.2x%0.2x%0.2x\n",
					full_colors[c2].red&0xFF,
					full_colors[c2].green&0xFF,
					full_colors[c2].blue&0xFF);
	}
	exit(0);
}
