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
#include "netplay.h"
#include "object.h"
#include "player.h"
#include "objects.h"
#include "game.h"


/* ----------------------------------------------------------------- */
/* -- The thrust sound callback */

static void ThrustCallback(Uint8 theChannel)
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if ( gPlayers[i]->IsThrusting() ) {
			sound->PlaySound(gThrusterSound,1,theChannel,ThrustCallback);
			break;
		}
	}
}	/* -- ThrustCallback */


/* ----------------------------------------------------------------- */
/* -- The Player class */

Player:: Player(int index) : Object(0, 0, 0, 0, gPlayerShip[index], NO_PHASE_CHANGE)
{
	int i;

	Valid = 0;
	Index = index;
	Score = 0;
	for ( i=0; i<MAX_SHOTS; ++i ) {
		shots[i] = new Shot;
		shots[i]->damage = 1;
	}
	numshots = 0;

	/* Create a colored dot for this player */
	ship_color = screen->MapRGB(gPlayerColors[index][0],
				    gPlayerColors[index][1],
				    gPlayerColors[index][2]);

	--gNumSprites;		// We aren't really a sprite
}
Player::~Player()
{
	for ( int i=0; i<MAX_SHOTS; ++i )
		delete shots[i];
}

/* Note that the lives argument is ignored during deathmatches */
void
Player::NewGame(int lives)
{
	Playing = 1;
	if ( gGameInfo.IsDeathmatch() )
		Lives = 1;
	else
		Lives = lives;
	Score = 0;
	Frags = 0;
	special = 0;
	Ghost = 0;
	NewShip();
}
void
Player::Continue(int lives)
{
	Playing = 1;
	Lives = lives;
	Ghost = 0;
	NewShip();
}
void 
Player::NewWave(void)
{
	int i;

	/* If we were exploding, rejuvinate us */
	if ( Exploding || (!Alive() && Playing) ) {
		IncrLives(1);
		NewShip();
	}
	Bonus = INITIAL_BONUS;
	BonusMult = 1;
	CutBonus = BONUS_DELAY;
	ShieldOn = 0;
	AutoShield = SAFE_TIME;
	WasShielded = 0;
	Sphase = 0;
	SetPos(
		((GAME_WIDTH/2-((gGameInfo.GetNumPlayers()/2-Index)*(2*SPRITES_WIDTH)))*SCALE_FACTOR),
		((GAME_HEIGHT/2)*SCALE_FACTOR)
	);
	xvec = yvec = 0;
	Thrusting = 0;
	NoThrust = 0;
	ThrustBlit = gThrust1;
	Shooting = 0;
	WasShooting = 0;
	Rotating = 0;
	phase = 0;
	OBJ_LOOP(i, numshots)
		KillShot(i);
}
/* Returns the number of lives left */
int 
Player::NewShip(void)
{
	if ( Lives == 0 ) {
		if (gGameInfo.IsMultiplayer() && !gGameInfo.IsDeathmatch()) {
			// We can live on!
			Ghost = 1;
		} else {
			return(-1);
		}
	}
	controlState = 0;
	solid = 1;
	shootable = 1;
	Set_Blit(gPlayerShip[Index]);
	Set_Points(PLAYER_PTS);
	Set_HitPoints(PLAYER_HITS);
	ShieldOn = 0;
	ShieldLevel = INITIAL_SHIELD;
	AutoShield = SAFE_TIME;
	WasShielded = 1;
	Sphase = 0;
	xvec = yvec = 0;
	Thrusting = 0;
	WasThrusting = 0;
	ThrustBlit = gThrust1;
	Shooting = 0;
	WasShooting = 0;
	Rotating = 0;
	phase = 0;
	phasetime = NO_PHASE_CHANGE;
	Dead = 0;
	Exploding = 0;
	Set_TTL(-1);
	if ( ! gGameInfo.IsDeathmatch() ) {
		if (Lives > 0) {
			--Lives;
		}
	}

	// In Kid Mode you automatically get air brakes
	if ( gGameInfo.IsKidMode() ) {
		special |= AIR_BRAKES;
	}
	return(Lives);
}

