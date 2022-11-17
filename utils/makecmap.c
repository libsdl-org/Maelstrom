
#include <stdio.h>
#include <string.h>

#define MATCH(A, B)	(A == B)

struct color {
	unsigned short red;
	unsigned short green;
	unsigned short blue;
	} full_colors[256];

main(int argc, char *argv[])
{
	char buffer[4096];
	char redb[3], greenb[3], blueb[3];
	int red, green, blue;
	int i, a, c, c2;
	int width, height, num_colors;
	char *ptr;

	for ( i=0; i<256; ++i ) {
		full_colors[i].red = 0;
		full_colors[i].green = 0;
		full_colors[i].blue = 0;
	}

	for ( a=1; a<argc; ++a ) {
		FILE *f;

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
		fgets(buffer, 4095, f);
		fgets(buffer, 4095, f);
		fgets(buffer, 4095, f);
		sscanf(buffer, "\" %d %d %d ", &width, &height, &num_colors);
		fgets(buffer, 4095, f);
		for ( c=0; c<num_colors; ++c ) {
			red = green = blue = 0;
			fgets(buffer, 4095, f);
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
			for ( c2=1; c2<256; ++c2 ) {
				if ( !full_colors[c2].red &&
				     !full_colors[c2].green &&
				     !full_colors[c2].blue ) {
					full_colors[c2].red = red;
					full_colors[c2].green = green;
					full_colors[c2].blue = blue;
				}
				if ( MATCH(red, full_colors[c2].red) &&
				     MATCH(green, full_colors[c2].green) &&
				     MATCH(blue, full_colors[c2].blue) ) {
					break;
				}
			}
			if ( c2 == 256 ) {
				printf(
	"Colortable is full!  Color = (0x%x, 0x%x, 0x%x)\n", red, green, blue);
			}
		}
	}
	printf("New colortable:\n");
	printf("struct Color color_table[256] = {\n");
	for ( c=0; c<256; ++c ) {
		printf("\t{ 0x%x, 0x%x, 0x%x },\n", 
		full_colors[c].red, full_colors[c].green, full_colors[c].blue);
	}
	printf("};\n");
}
			
