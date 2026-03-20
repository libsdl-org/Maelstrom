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

#ifdef COMPUTE_VELTABLE
#include <math.h>
#endif

#include "Maelstrom_Globals.h"
#include "load.h"
#include "init.h"
#include "game.h"
#include "player.h"
#include "colortable.h"
#include "fastrand.h"
#include "MaelstromUI.h"
#include "../screenlib/UIElement.h"

#include "../utils/loadxml.h"
#include "../utils/files.h"


#define GAME_PREFS_FILE	"Maelstrom_Prefs.txt"

// Global variables set in this file...
Prefs    *prefs = nullptr;
Sound    *sound = nullptr;
FontServ *fontserv = nullptr;
FrameBuf *screen = nullptr;
UIManager *ui = nullptr;

array<Resolution> gResolutions;
int	gResolutionIndex;
char   *gReplayFile = nullptr;
Sint32	gLastHigh;
Uint64	gLastDrawn;
int     gNumSprites;
SDL_Rect gScrnRect;
int	gStatusLine;
MPoint	gShotOrigins[SHIP_FRAMES];
MPoint	gThrustOrigins[SHIP_FRAMES];
MPoint	gVelocityTable[SHIP_FRAMES];
StarPtr	gTheStars[MAX_STARS];
Uint32	gStarColors[20];
Uint32	gSpriteCRC = 0;

/* -- The blit'ers we use */
BlitPtr	gRock1R, gRock2R, gRock3R, gDamagedShip;
BlitPtr	gRock1L, gRock2L, gRock3L, gShipExplosion;
BlitPtr	gPlayerShip[MAX_PLAYERS], gExplosion, gNova, gEnemyShip, gEnemyShip2;
BlitPtr	gMult[4], gSteelRoidL;
BlitPtr	gSteelRoidR, gPrize, gBonusBlit, gPointBlit;
BlitPtr	gVortexBlit, gMineBlitL, gMineBlitR, gShieldBlit;
BlitPtr	gThrust1, gThrust2, gShrapnel1[MAX_PLAYERS], gShrapnel2[MAX_PLAYERS];

/* -- The prize CICN's */

UITexture *gAutoFireIcon, *gAirBrakesIcon, *gMult2Icon, *gMult3Icon;
UITexture *gMult4Icon, *gMult5Icon, *gLuckOfTheIrishIcon, *gLongFireIcon;
UITexture *gTripleFireIcon, *gShieldIcon;

enum LoadingStage
{
	LOAD_STAGE_WAITING,
	LOAD_STAGE_STARTING,
	LOAD_STAGE_BLITS1,
	LOAD_STAGE_BLITS2,
	LOAD_STAGE_BLITS3,
	LOAD_STAGE_BLITS4,
	LOAD_STAGE_BLITS5,
	LOAD_STAGE_BLITS6,
	LOAD_STAGE_BLITS7,
	LOAD_STAGE_BLITS8,
	LOAD_STAGE_BLITS9,
	LOAD_STAGE_BLITS10,
	LOAD_STAGE_BLITS11,
	LOAD_STAGE_BLITS12,
	LOAD_STAGE_BLITS13,
	LOAD_STAGE_BLITS14,
	LOAD_STAGE_BLITS15,
	LOAD_STAGE_BLITS16,
	LOAD_STAGE_BLITS17,
	LOAD_STAGE_BLITS18,
	LOAD_STAGE_BLITS19,
	LOAD_STAGE_BLITS20,
	LOAD_STAGE_BLITS21,
	LOAD_STAGE_BLITS22,
	LOAD_STAGE_BLITS23,
	LOAD_STAGE_BLITS24,
	LOAD_STAGE_BLITS25,
	LOAD_STAGE_SHOTS,
	LOAD_STAGE_SPRITES,
	LOAD_STAGE_FILESYSTEM,
	LOAD_STAGE_COMPLETE
};
static int gLoadingStage = LOAD_STAGE_WAITING;

// Local functions used in this file.
static void DrawLoadBar();
static int InitSprites(void);
static int LoadBlits(void);
static int LoadCICNS(void);
static void BackwardsSprite(BlitPtr *theBlit, BlitPtr oldBlit);
static int LoadLargeSprite(BlitPtr *theBlit, int baseID, int numFrames);
static int LoadSmallSprite(BlitPtr *theBlit, int baseID, int numFrames);

/* ----------------------------------------------------------------- */
/* -- Load the list of supported resolutions and pick the best one */

static bool InitResolutions(int &w, int &h)
{
	char *buffer;
	rapidxml::xml_document<> doc;

	if (!LoadXML("resolutions.xml", buffer, doc)) {
		return false;
	}

	rapidxml::xml_node<> *node = doc.first_node();
	rapidxml::xml_attribute<> *attr;
	Resolution resolution;

	for (node = node->first_node(); node; node = node->next_sibling()) {
		attr = node->first_attribute("w", 0, false);
		if (!attr) {
			error("Resolution missing 'w' attribute in resolutions.xml\n");
			SDL_free(buffer);
			return false;
		}
		resolution.w = SDL_atoi(attr->value());

		attr = node->first_attribute("h", 0, false);
		if (!attr) {
			error("Resolution missing 'h' attribute in resolutions.xml\n");
			SDL_free(buffer);
			return false;
		}
		resolution.h = SDL_atoi(attr->value());

		attr = node->first_attribute("path_suffix", 0, false);
		if (!attr) {
			error("Resolution missing 'path_suffix' attribute in resolutions.xml\n");
			SDL_free(buffer);
			return false;
		}
		SDL_strlcpy(resolution.path_suffix, attr->value(), sizeof(resolution.path_suffix));

		attr = node->first_attribute("file_suffix", 0, false);
		if (!attr) {
			error("Resolution missing 'file_suffix' attribute in resolutions.xml\n");
			SDL_free(buffer);
			return false;
		}
		SDL_strlcpy(resolution.file_suffix, attr->value(), sizeof(resolution.file_suffix));

		attr = node->first_attribute("scale", 0, false);
		if (!attr) {
			error("Resolution missing 'scale' attribute in resolutions.xml\n");
			SDL_free(buffer);
			return false;
		}
		int numerator, denominator;
		SDL_sscanf(attr->value(), "%d/%d", &numerator, &denominator);
		resolution.scale = ((float)numerator/denominator);

		gResolutions.add(resolution);
	}
	SDL_free(buffer);

    w = GAME_WIDTH;
    h = GAME_HEIGHT;
    gResolutionIndex = gResolutions.length()-1;
	return true;
}

