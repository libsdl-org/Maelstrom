
/* The down low and dirty driver for the game. 
   It handles the frame update and blitting.

*grin*  This used to be 4800 lines of 68K assembly, written by Andrew Welch!

Whew, translated to C++ by me, Sam Lantinga... and I don't even know 68K
assembly! :)

*/
/* 
	We assume in this file that gShipSprite == 0 
If not, modify all of the loops:

	for ( i=(gNumSprites-1); i; --i )

to:

	for ( i=(gNumSprites-1); i>=0; --i )
*/

#include "Maelstrom_Globals.h"
#include "globals.h"
#include "make.h"

// Global variables set in this file...
int	gThrustWasOn=0;
ULONG	gDeadTime;


/* ----------------------------------------------------------------- */
/* -- The thrust sound callback */

static void ThrustCallback(unsigned short theChannel)
{
#ifdef DEBUG
error("Thrust called back on channel %hu\n", theChannel);
#endif
	/* -- Check the control key */
	if ( pressed.gThrustControl )
		sound->PlayChannel(gThrusterSound,1,theChannel,ThrustCallback);
}	/* -- ThrustCallback */

/* ----------------------------------------------------------------- */
/* -- Limit the speed */

static inline void limitSpeed(Sprite *sprite)
{
	if ( sprite->yVel > VEL_MAX )
		sprite->yVel = VEL_MAX;
	else if ( sprite->yVel < -VEL_MAX )
		sprite->yVel = -VEL_MAX;

	if ( sprite->xVel > VEL_MAX )
		sprite->xVel = VEL_MAX;
	else if ( sprite->xVel < -VEL_MAX )
		sprite->xVel = -VEL_MAX;
}

static inline void BlitSprite(int x, int y, Sprite *sprite)
{
	Blit *blit=sprite->theBlit;

	win->Blit_CSprite(x, y, blit->sprite[sprite->phaseOn]);
}

static inline void UnBlitSprite(int x, int y, Sprite *sprite)
{
	Blit *blit=sprite->theBlit;

	win->UnBlit_CSprite(x, y, blit->sprite[sprite->phaseOn]);
}

/* -- See if the Rects intersect */

static inline int checkCollision(Rect *R1, Rect *R2)
{
	/* If the top of R1 is below the bottom of R2, they can't overlap */
	if ( (R1->top > R2->bottom) ||
	/* If the bottom R1 is above the top of R2, they can't overlap */
	     (R1->bottom < R2->top) ||
	/* If the left of R1 is greater than the right of R2, no overlap */
	     (R1->left > R2->right) ||
	/* If the right of R1 is less than the left of R2, no overlap */
	     (R1->right < R2->left) )
		return(0);
	return(1);
}

/* ----------------------------------------------------------------- */
/* -- Create an explosion */

static inline void MakeExplosion(int which)
{
	Sprite *sprite = gSprites[which];

	/* Clear the sprite from the screen */
	UnBlitSprite(sprite->xCoord>>SPRITE_PRECISION,
			sprite->yCoord>>SPRITE_PRECISION, sprite);

	sprite->spriteType = EXPLOSION;
	sprite->phaseOn = 0;
	sprite->changeCount = 0;
	sprite->xVel = 0;
	sprite->yVel = 0;

	if ( (sprite->theBlit == gPlayerShip) || 
	     (sprite->theBlit == gDamagedShip) ) {
		int	x, y, xVel, yVel, rx, index;
		
		sprite->theBlit = gShipExplosion;
		sprite->numPhases = gShipExplosion->numFrames;
		sprite->phaseChange = 2;

		/* -- Make some shrapnel */
		for (index = 0; index < 1; index++) {
			x = sprite->xCoord;
			y = sprite->yCoord;
	
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
		
			(void) NewSprite(gShrapnel1, 2, x, y, 
							xVel, yVel, EXPLOSION);
	
			sprite->phaseOn = 0;
		}

		for (index = 0; index < 1; index++) {
			x = sprite->xCoord;
			y = sprite->yCoord;
	
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
		
			(void) NewSprite(gShrapnel2, 2, x, y, 
							xVel, yVel, EXPLOSION);
	
			sprite->phaseOn = 0;
		}
	} else {
		sprite->phaseChange = 2;
		sprite->theBlit = gExplosion;
		sprite->numPhases = gExplosion->numFrames;
	}

}	/* -- MakeExplosion */

/* ----------------------------------------------------------------- */
/* -- Kill the sprite */

void KillSprite(int which)
{
	int     index;
	Sprite *sprite = gSprites[which];

	/* Clear it from the table */
	if (gEnemySprite != 0)
		if (which < gEnemySprite)
			gEnemySprite--;
	for (index = (which + 1); index < gNumSprites; index++)
		gSprites[index - 1] = gSprites[index];
	gNumSprites--;
	gSprites[gNumSprites] = sprite;
}	/* -- KillSprite */

/* ----------------------------------------------------------------- */
/* -- Waste the sprite */

