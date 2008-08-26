
/* Here we define all of the strange and wonderous objects in the game */


class Prize : public Object {

public:
	Prize(int X, int Y, int xVel, int yVel);
	~Prize() { }

	/* When we are run over, we give prizes! */
	int BeenRunOver(Object *ship) {
		int i;

		if ( ! ship->IsPlayer() || ! ship->Alive() )
			return(0);

		switch (FastRandom(NUM_PRIZES)) {
			case 0:
				/* -- They got machine guns! */
				ship->SetSpecial(MACHINE_GUNS);
				break;
			case 1:
				/* -- They got Air brakes */
				ship->SetSpecial(AIR_BRAKES);
				break;
			case 2:
				/* -- They might get Lucky */
				ship->SetSpecial(LUCKY_IRISH);
				break;
			case 3:
				/* -- They triple fire */
				ship->SetSpecial(TRIPLE_FIRE);
				break;
			case 4:
				/* -- They got long range */
				ship->SetSpecial(LONG_RANGE);
				break;
			case 5:
				/* -- They got more shields */
				ship->IncrShieldLevel((MAX_SHIELD/5)+
						FastRandom(MAX_SHIELD/2));
				break;
			case 6:
				/* -- Put 'em on ICE */
				sound->PlaySound(gFreezeSound, 4);
				gFreezeTime = FREEZE_DURATION;
				break;
			case 7:
				/* Blow up everything */
				sound->PlaySound(gNovaBoom, 5);
				OBJ_LOOP(i, gNumSprites) {
					if ( gSprites[i] == this )
						continue;
					if (gSprites[i]->BeenDamaged(1) < 0) {
						delete gSprites[i];
						gSprites[i] = gSprites[gNumSprites];
					}
				}
				OBJ_LOOP(i, gNumPlayers)
					gPlayers[i]->CutThrust(SHAKE_DURATION);
				gShakeTime = SHAKE_DURATION;
				break;
		}
		sound->PlaySound(gGotPrize, 4);
		return(1);
	}

	int BeenTimedOut(void) {
		/* If we time out, we explode, then die. */
		if ( Exploding )
			return(-1);
		else
			return(Explode());
	}

	void ExplodeSound(void) {
		sound->PlaySound(gIdiotSound, 4);
	}
};


class Multiplier : public Object {

public:
	Multiplier(int X, int Y, int Mult);
	~Multiplier() { }

	int BeenShot(Object *ship, Shot *shot) {
		ship->Multiplier(multiplier);
		sound->PlaySound(gMultShotSound, 4);
		return(1);
	}
	int BeenDamaged(int damage) {
		return(0);
	}
	int BeenTimedOut(void) {
		sound->PlaySound(gMultiplierGone, 4);
		return(-1);
	}
	void Shake(int shakiness) { }

protected:
	int multiplier;
};


class Nova : public Object {

public:
	Nova(int X, int Y);
	~Nova() { }

	int BeenTimedOut(void) {
		if ( ! Exploding ) {
			int i;
			sound->PlaySound(gNovaBoom, 5);
			OBJ_LOOP(i, gNumSprites) {
				if ( gSprites[i] == this )
					continue;
				if (gSprites[i]->BeenDamaged(1) < 0) {
					delete gSprites[i];
					gSprites[i] = gSprites[gNumSprites];
				}
			}
			OBJ_LOOP(i, gNumPlayers)
				gPlayers[i]->CutThrust(SHAKE_DURATION);
			gShakeTime = SHAKE_DURATION;
		}
		return(-1);
	}
	void Shake(int shakiness) { }
};

class Bonus : public Object {

public:
	Bonus(int X, int Y, int xVel, int yVel, int Bonus);
	~Bonus() { }

