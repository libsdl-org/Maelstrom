
/* -- Return a random value between 0 and range - 1 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "SDL_types.h"

static Uint32 randomSeed;

void SeedRandom(Uint32 Seed)
{
#ifdef SERIOUS_DEBUG
  fprintf(stderr, "SeedRandom(%lu)\n", Seed);
#endif
	if ( ! Seed ) {
		srand(time(NULL));
		Seed = (((rand()%0xFFFF)<<16)|(rand()%0xFFFF));
	}
	randomSeed = Seed;
}

Uint32 GetRandSeed(void)
{
	return(randomSeed);
}

/* This magic is wholly the result of Andrew Welch, not me. :-) */
Uint16 FastRandom(Uint16 range)
{
	Uint16 result;
	register Uint32 calc;
	register Uint32 regD0;
	register Uint32 regD1;
	register Uint32 regD2;

#ifdef SERIOUS_DEBUG
  fprintf(stderr, "FastRandom(%hd)  Seed in: %lu ", range, randomSeed);
#endif
	calc = randomSeed;
	regD0 = 0x41A7;
	regD2 = regD0;
	
	regD0 *= calc & 0x0000FFFF;
	regD1 = regD0;
	
	regD1 = regD1 >> 16;
	
	regD2 *= calc >> 16;
	regD2 += regD1;
	regD1 = regD2;
	regD1 += regD1;
	
	regD1 = regD1 >> 16;
	
	regD0 &= 0x0000FFFF;
	regD0 -= 0x7FFFFFFF;
	
	regD2 &= 0x00007FFF;
	regD2 = (regD2 << 16) + (regD2 >> 16);
	
	regD2 += regD1;
	regD0 += regD2;
	
	/* An unsigned value < 0 is always 0 */
	/*************************************
	 Not compiled:
	if (regD0 < 0)
		regD0 += 0x7FFFFFFF;
	 *************************************/
	
	randomSeed = regD0;
#ifdef SERIOUS_DEBUG
  fprintf(stderr, "Seed out: %lu ", randomSeed);
#endif
	
	if ((regD0 & 0x0000FFFF) == 0x8000)
		regD0 &= 0xFFFF0000;

/* -- Now that we have our pseudo random number, pin it to the range we want */

	regD1 = range;
	regD1 *= (regD0 & 0x0000FFFF);
	regD1 = (regD1 >> 16);
	
	result = regD1;
#ifdef SERIOUS_DEBUG
  fprintf(stderr, "Result: %hu\n", result);
#endif
	
	return result;
}
