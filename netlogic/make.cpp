
#include "Maelstrom_Globals.h"
#include "netplay.h"
#include "object.h"
#include "player.h"
#include "globals.h"
#include "objects.h"
#include "shinobi.h"


/* ----------------------------------------------------------------- */
/* -- Make an enemy Shenobi fighter! */

void MakeEnemy(void)
{
	int  newsprite, x, y;

	y = FastRandom(gScrnRect.bottom-gScrnRect.top-SPRITES_WIDTH)
							 + SPRITES_WIDTH;
	y *= SCALE_FACTOR;
	x = 0;

	newsprite = gNumSprites;
	if (FastRandom(5 + gWave) > 10)
		gSprites[newsprite] = new LittleShinobi(x, y);
	else
		gSprites[newsprite] = new BigShinobi(x, y);
}	/* -- MakeEnemy */


/* ----------------------------------------------------------------- */
/* -- Make a Prize */

void MakePrize(void)
{
	int	x, y, newsprite, xVel, yVel, rx;
	int	index, cap;

	if (FastRandom(BLUE_MOON) == 0)
		cap = (FastRandom(MOON_FACTOR) + 2) * 2;
	else
		cap = 1;
	
	for (index = 0; index < cap; index++) {
		x = FastRandom(gScrnRect.right - gScrnRect.left -
						SPRITES_WIDTH) + SPRITES_WIDTH;
		y = 0;
	
		x *= SCALE_FACTOR;
		y *= SCALE_FACTOR;
	
		rx = (VEL_FACTOR + (gWave / 6)) * (SCALE_FACTOR);
	
		xVel = yVel = 0;
		while (xVel == 0)
			xVel = FastRandom(rx) - (rx / 2);
		if (xVel > 0)
			xVel += (1 * SCALE_FACTOR);
		else
			xVel -= (1 * SCALE_FACTOR);
	
		while (yVel == 0)
			yVel = FastRandom(rx) - (rx / 2);
		if (yVel > 0)
			yVel += (1 * SCALE_FACTOR);
		else
			yVel -= (1 * SCALE_FACTOR);
	
		newsprite = gNumSprites;
		gSprites[newsprite] = new Prize(x, y, xVel, yVel);
	}
	return;
}	/* -- MakePrize */


/* ----------------------------------------------------------------- */
/* -- Make a multiplier */

void MakeMultiplier(void)
{
	int	newsprite, x, y;

	x = FastRandom(gClipRect.right - gClipRect.left - 
						SPRITES_WIDTH) + SPRITES_WIDTH;
	y = FastRandom(gClipRect.bottom - gClipRect.top - 
				SPRITES_WIDTH - STATUS_HEIGHT) + SPRITES_WIDTH;

	x *= SCALE_FACTOR;
	y *= SCALE_FACTOR;

	newsprite = gNumSprites;
	gSprites[newsprite] = new Multiplier(x, y, FastRandom(4)+2);
	return;
}	/* -- MakeMultiplier */


/* ----------------------------------------------------------------- */
/* -- Make a nova... (!) */

void MakeNova(void)
{
	int	newsprite, min_bad_distance, i, x, y;

	min_bad_distance = MIN_BAD_DISTANCE;
tryAgain:
	if ( min_bad_distance )
		--min_bad_distance;

	x = FastRandom(gClipRect.right - gClipRect.left - 
						SPRITES_WIDTH) + SPRITES_WIDTH;
	y = FastRandom(gClipRect.bottom - gClipRect.top - 
				SPRITES_WIDTH - STATUS_HEIGHT) + SPRITES_WIDTH;

	x *= SCALE_FACTOR;
	y *= SCALE_FACTOR;

// -- Make sure it isn't appearing right next to the ship

	for ( i=gNumPlayers; i--; ) {
		int	xDist, yDist;
	
		/* Make sure the player is alive. :) */
		if ( ! gPlayers[i]->Alive() )
			continue;

		gPlayers[i]->GetPos(&xDist, &yDist);
		xDist = abs(xDist-x);
		yDist = abs(yDist-y);
	
		if ( (xDist < (SCALE_FACTOR * min_bad_distance)) || 
		     (yDist < (SCALE_FACTOR * min_bad_distance)) )
			goto tryAgain;
	}

	newsprite = gNumSprites;
	gSprites[newsprite] = new Nova(x, y);
}	/* -- MakeNova */


