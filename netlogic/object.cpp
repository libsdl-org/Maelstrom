
#include "Maelstrom_Globals.h"
#include "object.h"


/* The screen object class */

Object::Object(int X, int Y, int Xvec, int Yvec, Blit *blit, int PhaseTime)
{
	Points = DEFAULT_POINTS;

	Set_Blit(blit);
	if ( (phasetime=PhaseTime) != NO_PHASE_CHANGE )
		phase = FastRandom(myblit->numFrames);
	else
		phase = 0;
	nextphase = 0;

	playground.left = (gScrnRect.left<<SPRITE_PRECISION);
	playground.right = (gScrnRect.right<<SPRITE_PRECISION);
	playground.top = (gScrnRect.top<<SPRITE_PRECISION);
	playground.bottom = (gScrnRect.bottom<<SPRITE_PRECISION);

	SetPos(X, Y);
	xvec = Xvec;
	yvec = Yvec;

	solid = 1;
	shootable = 1;
	HitPoints = DEFAULT_HITS;
	Exploding = 0;
	Set_TTL(-1);
	onscreen = 0;
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
	screen->QueueBlit(x>>SPRITE_PRECISION, y>>SPRITE_PRECISION,
							myblit->sprite[phase]);
	onscreen = 1;
}
void
Object::UnBlitSprite(void)
{
	/* Only unblit if we were onscreen */
	if ( ! onscreen )
		return;

	if ( myblit->isSmall ) {
		screen->Clear(x>>SPRITE_PRECISION, y>>SPRITE_PRECISION, 16, 16,
									DOCLIP);
	} else {
		screen->Clear(x>>SPRITE_PRECISION, y>>SPRITE_PRECISION, 32, 32,
									DOCLIP);
	}
	onscreen = 0;
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