static inline void wasteIt(ShotPtr shot, Sprite *sprite, int which, int Ship)
{
	int i;
	Sprite *ship;

	if ( Ship == gShipSprite )
		ship = gSprites[gShipSprite];
	else
		ship = gSprites[gEnemySprite];

	switch (sprite->spriteType) {

		/* -- Handle shooting big asteroids */
		case BIG_ASTEROID:
			gScore += BIG_ROID_PTS;
			--gBoomDelay;
			--gNumRocks;
			if ( gBoomDelay < BOOM_MIN )
				gBoomDelay = BOOM_MIN;

			/* -- Make two-three new medium rocks */
			i=FastRandom(3);
			do {
				MakeMediumRock(sprite->xCoord, sprite->yCoord);
			} while ( i-- );

			sound->PlaySound(gExplosionSound, 3, NULL);
			MakeExplosion(which);
			if ( shot )
				shot->shotVis = -1;
			break;

		/* -- Handle shooting medium asteroids */
		case MEDIUM_ASTEROID:
			gScore += MEDIUM_ROID_PTS;
			--gBoomDelay;
			--gNumRocks;
			if ( gBoomDelay < BOOM_MIN )
				gBoomDelay = BOOM_MIN;

			/* -- Make two-three new small rocks */
			i=FastRandom(3);
			do {
				MakeSmallRock(sprite->xCoord, sprite->yCoord);
			} while ( i-- );

			sound->PlaySound(gExplosionSound, 3, NULL);
			MakeExplosion(which);
			if ( shot )
				shot->shotVis = -1;
			break;

		/* -- Handle shooting small asteroids */
		case SMALL_ASTEROID:
			gScore += SMALL_ROID_PTS;
			--gBoomDelay;
			--gNumRocks;
			if ( gBoomDelay < BOOM_MIN )
				gBoomDelay = BOOM_MIN;
			sound->PlaySound(gExplosionSound, 3, NULL);
			MakeExplosion(which);
			if ( shot ) 
				shot->shotVis = -1;
			break;

		/* -- Handle shooting multiplier */
		case MULTIPLIER:
			gMultFactor = sprite->spriteTag;
			sound->PlaySound(gMultShotSound, 4, NULL);
			sprite->visible = HIDE_SPRITE;
			if ( shot ) 
				shot->shotVis = -1;
			break;

		/* -- Handle shooting steel asteroids */
		case STEEL_ASTEROID:
			gScore += STEEL_PTS;
			if ( ++sprite->hitFlag == STEEL_SPECIAL ) {
			/* They wacked it enough times, either do nothing, 
						waste it, or be happy! */
				sprite->hitFlag = 0;
				switch (FastRandom(10)) {
					/* -- Make this a medium asteroid */
					case 0:
						++gNumRocks;
						sound->PlaySound(gFunk,4,NULL);
						sprite->spriteType = MEDIUM_ASTEROID;
						sprite->theBlit = gRock2R;
						goto Clank;

					/* -- Blow up this steel asteroid */
					case 1:
						gScore += (STEEL_PTS*50);
						sound->PlaySound(gExplosionSound, 3, NULL);
						sprite->xVel = 0;
						sprite->yVel = 0;
						MakeExplosion(which);
						goto Clank;

					/* -- Turn it into a mine! */
					case 2:
						sound->PlaySound(gHomingAppears, 4, NULL);
	
						sprite->spriteType = HOMING_PIGEON;
						sprite->theBlit = gMineBlitR;
						goto Clank;
				}
			}
			sound->PlaySound(gSteelHit, 3, NULL);

		Clank:
			if ( shot ) {
				shot->shotVis = -1;
				sprite->xVel += shot->xVel/2;
				sprite->yVel += shot->yVel/2;
				limitSpeed(sprite);
			} else {
				if ( Ship != gShipSprite ||
					ship->spriteTag != SHIP_DEAD ) {
					ship->xVel += sprite->xVel/2;
					ship->yVel += sprite->yVel/2;
				}
				limitSpeed(ship);
			}
			break;

		/* -- Handle blowing up prizes */
		case PRIZE:
			sound->PlaySound(gIdiotSound, 4, NULL);
			MakeExplosion(which);
			if ( shot )
				shot->shotVis = -1;
			break;

		/* -- Handle shooting a bonus */
		case BONUS:
			if ( shot && (sprite->theBlit != gPointBlit) ) {
				UnBlitSprite(sprite->xCoord>>SPRITE_PRECISION,
					sprite->yCoord>>SPRITE_PRECISION,
								sprite);
				gBonus += sprite->spriteTag;
				sound->PlaySound(gBonusShot, 4, NULL);
				sprite->xVel = 0;
				sprite->yVel = 0;
				sprite->phaseChange = NO_PHASE_CHANGE;
				sprite->theBlit = gPointBlit;
				sprite->phaseOn = (sprite->spriteTag/1000);
				gBonusTime = (gOurTicks - POINT_DURATION);
				shot->shotVis = -1;
			}
			break;

		/* -- Handle blowing gravity sprites */
		case GRAVITY:
			sound->PlaySound(gExplosionSound, 3, NULL);
			MakeExplosion(which);
			gScore += GRAVITY_PTS;
			if ( shot )
				shot->shotVis = -1;
			break;

		/* -- Handle blowing homing pigeons */
		case HOMING_PIGEON:
			if ( ++sprite->hitFlag >= HOMING_HITS ) {
				sound->PlaySound(gExplosionSound, 3, NULL);
				MakeExplosion(which);
				gScore += HOMING_PTS;
				if ( shot )
					shot->shotVis = -1;
			} else {
				if ( shot ) {
					sound->PlaySound(gSteelHit, 3, NULL);
					shot->shotVis = -1;
					sprite->xVel += (shot->xVel/2);
					sprite->yVel += (shot->yVel/2);
				} else {
					if ( ship->spriteTag != SHIP_DEAD ) {
						ship->xVel += (sprite->xVel/2);
						ship->yVel += (sprite->yVel/2);
						limitSpeed(ship);
					}
				}
			}
			break;

		/* -- Handle blowing up novas */
		case NOVA:
			sound->PlaySound(gExplosionSound, 3, NULL);
			MakeExplosion(which);
			gScore += NOVA_PTS;
			if ( shot )
				shot->shotVis = -1;
			break;

		/* -- Handle blowing up damaged ships */
		case DAMAGED_SHIP:
			sound->PlaySound(gShipHitSound, 5, NULL);
			MakeExplosion(which);
			if ( shot )
				shot->shotVis = -1;
			break;

		/* -- Handle blowing up enemy ships */
		case ENEMY_SHIP:
			if ( ++sprite->hitFlag >= ENEMY_HITS ) {
				gEnemySprite = 0;
				if ( sprite->spriteTag == SMALL_ENEMY )
					gScore += ENEMY_PTS;
				gScore += ENEMY_PTS;
				sound->PlaySound(gExplosionSound, 3, NULL);
				MakeExplosion(which);
				if ( shot )
					shot->shotVis = -1;
			} else {
				if ( shot ) {
					sound->PlaySound(gBonk, 3, NULL);
					shot->shotVis = -1;
					sprite->xVel += (shot->xVel/2);
					sprite->yVel += (shot->yVel/2);
					limitSpeed(sprite);
					break;
				} else {
					if ( ship->spriteTag != SHIP_DEAD ) {
						ship->xVel += (sprite->xVel/2);
						ship->yVel += (sprite->yVel/2);
					}
				}
			}
			limitSpeed(ship);
			break;
			
	}
}

