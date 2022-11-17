
/* -- Return a random value between 0 and range - 1 */

#include <sys/time.h>


static	unsigned long	   randomSeed;

void SeedRandom(unsigned long Seed)
{
	struct timeval tv;

#ifdef SERIOUS_DEBUG
printf("SeedRandom(%lu)\n", Seed);
#endif
	if ( ! Seed ) {
		gettimeofday(&tv, NULL);
		randomSeed = ((tv.tv_usec<<16)|((tv.tv_sec^tv.tv_usec)&0xFFFF));
		return;
	}
	randomSeed = Seed;
}

unsigned long GetRandSeed(void)
{
	return(randomSeed);
}

/* This magic is wholly the result of Andrew Welch, not me. :-) */
short FastRandom(short range)
{
	short			result;
	register unsigned long 	  calc;
	register unsigned long 	  regD0;
	register unsigned long 	  regD1;
	register unsigned long 	  regD2;

#ifdef SERIOUS_DEBUG
printf("FastRandom(%hd)  Seed in: %lu ", range, randomSeed);
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
printf("Seed out: %lu ", randomSeed);
#endif
	
	if ((regD0 & 0x0000FFFF) == 0x8000)
		regD0 &= 0xFFFF0000;

/* -- Now that we have our pseudo random number, pin it to the range we want */

	regD1 = range;
	regD1 *= (regD0 & 0x0000FFFF);
	regD1 = (regD1 >> 16);
	
	result = regD1;
#ifdef SERIOUS_DEBUG
printf("Result: %hu\n", result);
#endif
	
	return result;
}
