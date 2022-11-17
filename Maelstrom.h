
/* Definitions useful for our sprite world */
/* Definitions that are particular to a particular logic module are
   in "logicdir/logicdir.h", i.e. netlogic/netlogic.h
   Note especially that this includes timing constants, etc.
*/

/* Maelstrom version... */
#define	VERSION			"2.0.7"
#define	SOUND_DELAY		6
#define	FADE_STEPS		40

#ifndef PI
#define	PI			(22.0 / 7.0)
#endif

#define	SPRITES_NAME	"Maelstrom Sprites"

/* Time in 60'th of second between frames */
#define FRAME_DELAY		2

/* Number of frames between keyboard poll (default of 1 -- poll every frame) */
#define KEYCHK_FRAMES		1

#define MAX_SPRITES		100
#define MAX_SPRITE_FRAMES	60
#define	MAX_STARS		30
#define	SHIP_FRAMES		48
#define	SPRITES_WIDTH		32
#define SPRITE_PRECISION	4	/* internal <--> screen precision */
#define	VEL_FACTOR		4
#define	VEL_MAX			(8<<SPRITE_PRECISION)
#define	SCALE_FACTOR		16
#define	SHAKE_FACTOR		256
#define	MIN_BAD_DISTANCE	64

#define NO_PHASE_CHANGE		-1	/* Sprite doesn't change phase */

#define	MAX_SHOTS		18
#define	SHOT_SIZE		4
#define	SHOT_SCALE_FACTOR	4

#define	STATUS_HEIGHT		14
#define	SHIELD_WIDTH		55
#define	INITIAL_BONUS		2000

#define	ENEMY_HITS		3
#define	HOMING_HITS		9
#define	STEEL_SPECIAL		10
#define DEFAULT_HITS		1

#define	NEW_LIFE		50000
#define	SMALL_ROID_PTS		300
#define	MEDIUM_ROID_PTS		100
#define	BIG_ROID_PTS		50
#define	GRAVITY_PTS		500
#define	HOMING_PTS		700
#define	NOVA_PTS		1000
#define	STEEL_PTS		100
#define	ENEMY_PTS		1000

#define	HOMING_MOVE		6
#define GRAVITY_MOVE		3

#define	BLUE_MOON		50
#define	MOON_FACTOR		4
#define	NUM_PRIZES		8
#define	LUCK_ODDS		3

/* ----------------------------------------------------------------- */
/* -- Structures and typedefs */

typedef struct {
	int h;
	int v;
	} MPoint;

typedef struct {
	int		xCoord;
	int		yCoord;
	unsigned long	color;
	} Star;
typedef Star* StarPtr;

/* Sprite blitting information structure */
typedef	struct {
	int      numFrames;
	int      isSmall;
	Rect     hitRect;
	CSprite *sprite[MAX_SPRITE_FRAMES];
	unsigned char *spriteImages[MAX_SPRITE_FRAMES];
	unsigned char *imageMasks[MAX_SPRITE_FRAMES];
	} Blit;
typedef Blit *BlitPtr;