/* ----------------------------------------------------------------- */
/* -- Draw a loading status bar */

#define	MAX_BAR	26

static void DrawLoadBar()
{
	static int stage = 1;
	UIPanel *panel;
	UIElement *progress = NULL;
	int fact;
	const int FULL_WIDTH = 196;

	panel = ui->GetCurrentPanel();
	if (panel) {
		progress = panel->GetElement<UIElement>("progress");
	}
	if (progress) {
		fact = (FULL_WIDTH * stage) / MAX_BAR;
		progress->SetWidth(fact);
	}
	++stage;
}	/* -- DrawLoadBar */


/* ----------------------------------------------------------------- */
/* -- Set a star */

void SetStar(int which)
{
	int x = FastRandom(GAME_WIDTH - 2*SPRITES_WIDTH) + SPRITES_WIDTH;
	int y = FastRandom(GAME_HEIGHT - 2*SPRITES_WIDTH) + SPRITES_WIDTH;

	gTheStars[which]->xCoord = x;
	gTheStars[which]->yCoord = y;
	gTheStars[which]->color = gStarColors[FastRandom(20)];
}	/* -- SetStar */

/* ----------------------------------------------------------------- */
/* -- Initialize the stars */

static void InitStars(void)
{
	int index;

	/* Map star pixel colors to new colormap */
	gStarColors[0] = screen->MapRGB(colors[gGammaCorrect][0xEB].r,
					colors[gGammaCorrect][0xEB].g,
					colors[gGammaCorrect][0xEB].b);
	gStarColors[1] = screen->MapRGB(colors[gGammaCorrect][0xEC].r,
					colors[gGammaCorrect][0xEC].g,
					colors[gGammaCorrect][0xEC].b);
	gStarColors[2] = screen->MapRGB(colors[gGammaCorrect][0xED].r,
					colors[gGammaCorrect][0xED].g,
					colors[gGammaCorrect][0xED].b);
	gStarColors[3] = screen->MapRGB(colors[gGammaCorrect][0xEE].r,
					colors[gGammaCorrect][0xEE].g,
					colors[gGammaCorrect][0xEE].b);
	gStarColors[4] = screen->MapRGB(colors[gGammaCorrect][0xEF].r,
					colors[gGammaCorrect][0xEF].g,
					colors[gGammaCorrect][0xEF].b);
	gStarColors[5] = screen->MapRGB(colors[gGammaCorrect][0xD9].r,
					colors[gGammaCorrect][0xD9].g,
					colors[gGammaCorrect][0xD9].b);
	gStarColors[6] = screen->MapRGB(colors[gGammaCorrect][0xDA].r,
					colors[gGammaCorrect][0xDA].g,
					colors[gGammaCorrect][0xDA].b);
	gStarColors[7] = screen->MapRGB(colors[gGammaCorrect][0xDB].r,
					colors[gGammaCorrect][0xDB].g,
					colors[gGammaCorrect][0xDB].b);
	gStarColors[8] = screen->MapRGB(colors[gGammaCorrect][0xDC].r,
					colors[gGammaCorrect][0xDC].g,
					colors[gGammaCorrect][0xDC].b);
	gStarColors[9] = screen->MapRGB(colors[gGammaCorrect][0xDD].r,
					colors[gGammaCorrect][0xDD].g,
					colors[gGammaCorrect][0xDD].b);
	gStarColors[10] = screen->MapRGB(colors[gGammaCorrect][0xE4].r,
					colors[gGammaCorrect][0xE4].g,
					colors[gGammaCorrect][0xE4].b);
	gStarColors[11] = screen->MapRGB(colors[gGammaCorrect][0xE5].r,
					colors[gGammaCorrect][0xE5].g,
					colors[gGammaCorrect][0xE5].b);
	gStarColors[12] = screen->MapRGB(colors[gGammaCorrect][0xE6].r,
					colors[gGammaCorrect][0xE6].g,
					colors[gGammaCorrect][0xE6].b);
	gStarColors[13] = screen->MapRGB(colors[gGammaCorrect][0xE7].r,
					colors[gGammaCorrect][0xE7].g,
					colors[gGammaCorrect][0xE7].b);
	gStarColors[14] = screen->MapRGB(colors[gGammaCorrect][0xE8].r,
					colors[gGammaCorrect][0xE8].g,
					colors[gGammaCorrect][0xE8].b);
	gStarColors[15] = screen->MapRGB(colors[gGammaCorrect][0xF7].r,
					colors[gGammaCorrect][0xF7].g,
					colors[gGammaCorrect][0xF7].b);
	gStarColors[16] = screen->MapRGB(colors[gGammaCorrect][0xF8].r,
					colors[gGammaCorrect][0xF8].g,
					colors[gGammaCorrect][0xF8].b);
	gStarColors[17] = screen->MapRGB(colors[gGammaCorrect][0xF9].r,
					colors[gGammaCorrect][0xF9].g,
					colors[gGammaCorrect][0xF9].b);
	gStarColors[18] = screen->MapRGB(colors[gGammaCorrect][0xFA].r,
					colors[gGammaCorrect][0xFA].g,
					colors[gGammaCorrect][0xFA].b);
	gStarColors[19] = screen->MapRGB(colors[gGammaCorrect][0xFB].r,
					colors[gGammaCorrect][0xFB].g,
					colors[gGammaCorrect][0xFB].b);

	for (index = 0; index < MAX_STARS; index++) {
		gTheStars[index] = new Star;
		gTheStars[index]->color = 0L;
		SetStar(index);
	}
}	/* -- InitStars */


/* ----------------------------------------------------------------- */
/* -- Build the ship's velocity table */

