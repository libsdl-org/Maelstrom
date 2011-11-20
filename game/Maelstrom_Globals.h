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

#ifndef _Maelstrom_Globals_h
#define _Maelstrom_Globals_h

#include <stdlib.h>

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

// Preferences keys
#define PREFERENCES_HANDLE "Handle"
#define PREFERENCES_DEATHMATCH "Network.Deathmatch"

// The Font Server :)
extern FontServ *fontserv;
enum {
	CHICAGO_12,
	GENEVA_9,
	NEWYORK_14,
	NEWYORK_18,
	NUM_FONTS
};
extern MFont *fonts[NUM_FONTS];

// The Preferences
extern Prefs *prefs;

// The Sound Server *grin*
extern Sound *sound;

// The SCREEN!! :)
extern FrameBuf *screen;

// The UI system
extern UIManager *ui;

// The sprites
extern Mac_Resource *spriteres;

/* Boolean type */
typedef Uint8 Bool;
#ifndef true
#define true	1
#endif
#ifndef false
#define false	0
#endif

// Functions from main.cpp
extern void   PrintUsage(void);
extern int    DrawText(int x, int y, const char *text, MFont *font, Uint8 style,
						Uint8 R, Uint8 G, Uint8 B);
extern void   Message(const char *message);
extern void   DelayFrame(void);

// Functions from init.cpp
extern void  SetStar(int which);

// External variables...
// in main.cpp : 
extern Bool	gUpdateBuffer;
extern Bool	gRunning;

// in init.cpp : 
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

// in game.cpp :
extern int	gScore;
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
extern Scores	hScores[];

// -- Variables specific to each game 
// in game.cpp : 
extern GameInfo gGameInfo;
// in init.cpp : 
extern Uint32	gLastDrawn;
extern int	gNumSprites;

// UI panel definitions...
#define PANEL_SPLASH	"splash"
#define PANEL_LOADING	"loading"
#define PANEL_MAIN	"main"
#define PANEL_GAME	"game"
#define PANEL_BONUS	"bonus"
#define PANEL_GAMEOVER	"gameover"
#define PANEL_ABOUT	"about_story"
#define DIALOG_LOBBY	"lobby"
#define DIALOG_CONTROLS	"controls"
#define DIALOG_ZAP	"zap"
#define DIALOG_DAWN	"dawn"
#define DIALOG_CHEAT	"cheat"

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

extern SDL_Texture *gAutoFireIcon, *gAirBrakesIcon, *gMult2Icon, *gMult3Icon;
extern SDL_Texture *gMult4Icon, *gMult5Icon, *gLuckOfTheIrishIcon;
extern SDL_Texture *gLongFireIcon, *gTripleFireIcon, *gShieldIcon;

#endif // _Maelstrom_Globals_h