	int BeenShot(Object *ship, Shot *shot) {

		/* Increment the ship's bonus. :) */
		ship->IncrBonus(bonus);
		sound->PlaySound(gBonusShot, 4);

		/* Display point bonus */
		shootable = 0;
		Set_Blit(gPointBlit);
		phasetime = NO_PHASE_CHANGE;
		phase = (bonus/1000);
		Set_TTL(POINT_DURATION);
		xvec = yvec = 0;

		bonus = 0;
		return(0);
	}
	int BeenDamaged(int damage) {
		return(0);
	}
	int BeenTimedOut(void) {
		if ( bonus )
			sound->PlaySound(gMultiplierGone, 4);
		return(-1);
	}
	void Shake(int shakiness) { }

protected:
	int bonus;
};


class Shrapnel : public Object {

public:
	Shrapnel(int X, int Y, int xVel, int yVel, Blit *blit);
	~Shrapnel() { }

	int BeenDamaged(int damage) {
		return(0);
	}
};


class DamagedShip : public Object {

public:
	DamagedShip(int X, int Y, int xVel, int yVel);
	~DamagedShip() { }

	int BeenRunOver(Object *ship) {
		ship->IncrLives(1);
		sound->PlaySound(gSavedShipSound, 4);
		return(1);
	}

	int BeenTimedOut(void) {
		if ( ! Exploding )
			return(Explode());
		else
			return(-1);
	}

	int Explode(void) {
		/* Create some shrapnel */
		int newsprite, xVel, yVel, rx;

		/* Don't do anything if we're already exploding */
		if ( Exploding ) {
			return(0);
		}

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

		/* Finish our explosion */
		Exploding = 1;
		solid = 0;
		shootable = 0;
		phase = 0;
		nextphase = 0;
		phasetime = 2;
		xvec = yvec = 0;
		myblit = gShipExplosion;
		TTL = (myblit->numFrames*phasetime);
		ExplodeSound();
		return(0);
	}

	void ExplodeSound(void) {
		sound->PlaySound(gShipHitSound, 5);
	}
};


class Gravity : public Object {

public:
	Gravity(int X, int Y);
	~Gravity() { }