static void InitShots(void)
{
	int xx = 30;

	/* Load the shot images */
	gPlayerShot = Load_Image(screen, "shot_player");
	gEnemyShot = Load_Image(screen, "shot_enemy");

	/* Now setup the shot origin table */

	gShotOrigins[0].h = 15 * SCALE_FACTOR;
	gShotOrigins[0].v = 12 * SCALE_FACTOR;

	gShotOrigins[1].h = 16 * SCALE_FACTOR;
	gShotOrigins[1].v = 12 * SCALE_FACTOR;

	gShotOrigins[2].h = 18 * SCALE_FACTOR;
	gShotOrigins[2].v = 12 * SCALE_FACTOR;

	gShotOrigins[3].h = 21 * SCALE_FACTOR;
	gShotOrigins[3].v = 12 * SCALE_FACTOR;

	gShotOrigins[4].h = xx * SCALE_FACTOR;
	gShotOrigins[4].v = xx * SCALE_FACTOR;

	gShotOrigins[5].h = xx * SCALE_FACTOR;
	gShotOrigins[5].v = xx * SCALE_FACTOR;

	gShotOrigins[6].h = xx * SCALE_FACTOR;
	gShotOrigins[6].v = xx * SCALE_FACTOR;

	gShotOrigins[7].h = xx * SCALE_FACTOR;
	gShotOrigins[7].v = xx * SCALE_FACTOR;

	gShotOrigins[8].h = xx * SCALE_FACTOR;
	gShotOrigins[8].v = xx * SCALE_FACTOR;

	gShotOrigins[9].h = xx * SCALE_FACTOR;
	gShotOrigins[9].v = xx * SCALE_FACTOR;

	gShotOrigins[10].h = xx * SCALE_FACTOR;
	gShotOrigins[10].v = xx * SCALE_FACTOR;

	gShotOrigins[11].h = xx * SCALE_FACTOR;
	gShotOrigins[11].v = xx * SCALE_FACTOR;

	gShotOrigins[12].h = xx * SCALE_FACTOR;
	gShotOrigins[12].v = xx * SCALE_FACTOR;

	gShotOrigins[13].h = xx * SCALE_FACTOR;
	gShotOrigins[13].v = xx * SCALE_FACTOR;

	gShotOrigins[14].h = xx * SCALE_FACTOR;
	gShotOrigins[14].v = xx * SCALE_FACTOR;

	gShotOrigins[15].h = xx * SCALE_FACTOR;
	gShotOrigins[15].v = xx * SCALE_FACTOR;

	gShotOrigins[16].h = xx * SCALE_FACTOR;
	gShotOrigins[16].v = xx * SCALE_FACTOR;

	gShotOrigins[17].h = xx * SCALE_FACTOR;
	gShotOrigins[17].v = xx * SCALE_FACTOR;

	gShotOrigins[18].h = xx * SCALE_FACTOR;
	gShotOrigins[18].v = xx * SCALE_FACTOR;

	gShotOrigins[19].h = xx * SCALE_FACTOR;
	gShotOrigins[19].v = xx * SCALE_FACTOR;

	gShotOrigins[20].h = xx * SCALE_FACTOR;
	gShotOrigins[20].v = xx * SCALE_FACTOR;

	gShotOrigins[21].h = xx * SCALE_FACTOR;
	gShotOrigins[21].v = xx * SCALE_FACTOR;

	gShotOrigins[22].h = xx * SCALE_FACTOR;
	gShotOrigins[22].v = xx * SCALE_FACTOR;

	gShotOrigins[23].h = xx * SCALE_FACTOR;
	gShotOrigins[23].v = xx * SCALE_FACTOR;

	gShotOrigins[24].h = xx * SCALE_FACTOR;
	gShotOrigins[24].v = xx * SCALE_FACTOR;

	gShotOrigins[25].h = xx * SCALE_FACTOR;
	gShotOrigins[25].v = xx * SCALE_FACTOR;

	gShotOrigins[26].h = xx * SCALE_FACTOR;
	gShotOrigins[26].v = xx * SCALE_FACTOR;

	gShotOrigins[27].h = xx * SCALE_FACTOR;
	gShotOrigins[27].v = xx * SCALE_FACTOR;

	gShotOrigins[28].h = xx * SCALE_FACTOR;
	gShotOrigins[28].v = xx * SCALE_FACTOR;

	gShotOrigins[29].h = xx * SCALE_FACTOR;
	gShotOrigins[29].v = xx * SCALE_FACTOR;

	gShotOrigins[30].h = xx * SCALE_FACTOR;
	gShotOrigins[30].v = xx * SCALE_FACTOR;

	gShotOrigins[31].h = xx * SCALE_FACTOR;
	gShotOrigins[31].v = xx * SCALE_FACTOR;

	gShotOrigins[32].h = xx * SCALE_FACTOR;
	gShotOrigins[32].v = xx * SCALE_FACTOR;

	gShotOrigins[33].h = xx * SCALE_FACTOR;
	gShotOrigins[33].v = xx * SCALE_FACTOR;

	gShotOrigins[34].h = xx * SCALE_FACTOR;
	gShotOrigins[34].v = xx * SCALE_FACTOR;

	gShotOrigins[35].h = xx * SCALE_FACTOR;
	gShotOrigins[35].v = xx * SCALE_FACTOR;

	gShotOrigins[36].h = xx * SCALE_FACTOR;
	gShotOrigins[36].v = xx * SCALE_FACTOR;

	gShotOrigins[37].h = xx * SCALE_FACTOR;
	gShotOrigins[37].v = xx * SCALE_FACTOR;

	gShotOrigins[38].h = xx * SCALE_FACTOR;
	gShotOrigins[38].v = xx * SCALE_FACTOR;

	gShotOrigins[39].h = xx * SCALE_FACTOR;
	gShotOrigins[39].v = xx * SCALE_FACTOR;

	gShotOrigins[40].h = xx * SCALE_FACTOR;
	gShotOrigins[40].v = xx * SCALE_FACTOR;

	gShotOrigins[41].h = xx * SCALE_FACTOR;
	gShotOrigins[41].v = xx * SCALE_FACTOR;

	gShotOrigins[42].h = xx * SCALE_FACTOR;
	gShotOrigins[42].v = xx * SCALE_FACTOR;

	gShotOrigins[43].h = xx * SCALE_FACTOR;
	gShotOrigins[43].v = xx * SCALE_FACTOR;

	gShotOrigins[44].h = xx * SCALE_FACTOR;
	gShotOrigins[44].v = xx * SCALE_FACTOR;

	gShotOrigins[45].h = xx * SCALE_FACTOR;
	gShotOrigins[45].v = xx * SCALE_FACTOR;

	gShotOrigins[46].h = xx * SCALE_FACTOR;
	gShotOrigins[46].v = xx * SCALE_FACTOR;

	gShotOrigins[47].h = xx * SCALE_FACTOR;
	gShotOrigins[47].v = xx * SCALE_FACTOR;

/* -- Now setup the thruster origin table */

	gThrustOrigins[0].h = 8 * SCALE_FACTOR;
	gThrustOrigins[0].v = 22 * SCALE_FACTOR;

	gThrustOrigins[1].h = 6 * SCALE_FACTOR;
	gThrustOrigins[1].v = 22 * SCALE_FACTOR;

	gThrustOrigins[2].h = 4 * SCALE_FACTOR;
	gThrustOrigins[2].v = 21 * SCALE_FACTOR;

	gThrustOrigins[3].h = 1 * SCALE_FACTOR;
	gThrustOrigins[3].v = 20 * SCALE_FACTOR;

	gThrustOrigins[4].h = 0 * SCALE_FACTOR;
	gThrustOrigins[4].v = 19 * SCALE_FACTOR;

	gThrustOrigins[5].h = -1 * SCALE_FACTOR;
	gThrustOrigins[5].v = 19 * SCALE_FACTOR;

	gThrustOrigins[6].h = -3 * SCALE_FACTOR;
	gThrustOrigins[6].v = 16 * SCALE_FACTOR;

	gThrustOrigins[7].h = -5 * SCALE_FACTOR;
	gThrustOrigins[7].v = 15 * SCALE_FACTOR;

	gThrustOrigins[8].h = -6 * SCALE_FACTOR;
	gThrustOrigins[8].v = 13 * SCALE_FACTOR;

	gThrustOrigins[9].h = -9 * SCALE_FACTOR;
	gThrustOrigins[9].v = 11 * SCALE_FACTOR;

	gThrustOrigins[10].h = -10 * SCALE_FACTOR;
	gThrustOrigins[10].v = 10 * SCALE_FACTOR;

	gThrustOrigins[11].h = -11 * SCALE_FACTOR;
	gThrustOrigins[11].v = 7 * SCALE_FACTOR;

	gThrustOrigins[12].h = -9 * SCALE_FACTOR;
	gThrustOrigins[12].v = 7 * SCALE_FACTOR;

	gThrustOrigins[13].h = -9 * SCALE_FACTOR;
	gThrustOrigins[13].v = 4 * SCALE_FACTOR;

	gThrustOrigins[14].h = -7 * SCALE_FACTOR;
	gThrustOrigins[14].v = 2 * SCALE_FACTOR;

	gThrustOrigins[15].h = -6 * SCALE_FACTOR;
	gThrustOrigins[15].v = 0 * SCALE_FACTOR;

	gThrustOrigins[16].h = -9 * SCALE_FACTOR;
	gThrustOrigins[16].v = 1 * SCALE_FACTOR;

	gThrustOrigins[17].h = -3 * SCALE_FACTOR;
	gThrustOrigins[17].v = -3 * SCALE_FACTOR;

	gThrustOrigins[18].h = -1 * SCALE_FACTOR;
	gThrustOrigins[18].v = -2 * SCALE_FACTOR;

	gThrustOrigins[19].h = 0 * SCALE_FACTOR;
	gThrustOrigins[19].v = -4 * SCALE_FACTOR;

	gThrustOrigins[20].h = 4 * SCALE_FACTOR;
	gThrustOrigins[20].v = -6 * SCALE_FACTOR;

	gThrustOrigins[21].h = 5 * SCALE_FACTOR;
	gThrustOrigins[21].v = -8 * SCALE_FACTOR;

	gThrustOrigins[22].h = 5 * SCALE_FACTOR;
	gThrustOrigins[22].v = -6 * SCALE_FACTOR;

	gThrustOrigins[23].h = 8 * SCALE_FACTOR;
	gThrustOrigins[23].v = -7 * SCALE_FACTOR;

	gThrustOrigins[24].h = 9 * SCALE_FACTOR;
	gThrustOrigins[24].v = -7 * SCALE_FACTOR;

	gThrustOrigins[25].h = 12 * SCALE_FACTOR;
	gThrustOrigins[25].v = -6 * SCALE_FACTOR;

	gThrustOrigins[26].h = 13 * SCALE_FACTOR;
	gThrustOrigins[26].v = -6 * SCALE_FACTOR;

	gThrustOrigins[27].h = 15 * SCALE_FACTOR;
	gThrustOrigins[27].v = -7 * SCALE_FACTOR;

	gThrustOrigins[28].h = 17 * SCALE_FACTOR;
	gThrustOrigins[28].v = -6 * SCALE_FACTOR;

	gThrustOrigins[29].h = 18 * SCALE_FACTOR;
	gThrustOrigins[29].v = -4 * SCALE_FACTOR;

	gThrustOrigins[30].h = 20 * SCALE_FACTOR;
	gThrustOrigins[30].v = -2 * SCALE_FACTOR;

	gThrustOrigins[31].h = 19 * SCALE_FACTOR;
	gThrustOrigins[31].v = -1 * SCALE_FACTOR;

	gThrustOrigins[32].h = 21 * SCALE_FACTOR;
	gThrustOrigins[32].v = 0 * SCALE_FACTOR;

	gThrustOrigins[33].h = 22 * SCALE_FACTOR;
	gThrustOrigins[33].v = 2 * SCALE_FACTOR;

	gThrustOrigins[34].h = 24 * SCALE_FACTOR;
	gThrustOrigins[34].v = 3 * SCALE_FACTOR;

	gThrustOrigins[35].h = 25 * SCALE_FACTOR;
	gThrustOrigins[35].v = 5 * SCALE_FACTOR;

	gThrustOrigins[36].h = 26 * SCALE_FACTOR;
	gThrustOrigins[36].v = 7 * SCALE_FACTOR;

	gThrustOrigins[37].h = 25 * SCALE_FACTOR;
	gThrustOrigins[37].v = 7 * SCALE_FACTOR;

	gThrustOrigins[38].h = 24 * SCALE_FACTOR;
	gThrustOrigins[38].v = 10 * SCALE_FACTOR;

	gThrustOrigins[39].h = 23 * SCALE_FACTOR;
	gThrustOrigins[39].v = 11 * SCALE_FACTOR;

	gThrustOrigins[40].h = 23 * SCALE_FACTOR;
	gThrustOrigins[40].v = 12 * SCALE_FACTOR;

	gThrustOrigins[41].h = 20 * SCALE_FACTOR;
	gThrustOrigins[41].v = 14 * SCALE_FACTOR;

	gThrustOrigins[42].h = 20 * SCALE_FACTOR;
	gThrustOrigins[42].v = 16 * SCALE_FACTOR;

	gThrustOrigins[43].h = 18 * SCALE_FACTOR;
	gThrustOrigins[43].v = 18 * SCALE_FACTOR;

	gThrustOrigins[44].h = 15 * SCALE_FACTOR;
	gThrustOrigins[44].v = 18 * SCALE_FACTOR;

	gThrustOrigins[45].h = 15 * SCALE_FACTOR;
	gThrustOrigins[45].v = 20 * SCALE_FACTOR;

	gThrustOrigins[46].h = 12 * SCALE_FACTOR;
	gThrustOrigins[46].v = 21 * SCALE_FACTOR;

	gThrustOrigins[47].h = 9 * SCALE_FACTOR;
	gThrustOrigins[47].v = 22 * SCALE_FACTOR;

}	/* -- InitShots */


