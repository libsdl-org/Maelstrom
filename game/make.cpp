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

#include "Maelstrom_Globals.h"
#include "make.h"
#include "game.h"
#include "netplay.h"
#include "object.h"
#include "player.h"
#include "objects.h"
#include "shinobi.h"


/* ----------------------------------------------------------------- */
/* -- Make an enemy Shenobi fighter! */

void MakeEnemy(void)
{
	int  newsprite, x, y;

	y = FastRandom(GAME_HEIGHT - SPRITES_WIDTH) + SPRITES_WIDTH;
	y *= SCALE_FACTOR;
	x = 0;

	newsprite = gNumSprites;
	if (FastRandom(5 + gWave) > 10)
		gSprites[newsprite] = new LittleShinobi(x, y);
	else
		gSprites[newsprite] = new BigShinobi(x, y);

	SetSteamTimelineEvent(STEAM_TIMELINE_EVENT_ENEMY);
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
		x = FastRandom(GAME_WIDTH - SPRITES_WIDTH) + SPRITES_WIDTH;
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
}	/* -- MakePrize */


/* ----------------------------------------------------------------- */
/* -- Make a multiplier */

void MakeMultiplier(void)
{
	int	newsprite, x, y;

	x = FastRandom(GAME_WIDTH - 3*SPRITES_WIDTH) + SPRITES_WIDTH;
	y = FastRandom(GAME_HEIGHT - 3*SPRITES_WIDTH) + SPRITES_WIDTH;

	x *= SCALE_FACTOR;
	y *= SCALE_FACTOR;

	newsprite = gNumSprites;
	gSprites[newsprite] = new Multiplier(x, y, FastRandom(4)+2);
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

	x = FastRandom(GAME_WIDTH - 3*SPRITES_WIDTH) + SPRITES_WIDTH;
	y = FastRandom(GAME_HEIGHT - 3*SPRITES_WIDTH) + SPRITES_WIDTH;

	x *= SCALE_FACTOR;
	y *= SCALE_FACTOR;

// -- Make sure it isn't appearing right next to the ship

	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}

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

	SetSteamTimelineEvent(STEAM_TIMELINE_EVENT_NOVA);
}	/* -- MakeNova */


/* ----------------------------------------------------------------- */
/* -- Make a bonus */

void MakeBonus(void)
{
	int	newsprite, x, y, which;
	int	rx, xVel, yVel;
	int	index, cap;
	int	multFact;

	if (FastRandom(BLUE_MOON) == 0)
		cap = (FastRandom(MOON_FACTOR) + 2) * 2;
	else
		cap = 1;

	for (index = 0; index < cap; index++) {
		x = FastRandom(GAME_WIDTH - SPRITES_WIDTH) + SPRITES_WIDTH;
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

	x = FastRandom(GAME_WIDTH - SPRITES_WIDTH) + SPRITES_WIDTH;
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
	int	newsprite, i;
	int	x, y, min_bad_distance;
	int	index, cap;

	if (FastRandom(BLUE_MOON) == 0)
		cap = (FastRandom(MOON_FACTOR) + 2) * 2;
	else
		cap = 1;
	
	for (index = 0; index < cap; index++) {
		min_bad_distance = MIN_BAD_DISTANCE;
	tryAgain:
		if ( min_bad_distance )
			--min_bad_distance;
	
		x = FastRandom(GAME_WIDTH - 3*SPRITES_WIDTH) + SPRITES_WIDTH;
		y = FastRandom(GAME_HEIGHT - 3*SPRITES_WIDTH) + SPRITES_WIDTH;

		x *= SCALE_FACTOR;
		y *= SCALE_FACTOR;
	
	// -- Make sure it isn't appearing right next to the ship

		OBJ_LOOP(i, MAX_PLAYERS) {
			if (!gPlayers[i]->IsValid()) {
				continue;
			}

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
	SetSteamTimelineEvent(STEAM_TIMELINE_EVENT_GRAVITY);
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
	
		x = FastRandom(GAME_WIDTH - SPRITES_WIDTH) + SPRITES_WIDTH;
		y = 0;
	
		x *= SCALE_FACTOR;
		y *= SCALE_FACTOR;
	
		newsprite = gNumSprites;
		gSprites[newsprite] = new Homing(x, y, xVel, yVel);
	}
	SetSteamTimelineEvent(STEAM_TIMELINE_EVENT_MINE);
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

