
/* Maelstrom version... */
#define	VERSION_STRING		VERSION ".N"

#define	ENEMY_SHOT_DELAY	(10/FRAME_DELAY)

#define	INITIAL_SHIELD		((60/FRAME_DELAY) * 3)
#define	SAFE_TIME		(120/FRAME_DELAY)
#define	MAX_SHIELD		((60/FRAME_DELAY) * 5)
#define DISPLAY_DELAY		(60/FRAME_DELAY)
#define	BONUS_DELAY		(30/FRAME_DELAY)
#define	STAR_DELAY		(30/FRAME_DELAY)
#define	DEAD_DELAY		(3 * (60/FRAME_DELAY))
#define	BOOM_MIN		(20/FRAME_DELAY)

#define PLAYER_HITS		3
#define VAPOROUS		0

#define PLAYER_PTS		1000
#define DEFAULT_POINTS		0

#define	PRIZE_DURATION		(10 * (60/FRAME_DELAY))
#define	MULT_DURATION		(6 * (60/FRAME_DELAY))
#define	BONUS_DURATION		(10 * (60/FRAME_DELAY))
#define	SHOT_DURATION		(1 * (60/FRAME_DELAY))
#define	POINT_DURATION		(2 * (60/FRAME_DELAY))
#define	DAMAGED_DURATION	(10 * (60/FRAME_DELAY))
#define	FREEZE_DURATION		(10 * (60/FRAME_DELAY))
#define	SHAKE_DURATION		(5 * (60/FRAME_DELAY))


/* ----------------------------------------------------------------- */
/* -- Structures and typedefs */

typedef struct {
	int damage;
	int x;
	int y;
	int xvel;
	int yvel;
	int ttl;
	Rect hitRect;
	} Shot;
typedef Shot *ShotPtr;