/* -- Check the masks for a collision; A4 = *sprite1, A6 = *sprite2 */

static inline int maskHitDetect(Sprite *sprite1, Sprite *sprite2)
{
	int  size1, size2;
	int  xoff1, xoff2;
	int  roff1, roff2;
	Rect maskRect1, maskRect2;
	unsigned char *mask1, *mask2;
	int checkwidth, checkheight, w;

	/* Set up the sizes of the sprites */
	if ( (sprite1->theBlit)->isSmall )
		size1 = 16;
	else
		size1 = 32;

	if ( (sprite2->theBlit)->isSmall )
		size2 = 16;
	else
		size2 = 32;

	/* -- Set up the rectangle */
	maskRect1.left = (sprite1->xCoord>>SPRITE_PRECISION);
	maskRect1.right = (sprite1->xCoord>>SPRITE_PRECISION)+size1;
	maskRect1.top = (sprite1->yCoord>>SPRITE_PRECISION);
	maskRect1.bottom = (sprite1->yCoord>>SPRITE_PRECISION)+size1;

	/* -- Load the ptrs to the sprite masks */
	mask1 = (sprite1->theBlit)->imageMasks[sprite1->phaseOn];

	/* -- Set up the rectangle */
	maskRect2.left = (sprite2->xCoord>>SPRITE_PRECISION);
	maskRect2.right = (sprite2->xCoord>>SPRITE_PRECISION)+size2;
	maskRect2.top = (sprite2->yCoord>>SPRITE_PRECISION);
	maskRect2.bottom = (sprite2->yCoord>>SPRITE_PRECISION)+size2;

	/* -- Load the ptrs to the sprite masks */
	mask2 = (sprite2->theBlit)->imageMasks[sprite2->phaseOn];

	/* -- See where the sprites are relative to eachother, x-Axis */
	if ( maskRect2.left < maskRect1.left ) {
		/* -- The second sprite is left of the first one */
		checkwidth = (maskRect2.right-maskRect1.left);
		xoff2 = maskRect1.left-maskRect2.left;
		xoff1 = 0;
	} else {
		/* -- The first sprite is left of the second one */
		checkwidth = (maskRect1.right-maskRect2.left);
		xoff1 = maskRect2.left-maskRect1.left;
		xoff2 = 0;
	}

	/* -- See where the sprites are relative to eachother, y-Axis */
	if ( maskRect2.top < maskRect1.top ) {
		/* -- The second sprite is on top of the second one */
//printf(" 2 above 1, 2bottom = %d, 1top = %d\n",maskRect2.bottom,maskRect1.top);
		checkheight = (maskRect2.bottom-maskRect1.top);
		mask2 += (maskRect1.top-maskRect2.top)*size2;
	} else {
//printf(" 1 above 2, 1bottom = %d, 2top = %d\n",maskRect1.bottom,maskRect2.top);
		/* -- The first sprite is on top of the second one */
		checkheight = (maskRect1.bottom-maskRect2.top);
		mask1 += (maskRect2.top-maskRect1.top)*size1;
	}

	/* -- Do the actual mask hit detection */
	/* Hmmmm, do we see an oversight in here? ... ? */
//printf("Checkheight = %d, Checkwidth = %d\n", checkheight, checkwidth);
	while ( checkheight-- ) {
		for ( roff1=roff2=0, w=checkwidth; w; --w ) {
			if ( mask1[xoff1+(roff1++)] && mask2[xoff2+(roff2++)] )
				return(1);
		}
		mask1 += size1;
		mask2 += size2;
	}
	return(0);
}
	
/* ----------------------------------------------------------------- */
/* -- Check for collisions! */

