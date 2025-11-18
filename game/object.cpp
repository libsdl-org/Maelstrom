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
#include "game.h"


/* The screen object class */

Object::Object(int X, int Y, int Xvec, int Yvec, Blit *blit, int PhaseTime)
{
	Points = DEFAULT_PTS;

	Set_Blit(blit);
	if ( (phasetime=PhaseTime) != NO_PHASE_CHANGE )
		phase = FastRandom(myblit->numFrames);
	else
		phase = 0;
	nextphase = 0;

	playground.left = (0 << SPRITE_PRECISION);
	playground.right = (GAME_WIDTH << SPRITE_PRECISION);
	playground.top = (0 << SPRITE_PRECISION);
	playground.bottom = (GAME_HEIGHT << SPRITE_PRECISION);

	SetPos(X, Y);
	xvec = Xvec;
	yvec = Yvec;

	solid = 1;
	shootable = 1;
	HitPoints = DEFAULT_HITS;
	Exploding = 0;
	Set_TTL(-1);
	++gNumSprites;
}

Object::~Object()
{
//error("Object destructor called!\n");
	--gNumSprites;
}

/* What happens when we have been shot up or crashed into */
/* Returns 1 if we die here, instead of go into explosion */
int 
Object::Explode(void)
{
	if ( Exploding )
		return(0);
	Exploding = 1;
	solid = 0;
	shootable = 0;
	phase = 0;
	nextphase = 0;
	phasetime = 2;
	xvec = yvec = 0;
	Set_Blit(gExplosion);
	Set_TTL(myblit->numFrames*phasetime);
	ExplodeSound();
	return(0);
}

/* Movement */
/* This function returns 0, or -1 if the sprite died */
int 
Object::Move(int Frozen)		// This is called every timestep.
{
	if ( ! Frozen )
		SetPos(x+xvec, y+yvec);

	/* Phase, but don't draw our new position */
	Phase();

	/* Does this object have a lifetime? */
	if ( TTL && (--TTL == 0) ) {	// This sprite died...
		return(BeenTimedOut());
	}
	return(0);
}
void
Object::BlitSprite(void)
{
	RenderSprite(myblit->sprite[phase], x, y, xsize, ysize);
}

/* Sound functions */
void 
Object::HitSound(void)
{
	sound->PlaySound(gSteelHit, 3);
}
void 
Object::ExplodeSound(void)
{
	sound->PlaySound(gExplosionSound, 3);
}

/* The objects!! */
Object *gSprites[MAX_SPRITES];
