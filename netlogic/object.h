
#ifndef _object_h
#define _object_h


/* The screen object class */
class Object {

public:
	Object(int X, int Y, int Xvec, int Yvec, Blit *blit, int PhaseTime);
	virtual ~Object();

	/* Flags */
	virtual int IsPlayer(void) {
		return(0);
	}
	virtual int Alive(void) {
		return(1);
	}

	/* Settings */
	void Set_Blit(Blit *blit) {
		myblit = blit;
		if ( myblit->isSmall )
			xsize = ysize = 16;
		else
			xsize = ysize = 32;
	}
	virtual void Set_Points(int points) {
		Points = points;
	}
	virtual void Set_HitPoints(int hp) {
		HitPoints = hp;
	}
	virtual void Set_TTL(int ttl) {
		/* We use ttl+1 because of the way Move() checks TTL */
		TTL = ttl+1;
	}
	virtual void IncrScore(int pts) {
		return;
	}
	virtual void IncrFrags(void) {
		return;
	}

	/* Shot and hit detection */
	virtual Shot *ShotHit(Rect *hitRect) {
		/* This function is called to see if we shot something */
		return(NULL);
	}
	virtual int Collide(Object *object) {
		/* Set up the location rectangles */
		Rect *R1=&HitRect, *R2=&object->HitRect;

		/* No collision if no overlap */
		if ( ! Overlap(R1, R2) )
			return(0);

		/* Check the bitmasks to see if the sprites really intersect */
		int  xoff1, xoff2;
		int  roff;
		unsigned char *mask1, *mask2;
		int checkwidth, checkheight, w;

		/* -- Load the ptrs to the sprite masks */
		mask1 = myblit->mask[phase];
		mask2 = (object->myblit)->mask[object->phase];

		/* See where the sprites are relative to eachother, x-Axis */
		if ( R2->left < R1->left ) {
			/* The second sprite is left of the first one */
			checkwidth = (R2->right-R1->left);
			xoff2 = R1->left-R2->left;
			xoff1 = 0;
		} else {
			/* The first sprite is left of the second one */
			checkwidth = (R1->right-R2->left);
			xoff1 = R2->left-R1->left;
			xoff2 = 0;
		}

		/* See where the sprites are relative to eachother, y-Axis */
		if ( R2->top < R1->top ) {
			/* The second sprite is above of the first one */
			checkheight = (R2->bottom-R1->top);
			mask2 += (R1->top-R2->top)*object->xsize;
		} else {
			/* The first sprite is on top of the second one */
			checkheight = (R1->bottom-R2->top);
			mask1 += (R2->top-R1->top)*xsize;
		}

		/* Do the actual mask hit detection */
		while ( checkheight-- ) {
			for ( roff=0, w=checkwidth; w; --w, ++roff ) {
				if ( mask1[xoff1+roff] && mask2[xoff2+roff] )
					return(1);
			}
			mask1 += xsize;
			mask2 += object->xsize;
		}
		return(0);
	}
	/* Should be called in main loop -- return (-1) if dead */
	virtual int HitBy(Object *ship) {
		Shot *shot;
		while ( shootable && (shot=ship->ShotHit(&HitRect)) ) {
			if ( BeenShot(ship, shot) > 0 )
				return(-1);
		}
		if ( (solid && ship->solid) && 
				Collide(ship) && (BeenRunOver(ship) > 0) )
			return(-1);
		return(0);
	}

	/* We've been shot!  (returns 1 if we are dead) */
	virtual int BeenShot(Object *ship, Shot *shot) {
		if ( (HitPoints -= shot->damage) <= 0 ) {
			ship->IncrScore(Points);
			if ( IsPlayer() )
				ship->IncrFrags();
			return(Explode());
		} else {
			HitSound();
			Accelerate(shot->xvel/2, shot->yvel/2);
		}
		return(0);
	}

	/* We've been run over!  (returns 1 if we are dead) */
	virtual int BeenRunOver(Object *ship) {
		if ( ship->IsPlayer() )
			ship->BeenDamaged(PLAYER_HITS);
		if ( (HitPoints -= 1) <= 0 ) {
			ship->IncrScore(Points);
			return(Explode());
		} else {
			HitSound();
			ship->Accelerate(xvec/2, yvec/2);
		}
		return(0);
	}

	/* We've been globally damaged!  (returns 1 if we are dead) */
	virtual int BeenDamaged(int damage) {
		if ( (HitPoints -= damage) <= 0 ) {
			return(Explode());
		} else
			HitSound();
		return(0);
	}

