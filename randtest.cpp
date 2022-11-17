
/* Just a simple thing to test FastRandom() across different architectures */

#include "fastrand.cc"

main()
{
	SeedRandom(1L);
	while ( 1 ) {
		printf(" ( 2 : %hd ) ", FastRandom(2));
		printf(" ( 3 : %hd ) ", FastRandom(3));
		printf(" ( 3600 : %hd )\n", FastRandom(3600));
		sleep(1);
	}
}