/* Increment the number of frags, and handle DeathMatch finale */
void
Player::IncrFrags(void)
{
	++Frags;
	if ( gGameInfo.IsDeathmatch() && (Frags >= gGameInfo.deathMatch) ) {
		/* Game over, we got a stud. :) */
		int i;
		OBJ_LOOP(i, MAX_PLAYERS) {
			if (!gPlayers[i]->IsValid()) {
				continue;
			}
			gPlayers[i]->IncrLives(-1);
			gPlayers[i]->Explode();
#ifdef DEBUG
error("Killing player %d\n", i+1);
#endif
		}
	}
}

void
Player::IncrLives(int lives)
{
	if ( gGameInfo.IsDeathmatch() && (lives > 0) )
		return;
	Lives += lives;
}

/* We've been shot!  (returns 1 if we are dead) */
int
Player::BeenShot(Object *ship, Shot *shot)
{
	if ( Exploding || !Alive() )
		return(0);
	if ( AutoShield || (ShieldOn && (ShieldLevel > 0)) )
		return(0);
	if ( (special & LUCKY_IRISH) && (FastRandom(LUCK_ODDS) == 0) ) {
		sound->PlaySound(gLuckySound, 4);
		return(0);
	}
	return(Object::BeenShot(ship, shot));
}

/* We've been run over!  (returns 1 if we are dead) */
int
Player::BeenRunOver(Object *ship)
{
	if ( Exploding || !Alive() )
		return(0);
	if ( AutoShield || (ShieldOn && (ShieldLevel > 0)) )
		return(0);
	if ( ship->IsPlayer() )		/* Players phase through eachother */
		return(0);
	if ( (special & LUCKY_IRISH) && (FastRandom(LUCK_ODDS) == 0) ) {
		sound->PlaySound(gLuckySound, 4);
		return(0);
	}
	return(Object::BeenRunOver(ship));
}

/* We've been run over by a rock or something */
int
Player::BeenDamaged(int damage)
{
	if ( Exploding || !Alive() )
		return(0);
	if ( AutoShield || (ShieldOn && (ShieldLevel > 0)) )
		return(0);
	if ( (special & LUCKY_IRISH) && (FastRandom(LUCK_ODDS) == 0) ) {
		sound->PlaySound(gLuckySound, 4);
		return(0);
	}
	return(Object::BeenDamaged(damage));
}

/* We expired (returns -1 if our sprite should be deleted) */
int 
Player::BeenTimedOut(void)
{
	Exploding = 0;
	SetPos(
		((GAME_WIDTH/2-((gGameInfo.GetNumPlayers()/2-Index)*(2*SPRITES_WIDTH)))*SCALE_FACTOR),
		((GAME_HEIGHT/2)*SCALE_FACTOR)
	);
	if ( gGameInfo.IsDeathmatch() )
		Dead = (DEAD_DELAY/2);
	else
		Dead = DEAD_DELAY;

	// If we're the last life in a co-op multiplayer game, we're done
	if (gGameInfo.IsMultiplayer() && !gGameInfo.IsDeathmatch() && !Lives) {
		int i;
		bool allGhosts = true;
		OBJ_LOOP(i, MAX_PLAYERS) {
			if (!gPlayers[i]->IsValid()) {
				continue;
			}
			if ( i != Index && !gPlayers[i]->Ghost) {
				allGhosts = false;
				break;
			}
		}
		if (allGhosts) {
			OBJ_LOOP(i, MAX_PLAYERS) {
				if (!gPlayers[i]->IsValid()) {
					continue;
				}
				gPlayers[i]->Playing = 0;
			}
		}
	}

	return(0);
}