/* ----------------------------------------------------------------- */
/* -- Make a bonus */

void MakeBonus(void)
{
	int	newsprite, x, y, which;
	int	rx, xVel, yVel;
	int	index, cap;
	long	multFact;

	if (FastRandom(BLUE_MOON) == 0)
		cap = (FastRandom(MOON_FACTOR) + 2) * 2;
	else
		cap = 1;

	for (index = 0; index < cap; index++) {
		x = FastRandom(gScrnRect.right - gScrnRect.left - 
						SPRITES_WIDTH) + SPRITES_WIDTH;
		y = 0;
	
		x *= SCALE_FACTOR;
		y *= SCALE_FACTOR;
	
		which = FastRandom(6);
		if (which == 0)
			multFact = 500L;
		else
			multFact = which * 1000L;
	
		rx = (VEL_FACTOR + (gWave / 6)) * (SCALE_FACTOR);
	
		xVel = yVel = 0;
		while (xVel == 0)
			xVel = FastRandom(rx / 2);
		xVel += (3 * SCALE_FACTOR);
	
		yVel = xVel;
	
		newsprite = gNumSprites;
		gSprites[newsprite] = new Bonus(x, y, xVel, yVel, multFact);
	}
}	/* -- MakeBonus */


/* ----------------------------------------------------------------- */
/* -- Make a damaged ship */

void MakeDamagedShip(void)
{
	int	newsprite, x, y, xVel, yVel, rx;

	x = FastRandom(gScrnRect.right - gScrnRect.left - 
						SPRITES_WIDTH) + SPRITES_WIDTH;
	y = 0;

	x *= SCALE_FACTOR;
	y *= SCALE_FACTOR;

	rx = (VEL_FACTOR) * (SCALE_FACTOR);

	xVel = yVel = 0;
	while (xVel == 0)
		xVel = FastRandom(rx) - (rx / 2);
	if (xVel > 0)
		xVel += (0 * SCALE_FACTOR);
	else
		xVel -= (0 * SCALE_FACTOR);

	while (yVel == 0)
		yVel = FastRandom(rx) - (rx / 2);
	if (yVel > 0)
		yVel += (1 * SCALE_FACTOR);
	else
		yVel -= (1 * SCALE_FACTOR);

	newsprite = gNumSprites;
	gSprites[newsprite] = new DamagedShip(x, y, xVel, yVel);
}	/* -- MakeDamagedShip */


/* ----------------------------------------------------------------- */
/* -- Create a gravity sprite */

void MakeGravity(void)
{
	int	newsprite, i, rx;
	int	x, y, min_bad_distance;
	int	index, cap;

	if (FastRandom(BLUE_MOON) == 0)
		cap = (FastRandom(MOON_FACTOR) + 2) * 2;
	else
		cap = 1;
	
	for (index = 0; index < cap; index++) {
		rx = (VEL_FACTOR + (gWave / 6)) * (SCALE_FACTOR);
	
		min_bad_distance = MIN_BAD_DISTANCE;
	tryAgain:
		if ( min_bad_distance )
			--min_bad_distance;
	
		x = FastRandom(gClipRect.right - gClipRect.left - 
						SPRITES_WIDTH) + SPRITES_WIDTH;
		y = FastRandom(gClipRect.bottom - gClipRect.top - 
				SPRITES_WIDTH - STATUS_HEIGHT) + SPRITES_WIDTH;
		x *= SCALE_FACTOR;
		y *= SCALE_FACTOR;
	
	// -- Make sure it isn't appearing right next to the ship

		for ( i=gNumPlayers; i--; ) {
			int	xDist, yDist;
	
			/* Make sure the player is alive. :) */
			if ( ! gPlayers[i]->Alive() )
				continue;

			gPlayers[i]->GetPos(&xDist, &yDist);
			xDist = abs(xDist-x);
			yDist = abs(yDist-y);
	
			if ( (xDist < (SCALE_FACTOR * min_bad_distance)) || 
			     (yDist < (SCALE_FACTOR * min_bad_distance)) )
				goto tryAgain;
		}
		newsprite = gNumSprites;
		gSprites[newsprite] = new Gravity(x, y);
	}
}	/* -- MakeGravity */


