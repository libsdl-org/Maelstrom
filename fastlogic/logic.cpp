
#include "Maelstrom_Globals.h"
#include "globals.h"

/* Stuff previously in init.cc, now set in InitPlayerSprites() */
Controls pressed  =
	{ 0,        0,        0,       0,       0,       0,      0         };
ULONG	gLastKeyPolled;
int	gShipSprite;
long    gScore;
Sprite *gSprites[MAX_SPRITES];


/* Extra options specific to this logic module */
void LogicUsage(void)
{
}

/* Initialize special logic data */
void InitLogicData(void)
{
}

/* Parse logic-specific command line arguments */
int LogicParseArgs(char ***argvptr, int *argcptr)
{
	return(-1);
}

/* Do final logic initialization */
int InitLogic(void)
{
	int i;

	for ( i=0; i<MAX_SHOTS; ++i ) {
		gTheShots[i] = new Shot;
		gTheShots[i]->shotVis = 0;
		gEnemyShots[i] = new Shot;
		gEnemyShots[i]->shotVis = 0;
	}
	return(0);
}

void HaltLogic(void)
{
}

/* Initialize the sprites (really from the original init.cc) */
int InitPlayerSprites(void)
{
	int index;

	/* Initialize sprite variables */
	gGameOn = 0;
	gShipSprite = 0;
	gNumSprites = 0;
	gLastDrawn = 0L;
	gLastKeyPolled = 0L;

	gScore = 0L;

	/* Create the sprites */
	for (index = 0; index < MAX_SPRITES; index++)
		gSprites[index] = new Sprite;

	return(0);
}

int SpecialKey(KeySym key)
{
	Unused(key);		/* Only if fast logic has no special keys */
	return(-1);
}

void SetControl(unsigned char which, int toggle)
{
	/* Check for various control keys */
	switch (which) {
		case FIRE_KEY:
				pressed.gFireControl=toggle;
				break;
		case RIGHT_KEY:
				pressed.gTurnRControl=toggle;
				break;
		case LEFT_KEY:
				pressed.gTurnLControl=toggle;
				break;
		case SHIELD_KEY:
				pressed.gShieldControl=toggle;
				break;
		case THRUST_KEY:
				pressed.gThrustControl=toggle;
				break;
		case PAUSE_KEY:
				if ( toggle ) {
					/* Special Key */
					if ( pressed.gPauseControl )
						pressed.gPauseControl = 0;
					else
						pressed.gPauseControl = 1;
				}
				break;
		case ABORT_KEY:	if ( toggle )
					pressed.gQuitControl=1;
				break;
		default:
				break;
	}
}

int GetScore(void)
{
	return(gScore);
}