/* ----------------------------------------------------------------- */
/* -- Build the ship's velocity table */

static void BuildVelocityTable(void)
{
#ifdef COMPUTE_VELTABLE
	/* Calculate the appropriate values */
	int	index;
	double	factor;
	double	ss;

	ss = SHIP_FRAMES;
	factor = (360.0 / ss);

	for (index = 0; index < SHIP_FRAMES; index++) {
		ss = index;
		ss = -(((ss * factor) * PI) / 180.0);
		gVelocityTable[index].h = (int)(sin(ss) * -8.0);
		gVelocityTable[index].v = (int)(cos(ss) * -8.0);
#ifdef PRINT_TABLE
		printf("\tgVelocityTable[%d].h = %d;\n", index,
						gVelocityTable[index].h);
		printf("\tgVelocityTable[%d].v = %d;\n", index,
						gVelocityTable[index].v);
#endif
	}
#else
	/* Because PI, sin() and cos() return _slightly_ different
	   values across architectures, we need to precompute our
	   velocity table -- make it standard across compilations. :)
	*/
	gVelocityTable[0].h = 0;
	gVelocityTable[0].v = -8;
	gVelocityTable[1].h = 1;
	gVelocityTable[1].v = -7;
	gVelocityTable[2].h = 2;
	gVelocityTable[2].v = -7;
	gVelocityTable[3].h = 3;
	gVelocityTable[3].v = -7;
	gVelocityTable[4].h = 4;
	gVelocityTable[4].v = -6;
	gVelocityTable[5].h = 4;
	gVelocityTable[5].v = -6;
	gVelocityTable[6].h = 5;
	gVelocityTable[6].v = -5;
	gVelocityTable[7].h = 6;
	gVelocityTable[7].v = -4;
	gVelocityTable[8].h = 6;
	gVelocityTable[8].v = -4;
	gVelocityTable[9].h = 7;
	gVelocityTable[9].v = -3;
	gVelocityTable[10].h = 7;
	gVelocityTable[10].v = -2;
	gVelocityTable[11].h = 7;
	gVelocityTable[11].v = -1;
	gVelocityTable[12].h = 8;
	gVelocityTable[12].v = 0;
	gVelocityTable[13].h = 7;
	gVelocityTable[13].v = 1;
	gVelocityTable[14].h = 7;
	gVelocityTable[14].v = 2;
	gVelocityTable[15].h = 7;
	gVelocityTable[15].v = 3;
	gVelocityTable[16].h = 6;
	gVelocityTable[16].v = 3;
	gVelocityTable[17].h = 6;
	gVelocityTable[17].v = 4;
	gVelocityTable[18].h = 5;
	gVelocityTable[18].v = 5;
	gVelocityTable[19].h = 4;
	gVelocityTable[19].v = 6;
	gVelocityTable[20].h = 3;
	gVelocityTable[20].v = 6;
	gVelocityTable[21].h = 3;
	gVelocityTable[21].v = 7;
	gVelocityTable[22].h = 2;
	gVelocityTable[22].v = 7;
	gVelocityTable[23].h = 1;
	gVelocityTable[23].v = 7;
	gVelocityTable[24].h = 0;
	gVelocityTable[24].v = 8;
	gVelocityTable[25].h = -1;
	gVelocityTable[25].v = 7;
	gVelocityTable[26].h = -2;
	gVelocityTable[26].v = 7;
	gVelocityTable[27].h = -3;
	gVelocityTable[27].v = 7;
	gVelocityTable[28].h = -4;
	gVelocityTable[28].v = 6;
	gVelocityTable[29].h = -4;
	gVelocityTable[29].v = 6;
	gVelocityTable[30].h = -5;
	gVelocityTable[30].v = 5;
	gVelocityTable[31].h = -6;
	gVelocityTable[31].v = 4;
	gVelocityTable[32].h = -6;
	gVelocityTable[32].v = 4;
	gVelocityTable[33].h = -7;
	gVelocityTable[33].v = 3;
	gVelocityTable[34].h = -7;
	gVelocityTable[34].v = 2;
	gVelocityTable[35].h = -7;
	gVelocityTable[35].v = 1;
	gVelocityTable[36].h = -8;
	gVelocityTable[36].v = 0;
	gVelocityTable[37].h = -7;
	gVelocityTable[37].v = -1;
	gVelocityTable[38].h = -7;
	gVelocityTable[38].v = -2;
	gVelocityTable[39].h = -7;
	gVelocityTable[39].v = -3;
	gVelocityTable[40].h = -6;
	gVelocityTable[40].v = -4;
	gVelocityTable[41].h = -6;
	gVelocityTable[41].v = -4;
	gVelocityTable[42].h = -5;
	gVelocityTable[42].v = -5;
	gVelocityTable[43].h = -4;
	gVelocityTable[43].v = -6;
	gVelocityTable[44].h = -4;
	gVelocityTable[44].v = -6;
	gVelocityTable[45].h = -3;
	gVelocityTable[45].v = -7;
	gVelocityTable[46].h = -2;
	gVelocityTable[46].v = -7;
	gVelocityTable[47].h = -1;
	gVelocityTable[47].v = -7;
#endif /* COMPUTE_VELTABLE */
}	/* -- BuildVelocityTable */


