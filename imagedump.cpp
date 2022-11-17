
#include <stdio.h>
#include <string.h>

#include "mydebug.h"
#include "imagedump.h"


#if IMAGE_FORMAT == IMAGE_GIF
#include "gifencode.cpp"

/* Default to no transparency */
static int transparent = -1;
void SetTransparency(int which)
{
	transparent = which;
}

/* The global 8-bit image dump function */
void ImageDump(FILE *dumpfp, int width, int height, int ncolors,
				unsigned long *new_colors, unsigned char *data)
{
	int bpp, i;
	int *Red, *Green, *Blue;

	/* Find out how many bits of color required to represent our colors */
	for ( bpp=1, i=2; i < ncolors; ++bpp )
		i *= 2;
#ifdef DEBUG
error("Dump: width = %d, height = %d, ncolors = %d, bpp = %d\n",
						width, height, ncolors, bpp);
#endif

	/* Allocate and fill the colors */
	Red = new int[i]; memset(Red, 0, i);
	Green = new int[i]; memset(Green, 0, i);
	Blue = new int[i]; memset(Blue, 0, i);
	for ( i=0; i<ncolors; ++i ) {
		Red[i] = (new_colors[i]>>16)&0xFF;
		Green[i] = (new_colors[i]>>8)&0xFF;
		Blue[i] = (new_colors[i]&0xFF);
	}
	if ( transparent >= 0 ) {
		for ( i=0; i<ncolors; ++i ) {
			if ( transparent == new_colors[i] ) {
				transparent = i;
				break;
			}
		}
		if ( i == ncolors ) {
			error(
	"Warning: transparent color 0x%.6x not found\n", transparent);
			transparent = -1;
		}
	}
	GIFEncode(dumpfp, width, height, 1, 0, transparent, bpp,
						Red, Green, Blue, data);
	delete[] Red;
	delete[] Green;
	delete[] Blue;
}

/* The global > 8-bit image dump function */
void ImageDump(FILE *dumpfp, int width, int height, int bpp,
							unsigned char *data)
{
	int i, c;
	int ri, gi, bi;			/* Color shades we want */
	int rd, gd, bd;			/* Color shades we have */
	long mdist, close, d;		/* Distance and closest pixel */
	int *Red, *Green, *Blue;
	unsigned char *newpixels;

	/* Allocate a new colormap 4x8x4 */
	const int ncolors = 4*8*4;
	Red = new int[ncolors];
	Green = new int[ncolors];
	Blue = new int[ncolors];
	i = 0;
	memset(Red, 0, ncolors*sizeof(int));
	memset(Green, 0, ncolors*sizeof(int));
	memset(Blue, 0, ncolors*sizeof(int));
	for ( int r = 0; r < 4; ++r ) {
		for ( int g = 0; g < 8; ++g ) {
			for ( int b = 0; b < 4; ++b ) {
				Red[i] = ((r*255)/3);
				Green[i] = ((g*255)/7);
				Blue[i] = ((b*255)/3);
				++i;
#ifdef DEBUG
printf("Created colortable entry #%d: (%d,%d,%d)\n", 
			i-1, Red[i-1], Green[i-1], Blue[i-1]);
#endif
			}
		}
	}

	/* Map the data into the new colormap */
	newpixels = new unsigned char[width*height];
	for ( i = 0; i < (width*height); ++i, data += bpp ) {
		mdist = 100000; close=0;

		switch (bpp) {
				/* Assuming 5-6-5 pixel format */
			case 2: {
				unsigned long newpixel;
				memcpy(&newpixel, data, bpp);
				ri = ((newpixel>>11)&0x1F);
				gi = ((newpixel>>5)&0x2F);
				bi = ((newpixel)&0x1F);
				}
				break;
			case 3:
			case 4: {
				unsigned long newpixel;
				memcpy(&newpixel, data, bpp);
				ri = ((newpixel>>16)&0xFF);
				gi = (newpixel>>8&0xFF);
				bi = (newpixel&0xFF);
				}
				break;
			default:
				/* Huh? */
				ri = 0;
				gi = 0;
				bi = 0;
				break;
		}

		/* Cycle through the colors we have */
		for ( c = 0; c<ncolors; ++c ) {
			rd = ri - Red[c];
			gd = gi - Green[c];
			bd = bi - Blue[c];
			d = rd*rd + gd*gd + bd*bd;

			if ( d < mdist ) {
				mdist = d;
				close = c;
			}
		}
#ifdef DEBUG
error("Desired RGB (%d,%d,%d) mapped to existing RGB (%d,%d,%d)\n",
		ri, gi, bi, Red[close], Green[close], Blue[close]);
#endif
		newpixels[i] = close;
	}

	/* No transparency .. */
	GIFEncode(dumpfp, width, height, 1, 0, -1, 2+3+2,
						Red, Green, Blue, newpixels);
	delete[] Red;
	delete[] Green;
	delete[] Blue;
	delete[] newpixels;
}
#endif /* IMAGE_FORMAT is GIF */


#if IMAGE_FORMAT == IMAGE_XPM
static char *xpmchars =
	".#abcdefghijklmnopqrstuvwxyzABCDEFGHIKLMNOPQRSTUVWXYZ0123456789";
const  int  nxpmchars = 64;

/* The global image dump function */
void ImageDump(FILE *dumpfp, int width, int height, int ncolors,
				unsigned long *new_colors, unsigned char *data)
{
	int i, x, len = (width*height);

	/* Output the XPM. :-) */
	fprintf(dumpfp, "/* XPM */\n");
	fprintf(dumpfp, "static char *image[] = {\n");
	fprintf(dumpfp, "/* width height num_colors chars_per_pixel */\n");
	fprintf(dumpfp, "\"   %d    %d        %d          %d\",\n",
				width, height, ncolors, (ncolors/nxpmchars)+1);
	fprintf(dumpfp, "/* colors */\n");
	if ( ncolors > nxpmchars ) {
		for ( i=0; i<ncolors; ++i ) {
			fprintf(dumpfp, "\"%c%c c #%6.6x\",\n",
				xpmchars[i/nxpmchars], xpmchars[i%nxpmchars],
								new_colors[i]);
		}
	} else {
		for ( i=0; i<ncolors; ++i ) {
			fprintf(dumpfp, "\"%c c #%6.6x\",\n",
				xpmchars[i%nxpmchars], new_colors[i]);
		}
	}
	fprintf(dumpfp, "/* pixels */\n");
	if ( ncolors > nxpmchars ) {
		for ( i=0, x=0; i<len; ++i ) {
			if ( x == 0 )
				fprintf(dumpfp, "\"");
			fprintf(dumpfp, "%c%c", xpmchars[data[i]/nxpmchars],
						xpmchars[data[i]%nxpmchars]);
			if ( ++x == width ) {
				fprintf(dumpfp, "\",\n");
				x = 0;
			}
		}
	} else {
		for ( i=0, x=0; i<len; ++i ) {
			if ( x == 0 )
				fprintf(dumpfp, "\"");
			fprintf(dumpfp, "%c", xpmchars[data[i]%nxpmchars]);
			if ( ++x == width ) {
				fprintf(dumpfp, "\",\n");
				x = 0;
			}
		}
	}
	fprintf(dumpfp, "};\n/* A Maelstrom ScreenShot */\n");
}
#endif /* IMAGE_FORMAT is XPM */