/* ----------------------------------------------------------------- */
/* -- Create a homing pigeon */

void MakeHoming(void)
{
	int	newsprite, xVel, yVel, rx;
	int	x, y;
	int	index, cap;

	if (FastRandom(BLUE_MOON) == 0)
		cap = (FastRandom(MOON_FACTOR) + 2) * 2;
	else
		cap = 1;
	
	for (index = 0; index < cap; index++) {
		rx = (VEL_FACTOR + (gWave / 6)) * (SCALE_FACTOR);
	
		xVel = yVel = 0;
		while (xVel == 0)
			xVel = FastRandom(rx) - (rx / 2);
		if (xVel > 0)
			xVel += (0 * SCALE_FACTOR);
		else
			xVel -= (0 * SCALE_FACTOR);
	
		while (yVel == 0)
			yVel = FastRandom(rx) - (rx / 2);
		if (yVel > 0)
			yVel += (0 * SCALE_FACTOR);
		else
			yVel -= (0 * SCALE_FACTOR);
	
		x = FastRandom(gScrnRect.right - gScrnRect.left -
						SPRITES_WIDTH) + SPRITES_WIDTH;
		y = 0;
	
		x *= SCALE_FACTOR;
		y *= SCALE_FACTOR;
	
		newsprite = gNumSprites;
		gSprites[newsprite] = new Homing(x, y, xVel, yVel);
	}
}	/* -- MakeHoming */


/* ----------------------------------------------------------------- */
/* -- Create a large rock */

void MakeLargeRock(int x, int y)
{
	int	newsprite, xVel, yVel, phaseFreq, rx;

	rx = (VEL_FACTOR + (gWave / 6)) * (SCALE_FACTOR);

	xVel = yVel = 0;
	while (xVel == 0)
		xVel = FastRandom(rx) - (rx / 2);
	if (xVel > 0)
		xVel += (0 * SCALE_FACTOR);
	else
		xVel -= (0 * SCALE_FACTOR);

	while (yVel == 0)
		yVel = FastRandom(rx) - (rx / 2);
	if (yVel > 0)
		yVel += (0 * SCALE_FACTOR);
	else
		yVel -= (0 * SCALE_FACTOR);

	phaseFreq = (FastRandom(3) + 2);

	newsprite = gNumSprites;
	gSprites[newsprite] = new LargeRock(x, y, xVel, yVel, phaseFreq);
}	/* -- MakeLargeRock */


/* ----------------------------------------------------------------- */
/* -- Create a steel asteroid */

void MakeSteelRoid(int x, int y)
{
	int	newsprite, xVel, yVel, rx;

	rx = (VEL_FACTOR + (gWave / 6)) * (SCALE_FACTOR);

	xVel = yVel = 0;
	while (xVel == 0)
		xVel = FastRandom(rx) - (rx / 2);
	if (xVel > 0)
		xVel += (1 * SCALE_FACTOR);
	else
		xVel -= (1 * SCALE_FACTOR);

	while (yVel == 0)
		yVel = FastRandom(rx) - (rx / 2);
	if (yVel > 0)
		yVel += (2 * SCALE_FACTOR);
	else
		yVel -= (2 * SCALE_FACTOR);

	newsprite = gNumSprites;
	gSprites[newsprite] = new SteelRoid(x, y, xVel, yVel);
}	/* -- MakeSteelRoid */

