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