/* This function needs to be able to properly clean up from any stage
   of the program at any time, including itself if interrupted during
   cleanup.  *sigh*  reentrant multi-threading can be a pain. :)
*/
void CleanUp(void)
{
	FreeScores();
	SaveControls();
	QuitPlayerControls();
	if ( ui ) {
		delete ui;
		ui = NULL;
	}
	if ( fontserv ) {
		/* This will free the allocated fonts */
		delete fontserv;
		fontserv = NULL;
	}
	if ( screen ) {
		delete screen;
		screen = NULL;
	}
	if ( sound ) {
		delete sound;
		sound = NULL;
	}
	if ( prefs ) {
		prefs->Save();
		delete prefs;
		prefs = NULL;
	}
	NET_Quit();
	SDL_Quit();
}

/* ----------------------------------------------------------------- */
/* -- Perform some initializations and report failure if we choke */
bool StartInitialization(int window_width, int window_height, Uint32 window_flags)
{
	int w, h;
	SDL_Surface *icon = nullptr;

	gInitializing = true;

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
		error("Couldn't initialize SDL: %s\n", SDL_GetError());
		return false;
	}

	// It's okay if this fails, we'll disable multiplayer in that case
	gNetworkAvailable = NET_Init();

	// -- Initialize some variables
	prefs = new Prefs(GAME_PREFS_FILE);
	gLastHigh = -1;

	if (!InitFilesystem(MAELSTROM_ORGANIZATION, MAELSTROM_NAME)) {
		return false;
	}

	/* Load the Maelstrom icon */
