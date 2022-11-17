
#include "Maelstrom_Globals.h"
#include "globals.h"
#include "blit.h"

// Global variables set in this file...
ULONG	gPrizeTime;
ULONG	gBonusTime;
ULONG	gDamagedTime;

/* ----------------------------------------------------------------- */
/* -- Create a new sprite */

int NewSprite(BlitPtr aBlit, int changeTime, 
			int xCoord, int yCoord, int xVel, int yVel, long sType)
{
/* -- Initialize some variables to the defaults */

	gSprites[gNumSprites]->spriteType = sType;
	gSprites[gNumSprites]->xCoord = xCoord;
	gSprites[gNumSprites]->yCoord = yCoord;
	gSprites[gNumSprites]->oldXCoord = xCoord;
	gSprites[gNumSprites]->oldYCoord = yCoord;
	gSprites[gNumSprites]->hitFlag = 0;
	gSprites[gNumSprites]->visible = 1;
	gSprites[gNumSprites]->xVel = xVel;
	gSprites[gNumSprites]->yVel = yVel;
	gSprites[gNumSprites]->numPhases = aBlit->numFrames;
	gSprites[gNumSprites]->phaseOn = FastRandom(aBlit->numFrames);
	gSprites[gNumSprites]->phaseChange = changeTime;
	gSprites[gNumSprites]->changeCount = 0;
	gSprites[gNumSprites]->theBlit = aBlit;
	gSprites[gNumSprites]->onScreen = 0;

	if (changeTime == NO_PHASE_CHANGE)
		gSprites[gNumSprites]->phaseOn = 0;
	
	gNumSprites++;
	return(0);
}	/* -- NewSprite */


/* ----------------------------------------------------------------- */
/* -- Make an enemy Shenobi fighter! */

int MakeEnemy(void)
{
	int  err, x, y;

	y = FastRandom(gScrnRect.bottom-gScrnRect.top-SPRITES_WIDTH)
							 + SPRITES_WIDTH;
	y *= SCALE_FACTOR;
	x = 0;

	if (FastRandom(5 + gWave) > 10) {
		err = NewSprite(gEnemyShip2, 1, x, y, 0, 0, ENEMY_SHIP);
		gSprites[gNumSprites - 1]->spriteTag = SMALL_ENEMY;
	} else {
		err = NewSprite(gEnemyShip, 1, x, y, 0, 0, ENEMY_SHIP);
		gSprites[gNumSprites - 1]->spriteTag = BIG_ENEMY;
	}
	gSprites[gNumSprites - 1]->phaseOn = 0;

	gEnemySprite = gNumSprites -1;

	gWhenEnemy = 0L;

	sound->PlaySound(gEnemyAppears, 4, NULL);
	return(err);
}	/* -- MakeEnemy */


/* ----------------------------------------------------------------- */
/* -- Make a Prize */

int MakePrize(void)
{
	int	x, y, which, xVel, yVel, rx;
	int	err = -1, index, cap;

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
	
		which = FastRandom(3);
	
		err = NewSprite(gPrize, 2, x, y, xVel, yVel, PRIZE);
		gSprites[gNumSprites - 1]->spriteTag = which;
	
		gPrizeTime = gOurTicks;
		gPrizeShown = 1;
	}
	sound->PlaySound(gPrizeAppears, 4, NULL);

	return(err);
}	/* -- MakePrize */


/* ----------------------------------------------------------------- */
/* -- Make a multiplier */

int MakeMultiplier(void)
{
	int	err = -1, x, y, which;

	x = FastRandom(gClipRect.right - gClipRect.left - 
						SPRITES_WIDTH) + SPRITES_WIDTH;
	y = FastRandom(gClipRect.bottom - gClipRect.top - 
				SPRITES_WIDTH - STATUS_HEIGHT) + SPRITES_WIDTH;

	x *= SCALE_FACTOR;
	y *= SCALE_FACTOR;

	which = FastRandom(4);

	switch (which) {
		case 0:
			err = NewSprite(gMult[2-2], NO_PHASE_CHANGE, x, y, 0, 
								0, MULTIPLIER);
			gSprites[gNumSprites - 1]->spriteTag = 2;
			break;
		case 1:
			err = NewSprite(gMult[3-2], NO_PHASE_CHANGE, x, y, 0, 
								0, MULTIPLIER);
			gSprites[gNumSprites - 1]->spriteTag = 3;
			break;
		case 2:
			err = NewSprite(gMult[4-2], NO_PHASE_CHANGE, x, y, 0, 
								0, MULTIPLIER);
			gSprites[gNumSprites - 1]->spriteTag = 4;
			break;
		case 3:
			err = NewSprite(gMult[5-2], NO_PHASE_CHANGE, x, y, 0, 
								0, MULTIPLIER);
			gSprites[gNumSprites - 1]->spriteTag = 5;
			break;
	}

	gMultTime = gOurTicks;
	gMultiplierShown = 1;

	sound->PlaySound(gMultiplier, 4, NULL);
	return(err);
}	/* -- MakeMultiplier */


