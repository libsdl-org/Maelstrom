
#include <stdio.h>

main()
{
	int i, c;

	printf("static char array[] = {");
	for ( i=0; (c=getchar()) != EOF; ++i ) {
		if ( ! (i%8) )
			printf("\n\t");
		printf("0x%.2x, ", c);
	}
	printf("\n};\n");
}