	/* We expired (returns -1 if we are dead) */
	virtual int BeenTimedOut(void) {
		return(-1);
	}

	/* What happens when we have been shot up or crashed into */
	/* Returns 1 if we die here, instead of go into explosion */
	virtual int Explode(void);

	/* Movement */
	void GetPos(int *X, int *Y) {
		*X = x;
		*Y = y;
	}
	virtual void SetPos(int X, int Y) {
		/* Set new X position */
		x = X;
		if ( x > playground.right )
			x = playground.left + (x-playground.right);
		else if ( x < playground.left )
			x = playground.right - (playground.left-x);

		/* Set new Y position */
		y = Y;
		if ( y > playground.bottom )
			y = playground.top + (y-playground.bottom);
		else if ( y < playground.top )
			y = playground.bottom - (playground.top-y);

		/* Set the new HitRect */
		HitRect.left = myblit->hitRect.left+(x>>SPRITE_PRECISION);
		HitRect.right = myblit->hitRect.right+(x>>SPRITE_PRECISION);
		HitRect.top = myblit->hitRect.top+(y>>SPRITE_PRECISION);
		HitRect.bottom = myblit->hitRect.bottom+(y>>SPRITE_PRECISION);
	}
	virtual void Shake(int shakiness) {
		int Xvec = ((xvec < 0) ? shakiness : -shakiness);
		int Yvec = ((yvec < 0) ? shakiness : -shakiness);
		Accelerate(Xvec, Yvec);
	}
	void Accelerate(int Xvec, int Yvec) {
		if ( Exploding )
			return;
		xvec += Xvec;
		if ( abs(xvec) > VEL_MAX )
			xvec = ((xvec > 0) ? VEL_MAX : -VEL_MAX);
		yvec += Yvec;
		if ( abs(yvec) > VEL_MAX )
			yvec = ((yvec > 0) ? VEL_MAX : -VEL_MAX);
	}
	virtual void Phase(void) {
		if ( phasetime != NO_PHASE_CHANGE ) {
			if ( nextphase++ >= phasetime ) {
				nextphase = 0;
				if ( ++phase >= myblit->numFrames )
					phase = 0;
			}
		}
	}
	/* This function returns 0, or -1 if the sprite died */
	virtual int Move(int Frozen);

	virtual void BlitSprite(void);
	virtual void UnBlitSprite(void);

	/* Sound functions */
	virtual void HitSound(void);
	virtual void ExplodeSound(void);

	/* Player access functions (not used here) */
	virtual void SetSpecial(unsigned char Spec) {
	}
	virtual void IncrShieldLevel(int level) {
	}
	virtual void Multiplier(int multiplier) {
	}
	virtual void IncrBonus(int bonus) {
	}
	virtual void IncrLives(int lives) {
	}

protected:
	int Points;
	int x, y;
	int xvec, yvec;
	int xsize, ysize;
	int solid;
	int shootable;
	int HitPoints;
	int TTL;

	int onscreen;
	int phase;
	int phasetime;
	int nextphase;
	Blit *myblit;
	Rect HitRect;
	Rect playground;
	int Exploding;

	/* See if two rectangles overlap */
	int Overlap(Rect *R1, Rect *R2) {
	/* If the top of R1 is below the bottom of R2, they can't overlap */
		if ( (R1->top > R2->bottom) ||
	/* If the bottom R1 is above the top of R2, they can't overlap */
		     (R1->bottom < R2->top) ||
	/* If the left of R1 is greater than the right of R2, no overlap */
		     (R1->left > R2->right) ||
	/* If the right of R1 is less than the left of R2, no overlap */
		     (R1->right < R2->left) )
			return(0);

		/* They must overlap */
		return(1);
	}
};

/* These macros are useful for manipulating the high speed object arrays
   in use for dynamically keeping track of objects on the screen.
*/
#define OBJ_LOOP(index, range)	for ( index=(range-1); index >= 0; --index )

/* Free a slot in the middle of the array, fill the slot with the object
   at the end of the array, and shrink the size of the array.
*/
#define OBJ_KILL(array, index, numobj, type) \
			if ( index != --numobj ) {		\
				type *tmp = array[index];	\
				array[index] = array[numobj];	\
				array[numobj] = tmp;		\
			}

/* The Objects!! */
extern Object *gSprites[MAX_SPRITES];

#endif /* _object_h */