void doHitDetection(void)
{
	Sprite *ship = gSprites[gShipSprite];
	Rect    rect1, rect2;
	int     i, n;

	if ( ! gGameOn )
		return;
	
	for ( i=(gNumSprites-1); i>=0; --i ) {
		Sprite *sprite = gSprites[i];

		/* -- Set up the sprite rectangle */
		rect1 = (ship->theBlit)->hitRect;
		rect1.left += (ship->xCoord>>SPRITE_PRECISION);
		rect1.right += (ship->xCoord>>SPRITE_PRECISION);
		rect1.top += (ship->yCoord>>SPRITE_PRECISION);
		rect1.bottom += (ship->yCoord>>SPRITE_PRECISION);

/* Why would there be a null sprite? */
		if ( ! sprite )
			continue;
	
		/* -- Set up the sprite rectangle */
		rect2 = (sprite->theBlit)->hitRect;
		rect2.left += (sprite->xCoord>>SPRITE_PRECISION);
		rect2.right += (sprite->xCoord>>SPRITE_PRECISION);
		rect2.top += (sprite->yCoord>>SPRITE_PRECISION);
		rect2.bottom += (sprite->yCoord>>SPRITE_PRECISION);
		
		/* -- If the nova just went off, waste everything! */
		if ( gNovaBlast ) {
			wasteIt(NULL, sprite, i, gShipSprite);
			continue;
		}

		/* -- See if this sprite is a prize */
		if ( sprite->spriteType == PRIZE ) {
			if ( (gOurTicks-gPrizeTime) >= PRIZE_DURATION ) {
				sound->PlaySound(gIdiotSound, 4, NULL);
				MakeExplosion(i);
				continue;
			}
		}

		/* -- See if this sprite is a multiplier */
		if ( sprite->spriteType == MULTIPLIER ) {
			if ( (gOurTicks-gMultTime) < MULT_DURATION )
				goto skipBleach;
			sound->PlaySound(gMultiplierGone, 4, NULL);
			sprite->visible = HIDE_SPRITE;
			continue;
		}

		/* -- See if this sprite is a damaged ship */
		if ( sprite->spriteType == DAMAGED_SHIP ) {
			if ( (gOurTicks-gDamagedTime) >= DAMAGED_DURATION ) {
				sound->PlaySound(gShipHitSound, 5, NULL);
				MakeExplosion(i);
				continue;
			}
		}

		/* -- See if this sprite is a bonus */
		if ( sprite->spriteType == BONUS ) {
			if ( (gOurTicks-gBonusTime) < BONUS_DURATION )
				goto skipBleach;
			if ( sprite->theBlit != gPointBlit )
				sound->PlaySound(gMultiplierGone, 4, NULL);
			sprite->visible = HIDE_SPRITE;
			continue;
		}
	
		/* -- See if this sprite is an explosion */
		if ( sprite->spriteType == EXPLOSION ) {
			if ( sprite->phaseOn != (sprite->numPhases-1) )
				continue;

			/* -- Waste the sprite and exit */
			sprite->visible = HIDE_SPRITE;
			continue;
		}

		/* -- See if this sprite is a nova */
		if ( sprite->spriteType == NOVA ) {
			if ( sprite->phaseOn == (sprite->numPhases-1) ) {
				/* -- Waste the sprite and exit */
				sound->PlaySound(gNovaBoom, 5, NULL);
				gShakeTime = gOurTicks;
				gShaking = 1;
				gNovaFlag = 1;
				sprite->visible = HIDE_SPRITE;
				continue;
			}
		}
	
		/* We don't need to hit-detect ourselves */
		if ( i == gShipSprite )
			continue;
	
		/* -- See if they intersect */
		if ( checkCollision(&rect1, &rect2) &&
		     		(ship->spriteTag != SHIP_DEAD) &&
		     				maskHitDetect(ship, sprite) ) {

			if ( sprite->spriteType == PRIZE ) {
				/* -- They ran over a prize! */
				sound->PlaySound(gGotPrize, 4, NULL);
				sprite->visible = HIDE_SPRITE;

				/* -- See which prize they got */
				switch (FastRandom(NUM_PRIZES)) {
					case 0:
						/* -- They got machine guns! */
						gAutoFire = 1;
						break;
					case 1:
						/* -- They got Air brakes */
						gAirBrakes = 1;
						break;
					case 2:
						/* -- They might get Lucky */
						gLuckOfTheIrish = 1;
						break;
					case 3:
						/* -- They triple fire */
						gTripleFire = 1;
						break;
					case 4:
						/* -- They got long range */
						gLongFire = 1;
						break;
					case 5:
						/* -- They got more shields */
						gShieldLevel +=
							((MAX_SHIELD/5)+
						FastRandom(MAX_SHIELD/2));
						break;
					case 6:
						/* -- Put 'em on ICE */
						sound->PlaySound(gFreezeSound,
								4, NULL);
						gFreezeTime = gOurTicks;
						break;
					case 7:
						/* Blow up everything */
						gNovaFlag = 1;
						break;
				}
				continue;
			}

			/* -- See if they ran over a damaged ship */
			if ( sprite->spriteType == DAMAGED_SHIP ) {
				sound->PlaySound(gSavedShipSound, 4, NULL);
				sprite->visible = HIDE_SPRITE;
				++gLives;
				continue;
			}
	
			/* -- See if they have the shield down! */
			if ( !gShieldOn ) {
				/* -- See if they get lucky */
				if (gLuckOfTheIrish && !FastRandom(LUCK_ODDS))
					sound->PlaySound(gLuckySound, 4, NULL);
				else {
					/* -- The ship has been hit */
					ship->spriteTag = SHIP_DEAD;
					MakeExplosion(gShipSprite);
					gDeadTime = gOurTicks;
					sound->PlaySound(gShipHitSound,5,NULL);
				}
			}

			/* -- The ship ran into something, blow it up! */
			wasteIt(NULL, sprite, i, gShipSprite);
		}

	skipBleach:
		/* -- Now see if the shots have hit anything */
		for ( n=(MAX_SHOTS-1); n>=0; --n ) {
			Shot *shot = gTheShots[n];

			if ( shot->shotVis ) {
				if ( checkCollision(&rect2, &shot->hitRect) )
					wasteIt(shot, sprite, i, gShipSprite);
			}
		}
	}
	if ( gNovaFlag ) {
		gNovaFlag  = 0;
		gNovaBlast = 1;
	} else
		gNovaBlast = 0;
}

/* ----------------------------------------------------------------- */
/* -- Check for collisions! */

