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

#ifndef _Maelstrom_Globals_h
#define _Maelstrom_Globals_h

#include <SDL3/SDL.h>

#include "../screenlib/SDL_FrameBuf.h"
#include "../screenlib/UIManager.h"

#include "../maclib/Mac_FontServ.h"
#include "../maclib/Mac_Sound.h"
#include "../maclib/Mac_Compat.h"

#include "../utils/prefs.h"

#include "Maelstrom.h"

#include "myerror.h"
#include "fastrand.h"
#include "replay.h"
#include "scores.h"
#include "controls.h"
#include "gameinfo.h"
#include "steam.h"

// Preferences keys
#define PREFERENCES_RESOLUTION "Resolution"
#define PREFERENCES_HANDLE "Handle"
#define PREFERENCES_DEATHMATCH "Network.Deathmatch"
#define PREFERENCES_KIDMODE "Cheat.KidMode"
#define PREFERENCES_CONTINUES "Cheat.Continues"

// The Font Server :)
extern FontServ *fontserv;

// The Preferences
extern Prefs *prefs;

// The Sound Server *grin*
extern Sound *sound;

// The SCREEN!! :)
extern FrameBuf *screen;

// The UI system
extern UIManager *ui;

/* Boolean type */
typedef Uint8 Bool;

// Functions from main.cpp
extern void   PrintUsage(void);
extern int    DrawText(int x, int y, const char *text, MFont *font, Uint8 style,
						Uint8 R, Uint8 G, Uint8 B);
extern void   DelayFrame(void);
extern void   DelayAndDraw(int ticks);

// Functions from init.cpp
extern void  SetStar(int which);

// External variables...
// in main.cpp : 
extern Bool	gUpdateBuffer;
extern Bool	gRunning;

// in init.cpp : 
struct Resolution {
	int w, h;
	char path_suffix[32];
	char file_suffix[32];
	float scale;
};
extern array<Resolution> gResolutions;
extern int	gResolutionIndex;
extern char    *gReplayFile;
extern Sint32	gLastHigh;
extern SDL_Rect gScrnRect;
extern MPoint	gShotOrigins[SHIP_FRAMES];
extern MPoint	gThrustOrigins[SHIP_FRAMES];
extern MPoint	gVelocityTable[SHIP_FRAMES];
extern StarPtr	gTheStars[MAX_STARS];
extern Uint32	gStarColors[];
extern Uint32	gSpriteCRC;

// in game.cpp :
extern int	gDisplayed;
extern int	gGameOn;
extern int	gPaused;
extern int	gBoomDelay;
extern int	gNextBoom;
extern int	gBoomPhase;
extern int	gNumRocks;
extern int	gLastStar;
extern int	gWhenDone;

extern int	gMultiplierShown;
extern int	gPrizeShown;
extern int	gBonusShown;
extern int	gWhenHoming;
extern int	gWhenGrav;
extern int	gWhenDamaged;

extern int	gWhenNova;
extern int	gShakeTime;
extern int	gFreezeTime;

extern int	gWave;
class Object;
extern Object  *gEnemySprite;
extern int	gWhenEnemy;
extern Replay   gReplay;

// in controls.cpp :
extern Controls	controls;
extern PrefsVariable<int> gSoundLevel;
extern PrefsVariable<int> gGammaCorrect;
// int scores.cpp :
extern Scores	hScores[NUM_SCORES];

// -- Variables specific to each game 
// in game.cpp : 
extern GameInfo gGameInfo;
// in init.cpp : 
extern Uint64	gLastDrawn;
extern int	gNumSprites;

// UI panel definitions...
#define PANEL_SPLASH	"splash"
#define PANEL_LOADING	"loading"
#define PANEL_MAIN	"main"
#define PANEL_GAME	"game"
#define PANEL_BONUS	"bonus"
#define PANEL_CONTINUE	"continue"
#define PANEL_GAMEOVER	"gameover"
#define PANEL_ABOUT	"about_story"
#define DIALOG_LOBBY	"lobby"
#define DIALOG_CONTROLS	"controls"
#define DIALOG_ZAP	"zap"
#define DIALOG_DAWN	"dawn"
#define DIALOG_CHEAT	"cheat"
#define DIALOG_FEATURE	"feature"
#define DIALOG_MESSAGE	"message"

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
extern BlitPtr	gPlayerShip[MAX_PLAYERS];
extern BlitPtr	gExplosion, gNova, gEnemyShip, gEnemyShip2;
extern BlitPtr	gMult[], gSteelRoidL;
extern BlitPtr	gSteelRoidR, gPrize, gBonusBlit, gPointBlit;
extern BlitPtr	gVortexBlit, gMineBlitL, gMineBlitR, gShieldBlit;
extern BlitPtr	gThrust1, gThrust2;
extern BlitPtr	gShrapnel1[MAX_PLAYERS], gShrapnel2[MAX_PLAYERS];

/* -- The prize CICN's */

extern UITexture *gAutoFireIcon, *gAirBrakesIcon, *gMult2Icon, *gMult3Icon;
extern UITexture *gMult4Icon, *gMult5Icon, *gLuckOfTheIrishIcon;
extern UITexture *gLongFireIcon, *gTripleFireIcon, *gShieldIcon;

#endif // _Maelstrom_Globals_h
