
/* Change the endianness of a Maelstrom icon or cicn resource */

#include <stdio.h>

#define bytesexs(x)	(x = (x << 8) | (x >> 8))

main(int argc, char *argv[])
{
	unsigned short width, height;
	FILE *fp;

	while ( argc != 1 ) {
		if ( (fp=fopen(argv[--argc], "r+")) ) {
			fread(&width, 2, 1, fp);
			fread(&height, 2, 1, fp);
			printf("Changing image: %s\n", argv[argc]);
			printf("Old width/height = %d/%d\n", width, height);
			fseek(fp, 0, SEEK_SET);
			bytesexs(width);
			fwrite(&width, 2, 1, fp);
			bytesexs(height);
			fwrite(&height, 2, 1, fp);
			printf("New width/height = %d/%d\n", width, height);
		}
	}
}
