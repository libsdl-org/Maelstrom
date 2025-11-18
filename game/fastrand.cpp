/*
    Maelstrom: Open Source version of the classic game by Ambrosia Software
    Copyright (C) 1997-2011  Sam Lantinga

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* -- Return a random value between 0 and range - 1 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "SDL_types.h"

#include "fastrand.h"

//#define SERIOUS_DEBUG


static Uint32 randomSeed;

void SeedRandom(Uint32 Seed)
{
#ifdef SERIOUS_DEBUG
  fprintf(stderr, "SeedRandom(%8.8x)\n", Seed);
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
	
	/* An unsigned value < 0 is always 0 */
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
