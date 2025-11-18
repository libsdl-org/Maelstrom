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
	int WasThrusting;
	int Shooting;
	int WasShooting;
	int Rotating;
	unsigned char special;
	int Playing;
	int Dead;
	int Ghost;

	Shot *shots[MAX_SHOTS];
	int nextshot;
	int shotodds;
	int target;
	int numshots;
	Uint32 ship_color;

	Uint8 controlType;
	Uint32 controlState;

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