int 
Player::Explode(void)
{
	/* Create some shrapnel */
	int newsprite, xVel, yVel, rx;

	/* Don't explode while already exploding. :)  (DeathMatch) */
	if ( Exploding || !Alive() )
		return(0);

	/* Type 1 shrapnel */
	rx = (SCALE_FACTOR);
	xVel = yVel = 0;

	while (xVel == 0)
		xVel = FastRandom(rx / 2) + SCALE_FACTOR;
	while (yVel == 0)
		yVel = FastRandom(rx) - (rx / 2);
	if (yVel > 0)
		yVel += SCALE_FACTOR;
	else
		yVel -= SCALE_FACTOR;

	newsprite = gNumSprites;
	gSprites[newsprite]=new Shrapnel(x, y, xVel, yVel, gShrapnel1[Index]);

	/* Type 2 shrapnel */
	rx = (SCALE_FACTOR);
	xVel = yVel = 0;

	while (xVel == 0)
		xVel = FastRandom(rx / 2) + SCALE_FACTOR;
	xVel *= -1;
	while (yVel == 0)
		yVel = FastRandom(rx) - (rx / 2);
	if (yVel > 0)
		yVel += SCALE_FACTOR;
	else
		yVel -= SCALE_FACTOR;
	
	newsprite = gNumSprites;
	gSprites[newsprite]=new Shrapnel(x, y, xVel, yVel, gShrapnel2[Index]);

	/* We may lose our special abilities */
	if ( (special & LUCKY_IRISH) && (FastRandom(LUCK_ODDS) == 0) )
		special &= ~LUCKY_IRISH;
	else
		special = 0;

	/* Finish our explosion */
	Exploding = 1;
	Thrusting = 0;
	Shooting = 0;
	ShieldOn = 0;
	solid = 0;
	shootable = 0;
	phase = 0;
	nextphase = 0;
	phasetime = 2;
	xvec = yvec = 0;
	Set_Blit(gShipExplosion);
	Set_TTL(myblit->numFrames*phasetime);
	ExplodeSound();
	return(0);
}

Shot *
Player::ShotHit(Rect *hitRect)
{
	int i;
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
int 
Player::Move(int Freeze)
{
	int i;

	/* Move and time out old shots */
#ifdef SERIOUS_DEBUG
printf("Shots(%d): ", numshots);
#endif
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
#ifdef SERIOUS_DEBUG
printf("  %d = (%d,%d)", i, shots[i]->x, shots[i]->y);
#endif
	}
#ifdef SERIOUS_DEBUG
printf("\n");
#endif

	/* Decrement the Bonus and NoThrust time */
	if ( Bonus && CutBonus-- == 0 ) {
		Bonus -= 10;
		CutBonus = BONUS_DELAY;
	}
	if ( NoThrust )
		--NoThrust;

	/* Check to see if we are dead... */
	if ( Dead ) {
		if ( --Dead == 0 ) {  // New Chance at Life!
			if ( NewShip() < 0 ) {
				/* Game Over */
				Dead = DEAD_DELAY;
				Playing = 0;
			}
		}
		return(0);
	}

	/* Update our status... :-) */
	if ( Alive() && ! Exploding ) {
		/* Airbrakes slow us down. :) */
		if ( special & AIR_BRAKES ) {
			if ( yvec > 0 )
				--yvec;
			else if ( yvec < 0 )
				++yvec;

			if ( xvec > 0 )
				--xvec;
			else if ( xvec < 0 )
				++xvec;
		}

		/* Thrust speeds us up! :)  */
		if ( Thrusting ) {
			if ( ! WasThrusting ) {
				sound->PlaySound(gThrusterSound,
							1, 3, ThrustCallback);
				WasThrusting = 1;
			}

			/* -- The thrust key is down, increase the thrusters! */
			if ( ! NoThrust ) {
				Accelerate(gVelocityTable[phase].h,
						gVelocityTable[phase].v);
			}
		} else
			WasThrusting = 0;

		/* Shoot baby, shoot. */
		if ( Shooting ) {
			if ( ! WasShooting || (special&MACHINE_GUNS) ) {
				WasShooting = 1;

				/* Make a single bullet */
				MakeShot(0);
				sound->PlaySound(gShotSound, 2);

				if ( special & TRIPLE_FIRE ) {
					/* Followed by two more.. */
					MakeShot(1);
					MakeShot(-1);
				}
			}
		} else
			WasShooting = 0;

		/* We be rotating. :-) */
		if ( Rotating & 0x01 ) {  // Heading right..
			if ( ++phase == myblit->numFrames )
				phase = 0;
		}
		if ( Rotating & 0x10 ) {  // Heading left..
			if ( --phase < 0 )
				phase = (myblit->numFrames-1);
		}

		/* Check the shields */
		if ( AutoShield ) {
			WasShielded = 1;
			--AutoShield;
		} else if ( ShieldOn ) {
			if ( ShieldLevel > 0 ) {
				if ( ! WasShielded ) {
					sound->PlaySound(gShieldOnSound, 1);
					WasShielded = 1;
				}
				--ShieldLevel;
			} else if ( ShieldOn & SHIELD_MANUAL ) {
				sound->PlaySound(gNoShieldSound, 2);
			}
		} else
			WasShielded = 0;
	}
	return(Object::Move(Freeze));
}

