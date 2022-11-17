
#include "protocol.h"

/* Special features of the player */
#define MACHINE_GUNS	0x01
#define AIR_BRAKES	0x02
#define TRIPLE_FIRE	0x04
#define LONG_RANGE	0x08
#define LUCKY_IRISH	0x80

class Player : public Object {

public:
	Player(int index);
	~Player();

	virtual int IsPlayer(void) {
		return(1);
	}
	virtual int Alive(void) {
		return(!Dead);
	}
	virtual int Kicking(void) {
		return(Playing);
	}
	virtual void NewGame(int lives);
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
	virtual void UnBlitSprite(void);

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
	virtual unsigned int GetScore(void) {
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

	virtual void ShowDot(void) {
		/* Draw our identity dot */
		int X, Y;
		X = (x>>SPRITE_PRECISION)+12;
		Y = (y>>SPRITE_PRECISION)+12;
		if ( (X > gClipRect.x) && (X < (gClipRect.x+gClipRect.w-4)) &&
		     (Y > gClipRect.y) && (Y < (gClipRect.y+gClipRect.h-4)) ) {
			screen->FillRect(X, Y, 4, 4, ship_color);
		}
	}
	virtual void AbortGame(void);

private:
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

	Shot *shots[MAX_SHOTS];
	int nextshot;
	int shotodds;
	int target;
	int numshots;
	Uint32 ship_color;

	struct sockaddr_in *myaddr;

	/* Create a new shot */
	int MakeShot(int offset);
	/* Rubout a flying shot */
	void KillShot(int index);
};

#define OurShip	gPlayers[gOurPlayer]
#define TheShip gPlayers[gDisplayed]

/* The Players!! */
extern Player *gPlayers[MAX_PLAYERS];
extern Uint8   gPlayerColors[MAX_PLAYERS][3];
