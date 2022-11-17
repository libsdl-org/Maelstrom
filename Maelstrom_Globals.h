
#include <stdlib.h>

#include "SDL_FrameBuf.h"
#include "Mac_FontServ.h"
#include "Mac_Sound.h"
#include "Mac_Compat.h"

#include "Maelstrom.h"

#include "myerror.h"
#include "fastrand.h"
#include "logic.h"
#include "scores.h"
#include "controls.h"


// The Font Server :)
extern FontServ *fontserv;

// The Sound Server *grin*
extern Sound *sound;

// The SCREEN!! :)
extern FrameBuf *screen;

/* Boolean type */
typedef Uint8 Bool;
#define true	1
#define false	0

// Functions from main.cc
extern void   PrintUsage(void);
extern int    DrawText(int x, int y, char *text, MFont *font, Uint8 style,
						Uint8 R, Uint8 G, Uint8 B);
extern void   Message(char *message);

// Functions from init.cc
extern void  SetStar(int which);

// Functions from netscore.cc
extern void  RegisterHighScore(Scores high);
extern int   NetLoadScores(void);

// External variables...
// in main.cc : 
extern Bool	gUpdateBuffer;
extern Bool	gRunning;
extern int	gNoDelay;

// in init.cc : 
extern Sint32	gLastHigh;
extern Rect	gScrnRect;
extern SDL_Rect	gClipRect;
extern int	gStatusLine;
extern int	gTop, gLeft, gBottom, gRight;
extern MPoint	gShotOrigins[SHIP_FRAMES];
extern MPoint	gThrustOrigins[SHIP_FRAMES];
extern MPoint	gVelocityTable[SHIP_FRAMES];
extern StarPtr	gTheStars[MAX_STARS];
extern Uint32	gStarColors[];
// in controls.cc :
extern Controls	controls;
extern Uint8	gSoundLevel;
extern Uint8	gGammaCorrect;
// int scores.cc :
extern Scores	hScores[];

// -- Variables specific to each game 
// in main.cc : 
extern int	gStartLives;
extern int	gStartLevel;
// in init.cc : 
extern Uint32	gLastDrawn;
extern int	gNumSprites;
// in scores.cc :
extern Bool	gNetScores;

// Sound resource definitions...
#define gShotSound	100
#define gMultiplier	101
#define gExplosionSound	102
#define gShipHitSound	103
#define gBoom1		104
#define gBoom2		105
#define gMultiplierGone	106
#define gMultShotSound	107
#define gSteelHit	108
#define gBonk		109
#define gRiff		110
#define gPrizeAppears	111
#define gGotPrize	112
#define gGameOver	113
#define gNewLife	114
#define gBonusAppears	115
#define gBonusShot	116
#define gNoBonus	117
#define gGravAppears	118
#define gHomingAppears	119
#define gShieldOnSound	120
#define gNoShieldSound	121
#define gNovaAppears	122
#define gNovaBoom	123
#define gLuckySound	124
#define gDamagedAppears	125
#define gSavedShipSound	126
#define gFunk		127
#define gEnemyAppears	128
#define gPrettyGood	131
#define gThrusterSound	132
#define gEnemyFire	133
#define gFreezeSound	134
#define gIdiotSound	135
#define gPauseSound	136

/* -- The blit'ers we use */
extern BlitPtr	gRock1R, gRock2R, gRock3R, gDamagedShip;
extern BlitPtr	gRock1L, gRock2L, gRock3L, gShipExplosion;
extern BlitPtr	gPlayerShip, gExplosion, gNova, gEnemyShip, gEnemyShip2;
extern BlitPtr	gMult[], gSteelRoidL;
extern BlitPtr	gSteelRoidR, gPrize, gBonusBlit, gPointBlit;
extern BlitPtr	gVortexBlit, gMineBlitL, gMineBlitR, gShieldBlit;
extern BlitPtr	gThrust1, gThrust2, gShrapnel1, gShrapnel2;

/* -- The prize CICN's */

extern SDL_Surface *gAutoFireIcon, *gAirBrakesIcon, *gMult2Icon, *gMult3Icon;
extern SDL_Surface *gMult4Icon, *gMult5Icon, *gLuckOfTheIrishIcon;
extern SDL_Surface *gLongFireIcon, *gTripleFireIcon, *gKeyIcon, *gShieldIcon;
