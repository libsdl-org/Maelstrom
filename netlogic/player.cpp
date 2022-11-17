
#include "Maelstrom_Globals.h"
#include "netplay.h"
#include "object.h"
#include "player.h"
#include "globals.h"
#include "objects.h"


/* ----------------------------------------------------------------- */
/* -- The thrust sound callback */

static void ThrustCallback(Uint8 theChannel)
{
#ifdef DEBUG
error("Thrust called back on channel %hu\n", theChannel);
#endif
	/* -- Check the control key */
	if ( gPlayers[gOurPlayer]->IsThrusting() )
		sound->PlaySound(gThrusterSound,1,theChannel,ThrustCallback);
}	/* -- ThrustCallback */


/* ----------------------------------------------------------------- */
/* -- The Player class */

Player:: Player(int index) : Object(0, 0, 0, 0, gPlayerShip, NO_PHASE_CHANGE)
{
	int i;

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
	if ( gDeathMatch )
		Lives = 1;
	else
		Lives = lives;
	Score = 0;
	Frags = 0;
	special = 0;
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
		((SCREEN_WIDTH/2-((gNumPlayers/2-Index)*(2*SPRITES_WIDTH)))
							*SCALE_FACTOR),
		((SCREEN_HEIGHT/2)*SCALE_FACTOR)
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
	if ( Lives == 0 )
		return(-1);
	solid = 1;
	shootable = 1;
	Set_Blit(gPlayerShip);
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
	if ( ! gDeathMatch )
		--Lives;
	return(Lives);
}

/* Increment the number of frags, and handle DeathMatch finale */
void
Player::IncrFrags(void)
{
	++Frags;
	if ( gDeathMatch && (Frags >= gDeathMatch) ) {
		/* Game over, we got a stud. :) */
		int i;
		OBJ_LOOP(i, gNumPlayers) {
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
	if ( gDeathMatch && (lives > 0) )
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
Player::BeenRunOver(Object *ship) {
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
		((SCREEN_WIDTH/2-((gNumPlayers/2-Index)*(2*SPRITES_WIDTH)))
							*SCALE_FACTOR),
		((SCREEN_HEIGHT/2)*SCALE_FACTOR)
	);
	if ( gDeathMatch )
		Dead = (DEAD_DELAY/2);
	else
		Dead = DEAD_DELAY;
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
	gSprites[newsprite]=new Shrapnel(x, y, xVel, yVel, gShrapnel1);

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
	gSprites[newsprite]=new Shrapnel(x, y, xVel, yVel, gShrapnel2);

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
			} else {
				sound->PlaySound(gNoShieldSound, 2);
			}
		} else
			WasShielded = 0;
	}
	return(Object::Move(Freeze));
}

void
Player::HandleKeys(void)
{
	/* Wait for keystrokes and sync() */
	unsigned char *inbuf;
	int len, i;

	if ( (len=GetSyncBuf(Index, &inbuf)) <= 0 )
		return;

	for ( i=0; i<len; ++i ) {
		switch(inbuf[i]) {
			case KEY_PRESS:
				/* Only handle Pause and Abort while dead */
				if ( ! Alive() || Exploding ) {
					if ( inbuf[++i] == PAUSE_KEY ) {
						if ( gPaused > 0 ) {
							--gPaused;
						} else {
							sound->PlaySound(gPauseSound, 5);
							++gPaused;
						}
					}
					if ( inbuf[i] == ABORT_KEY )
						gGameOn = 0;
					break;
				}
				/* Regular key press handling */
				switch(inbuf[++i]) {
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
						ShieldOn = 1;
						break;
					case FIRE_KEY:
						Shooting = 1;
						break;
					case PAUSE_KEY:
						if ( gPaused > 0 ) {
							--gPaused;
						} else {
							sound->PlaySound(gPauseSound, 5);
							++gPaused;
						}
						break;
					case ABORT_KEY:
						gGameOn = 0;
						break;
					default:
						break;
				}
				break;

			case KEY_RELEASE:
				switch(inbuf[++i]) {
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
						ShieldOn = 0;
						break;
					case FIRE_KEY:
						Shooting = 0;
						break;
					case ABORT_KEY:
						/* Do nothing on release */;
						break;
					default:
						break;
				}
				break;

			default:
				break;
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
		int X = (shots[i]->x>>SPRITE_PRECISION);
		int Y = (shots[i]->y>>SPRITE_PRECISION);
		screen->QueueBlit(X, Y, gPlayerShot);
	}
	/* Draw the shield, if necessary */
	if ( AutoShield || (ShieldOn && (ShieldLevel > 0)) ) {
		screen->QueueBlit(x>>SPRITE_PRECISION, y>>SPRITE_PRECISION,
						gShieldBlit->sprite[Sphase]);
	}
	/* Draw the thrust, if necessary */
	if ( Thrusting && ! NoThrust ) {
		int thrust_x, thrust_y;
		thrust_x = x + gThrustOrigins[phase].h;
		thrust_y = y + gThrustOrigins[phase].v;
		screen->QueueBlit(thrust_x>>SPRITE_PRECISION,
					thrust_y>>SPRITE_PRECISION,
						ThrustBlit->sprite[phase]);
	}
	
	/* Draw our ship */
	Object::BlitSprite();
}
void 
Player::UnBlitSprite(void)
{
	int i;

	if ( ! Alive() )
		return;

	/* Erase all old shots */
	OBJ_LOOP(i, numshots) {
		int X = (shots[i]->x>>SPRITE_PRECISION);
		int Y = (shots[i]->y>>SPRITE_PRECISION);
		screen->Clear(X, Y, SHOT_SIZE, SHOT_SIZE, DOCLIP);
	}
	/* Erase the thrust, if necessary */
	if ( WasThrusting ) {
		int thrust_x, thrust_y;
		thrust_x = (x + gThrustOrigins[phase].h)>>SPRITE_PRECISION;
		thrust_y = (y + gThrustOrigins[phase].v)>>SPRITE_PRECISION;
		screen->Clear(thrust_x, thrust_y, 16, 16, DOCLIP);
		if ( ThrustBlit == gThrust1 )
			ThrustBlit = gThrust2;
		else
			ThrustBlit = gThrust1;
	}
	if ( WasShielded ) {
		if ( Sphase )
			Sphase = 0;
		else
			Sphase = 1;
	}
	Object::UnBlitSprite();
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
Player::AbortGame(void)
{
	QueueKey(KEY_PRESS, ABORT_KEY);
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

Uint8 gPlayerShotColors[] = {
	0xF0, 0xCC, 0xCC, 0xF0,
	0xCC, 0x96, 0xC6, 0xCC,
	0xCC, 0xC6, 0xC6, 0xCC,
	0xF0, 0xCC, 0xCC, 0xF0
};
SDL_Surface *gPlayerShot;
Uint8 gEnemyShotColors[] = {
	0xDC, 0xDA, 0xDA, 0xDC,
	0xDA, 0x17, 0x23, 0xDA,
	0xDA, 0x23, 0x23, 0xDA,
	0xDC, 0xDA, 0xDA, 0xDC
};
SDL_Surface *gEnemyShot;

Uint8 gPlayerColors[MAX_PLAYERS][3] = {
	{ 0x00, 0x00, 0xF},		/* Player 1 */
	{ 0xFF, 0x99, 0xFF},		/* Player 2 */
	{ 0x33, 0x99, 0x00},		/* Player 3 */
};

/* The players!! */
Player *gPlayers[MAX_PLAYERS];