#if !defined(SDL_PLATFORM_APPLE) || defined(ENABLE_STEAM)
	icon = SDL_LoadSurface_IO(OpenRead("icon.png"), true);
	if ( icon == NULL ) {
		error("Fatal: Couldn't load icon: %s\n", SDL_GetError());
		return false;
	}
#endif

	/* Initialize the screen */
	screen = new FrameBuf;
	if (!InitResolutions(w, h)) {
		return false;
	}
	if (screen->Init(w, h, window_flags | SDL_WINDOW_HIDDEN, "Maelstrom", icon) < 0){
		error("Fatal: %s\n", screen->Error());
		return false;
	}
	if (window_width && window_height) {
		SDL_SetWindowSize(screen->GetWindow(), window_width, window_height);
	}
	SDL_ShowWindow(screen->GetWindow());
	SDL_DestroySurface(icon);

	/* Load the Font Server and fonts */
	fontserv = new FontServ(screen, "Maelstrom Fonts");
	if (fontserv->Error()) {
		error("Fatal: %s\n", fontserv->Error());
		return false;
	}

	/* Load the Sound Server and initialize sound */
	sound = new Sound("Maelstrom Sounds", gSoundLevel);
	if (sound->Error()) {
		error("Fatal: %s\n", sound->Error());
		return false;
	}

	/* Set up for the resolution we actually got */
	gScrnRect.x = 0;
	gScrnRect.y = 0;
	gScrnRect.w = screen->Width();
	gScrnRect.h = screen->Height();

	SDL_Rect clipRect;
	clipRect.x = (SPRITES_WIDTH << SPRITE_PRECISION);
	clipRect.y = (SPRITES_WIDTH << SPRITE_PRECISION);
	GetRenderCoordinates(clipRect.x, clipRect.y);
	clipRect.w = gScrnRect.w - (clipRect.x - gScrnRect.x)*2;
	clipRect.h = gScrnRect.h - (clipRect.y - gScrnRect.y)*2;
	screen->ClipBlit(&clipRect);

	/* Create the UI manager */
	ui = new MaelstromUI(screen, prefs);

#ifdef FAST_ITERATION
	/* Fast transition between panels */
	ui->SetPanelTransition(PANEL_TRANSITION_NONE);
#else
	/* Fade transition between panels */
	ui->SetPanelTransition(PANEL_TRANSITION_FADE);
#endif

	/* -- Throw up our intro screen */
	if (ui->GetPanelTransition() == PANEL_TRANSITION_FADE) {
		screen->FadeOut();
	}

	ui->ShowPanel(PANEL_LOADING);

	return true;
}

bool ContinueInitialization()
{
	bool failed;

	switch (gLoadingStage) {
	case LOAD_STAGE_STARTING:
		/* -- Load in the prize CICN's */
		if ( LoadCICNS() < 0 ) {
			return false;
		}

		/* -- Create the stars array */
		InitStars();

		/* -- Set up the velocity tables */
		BuildVelocityTable();

		DrawLoadBar();
		gLoadingStage = LOAD_STAGE_BLITS1;
		break;

	case LOAD_STAGE_BLITS1:
	case LOAD_STAGE_BLITS2:
	case LOAD_STAGE_BLITS3:
	case LOAD_STAGE_BLITS4:
	case LOAD_STAGE_BLITS5:
	case LOAD_STAGE_BLITS6:
	case LOAD_STAGE_BLITS7:
	case LOAD_STAGE_BLITS8:
	case LOAD_STAGE_BLITS9:
	case LOAD_STAGE_BLITS10:
	case LOAD_STAGE_BLITS11:
	case LOAD_STAGE_BLITS12:
	case LOAD_STAGE_BLITS13:
	case LOAD_STAGE_BLITS14:
	case LOAD_STAGE_BLITS15:
	case LOAD_STAGE_BLITS16:
	case LOAD_STAGE_BLITS17:
	case LOAD_STAGE_BLITS18:
	case LOAD_STAGE_BLITS19:
	case LOAD_STAGE_BLITS20:
	case LOAD_STAGE_BLITS21:
	case LOAD_STAGE_BLITS22:
	case LOAD_STAGE_BLITS23:
	case LOAD_STAGE_BLITS24:
	case LOAD_STAGE_BLITS25:
		if ( LoadBlits() < 0 ) {
			return false;
		}
		break;

	case LOAD_STAGE_SHOTS:
		/* -- Create the shots array */
		InitShots();

		gLoadingStage = LOAD_STAGE_SPRITES;

		// Fallthrough...
		//break;

	case LOAD_STAGE_SPRITES:
		/* -- Initialize the sprite manager - after we load blits and shots! */
		if ( InitSprites() < 0 ) {
			return false;
		}

		gLoadingStage = LOAD_STAGE_FILESYSTEM;

		// Fallthrough...
		//break;

	case LOAD_STAGE_FILESYSTEM:
		if (!FilesystemReady(&failed)) {
			if (failed) {
				return false;
			}
			break;
		}

		// -- Create our scores file
		LoadScores();

		// -- Load our preferences files
		prefs->Load();

		// -- Load our controls
		LoadControls();
		InitPlayerControls();

		gLoadingStage = LOAD_STAGE_COMPLETE;

		// Fallthrough...
		//break;

	case LOAD_STAGE_COMPLETE:
		ui->DeletePanel(PANEL_LOADING);

		gInitializing = false;

		ui->ShowPanel(PANEL_MAIN);

		gRunning = true;
		break;

	default:
		break;
	}

	return true;

}	/* -- DoInitializations */


