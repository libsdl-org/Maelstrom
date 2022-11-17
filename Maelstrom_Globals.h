
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>

#include "mydebug.h"
#include "Mac_Resource.h"
#include "sound.h"
#include "fontserv.h"
#include "framebuf.h"
#ifdef __WIN95__
#ifdef USE_MGL
#include "mgl_framebuf.h"
#else
#include "dx_framebuf.h"
#endif /* USE_MGL */
#else
#include "x11_framebuf.h"
#include "vga_framebuf.h"
#include "dga_framebuf.h"
#endif /* Win95 */

#include "Maelstrom.h"
#include "Maelstrom_Inline.h"
#include "logic.h"
#include "scores.h"
#include "controls.h"

/* Define true and false, if not already defined */
#ifndef true
#define true	1
#endif
#ifndef false
#define false	0
#endif
#ifndef True
#define True	1
#endif
#ifndef False
#define False	0
#endif

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480

// The Font Server :)
extern FontServ *fontserv;

// The Sound Server *grin*
extern SoundClient *sound;

// The SCREEN!! :)
extern FrameBuf *win;

// Functions from main.cc
extern void   PukeUsage(void);
extern void   CleanUp(void);
extern void   Quit(int status);
extern void   ReapChild(int sig);
extern void   Killed(int sig);
extern void   HandleMouse(XEvent *event);
extern void   DrawText(int x, int y, BitMap *text, unsigned long color);
extern void   UnDrawText(int x, int y, BitMap *text);
extern void   DoSplash(void);
extern void   Message(char *message);

// Functions from Utils.cc
extern int    Load_Title(struct Title *title, int title_id);
extern void   Free_Title(struct Title *title);
extern CIcon *GetCIcon(short cicn_id);
extern void   BlitCIcon(int x, int y, CIcon *cicn);
extern void   FreeCIcon(CIcon *cicn);
extern void   SetRect(Rect *R, int left, int top, int right, int bottom);
extern void   OffsetRect(Rect *R, int x, int y);
extern void   InsetRect(Rect *R, int x, int y);

// Functions from shared.cc
extern char  *file2libpath(char *filename);
extern void   select_usleep(unsigned long usec);

// Functions from init.cc
extern int   DoInitializations(int fullscreen, int private_cmap, int dofade);
extern void  SetStar(int which);

// Functions from netscore.cc
extern void	RegisterHighScore(Scores high);
extern int	NetLoadScores(void);

// External variables...
// in main.cc : 
extern Bool	gUpdateBuffer;
extern Bool	gRunning;
extern Bool	gFadeBack;
extern int	gNoDelay;
// in init.cc : 
extern int	gLastHigh;
extern Rect	gScrnRect;
extern Rect	gClipRect;
extern int	gStatusLine;
extern int	gTop, gLeft, gBottom, gRight;
extern MPoint	gShotOrigins[SHIP_FRAMES];
extern MPoint	gThrustOrigins[SHIP_FRAMES];
extern MPoint	gVelocityTable[SHIP_FRAMES];
extern StarPtr	gTheStars[MAX_STARS];
extern unsigned char *gStarColors;
// in controls.cc :
extern Controls	controls;
extern int	gSoundLevel;
extern int	gGammaCorrect;
extern int	gRefreshDisplay;
// int scores.cc :
extern Scores	hScores[];

// -- Variables specific to each game 
// in main.cc : 
extern int	gStartLives;
extern int	gStartLevel;
// in init.cc : 
extern unsigned long	gLastDrawn;
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

extern CIconPtr gAutoFireIcon, gAirBrakesIcon, gMult2Icon, gMult3Icon;
extern CIconPtr gMult4Icon, gMult5Icon, gLuckOfTheIrishIcon, gLongFireIcon;
extern CIconPtr gTripleFireIcon, gKeyIcon, gShieldIcon;