static inline void doEnemyDetection(void)
{
	Sprite *enemy = gSprites[gEnemySprite];
	Sprite *ship = gSprites[gShipSprite];
	Rect rect1, rect2;
	int  i, n;

	if ( !gEnemySprite )
		return;

	/* -- Set up the sprite rectangle */
	rect1 = (enemy->theBlit)->hitRect;
	rect1.left += (enemy->xCoord>>SPRITE_PRECISION);
	rect1.right += (enemy->xCoord>>SPRITE_PRECISION);
	rect1.top += (enemy->yCoord>>SPRITE_PRECISION);
	rect1.bottom += (enemy->yCoord>>SPRITE_PRECISION);

	for ( i=(gNumSprites-1); i>=0; --i ) {
		Sprite *sprite = gSprites[i];

		/* Why would there be a null sprite? */
		if ( ! sprite || (i == gEnemySprite) )
			continue;
	
		/* -- Set up the sprite rectangle */
		rect2 = (sprite->theBlit)->hitRect;
		rect2.left += (sprite->xCoord>>SPRITE_PRECISION);
		rect2.right += (sprite->xCoord>>SPRITE_PRECISION);
		rect2.top += (sprite->yCoord>>SPRITE_PRECISION);
		rect2.bottom += (sprite->yCoord>>SPRITE_PRECISION);

		/* -- See if they intersect */
		if ( checkCollision(&rect1, &rect2) &&
		     			maskHitDetect(enemy, sprite) ) {
			if ( i == gShipSprite ) {
				if ( (ship->spriteTag != SHIP_DEAD) && 
								!gShieldOn ) {
					/* -- The ship has been hit */
					ship->spriteTag = SHIP_DEAD;
					MakeExplosion(i);
					gDeadTime = gOurTicks;
					sound->PlaySound(gShipHitSound,5,NULL);
				}
			} else
				wasteIt(NULL, sprite, i, gEnemySprite);
		}

		/* -- Now see if the shots have hit anything */
		for ( n=(MAX_SHOTS-1); n>=0; --n ) {
			Shot *shot = gEnemyShots[n];

			/* -- do a check for this shot */
			if ( shot->shotVis && 
				checkCollision(&rect2, &shot->hitRect) ) {
				shot->shotVis = -1;
				if ( i == gShipSprite ) {
					if ( (ship->spriteTag != SHIP_DEAD) && 
								!gShieldOn ) {
						/* -- The ship has been hit */
						ship->spriteTag = SHIP_DEAD;
						MakeExplosion(i);
						gDeadTime = gOurTicks;
						sound->PlaySound(gShipHitSound,
								5, NULL);
					}
				} else
					wasteIt(shot, sprite, i, gEnemySprite);
			}
		}
	}
}
/* ----------------------------------------------------------------- */
/* -- Make a shot */

static inline int makeShot(Sprite *ship, int offset, Shot **shots)
{
	int i, phase;
	Shot  *shot;

	for ( i=0; i<MAX_SHOTS; ++i ) {
		if ( ! shots[i]->shotVis )
			break;
	}
	if ( i == MAX_SHOTS )
		return(0);

	/* -- Handle the velocity */
	if ( (phase = ship->phaseOn+offset) < 0 )
		phase = SHIP_FRAMES - 1;
	else if ( phase == SHIP_FRAMES )
		phase = 0;
	
	shot = shots[i];
	shot->yVel = (gVelocityTable[phase].v<<SHOT_SCALE_FACTOR);
	shot->xVel = (gVelocityTable[phase].h<<SHOT_SCALE_FACTOR);

	/* -- Handle the position */
	shot->xCoord = ship->xCoord;
	shot->yCoord = ship->yCoord;
	offset = ((SPRITES_WIDTH/2)-2)<<SPRITE_PRECISION;
	shot->xCoord += offset;
	shot->yCoord += offset;

	shot->xVel += ship->xVel;
	shot->xCoord -= ship->xVel;
	shot->yVel += ship->yVel;
	shot->yCoord -= ship->yVel;

	shot->fireTime = gOurTicks;
	shot->shotVis = 1;
	shot->virgin = 1;

	/* -- Setup the hit rectangle */
	offset = (shot->yCoord>>SPRITE_PRECISION);
	shot->hitRect.top = offset;
	shot->hitRect.bottom = offset+SHOT_SIZE;
	offset = (shot->xCoord>>SPRITE_PRECISION);
	shot->hitRect.left = offset;
	shot->hitRect.right = offset+SHOT_SIZE;

	return(1);
}

/* ----------------------------------------------------------------- */
/* -- Handle the enemy ship */

static inline void handleEnemy(Sprite *enemy)
{
	static int gLastPos=0;
	int     DX, DY, coin;
	int     slope, newpos, oldpos;
	Sprite *ship = gSprites[gShipSprite];

	/* -- Calculate the slope to the player's ship */
	DY = (ship->yCoord-enemy->yCoord);
	DX = (ship->xCoord-enemy->xCoord);
	
	slope = (abs(DX)-abs(DY));
	
	/* -- See if we should accelerate */
	/* -- figure out what sector we are in */
	if ( DY < 0 ) {
		if ( DX < 0 ) {
			/* -- We are in sector 4 */
			newpos = 6;
			if ( slope < 0 )
				++newpos;
		} else {
			/* -- We are in sector 1 */
			newpos = 0;
			if ( slope > 0 )
				++newpos;
		}
	} else {
		if ( DX < 0 ) {
			/* -- We are in sector 3 */
			newpos = 4;
			if ( slope > 0 )
				++newpos;
		} else {
			/* -- We are in sector 2 */
			newpos = 2;
			if ( slope < 0 )
				++newpos;
		}
	}

	newpos *= 6;
	newpos += FastRandom(6);

	/* -- Turn to a new one */
	enemy->xVel = 30;

	coin = FastRandom(100);
	if ( coin == 0 ) {
		enemy->yVel = 30;
	} else if ( coin == 1 ) {
		enemy->yVel = -30;
	} else if ( coin < 7 ) {
		enemy->yVel = 0;
	}

	oldpos = enemy->phaseOn;
	/* Linux version change: the small enemy is more accurate! */
	if ( enemy->spriteTag == SMALL_ENEMY ) {
		enemy->phaseOn = (gLastPos+newpos)/2;
		coin = 15;
	} else {
		enemy->phaseOn = newpos;
		coin = 30;
	}
	gLastPos = newpos;

	if ( (FastRandom(coin) == 0) &&
			((gOurTicks-gEnemyFireTime) >= ENEMY_SHOT_DELAY) ) {
		/* -- If we are within range and facing the ship, FIRE! */
		if ( makeShot(enemy, 0, gEnemyShots) )
			sound->PlaySound(gEnemyFire, 2, NULL);
	}
	enemy->phaseOn = oldpos;

	if ( ((enemy->xCoord+26)>>SPRITE_PRECISION) >= gRight ) {
		enemy->visible = HIDE_SPRITE;
		gEnemySprite = 0;
	}
}

