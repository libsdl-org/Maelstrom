
/* Here we define all of the strange and wonderous objects in the game */

#include "Maelstrom_Globals.h"
#include "netplay.h"
#include "object.h"
#include "player.h"
#include "globals.h"
#include "objects.h"


Prize::Prize(int X, int Y, int xVel, int yVel) :
			Object(X, Y, xVel, yVel, gPrize, 2)
{
	Set_TTL(PRIZE_DURATION);
	sound->PlaySound(gPrizeAppears, 4);
#ifdef SERIOUS_DEBUG
error("Created a prize!\n");
#endif
}

Multiplier::Multiplier(int X, int Y, int Mult) :
		Object(X, Y, 0, 0, gMult[Mult-2], NO_PHASE_CHANGE)
{
	Set_TTL(MULT_DURATION);
	multiplier = Mult;
	solid = 0;
	sound->PlaySound(gMultiplier, 4);
#ifdef SERIOUS_DEBUG
error("Created a multiplier!\n");
#endif
}

Nova::Nova(int X, int Y) : Object(X, Y, 0, 0, gNova, 4)
{
	Set_TTL(gNova->numFrames*phasetime);
	Set_Points(NOVA_PTS);
	phase = 0;
	sound->PlaySound(gNovaAppears, 4);
#ifdef SERIOUS_DEBUG
error("Created a nova!\n");
#endif
}

Bonus::Bonus(int X, int Y, int xVel, int yVel, int Bonus) :
		Object(X, Y, xVel, yVel, gBonusBlit, 2)
{
	Set_TTL(BONUS_DURATION);
	solid = 0;
	bonus = Bonus;
	sound->PlaySound(gBonusAppears, 4);
#ifdef SERIOUS_DEBUG
error("Created a bonus!\n");
#endif
}

Shrapnel::Shrapnel(int X, int Y, int xVel, int yVel, Blit *blit) :
			Object(X, Y, xVel, yVel, blit, 2)
{
	solid = 0;
	shootable = 0;
	phase = 0;
	TTL = (myblit->numFrames*phasetime);
#ifdef SERIOUS_DEBUG
error("Created a shrapnel!\n");
#endif
}

DamagedShip::DamagedShip(int X, int Y, int xVel, int yVel) :
		Object(X, Y, xVel, yVel, gDamagedShip, 1)
{
	Set_TTL(DAMAGED_DURATION*phasetime);
	sound->PlaySound(gDamagedAppears, 4);
#ifdef SERIOUS_DEBUG
error("Created a damaged ship!\n");
#endif
}

Gravity::Gravity(int X, int Y) : Object(X, Y, 0, 0, gVortexBlit, 2)
{
	Set_Points(GRAVITY_PTS);
	sound->PlaySound(gGravAppears, 4);
#ifdef SERIOUS_DEBUG
error("Created a gravity well!\n");
#endif
}

Homing::Homing(int X, int Y, int xVel, int yVel) :
	Object(X, Y, xVel, yVel, 
		((xVel > 0) ? gMineBlitR : gMineBlitL), 2)
{
	Set_HitPoints(HOMING_HITS);
	Set_Points(HOMING_PTS);
	target=AcquireTarget();
	sound->PlaySound(gHomingAppears, 4);
#ifdef SERIOUS_DEBUG
error("Created a homing mine!\n");
#endif
}

SmallRock::SmallRock(int X, int Y, int xVel, int yVel, int phaseFreq) :
	Object(X, Y, xVel, yVel, 
		((xVel > 0) ? gRock3R : gRock3L), phaseFreq)
{
	Set_Points(SMALL_ROID_PTS);
	++gNumRocks;
#ifdef SERIOUS_DEBUG
error("+   Small rock! (%d)\n", gNumRocks);
#endif
}

MediumRock::MediumRock(int X, int Y, int xVel, int yVel, int phaseFreq) :
	Object(X, Y, xVel, yVel, 
		((xVel > 0) ? gRock2R : gRock2L), phaseFreq)
{
	Set_Points(MEDIUM_ROID_PTS);
	++gNumRocks;
#ifdef SERIOUS_DEBUG
error("++  Medium rock! (%d)\n", gNumRocks);
#endif
}

LargeRock::LargeRock(int X, int Y, int xVel, int yVel, int phaseFreq) :
	Object(X, Y, xVel, yVel, 
		((xVel > 0) ? gRock1R : gRock1L), phaseFreq)
{
	Set_Points(BIG_ROID_PTS);
	++gNumRocks;
#ifdef SERIOUS_DEBUG
error("+++ Large rock! (%d)\n", gNumRocks);
#endif
}

SteelRoid::SteelRoid(int X, int Y, int xVel, int yVel) :
	Object(X, Y, xVel, yVel, 
		((xVel > 0) ? gSteelRoidR : gSteelRoidL), 3)
{
	Set_HitPoints(STEEL_SPECIAL);
	Set_Points(STEEL_PTS);
#ifdef SERIOUS_DEBUG
error("Created a steel asteroid!\n");
#endif
}