/* ----------------------------------------------------------------- */
/* -- Make a nova... (!) */

int MakeNova(void)
{
	int	err = -1, x, y;

tryAgain:

	x = FastRandom(gClipRect.right - gClipRect.left - 
						SPRITES_WIDTH) + SPRITES_WIDTH;
	y = FastRandom(gClipRect.bottom - gClipRect.top - 
				SPRITES_WIDTH - STATUS_HEIGHT) + SPRITES_WIDTH;

	x *= SCALE_FACTOR;
	y *= SCALE_FACTOR;

// -- Make sure it isn't appearing right next to the ship

	{
		int	xDist, yDist;

		xDist = (gSprites[0]->xCoord - x);
		if (xDist < 0)
			xDist *= -1;

		yDist = (gSprites[0]->yCoord - y);
		if (yDist < 0)
			yDist *= -1;

		if ( (xDist < (SCALE_FACTOR * MIN_BAD_DISTANCE)) || 
		     (yDist < (SCALE_FACTOR * MIN_BAD_DISTANCE)) )
			goto tryAgain;
	}

	err = NewSprite(gNova, 4, x, y, 0, 0, NOVA);
	gSprites[gNumSprites - 1]->phaseOn = 0;

	gWhenNova = 0L;

	sound->PlaySound(gNovaAppears, 4, NULL);
	return(err);
}	/* -- MakeNova */


/* ----------------------------------------------------------------- */
/* -- Make a bonus */

int MakeBonus(void)
{
	int	x, y, which;
	int	rx, xVel, yVel;
	int	err = -1, index, cap;
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
	
		err = NewSprite(gBonusBlit, 2, x, y, xVel, yVel, BONUS);
		gSprites[gNumSprites - 1]->spriteTag = 0L;
		gSprites[gNumSprites - 1]->spriteTag = multFact;
	
		gBonusTime = gOurTicks;
		gBonusShown = 1;
	}

	sound->PlaySound(gBonusAppears, 4, NULL);
	return(err);
}	/* -- MakeBonus */


/* ----------------------------------------------------------------- */
/* -- Make a damaged ship */

int MakeDamagedShip(void)
{
	int	err = -1, x, y, xVel, yVel, rx;

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

	err = NewSprite(gDamagedShip, 1, x, y, xVel, yVel, DAMAGED_SHIP);

	gWhenDamaged = 0L;
	gDamagedTime = gOurTicks;

	sound->PlaySound(gDamagedAppears, 4, NULL);
	return(err);
}	/* -- MakeDamagedShip */


/* ----------------------------------------------------------------- */
/* -- Create a gravity sprite */

int MakeGravity(void)
{
	int	xVel, yVel, rx;
	int	x, y;
	int	err = -1, index, cap;

	if (FastRandom(BLUE_MOON) == 0)
		cap = (FastRandom(MOON_FACTOR) + 2) * 2;
	else
		cap = 1;
	
	for (index = 0; index < cap; index++) {
		rx = (VEL_FACTOR + (gWave / 6)) * (SCALE_FACTOR);
	
	tryAgain:
	
		xVel = yVel = 0;
		x = FastRandom(gClipRect.right - gClipRect.left - 
						SPRITES_WIDTH) + SPRITES_WIDTH;
		y = FastRandom(gClipRect.bottom - gClipRect.top - 
				SPRITES_WIDTH - STATUS_HEIGHT) + SPRITES_WIDTH;
		x *= SCALE_FACTOR;
		y *= SCALE_FACTOR;
	
	// -- Make sure it isn't appearing right next to the ship

		{
			int	xDist, yDist;
	
			xDist = (gSprites[0]->xCoord - x);
			if (xDist < 0)
				xDist *= -1;
	
			yDist = (gSprites[0]->yCoord - y);
			if (yDist < 0)
				yDist *= -1;
	
			if ( (xDist < (SCALE_FACTOR * MIN_BAD_DISTANCE)) || 
			     (yDist < (SCALE_FACTOR * MIN_BAD_DISTANCE)) )
				goto tryAgain;
		}
		err = NewSprite(gVortexBlit, 2, x, y, xVel, yVel, GRAVITY);
	
		gWhenGrav = 0L;
	}
	sound->PlaySound(gGravAppears, 4, NULL);
	return(err);
}	/* -- MakeGravity */


