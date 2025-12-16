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
#include "object.h"
#include "about.h"
#include "game.h"


void
AboutPanelDelegate::OnShow()
{
	int centerX, centerY;
	int x, y, off;

	centerX = GAME_WIDTH / 2;
	centerY = GAME_HEIGHT / 2;

	x = (centerX - 240) * SCALE_FACTOR;
	y = (centerY - 104) * SCALE_FACTOR;
	off = 39 * SCALE_FACTOR;

	objects[numsprites++] = 
		new Object(x, y, 0, 0, gPlayerShip[0], 1);
	y += off;
	objects[numsprites++] = 
		new Object(x, y, 0, 0, gPrize, 2);
	y += off;
	objects[numsprites++] = 
		new Object(x, y, 0, 0, gBonusBlit, 2);
	y += off;
	objects[numsprites++] = 
		new Object(x, y, 0, 0, gMult[3], 1);
	y += off;
	objects[numsprites++] = 
		new Object(x, y, 0, 0, gDamagedShip, 1);
	y += off;

	/* -- Now for the second column */
	x = (centerX + 20) * SCALE_FACTOR;
	y = (centerY - 104) * SCALE_FACTOR;
	off = 39 * SCALE_FACTOR;

	objects[numsprites++] = 
		new Object(x, y, 0, 0, gRock1R, 1);
	y += off;
	objects[numsprites++] = 
		new Object(x, y, 0, 0, gSteelRoidR, 1);
	y += off;
	objects[numsprites++] = 
		new Object(x, y, 0, 0, gNova, 4);
	y += off;
	objects[numsprites++] = 
		new Object(x, y, 0, 0, gMineBlitL, 1);
	y += off;
	objects[numsprites++] = 
		new Object(x, y, 0, 0, gVortexBlit, 3);
	y += off;
	objects[numsprites++] = 
		new Object(x, y, 0, 0, gEnemyShip, 1);
	y += off;

}

void
AboutPanelDelegate::OnHide()
{
	int i;

	for ( i=0; i<numsprites; ++i )
		delete objects[i];
	numsprites = 0;
}

void
AboutPanelDelegate::OnDraw(DRAWLEVEL drawLevel)
{
	int i;

	if ( drawLevel != DRAWLEVEL_NORMAL ) {
		return;
	}

	for ( i=0; i<numsprites; ++i ) {
		objects[i]->Move(0);
		objects[i]->BlitSprite();
	}
}