/* ----------------------------------------------------------------- */
/* -- Load in the blits */

static int LoadBlits(void)
{
	int i;
	short id;

	switch (gLoadingStage) {
	case LOAD_STAGE_BLITS1:
		/* -- Load in the thrusters */
		if ( LoadSmallSprite(&gThrust1, 400, SHIP_FRAMES) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS2:
		if ( LoadSmallSprite(&gThrust2, 500, SHIP_FRAMES) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS3:
		/* -- Load in the player's ship */
		for (i = 0; i < MAX_PLAYERS; ++i) {
			id = i*10000 + 200;
			if ( LoadLargeSprite(&gPlayerShip[i], id, SHIP_FRAMES) < 0 )
				return(-1);
		}
		break;

	case LOAD_STAGE_BLITS4:
		/* -- Load in the large rock */
		if ( LoadLargeSprite(&gRock1R, 500, 60) < 0 ) {
			return(-1);
		}
		BackwardsSprite(&gRock1L, gRock1R);
		break;

	case LOAD_STAGE_BLITS5:
		/* -- Load in the medium rock */
		if ( LoadLargeSprite(&gRock2R, 400, 40) < 0 ) {
			return(-1);
		}
		BackwardsSprite(&gRock2L, gRock2R);
		break;

	case LOAD_STAGE_BLITS6:
		/* -- Load in the small rock */
		if ( LoadSmallSprite(&gRock3R, 300, 20) < 0 ) {
			return(-1);
		}
		BackwardsSprite(&gRock3L, gRock3R);
		break;

	case LOAD_STAGE_BLITS7:
		/* -- Load in the explosion */
		if ( LoadLargeSprite(&gExplosion, 600, 12) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS8:
		/* -- Load in the 2x multiplier */
		if ( LoadLargeSprite(&gMult[0], 2000, 1) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS9:
		/* -- Load in the 3x multiplier */
		if ( LoadLargeSprite(&gMult[1], 2002, 1) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS10:
		/* -- Load in the 4x multiplier */
		if ( LoadLargeSprite(&gMult[2], 2004, 1) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS11:
		/* -- Load in the 5x multiplier */
		if ( LoadLargeSprite(&gMult[3], 2006, 1) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS12:
		/* -- Load in the steel asteroid */
		if ( LoadLargeSprite(&gSteelRoidL, 700, 40) < 0 ) {
			return(-1);
		}
		BackwardsSprite(&gSteelRoidR, gSteelRoidL);
		break;

	case LOAD_STAGE_BLITS13:
		/* -- Load in the prize */
		if ( LoadLargeSprite(&gPrize, 800, 30) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS14:
		/* -- Load in the bonus */
		if ( LoadLargeSprite(&gBonusBlit, 900, 10) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS15:
		/* -- Load in the bonus */
		if ( LoadLargeSprite(&gPointBlit, 1000, 6) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS16:
		/* -- Load in the vortex */
		if ( LoadLargeSprite(&gVortexBlit, 1100, 10) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS17:
		/* -- Load in the homing mine */
		if ( LoadLargeSprite(&gMineBlitR, 1200, 40) < 0 ) {
			return(-1);
		}
		BackwardsSprite(&gMineBlitL, gMineBlitR);
		break;

	case LOAD_STAGE_BLITS18:
		/* -- Load in the shield */
		if ( LoadLargeSprite(&gShieldBlit, 1300, 2) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS19:
		/* -- Load in the nova */
		if ( LoadLargeSprite(&gNova, 1400, 18) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS20:
		/* -- Load in the ship explosion */
		if ( LoadLargeSprite(&gShipExplosion, 1500, 21) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS21:
		/* -- Load in the shrapnel */
		for (i = 0; i < MAX_PLAYERS; ++i) {
			id = i*10000 + 1800;
			if ( LoadLargeSprite(&gShrapnel1[i], id, 48) < 0 )
				return(-1);
		}
		break;

	case LOAD_STAGE_BLITS22:
		for (i = 0; i < MAX_PLAYERS; ++i) {
			id = i*10000 + 1900;
			if ( LoadLargeSprite(&gShrapnel2[i], id, 40) < 0 )
				return(-1);
		}
		break;

	case LOAD_STAGE_BLITS23:
		/* -- Load in the damaged ship */
		if ( LoadLargeSprite(&gDamagedShip, 1600, 10) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS24:
		/* -- Load in the enemy ship */
		if ( LoadLargeSprite(&gEnemyShip, 1700, 40) < 0 ) {
			return(-1);
		}
		break;

	case LOAD_STAGE_BLITS25:
		/* -- Load in the enemy ship */
		if ( LoadLargeSprite(&gEnemyShip2, 2100, 40) < 0 ) {
			return(-1);
		}
		break;
	}

	DrawLoadBar();
	gLoadingStage += 1;

	return(0);
}	/* -- LoadBlits */


/* -- Initialize our sprites */

int InitSprites(void)
{
	/* Initialize sprite variables */
	gNumSprites = 0;
	gLastDrawn = 0L;

	/* Initialize player sprites */
	return(InitPlayerSprites());
}	/* -- InitSprites */


/* ----------------------------------------------------------------- */
/* -- Load in the sprites we use */

static void BackwardsSprite(BlitPtr *theBlit, BlitPtr oldBlit)
{
	BlitPtr	aBlit;
	int index, nFrames;

	aBlit = new Blit;
	nFrames = oldBlit->numFrames;
	aBlit->numFrames = nFrames;
	aBlit->hitRect.left = oldBlit->hitRect.left;
	aBlit->hitRect.right = oldBlit->hitRect.right;
	aBlit->hitRect.top = oldBlit->hitRect.top;
	aBlit->hitRect.bottom = oldBlit->hitRect.bottom;
	aBlit->isSmall = oldBlit->isSmall;

	/* -- Reverse the sprite images */
	for (index = 0; index < aBlit->numFrames; index++) {
		aBlit->sprite[index] = oldBlit->sprite[nFrames - index - 1];
		aBlit->mask[index] = oldBlit->mask[nFrames - index - 1];
	}
	(*theBlit) = aBlit;
}	/* -- BackwardsSprite */


/* ----------------------------------------------------------------- */
/* -- Load in the prize CICN's */

static int LoadCICNS(void)
{
	if ( (gAutoFireIcon = GetCIcon(screen, 128)) == NULL )
		return(-1);
	if ( (gAirBrakesIcon = GetCIcon(screen, 129)) == NULL )
		return(-1);
	if ( (gMult2Icon = GetCIcon(screen, 130)) == NULL )
		return(-1);
	if ( (gMult3Icon = GetCIcon(screen, 131)) == NULL )
		return(-1);
	if ( (gMult4Icon = GetCIcon(screen, 132)) == NULL )
		return(-1);
	if ( (gMult5Icon = GetCIcon(screen, 134)) == NULL )
		return(-1);
	if ( (gLuckOfTheIrishIcon = GetCIcon(screen, 133)) == NULL )
		return(-1);
	if ( (gTripleFireIcon = GetCIcon(screen, 135)) == NULL )
		return(-1);
	if ( (gLongFireIcon = GetCIcon(screen, 136)) == NULL )
		return(-1);
	if ( (gShieldIcon = GetCIcon(screen, 137)) == NULL )
		return(-1);
	return(0);
}	/* -- LoadCICNS */


/* ----------------------------------------------------------------- */
/* -- Load in the sprites we use */

static int LoadSprite(bool large, BlitPtr *theBlit, int baseID, int numFrames)
{
	char	file[128];
	int	size;
	SDL_Surface *surface;
	int	index;
	BlitPtr	aBlit;
	Uint32	offset, length;
	int	top, left, bottom, right;
	int	row, col;
	Uint8	*mask;

	aBlit = new Blit;
	aBlit->numFrames = numFrames;
	aBlit->isSmall = !large;

	size = large ? 32 : 16;
	left = size;
	right = 0;
	top = size;
	bottom = 0;

	/* -- Load in the image data */
	for (index = 0; index < numFrames; index++) {
		SDL_snprintf(file, sizeof(file), "Sprites/%s#%d.png", large ? "icl8" : "ics8", baseID+index);
		surface = SDL_LoadSurface_IO(OpenRead(file), true);

		if ( surface == NULL ) {
			error("LoadSprite(): Couldn't load image %s\n", file);
			return(-1);
		}

		if ( surface->format != SDL_PIXELFORMAT_RGBA32 ) {
			SDL_Surface *convert = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
			SDL_DestroySurface(surface);
			if ( !convert ) {
				error("LoadSprite(): Couldn't convert image: %s\n", SDL_GetError());
				return(-1);
			}
			surface = convert;
		}

		if ( surface->w != size || surface->h != size ) {
			SDL_Surface *scaled = SDL_ScaleSurface(surface, size, size, SDL_SCALEMODE_NEAREST);
			SDL_DestroySurface(surface);
			if ( !scaled ) {
				error("LoadSprite(): Couldn't scale image: %s\n", SDL_GetError());
				return(-1);
			}
			surface = scaled;
		}

		mask = (Uint8*)surface->pixels + 3;

		/* -- Figure out the hit rectangle */
		/* -- Do the top/left first */
		for ( row=0; row<size; ++row ) {
			for ( col=0; col<size; ++col ) {
				offset = ((row*size)+col)*4;
				if (mask[offset]) {
					if ( row < top )
						top = row;
					if ( col < left )
						left = col;
				}
			}
		}
		for ( row=size-1; row>top; --row ) {
			for ( col=size-1; col>left; --col ) {
				offset = ((row*size)+col)*4;
				if (mask[offset]) {
					if ( row > bottom )
						bottom = row;
					if ( col > right )
						right = col;
				}
			}
		}
		SetRect(&aBlit->hitRect, left, top, right, bottom);

		/* Load the image */
		aBlit->sprite[index] = GetSprite(screen, baseID+index, large);
		if ( aBlit->sprite[index] == NULL ) {
			error("LoadSprite(): Couldn't convert sprite image to texture for %s\n", file);
			return(-1);
		}

		/* Create the bytemask */
		length = size*size;
		aBlit->mask[index] = new Uint8[length];
		for ( offset=0; offset<length; ++offset ) {
			aBlit->mask[index][offset] = mask[offset*4]; 
		}
		gSpriteCRC = SDL_crc32(gSpriteCRC, aBlit->mask[index], length);

		SDL_DestroySurface(surface);
	}
	(*theBlit) = aBlit;
	return(0);
}	/* -- LoadSprite */


/* ----------------------------------------------------------------- */
/* -- Load in the sprites we use */

static int LoadLargeSprite(BlitPtr *theBlit, int baseID, int numFrames)
{
	return LoadSprite(true, theBlit, baseID, numFrames);
}

static int LoadSmallSprite(BlitPtr *theBlit, int baseID, int numFrames)
{
	return LoadSprite(false, theBlit, baseID, numFrames);
}


/* ----------------------------------------------------------------- */
/* -- Delegate to handle the loading panel */

void LoadingPanelDelegate::OnShow()
{
#ifdef SDL_PLATFORM_EMSCRIPTEN
	StartWaiting();
#else
	StartLoading();
#endif
}

bool LoadingPanelDelegate::HandleEvent(const SDL_Event &event)
{
	if (gLoadingStage == LOAD_STAGE_WAITING) {
		switch (event.type) {
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			StartLoading();
			return true;
		default:
			break;
		}
	}
	return false;
}

void LoadingPanelDelegate::StartWaiting()
{
	UIPanel *panel = ui->GetPanel(PANEL_LOADING);
	if (panel) {
		UIElement *waiting_label = panel->GetElement<UIElement>("waiting");
		UIElement *loading_label = panel->GetElement<UIElement>("loading");
		if (waiting_label) {
			waiting_label->Show();
		}
		if (loading_label) {
			loading_label->Hide();
		}
	}
	gLoadingStage = LOAD_STAGE_WAITING;
}

void LoadingPanelDelegate::StartLoading()
{
	UIPanel *panel = ui->GetPanel(PANEL_LOADING);
	if (panel) {
		UIElement *waiting_label = panel->GetElement<UIElement>("waiting");
		UIElement *loading_label = panel->GetElement<UIElement>("loading");
		if (waiting_label) {
			waiting_label->Hide();
		}
		if (loading_label) {
			loading_label->Show();
		}
	}
	sound->PlaySound(gPrizeAppears);
	gLoadingStage = LOAD_STAGE_STARTING;
}

