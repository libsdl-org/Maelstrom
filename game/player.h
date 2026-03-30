/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "protocol.h"
#include "netplay.h"
#include "object.h"

/* Special features of the player */
#define MACHINE_GUNS	0x01
#define AIR_BRAKES	0x02
#define TRIPLE_FIRE	0x04
#define LONG_RANGE	0x08
#define LUCKY_IRISH	0x80

/* Different shield modes */
#define SHIELD_MANUAL	0x01
#define SHIELD_KIDS	0x02

class Player : public Object {

public:
	Player(int index);
	~Player();

	virtual int IsValid(void) {
		return(Valid);
	}
	virtual int IsPlayer(void) {
		return(1);
	}
	virtual int IsGhost(void) {
		return Ghost;
	}
	virtual int Alive(void) {
		return(!Dead);
	}
	virtual int Kicking(void) {
		return(Playing && !IsGhost());
	}
	virtual void NewGame(int lives);
	        void Continue(int lives);
	virtual void NewWave(void);
	/* NewShip() MUST be called before Move() */
	virtual int NewShip(void);
	virtual int BeenShot(Object *ship, Shot *shot);
	virtual int BeenRunOver(Object *ship);
	virtual int BeenDamaged(int damage);
	virtual int BeenTimedOut(void);
	virtual int Explode(void);
	virtual Shot *ShotHit(Rect *hitRect);
	virtual int Move(int Freeze);
	virtual void HandleKeys(void);
	virtual void BlitSprite(void);

	/* Small access functions */
	virtual Uint32 Color(void) {
		return(ship_color);
	}
	virtual void IncrLives(int lives);
	virtual int GetLives(void) {
		return(Lives);
	}
	virtual void IncrScore(int score) {
		Score += score;
	}
	virtual int GetScore(void) {
		if ( Score < 0 ) {
			return(0);
		} else {
			return(Score);
		}
	}
	virtual void IncrFrags(void);
	virtual int GetFrags(void) {
		return(Frags);
	}
	virtual void Multiplier(int multiplier) {
		BonusMult = multiplier;
	}
	virtual void MultBonus(void) {
		Bonus *= BonusMult;
	}
	virtual void IncrBonus(int bonus) {
		Bonus += bonus;
	}
	virtual int GetBonus(void) {
		return(Bonus);
	}
	virtual int GetBonusMult(void) {
		return(BonusMult);
	}
	virtual void IncrShieldLevel(int level) {
		ShieldLevel += level;
		if ( ShieldLevel > MAX_SHIELD )
			ShieldLevel = MAX_SHIELD;
		NoShieldsThisLevel = false;
	}
	virtual int GetShieldLevel(void) {
		return(ShieldLevel);
	}
	virtual void CutThrust(int duration) {
		NoThrust = duration;
	}
	virtual int IsThrusting(void) {
		return(Thrusting);
	}
	int IsBraking() {
		if ( GetSpecial( AIR_BRAKES ) ) {
			if ( gGameInfo.ControlBrakes() ) {
				return(Braking);
			} else {
				return(1);
			}
		} else {
			return(0);
		}
	}
	int IsManualBraking() {
		if ( GetSpecial(AIR_BRAKES) && gGameInfo.ControlBrakes() ) {
			return(Braking && (xvec || yvec));
		}
		return(0);
	}
	virtual void SetSpecial(unsigned char Spec) {
		special |= Spec;
	}
	virtual int GetSpecial(unsigned char Spec) {
		return(special&Spec);
	}
	virtual void HitSound(void);
	virtual void ExplodeSound(void);

	void SetKidShield(bool enabled);

	void SetControlType(Uint8 controlType);
	Uint8 GetControlType() {
		return controlType;
	}
	void SetControl(unsigned char which, bool enabled);

	bool CanGetSinglePlayerAchievement() {
		return CanGetAchievement() && !gGameInfo.IsMultiplayer();
	}
	bool CanGetMultiPlayerAchievement() {
		return CanGetAchievement() && gGameInfo.IsMultiplayer();
	}
	bool CanGetAchievement();

private:
	int Valid;
	int Index;
	int Lives;
	int Score;
	int Frags;
	int Bonus;
	int BonusMult;
	int CutBonus;
	int ShieldLevel;
	int ShieldOn;
	int AutoShield;
	int WasShielded;
	int Sphase;
	int Thrusting;
	int NoThrust;
	Blit *ThrustBlit;
	int Braking;
	int WasThrustingOrManualBraking;
	int Shooting;
	int WasShooting;
	int Rotating;
	unsigned char special;
	int Playing;
	int Dead;
	int Ghost;
	int LastWaveDied;

	Shot *shots[MAX_SHOTS];
	int numshots;
	Uint32 ship_color;

	Uint8 controlType;
	Uint32 controlState;

	bool NoShieldsThisLevel = false;

	/* Create a new shot */
	int MakeShot(int offset);
	/* Rubout a flying shot */
	void KillShot(int index);

	/* Encode/decode input */
	Uint8 EncodeInput(unsigned char which, bool enabled);
	bool DecodeInput(Uint8 value, unsigned char &which, bool &enabled);
};

#define TheShip gPlayers[gDisplayed]

/* The Players!! */
extern Player *gPlayers[MAX_PLAYERS];
extern Uint8   gPlayerColors[MAX_PLAYERS][3];

/* Their shots! */
extern UITexture *gPlayerShot;
extern UITexture *gEnemyShot;

/* Initialize the player sprites */
int InitPlayerSprites();

/* Get the player for a particular control type */
Player *GetControlPlayer(Uint8 controlType);

/* Function to switch the displayed player */
void RotatePlayerView();

/* Function to unlock a single player achievement */
void UnlockSinglePlayerAchievement(const char *achievement);
