
#include <stdio.h>
#include <string.h>

#include "colortable.h"		/* The standard Maelstrom colormap */

struct xpm_color {
	char xpm_pixel[5];
	unsigned  char pixel;
	};

/* Macro to set a bit in a bytely scanline */
#ifdef DEBUG
#define SETBIT(scanline, i, bit) \
		{ \
			if ( bit << (7 - (i)%8) ) \
				fprintf(stderr, "X"); \
			else \
				fprintf(stderr, " "); \
			(scanline[(i)/8] |= bit << (7 - (i)%8)); \
		} 
#else
#define SETBIT(scanline, i, bit) \
		(scanline[(i)/8] |= bit << (7 - (i)%8))
#endif /* DEBUG */
#define MASKWIDTH	(width/8 + (width%8 ? 1 : 0))

#define MATCH(A, B)	(A == B)

main(int argc, char *argv[])
{
	char buffer[4096], buflet[5], nothinglet[5];
	char redb[3], greenb[3], blueb[3];
	int red, green, blue;
	int i, a, c, c2;
	int num_colors, cpp;
	unsigned short width, height;
	struct xpm_color *Xpm_Colors;
	char *ptr, *mask;

	for ( a=1; a<argc; ++a ) {
		FILE *f, *o;

		if ( (f=fopen(argv[a], "r")) == NULL ) {
			perror(argv[a]);
			continue;
		}
		fgets(buffer, 4095, f);
		if ( !(ptr=strstr(buffer, "XPM")) ) {
			fprintf(stderr, "%s is not an XPM file!\n", argv[a]);
			fclose(f);
			continue;
		}
		if ( (ptr=strstr(argv[a], ".XPM")) )
			*ptr = '\0';
		sprintf(buffer, "%s.sprite", argv[a]);
		if ( (o=fopen(buffer, "w")) == NULL ) {
			perror(buffer);
			fclose(f);
			continue;
		}
		fgets(buffer, 4095, f);
		fgets(buffer, 4095, f);
		fgets(buffer, 4095, f);
		sscanf(buffer, "\" %hu %hu %d %d",&width,&height,&num_colors,&cpp);
		Xpm_Colors = (struct xpm_color *)
					malloc(num_colors*sizeof(struct  xpm_color));
		fwrite(&width, sizeof(unsigned short), 1, o);
		fwrite(&height, sizeof(unsigned short), 1, o);
		mask = (unsigned char *) malloc(MASKWIDTH*height);
		memset(mask, 0, MASKWIDTH*height);
		fgets(buffer, 4095, f);
		for ( c=0; c<num_colors; ++c ) {
			red = green = blue = 0;
			fgets(buffer, 4095, f);
			/* Look for the Nothing color */
			if ( strstr(buffer, "one") ) {
				strncpy(nothinglet, &buffer[1], cpp);
				nothinglet[cpp] = '\0';
				continue;
			}
			ptr=buffer+strlen(buffer)-9;
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
			for ( c2=0; c2<256; ++c2 ) {
				if ( MATCH(red, (full_colors[c2].red&0xFF)) &&
				     MATCH(green, (full_colors[c2].green&0xFF)) &&
				     MATCH(blue, (full_colors[c2].blue&0xFF)) ) {
					break;
				}
			}
			if ( c2 == 256 ) {
				printf(
	"Colortable is full!  Color = (0x%x, 0x%x, 0x%x)\n", red, green, blue);
				exit(1);
			}
			strncpy(Xpm_Colors[c].xpm_pixel, &buffer[1], cpp);
			Xpm_Colors[c].xpm_pixel[cpp] = '\0';
			Xpm_Colors[c].pixel = c2;
		}

		/* Now print out the pixel-map */
		fgets(buffer, 4095, f);
		for ( i=0; i<height; ++i ) {
			fgets(buffer, 4095, f);
			for ( c=0, ptr=&buffer[1]; c<width; ++c, ptr += cpp ) {
				int m_offset = (i*width+c);
				
				strncpy(buflet, ptr, cpp);
				buflet[cpp]='\0';
				if ( strcmp(buflet, nothinglet) == 0 ) {
					SETBIT(mask, m_offset, 0);
					fputc('\0', o);
					continue;
				} else {
					SETBIT(mask, m_offset, 1);
fprintf(stderr,"%d", m_offset);
				}

				for ( c2=0; c2<256; ++c2 ) {
					if ( strcmp(buflet, Xpm_Colors[c2].xpm_pixel) == 0 ) {
						fputc(Xpm_Colors[c2].pixel, o);
						break;
					}
				}
				if ( c2 == 256 ) {
					fprintf(stderr, 
			"Pixel '%s': No pixel match!? -- corrupt image\n", buflet);
				}
			}
#ifdef DEBUG
			fprintf(stderr, "\n");
#endif
		}
		/* Now write the mask */
fprintf(stderr, "fwrite(mask, 1, %d, o)\n", MASKWIDTH*height);
		fwrite(mask, 1, MASKWIDTH*height, o);
		fclose(f);
		fclose(o);
		free(Xpm_Colors);
		free(mask);
	}
}