/* ----------------------------------------------------------------- */
/* -- Create a homing pigeon */

int MakeHoming(void)
{
	int	xVel, yVel, phaseFreq, rx;
	int	x, y;
	int	err = -1, index, cap;

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
	
		phaseFreq = 2;
	
		if (xVel > 0)
			err = NewSprite(gMineBlitR, phaseFreq, x, y, 
						xVel, yVel, HOMING_PIGEON);
		else
			err = NewSprite(gMineBlitL, phaseFreq, x, y, 
						xVel, yVel, HOMING_PIGEON);
	
		gWhenHoming = 0L;
	}
	sound->PlaySound(gHomingAppears, 4, NULL);
	return(err);
}	/* -- MakeHoming */


/* ----------------------------------------------------------------- */
/* -- Create a large rock */

int MakeLargeRock(int x, int y)
{
	int	err, xVel, yVel, phaseFreq, rx;

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

	if (xVel > 0)
		err = NewSprite(gRock1R, phaseFreq, x, y, 
						xVel, yVel, BIG_ASTEROID);
	else
		err = NewSprite(gRock1L, phaseFreq, x, y, 
						xVel, yVel, BIG_ASTEROID);

	gNumRocks++;
	return(err);
}	/* -- MakeLargeRock */


/* ----------------------------------------------------------------- */
/* -- Create a medium rock */

int MakeMediumRock(int x, int y)
{
	int	err, xVel, yVel, phaseFreq, rx;

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

	phaseFreq = (FastRandom(3) + 2);

	if (xVel > 0)
		err = NewSprite(gRock2R, phaseFreq, x, y, 
						xVel, yVel, MEDIUM_ASTEROID);
	else
		err = NewSprite(gRock2L, phaseFreq, x, y, 
						xVel, yVel, MEDIUM_ASTEROID);
	
	gNumRocks++;
	return(err);
}	/* -- MakeMediumRock */


/* ----------------------------------------------------------------- */
/* -- Create a small rock */

int MakeSmallRock(int x, int y)
{
	int	err, xVel, yVel, phaseFreq, rx;

	rx = (VEL_FACTOR + (gWave / 6)) * (SCALE_FACTOR);

	xVel = yVel = 0;
	while (xVel == 0)
		xVel = FastRandom(rx) - (rx / 2);
	if (xVel > 0)
		xVel += (2 * SCALE_FACTOR);
	else
		xVel -= (2 * SCALE_FACTOR);

	while (yVel == 0)
		yVel = FastRandom(rx) - (rx / 2);
	if (yVel > 0)
		yVel += (2 * SCALE_FACTOR);
	else
		yVel -= (2 * SCALE_FACTOR);

	phaseFreq = (FastRandom(3) + 1);

	if (xVel > 0)
		err = NewSprite(gRock3R, phaseFreq, x, y, 
						xVel, yVel, SMALL_ASTEROID);
	else
		err = NewSprite(gRock3L, phaseFreq, x, y, 
						xVel, yVel, SMALL_ASTEROID);
	
	gNumRocks++;
	return(err);
}	/* -- MakeSmallRock */


/* ----------------------------------------------------------------- */
/* -- Create a steel asteroid */

int MakeSteelRoid(int x, int y)
{
	int	err, xVel, yVel, phaseFreq, rx;

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

	phaseFreq = 3;

	if (xVel > 0)
		err = NewSprite(gSteelRoidR, phaseFreq, x, y, 
						xVel, yVel, STEEL_ASTEROID);
	else
		err = NewSprite(gSteelRoidL, phaseFreq, x, y, 
						xVel, yVel, STEEL_ASTEROID);
	
	return(err);
}	/* -- MakeSteelRoid */

