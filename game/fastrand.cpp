/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* -- Return a random value between 0 and range - 1 */

#include <SDL3/SDL.h>

#include "fastrand.h"

//#define SERIOUS_DEBUG


static Uint32 randomSeed;

void SeedRandom(Uint32 Seed)
{
#ifdef SERIOUS_DEBUG
  fprintf(stderr, "SeedRandom(%8.8x)\n", Seed);
#endif
	if ( ! Seed ) {
		Seed = SDL_rand_bits();
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
	Uint32 calc;
	Uint32 regD0;
	Uint32 regD1;
	Uint32 regD2;

#ifdef SERIOUS_DEBUG
  fprintf(stderr, "FastRandom(%hd)  Seed in: %8.8x ", range, randomSeed);
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
	
	/* An unsigned value is never < 0 */
	/*************************************
	 Not compiled:
	if (regD0 < 0)
		regD0 += 0x7FFFFFFF;
	 *************************************/
	
	randomSeed = regD0;
#ifdef SERIOUS_DEBUG
  fprintf(stderr, "Seed out: %8.8x ", randomSeed);
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