/* ----------------------------------------------------------------- */
/* -- Handle the homing pigeon */

static inline void handleHoming(Sprite *homer)
{
	Sprite *ship = gSprites[gShipSprite];

	if ( ((ship->yCoord>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) <=
			((homer->yCoord>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) )
		homer->yVel -= HOMING_MOVE;
	else 
		homer->yVel += HOMING_MOVE;

	if ( ((ship->xCoord>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) <=
			((homer->xCoord>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) )
		homer->xVel -= HOMING_MOVE;
	else 
		homer->xVel += HOMING_MOVE;

	limitSpeed(homer);
}


/* ----------------------------------------------------------------- */
/* -- Handle the gravity sprite */

static inline void handleGravity(Sprite *gravity)
{
	Sprite *ship = gSprites[gShipSprite];

	if ( ship->spriteTag == SHIP_DEAD )
		return;

	if ( ((ship->yCoord>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) <=
		((gravity->yCoord>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) )
		ship->yVel += GRAVITY_MOVE;
	else
		ship->yVel -= GRAVITY_MOVE;

	if ( ((ship->xCoord>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) <=
		((gravity->xCoord>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) )
		ship->xVel += GRAVITY_MOVE;
	else
		ship->xVel -= GRAVITY_MOVE;

	limitSpeed(ship);
}

/* ----------------------------------------------------------------- */
/* -- Move the sprites to their new screen position */

static inline void moveSprites(Sprite *sprite)
{
	if ( ! gGameOn )
		return;

	/* Only the ship gets to move during freeze time */
	if ( gFreezeTime ) {
		switch (sprite->spriteType) {
			case PLAYER_SHIP:
			case THRUSTER:
			case EXPLOSION:
				break;
			default:
				return;
		}
	}
	switch (sprite->spriteType) {
		case ENEMY_SHIP:	handleEnemy(sprite);
					break;
		case HOMING_PIGEON:	handleHoming(sprite);
					break;
		case GRAVITY:		handleGravity(sprite);
					break;
	}

	if ( gShaking ) {
		int shakeV = FastRandom(SHAKE_FACTOR);

		if ( sprite->xVel < 0 )
			sprite->xVel += shakeV;
		else
			sprite->xVel -= shakeV;

		shakeV = FastRandom(SHAKE_FACTOR);

		if ( sprite->yVel < 0 )
			sprite->yVel += shakeV;
		else
			sprite->yVel -= shakeV;

		limitSpeed(sprite);
	}

	/* Update the X coordinate */
	sprite->xCoord += sprite->xVel;
	if ( (sprite->xCoord>>SPRITE_PRECISION) < gLeft )
		sprite->xCoord = (gRight<<SPRITE_PRECISION);
	else if ( (sprite->xCoord>>SPRITE_PRECISION) > gRight )
		sprite->xCoord = (gLeft<<SPRITE_PRECISION);

	/* Update the Y coordinate */
	sprite->yCoord += sprite->yVel;
	if ( (sprite->yCoord>>SPRITE_PRECISION) < gTop )
		sprite->yCoord = (gBottom<<SPRITE_PRECISION);
	else if ( (sprite->yCoord>>SPRITE_PRECISION) > gBottom )
		sprite->yCoord = (gTop<<SPRITE_PRECISION);
}

/* ----------------------------------------------------------------- */
/* -- Allow the user to move his/her ship */

static inline void handleKeys(void)
{
	int        phase;
	static int shiftClick = 0;
	Sprite    *ship = gSprites[gShipSprite];

	if ( ! gGameOn )
		return;
	
	HandleEvents(0);

	/* -- See if they want to abort the game */
	if ( pressed.gQuitControl )
		gGameOn = 0;

	/* If the ship is exploding, don't let user move it! */
	if ( ship->spriteTag == SHIP_DEAD )
		return;

	/* -- Subtract from the ships velocity */
	if ( gAirBrakes ) {
		if ( ship->yVel > 0 )
			--ship->yVel;
		else if ( ship->yVel < 0 )
			++ship->yVel;

		if ( ship->xVel > 0 )
			--ship->xVel;
		else if ( ship->xVel < 0 )
			++ship->xVel;
	}

	/* -- Check the thrust key */
	if ( pressed.gThrustControl ) {
		// -- See if the thruster sound is playing; if not, play it!
		if ( ! gThrustWasOn )
			sound->PlayChannel(gThrusterSound,1,3,ThrustCallback);

		/* -- The thrust key is down, increase the thrusters! */
		gThrustOn = 1;
		ship->yVel += gVelocityTable[ship->phaseOn].v;
		ship->xVel += gVelocityTable[ship->phaseOn].h;
		limitSpeed(ship);
	} else {
		if ( gThrustWasOn && sound->IsSoundPlaying(gThrusterSound) )
			sound->HaltChannel(3);
		gThrustOn = 0;
	}

	/* -- Check the fire key */
	if ( pressed.gFireControl ) {
		if ( gAutoFire || !shiftClick ) {
			/* -- Throw the shot sprite on the screen */
			if ( makeShot(ship, 0, gTheShots) ) {
				/* The shift key is down, fire torpedos! */
				sound->PlaySound(gShotSound, 2, NULL);
			}
			/* -- Handle triple fire */
			if ( gTripleFire ) {
				makeShot(ship, 1, gTheShots);
				makeShot(ship, -1, gTheShots);
			}
		}
		++shiftClick;
	} else
		shiftClick = 0;

	/* -- Check the turn control keys */
	if ( pressed.gTurnLControl ) {
		/* -- The turnL key is down, turn left! */
		UnBlitSprite(ship->xCoord>>SPRITE_PRECISION,
					ship->yCoord>>SPRITE_PRECISION, ship);
		if ( (phase=ship->phaseOn) == 0 )
			phase = ship->numPhases;
		ship->phaseOn = --phase;
		BlitSprite(ship->xCoord>>SPRITE_PRECISION,
					ship->yCoord>>SPRITE_PRECISION, ship);
	} else if ( pressed.gTurnRControl ) {
		/* -- The turnR key is down, turn right! */
		UnBlitSprite(ship->xCoord>>SPRITE_PRECISION,
					ship->yCoord>>SPRITE_PRECISION, ship);
		if ( (phase=(ship->phaseOn+1)) == ship->numPhases )
			ship->phaseOn = 0;
		else
			ship->phaseOn = phase;
		BlitSprite(ship->xCoord>>SPRITE_PRECISION,
					ship->yCoord>>SPRITE_PRECISION, ship);
	}

	/* -- Check the shield key */
	if ( pressed.gShieldControl ) {
		/* The space bar is down, make the ship have a shield */
		if ( ! gShieldOn ) {
			sound->PlaySound(gShieldOnSound, 1, NULL);
			gShieldOn = 1;
		}
		if ( gShieldLevel == 0 ) {
			/* -- Ack, they ran out of shields! */
			sound->PlaySound(gNoShieldSound, 2, NULL);
			gShieldOn = 0;
		} else
			--gShieldLevel;
	} else if ( ! gShieldTime )
			gShieldOn = 0;
}

static inline void MoveAndPhase(Sprite *sprite, int which)
{
	/* Erase where the sprite was with the saved bits */
	if ( sprite->onScreen ) {
		UnBlitSprite(sprite->xCoord>>SPRITE_PRECISION,
				sprite->yCoord>>SPRITE_PRECISION, sprite);
	} else
		sprite->onScreen = 1;

	moveSprites(sprite);

	/* The Enemy can go hidden within moveSprites() */
	if ( sprite->visible == HIDE_SPRITE ) {
		KillSprite(which);
		return;
	}

	if ( (sprite->phaseChange != NO_PHASE_CHANGE) &&
	     (++sprite->changeCount >= sprite->phaseChange) ) {
		/* We should change phases, take care of it */
		sprite->changeCount = 0;
		if ( ++sprite->phaseOn >= sprite->numPhases )
			sprite->phaseOn = 0;
	}
	BlitSprite(sprite->xCoord>>SPRITE_PRECISION,
				sprite->yCoord>>SPRITE_PRECISION, sprite);
}

/* ----------------------------------------------------------------- */
/* -- Handle the friendly ship -- hey, that's us! :) */

static inline void MoveAndPhaseShip(void)
{
	static Sprite  thrust;
	static int     ShieldWasOn=0, Sphase=0;
	MPoint        *origins;
	Sprite        *ship = gSprites[gShipSprite];

	/* Erase the shield */
	if ( ShieldWasOn ) {
		win->UnBlit_CSprite(ship->xCoord>>SPRITE_PRECISION,
					ship->yCoord>>SPRITE_PRECISION,
						gShieldBlit->sprite[Sphase]);
		ShieldWasOn = 0;
	}

	/* Erase the thrust */
	if ( gThrustWasOn ) {
		win->UnBlit_CSprite(thrust.xCoord>>SPRITE_PRECISION,
					thrust.yCoord>>SPRITE_PRECISION,
				(thrust.theBlit)->sprite[thrust.phaseOn]);
		gThrustWasOn = 0;
	}

	/* Erase where the sprite was with the saved bits */
	if ( ship->onScreen ) {
		UnBlitSprite(ship->xCoord>>SPRITE_PRECISION,
					ship->yCoord>>SPRITE_PRECISION, ship);
	} else
		ship->onScreen = 1;

	/* Nothing to do if the ship is dead */
	if ( ship->visible == HIDE_SPRITE ) {
		ship->onScreen = 0;
		return;
	}
	/* Just rotate if not playing */
	if ( (ship->spriteType != PLAYER_SHIP) || ! gGameOn ) {
		MoveAndPhase(ship, gShipSprite);
		return;
	}
	moveSprites(ship);

	BlitSprite(ship->xCoord>>SPRITE_PRECISION,
					ship->yCoord>>SPRITE_PRECISION, ship);

	/* Only handle thrust if it's current */
	if ( !gShaking && gThrustOn ) {
		origins = &gThrustOrigins[ship->phaseOn];
		thrust.xCoord = (ship->xCoord + origins->h);
		thrust.yCoord = (ship->yCoord + origins->v);
		thrust.xVel = ship->xVel;
		thrust.yVel = ship->xVel;
		thrust.phaseOn = ship->phaseOn;
		if ( thrust.theBlit == gThrust1 )
			thrust.theBlit = gThrust2;
		else
			thrust.theBlit = gThrust1;

		win->Blit_CSprite(thrust.xCoord>>SPRITE_PRECISION,
					thrust.yCoord>>SPRITE_PRECISION,
				(thrust.theBlit)->sprite[thrust.phaseOn]);
		gThrustWasOn = 1;
	}

	/* Draw the shield, if needed */
	if ( gShieldOn ) {
		if ( Sphase )
			Sphase = 0;
		else
			Sphase = 1;
		win->Blit_CSprite(ship->xCoord>>SPRITE_PRECISION,
					ship->yCoord>>SPRITE_PRECISION,
						gShieldBlit->sprite[Sphase]);
		ShieldWasOn = 1;
	}
}

/* ----------------------------------------------------------------- */
/* -- Composite the next frame of animation */

void CompositeFrame()
{
	if ( gNumSprites == 0 )
		return;

	/* -- Handle keyboard input */
	if ( gGameOn && ((gOurTicks-gLastKeyPolled) >= KEYBOARD_DELAY) )
		handleKeys();

	if ( gGameOn ) {
		/* Handle hit detection */
		doHitDetection();
		doEnemyDetection();

		/* -- Play the boom sounds */
		if ( (Ticks()-gLastBoom) >= gBoomDelay ) {
			if ( gBoomPhase ) {
				sound->PlaySound(gBoom1, 0, NULL);
				gBoomPhase = 0;
			} else {
				sound->PlaySound(gBoom2, 0, NULL);
				gBoomPhase = 1;
			}
			gLastBoom = Ticks();
		}
	}
}	/* -- CompositeFrame */



/* -- Should we draw the shot (is it on-screen?) */

static inline int shouldDrawShot(Shot *shot)
{
	if ( ! shot->shotVis )
		return(0);

	if ( shot->shotVis < 0 )
		shot->shotVis = 0;
	return(1);
}

/* ----------------------------------------------------------------- */
/* -- Move the shots to their new screen position */

static inline void moveShots(int ours, Shot *shot)
{
	ULONG fireTime;
	int  x, y;

	if ( ! shot )
		return;

	/* -- See if this shot has expired */
	fireTime = (gOurTicks - shot->fireTime);
	if ( ours && gLongFire )
		fireTime /= 2;
	if ( fireTime >= SHOT_DURATION ) {
		/* shot's time to live is up, shuffle off thy mortal coil */
		shot->shotVis = 0;
		return;
	}
	
	/* -- Update the x coordinate */
	x = ((shot->xCoord+shot->xVel)>>SPRITE_PRECISION);
	if ( x < gLeft )
		x = gRight;
	else if ( x > gRight )
		x = gLeft;
	shot->xCoord = (x<<SPRITE_PRECISION);

	/* -- Update the y coordinate */
	y = ((shot->yCoord+shot->yVel)>>SPRITE_PRECISION);
	if ( y < gTop )
		y = gBottom;
	else if ( y > gBottom )
		y = gTop;
	shot->yCoord = (y<<SPRITE_PRECISION);
}

/* ----------------------------------------------------------------- */
/* -- Blast a frame to the screen */
ShotPtr	gEnemyShots[MAX_SHOTS];
ShotPtr	gTheShots[MAX_SHOTS];
static unsigned char PlayerShotColors[] = {
	0xF0, 0xCC, 0xCC, 0xF0,
	0xCC, 0x96, 0xC6, 0xCC,
	0xCC, 0xC6, 0xC6, 0xCC,
	0xF0, 0xCC, 0xCC, 0xF0 };
unsigned char *gPlayerShotColors = PlayerShotColors;
static unsigned char EnemyShotColors[] = {
	0xDC, 0xDA, 0xDA, 0xDC,
	0xDA, 0x17, 0x23, 0xDA,
	0xDA, 0x23, 0x23, 0xDA,
	0xDC, 0xDA, 0xDA, 0xDC };
unsigned char *gEnemyShotColors = EnemyShotColors;

void BlastFrame()
{
	static unsigned char shotmask[] = {
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF };
	unsigned char *shotcolors;
	Shot **shots;
	ULONG ticks;
	int  i, ourshot;
	int  x, y;

	/* We only need to act if there's anything there... */
	if ( !gNumSprites )	
		return;

	/* Timing handling -- Delay the FRAME_DELAY */
	win->Flush(1);
	if ( ! gNoDelay ) {
		while ( ((ticks=Ticks())-gLastDrawn) < FRAME_DELAY ) {
//printf("ticks = %d, gLastDrawn = %d, A-B=%d\n", ticks, gLastDrawn, ticks-gLastDrawn);
			select_usleep(1);
		}
		gLastDrawn = ticks;
	}
	gOurTicks += FRAME_DELAY;
		
	/* -- Move the sprites */
	for ( i=(gNumSprites-1); i; --i )
		MoveAndPhase(gSprites[i], i);
	MoveAndPhaseShip();

	/* -- *** Shots handler *** --- */
	for ( ourshot=0; gGameOn&&(ourshot<2); ++ourshot ) {
		if ( ourshot ) {
			shots = gTheShots;
			shotcolors = gPlayerShotColors;
		} else {
			shots = gEnemyShots;
			shotcolors = gEnemyShotColors;
		}

		for ( i=0; i<MAX_SHOTS; ++i ) {
			Shot *shot = shots[i];

			/* -- Handle erasing the shots */
			if ( ! shot->virgin ) {
				if ( shouldDrawShot(shot) ) {
					x = (shot->xCoord>>SPRITE_PRECISION);
					y = (shot->yCoord>>SPRITE_PRECISION);
					/* -- Erase the shot! */
					win->UnClipBlit_Sprite(x, y, SHOT_SIZE, 
							SHOT_SIZE, shotmask);
				}
			} else 
				shot->fireTime = gOurTicks;

			/* -- Move the shots to their new positions */
			moveShots(ourshot, shot);

			/* -- Now draw the shots in their new position */
			if ( shouldDrawShot(shot) ) {
				shot->virgin = 0;

				/* -- Draw the shot! */
				x = (shot->xCoord>>SPRITE_PRECISION);
				y = (shot->yCoord>>SPRITE_PRECISION);
				win->ClipBlit_Sprite(x, y, SHOT_SIZE, 
					SHOT_SIZE, shotcolors, shotmask);

				/* -- Calculate the hit rectangle */
				shot->hitRect.top = y;
				shot->hitRect.bottom = (y + SHOT_SIZE);
				shot->hitRect.left = x;
				shot->hitRect.right = (x + SHOT_SIZE);
			}
		}
	}
}	/* -- BlastFrame */
