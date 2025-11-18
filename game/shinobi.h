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

/* Da da da dum, the ENEMY */

class Shinobi : public Object {

public:
	Shinobi(int X, int Y, Blit *blit, int ShotOdds) :
					Object(X, Y, 0, 0, blit, 1) {
		Set_Points(ENEMY_PTS);
		Set_HitPoints(ENEMY_HITS);

		for ( int i=0; i<MAX_SHOTS; ++i ) {
			shots[i] = new Shot;
			shots[i]->damage = PLAYER_HITS;
		}
		nextshot = 0;
		shotodds = ShotOdds;
		target = AcquireTarget();
		barrel = phase;
		numshots = 0;

		gEnemySprite = this;
		sound->PlaySound(gEnemyAppears, 4);
	}
	~Shinobi() {
		for ( int i=0; i<MAX_SHOTS; ++i )
			delete shots[i];
		gEnemySprite = NULL;
	}

	/* This is duplicated in the Homing class */
	virtual int AcquireTarget(void) {
		int targets[MAX_PLAYERS];
		int numTargets = 0;

		for ( int i=0; i < MAX_PLAYERS; ++i ) {
			if (!gPlayers[i]->IsValid()) {
				continue;
			}
			if ( gPlayers[i]->Alive() ) {
				targets[numTargets++] = i;
			}
		}
		if (numTargets > 0) {
			return targets[FastRandom(numTargets)];
		}
		return -1;
	}


	virtual Shot *ShotHit(Rect *hitRect) {
		int i;
		/* Shots are painless if we are exploding */
		if ( Exploding )
			return(NULL);

		/* Otherwise.. Ow! :-) */
		OBJ_LOOP(i, numshots) {
			if ( Overlap(&shots[i]->hitRect, hitRect) ) {
				/* KillShot() rearranges the shot[] array */
				Shot *shotputt = shots[i];
				KillShot(i);
				return(shotputt);
			}
		}
		return(NULL);
	}
	virtual int Move(int Frozen) {
		int DX, DY, slope;
		int newphase;
		int coin, i, alive;

		/* Move and time out old shots */
		OBJ_LOOP(i, numshots) {
			int offset;

			if ( --shots[i]->ttl == 0 ) {
				KillShot(i);
				continue;
			}

			/* Set new X position */
			shots[i]->x += shots[i]->xvel;
			if ( shots[i]->x > playground.right )
				shots[i]->x = playground.left +
						(shots[i]->x-playground.right);
			else if ( shots[i]->x < playground.left )
				shots[i]->x = playground.right -
						(playground.left-shots[i]->x);

			/* Set new Y position */
			shots[i]->y += shots[i]->yvel;
			if ( shots[i]->y > playground.bottom )
				shots[i]->y = playground.top +
						(shots[i]->y-playground.bottom);
			else if ( shots[i]->y < playground.top )
				shots[i]->y = playground.bottom -
						(playground.top-shots[i]->y);

			/* -- Setup the hit rectangle */
			offset = (shots[i]->y>>SPRITE_PRECISION);
			shots[i]->hitRect.top = offset;
			shots[i]->hitRect.bottom = offset+SHOT_SIZE;
			offset = (shots[i]->x>>SPRITE_PRECISION);
			shots[i]->hitRect.left = offset;
			shots[i]->hitRect.right = offset+SHOT_SIZE;
		}

		/* Do no shooting if we are exploding. */
		if ( Exploding ) {
			return(Object::Move(Frozen));
		}

		/* Find out where our target is */
		if ( ((target >= 0) && gPlayers[target]->Alive()) ||
					((target=AcquireTarget()) >= 0) ) {
			gPlayers[target]->GetPos(&DX, &DY);
			DX -= x;
			DY -= y;
			slope = (abs(DX)-abs(DY));
	
			/* -- See if we should accelerate */
			/* -- figure out what sector we are in */
			if ( DY < 0 ) {
				if ( DX < 0 ) {
					/* -- We are in sector 4 */
					newphase = 6;
					if ( slope < 0 )
						++newphase;
				} else {
					/* -- We are in sector 1 */
					newphase = 0;
					if ( slope > 0 )
						++newphase;
				}
			} else {
				if ( DX < 0 ) {
					/* -- We are in sector 3 */
					newphase = 4;
					if ( slope > 0 )
						++newphase;
				} else {
					/* -- We are in sector 2 */
					newphase = 2;
					if ( slope < 0 )
						++newphase;
				}
			}

			newphase *= 6;
			newphase += FastRandom(6);

			/* -- Turn to a new one */
			xvec = 30;

			coin = FastRandom(100);
			if ( coin == 0 )
				yvec = 30;
			else if ( coin == 1 )
				yvec = -30;
			else if ( coin < 7 )
				yvec = 0;

			barrel = (barrel + newphase)/2;
//error("phase = %d, blit = 0x%x\n", barrel, myblit);
			++nextshot;
			if ( (FastRandom(shotodds) == 0) &&
					(nextshot >= ENEMY_SHOT_DELAY) ) {
		/* -- If we are within range and facing the ship, FIRE! */
				(void) MakeShot(0);
			}
		}

		alive = Object::Move(Frozen);

		if ( ((x+26)>>SPRITE_PRECISION) >= GAME_WIDTH )
			alive = -1;
		return(alive);
	}
	virtual void BlitSprite(void) {
		/* Draw the new shots */
		int i;
		OBJ_LOOP(i, numshots) {
			RenderSprite(gEnemyShot, shots[i]->x, shots[i]->y, SHOT_SIZE, SHOT_SIZE);
		}
		Object::BlitSprite();
	}