	int Move(int Frozen) {
		int i;

		/* Don't gravitize while exploding */
		if ( Exploding )
			return(Object::Move(Frozen));

		/* Warp the courses of the players */
		OBJ_LOOP(i, gNumPlayers) {
			int X, Y, xAccel, yAccel;

			if ( ! gPlayers[i]->Alive() )
				continue;

			/* Gravitize! */
			gPlayers[i]->GetPos(&X, &Y);

			if ( ((X>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) <=
				((x>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) )
				xAccel = GRAVITY_MOVE;
			else
				xAccel = -GRAVITY_MOVE;

			if ( ((Y>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) <=
				((y>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) )
				yAccel = GRAVITY_MOVE;
			else
				yAccel = -GRAVITY_MOVE;

			gPlayers[i]->Accelerate(xAccel, yAccel);
		}

		/* Phase normally */
		return(Object::Move(Frozen));
	}
	void Shake(int shakiness) { }
};


class Homing : public Object {

public:
	Homing(int X, int Y, int xVel, int yVel);
	~Homing() { }

	/* This is duplicated in the Shinobi class */
	virtual int AcquireTarget(void) {
		int i, newtarget=(-1);

		for ( i=0; i<gNumPlayers; ++i ) {
			if ( gPlayers[i]->Alive() )
				break;
		}
		if ( i != gNumPlayers ) {	// Player(s) alive!
			do {
				newtarget = FastRandom(gNumPlayers);
			} while ( ! gPlayers[newtarget]->Alive() );
		}
		return(newtarget);
	}

	int Move(int Frozen) {
		if ( ((target >= 0) && gPlayers[target]->Alive()) ||
					((target=AcquireTarget()) >= 0) ) {
			int X, Y, xAccel=0, yAccel=0;

			gPlayers[target]->GetPos(&X, &Y);
			if ( ((X>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) <=
				((x>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) )
				xAccel -= HOMING_MOVE;
			else 
				xAccel += HOMING_MOVE;
			if ( ((Y>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) <=
				((y>>SPRITE_PRECISION)+(SPRITES_WIDTH/2)) )
				yAccel -= HOMING_MOVE;
			else 
				yAccel += HOMING_MOVE;
			Accelerate(xAccel, yAccel);
		}
		return(Object::Move(Frozen));
	}

protected:
	int target;
};


class SmallRock : public Object {

public:
	SmallRock(int X, int Y, int xVel, int yVel, int phaseFreq);
	~SmallRock() {
		--gNumRocks;
	}

	int Explode() {
		/* Don't do anything if we're already exploding */
		if ( Exploding ) {
			return(0);
		}

		/* Speed things up. :-) */
		if ( --gBoomDelay < BOOM_MIN )
			gBoomDelay = BOOM_MIN;
#ifdef SERIOUS_DEBUG
error("-   Small rock! (%d)\n", gNumRocks);
#endif

		return(Object::Explode());
	}
};
class MediumRock : public Object {

public:
	MediumRock(int X, int Y, int xVel, int yVel, int phaseFreq);
	~MediumRock() {
		--gNumRocks;
	}

	int Explode() {
		int newrocks;
		int  newsprite = gNumSprites;
		
		/* Don't do anything if we're already exploding */
		if ( Exploding ) {
			return(0);
		}

		/* Create 0-3 new rocks */
		newrocks = FastRandom(3);
		do {
			int	xVel, yVel, phaseFreq, rx;

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

			gSprites[newsprite++] =
				new SmallRock(x, y, xVel, yVel, phaseFreq);
		} while ( newrocks-- );

		/* Speed things up. :-) */
		if ( --gBoomDelay < BOOM_MIN )
			gBoomDelay = BOOM_MIN;
#ifdef SERIOUS_DEBUG
error("--  Medium rock! (%d)\n", gNumRocks);
#endif

		return(Object::Explode());
	}
};


class LargeRock : public Object {

public:
	LargeRock(int X, int Y, int xVel, int yVel, int phaseFreq);
	~LargeRock() {
		--gNumRocks;
	}

	int Explode() {
		int newrocks;
		int  newsprite = gNumSprites;
		
		/* Don't do anything if we're already exploding */
		if ( Exploding ) {
			return(0);
		}

		/* Create 0-3 new rocks */
		newrocks = FastRandom(3);
		do {
			int	xVel, yVel, phaseFreq, rx;

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

			gSprites[newsprite++] =
				new MediumRock(x, y, xVel, yVel, phaseFreq);
		} while ( newrocks-- );

		/* Speed things up. :-) */
		if ( --gBoomDelay < BOOM_MIN )
			gBoomDelay = BOOM_MIN;
#ifdef SERIOUS_DEBUG
error("--- Large rock! (%d)\n", gNumRocks);
#endif

		return(Object::Explode());
	}
};


class SteelRoid : public Object {

public:
	SteelRoid(int X, int Y, int xVel, int yVel);
	~SteelRoid() { }

	int Explode(void) {
		int newsprite;

		/* Don't do anything if we're already exploding */
		if ( Exploding ) {
			return(0);
		}

		Set_HitPoints(STEEL_SPECIAL);
		switch (FastRandom(10)) {

			/* Turn into an Asteroid */
			case 0:
				sound->PlaySound(gFunk, 4);
				newsprite = gNumSprites;
				gSprites[newsprite] = new LargeRock(x, y,
							xvec, yvec, phasetime);
				return(1);

			/* Blow up! */
			case 1:
				return(Object::Explode());

			/* Turn into a homing mine */
			case 2:
				sound->PlaySound(gHomingAppears, 4);
				newsprite = gNumSprites;
				gSprites[newsprite] =
					new Homing(x, y, xvec, yvec);
				return(1);

			default:
				break;
		}
		return(0);
	}
};