Uint8 
Player::EncodeInput(unsigned char which, bool enabled)
{
	Uint8 value = 0;

	assert(Index < 4);
	assert(which < 16);
	value |= (Index << 4);
	value |= which;
	if (enabled) {
		value |= 0x80;
	}
	return value;
}

bool
Player::DecodeInput(Uint8 value, unsigned char &which, bool &enabled)
{
	// Check to see if this input is for us
	if (((value >> 4) & 3) != Index) {
		return false;
	}

	which = (value & 0xf);
	if (value & 0x80) {
		enabled = true;
	} else {
		enabled = false;
	}
	return true;
}

void
Player::HandleKeys(void)
{
	/* Wait for keystrokes and sync() */
	Uint8 *inbuf;
	int len, i;
	unsigned char key;
	bool down;

	if ( (len=GetSyncBuf(&inbuf)) <= 0 )
		return;

	for ( i=0; i<len; ++i ) {
		if (!DecodeInput(inbuf[i], key, down)) {
			continue;
		}

		if (down) {
			if ( ! Alive() || Exploding ) {
				break;
			}
			/* Regular key press handling */
			switch(key) {
				case THRUST_KEY:
					Thrusting = 1;
					break;
				case RIGHT_KEY:
					Rotating |= 0x01;
					break;
				case LEFT_KEY:
					Rotating |= 0x10;
					break;
				case SHIELD_KEY:
					ShieldOn |= SHIELD_MANUAL;
					break;
				case FIRE_KEY:
					Shooting = 1;
					break;
				default:
					break;
			}
		} else {
			switch(key) {
				case THRUST_KEY:
					Thrusting = 0;
					if ( sound->Playing(gThrusterSound) )
						sound->HaltSound(3);
					break;
				case RIGHT_KEY:
					Rotating &= ~0x01;
					break;
				case LEFT_KEY:
					Rotating &= ~0x10;
					break;
				case SHIELD_KEY:
					ShieldOn &= ~SHIELD_MANUAL;
					break;
				case FIRE_KEY:
					Shooting = 0;
					break;
				default:
					break;
			}
		}
	}
}