	virtual void HitSound(void) {
		sound->PlaySound(gBonk, 3);
	}
	virtual void ExplodeSound(void) {
		sound->PlaySound(gExplosionSound, 3);
	}

private:
	Shot *shots[MAX_SHOTS];
	int nextshot;
	int shotodds;
	int target;
	int barrel;
	int numshots;

	virtual int MakeShot(int offset) {
		int shotphase;

		if ( numshots == MAX_SHOTS )
			return(-1);

		/* Handle the velocity */
		if ( (shotphase = barrel+offset) < 0 )
			shotphase = myblit->numFrames-1;
		else if ( shotphase == myblit->numFrames )
			shotphase = 0;
		shots[numshots]->yvel =
			(gVelocityTable[shotphase].v<<SHOT_SCALE_FACTOR);
		shots[numshots]->xvel =
			(gVelocityTable[shotphase].h<<SHOT_SCALE_FACTOR);

		/* Handle the position */
		shots[numshots]->x = x;
		shots[numshots]->y = y;
		offset = ((SPRITES_WIDTH/2)-2)<<SPRITE_PRECISION;
		shots[numshots]->x += offset;
		shots[numshots]->y += offset;

		shots[numshots]->xvel += xvec;
		shots[numshots]->x -= xvec;
		shots[numshots]->yvel += yvec;
		shots[numshots]->y -= yvec;

		/* -- Setup the hit rectangle */
		offset = (shots[numshots]->y>>SPRITE_PRECISION);
		shots[numshots]->hitRect.top = offset;
		shots[numshots]->hitRect.bottom = offset+SHOT_SIZE;
		offset = (shots[numshots]->x>>SPRITE_PRECISION);
		shots[numshots]->hitRect.left = offset;
		shots[numshots]->hitRect.right = offset+SHOT_SIZE;

		/* How LONG do they live? :) */
		shots[numshots]->ttl = SHOT_DURATION;
		return(++numshots);
	}

	virtual void KillShot(int index) {
		OBJ_KILL(shots, index, numshots, Shot);
	}
};


class BigShinobi : public Shinobi {

public:
	BigShinobi(int X, int Y) : Shinobi(X, Y, gEnemyShip, 30) {
	}
};

class LittleShinobi : public Shinobi {

public:
	LittleShinobi(int X, int Y) : Shinobi(X, Y, gEnemyShip2, 15) {
	}
};

