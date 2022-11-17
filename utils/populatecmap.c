
#include <stdio.h>

#include "colortable.h"

#define FIRST_OFFSET	143
#define NEXT_OFFSET	10

main(int argc, char *argv[])
{
	FILE *fp;
	char *mode, buf[24];
	int i;

	if ( argc != 2 ) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		exit(1);
	}
	if ( strcmp(argv[0], "populatecmap") == 0 )
		mode="r+";
	else if ( strcmp(argv[0], "unpopulatecmap") == 0 )
		mode="r";
	else {
		fprintf(stderr, "Inteterminate mode\n");
		exit(1);
	}
	if ( (fp=fopen(argv[1], mode)) == NULL ) { 
		perror(argv[1]);
		exit(1);
	}
	fseek(fp, FIRST_OFFSET, SEEK_SET);

	if ( ! *(mode+1) ) {
		/* We create a colormap from the XPM */
		for ( i=0; i<256; ++i ) {
			int r, g, b;

			fread(buf, 1, 6, fp);
			buf[6] = '\0';
			sscanf(&buf[4], "%x", &b);
			buf[4] = '\0';
			sscanf(&buf[2], "%x", &g);
			buf[2] = '\0';
			sscanf(&buf[0], "%x", &r);
			printf(
"\t\t{ 0x%2.2x%2.2x, 0x%2.2x%2.2x, 0x%2.2x%2.2x },  /* Pixel %d */\n",
						r, r, g, g, b, b, i);
			fseek(fp, NEXT_OFFSET, SEEK_CUR);
		}
	} else {
		/* We modify the XPM according to the colormap */
		for ( i=0; i<256; ++i ) {
			sprintf(buf, "%2.2x%2.2x%2.2x",
					full_colors[i].red&0xFF,
					full_colors[i].green&0xFF,
					full_colors[i].blue&0xFF);
			fwrite(buf, 1, strlen(buf), fp);
			fseek(fp, NEXT_OFFSET, SEEK_CUR);
		}
	}
	exit(0);
}