void 
Player::BlitSprite(void)
{
	int i;

	if ( ! Alive() )
		return;

	/* Draw the new shots */
	OBJ_LOOP(i, numshots) {
		RenderSprite(gPlayerShot, shots[i]->x, shots[i]->y, SHOT_SIZE, SHOT_SIZE);
	}
	/* Draw the shield, if necessary */
	if ( ! gPaused && (AutoShield || (ShieldOn && (ShieldLevel > 0))) ) {
		RenderSprite(gShieldBlit->sprite[Sphase], x, y, SHIELD_SIZE, SHIELD_SIZE);
		Sphase = !Sphase;
	}
	/* Draw the thrust, if necessary */
	if ( ! gPaused && Thrusting && ! NoThrust ) {
		int thrust_x, thrust_y;
		thrust_x = x + gThrustOrigins[phase].h;
		thrust_y = y + gThrustOrigins[phase].v;
		RenderSprite(ThrustBlit->sprite[phase], thrust_x, thrust_y, THRUST_SIZE, THRUST_SIZE);
		if ( ThrustBlit == gThrust1 )
			ThrustBlit = gThrust2;
		else
			ThrustBlit = gThrust1;
	}

	/* Draw our ship */
	if (Ghost) {
		SDL_SetTextureAlphaMod(myblit->sprite[phase]->Texture(), 0x80);
	}

	Object::BlitSprite();

	if (Ghost) {
		SDL_SetTextureAlphaMod(myblit->sprite[phase]->Texture(), 0xFF);
	}
}
void 
Player::HitSound(void)
{
	sound->PlaySound(gSteelHit, 3);
}
void 
Player::ExplodeSound(void)
{
	sound->PlaySound(gShipHitSound, 3);
}

void
Player::SetKidShield(bool enabled)
{
	if (enabled) {
		ShieldOn |= SHIELD_KIDS;
	} else {
		ShieldOn &= ~SHIELD_KIDS;
	}
}

void
Player::SetControlType(Uint8 controlType)
{
	if (controlType == CONTROL_NONE) {
		Valid = 0;
	} else {
		Valid = 1;
	}
	this->controlType = controlType;
}

void
Player::SetControl(unsigned char which, bool enabled)
{
	// Don't spam the network with duplicate state changes
	if (!!(controlState & (1 << which)) == enabled) {
		return;
	}
	if (enabled) {
		controlState |= (1 << which);
	} else {
		controlState &= ~(1 << which);
	}

	QueueInput(EncodeInput(which, enabled));
}

/* Private functions... */

int
Player::MakeShot(int offset)
{
	int shotphase;

	if ( numshots == MAX_SHOTS )
		return(-1);

	/* Handle the velocity */
	if ( (shotphase = phase+offset) < 0 )
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
	if ( special & LONG_RANGE )
		shots[numshots]->ttl = (SHOT_DURATION * 2);
	else
		shots[numshots]->ttl = SHOT_DURATION;
	return(++numshots);
}
void
Player::KillShot(int index)
{
	OBJ_KILL(shots, index, numshots, Shot);
}

/* The Shot sprites for the Shinobi and Player */

UITexture *gPlayerShot;
UITexture *gEnemyShot;

Uint8 gPlayerColors[MAX_PLAYERS][3] = {
	{ 0x00, 0x00, 0xFF },		/* Player 1 */
	{ 0xFF, 0x99, 0xFF },		/* Player 2 */
	{ 0x33, 0x99, 0x00 },		/* Player 3 */
};

/* The players!! */
Player *gPlayers[MAX_PLAYERS];

/* Initialize the player sprites */
int InitPlayerSprites(void)
{
	int index;

	OBJ_LOOP(index, MAX_PLAYERS)
		gPlayers[index] = new Player(index);
	return(0);
}

/* Get the player for a particular control type */
Player *GetControlPlayer(Uint8 controlType)
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (gPlayers[i]->GetControlType() & controlType) {
			return gPlayers[i];
		}
	}
	return NULL;
}

/* Function to switch the displayed player */
void RotatePlayerView()
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if ( ++gDisplayed == MAX_PLAYERS )
			gDisplayed = 0;

		if (gPlayers[gDisplayed]->IsValid())
			break;
	}
}
