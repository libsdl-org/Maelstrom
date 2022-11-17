

/* Maelstrom version... */
#define	VERSION_STRING		VERSION

typedef unsigned long ULONG;

#define KEYBOARD_DELAY		2	// delay in ticks between keyboard polls
#define	INITIAL_SHIELD		(60 * 3)
#define	MAX_SHIELD		(60 * 5)
#define	SAFE_TIME		120L
#define	ENEMY_SHOT_DELAY	10
#define	SHAKE_DURATION		(5 * 60)
#define	BOOM_MIN		20
#define	DEAD_DELAY		(3 * 60)
#define	STAR_DELAY		30
#define	BONUS_DELAY		30
#define	DISPLAY_DELAY		60
#define	FREEZE_DURATION		(10 * 60)

#define	HIDE_SPRITE		5
#define	KILL_SPRITE		7

#define	PRIZE_DURATION		(10 * 60)
#define	MULT_DURATION		(6 * 60)
#define	BONUS_DURATION		(10 * 60)
#define	SHOT_DURATION		(1 * 60)
#define	POINT_DURATION		(2 * 60)
#define	DAMAGED_DURATION	(10 * 60)

#define	PLAYER_SHIP		1
#define	BIG_ASTEROID		2
#define	MEDIUM_ASTEROID		3
#define	SMALL_ASTEROID		4
#define	EXPLOSION		5
#define	MULTIPLIER		6
#define	STEEL_ASTEROID		7
#define	PRIZE			8
#define	BONUS			9
#define	HOMING_PIGEON		10
#define	GRAVITY			11
#define	NOVA			12
#define	DAMAGED_SHIP		13
#define	THRUSTER		14
#define	ENEMY_SHIP		15
#define	SMALL_ENEMY		16
#define	BIG_ENEMY		17

/* -- Flags for the player's ship */

#define	SHIP_ALIVE		1
#define	SHIP_DEAD		0

/* ----------------------------------------------------------------- */
/* -- Structures and typedefs */

typedef struct {
	int     onScreen;
	int	visible;
	long	spriteType;
	long	spriteTag;
	
	int	xVel;
	int	yVel;
	int	xCoord;
	int	yCoord;
	int	oldXCoord;
	int	oldYCoord;
	int	hitFlag;
	
	int	numPhases;
	int	phaseOn;
	int	phaseChange;
	int	changeCount;
	Blit   *theBlit;
	} Sprite;
typedef Sprite* SpritePtr;

typedef struct {
	int	shotVis;
	int	virgin;
	int	xVel;
	int	yVel;
	int	xCoord;
	int	yCoord;
	ULONG	fireTime;
	Rect	hitRect;
	} Shot;
typedef Shot* ShotPtr;

