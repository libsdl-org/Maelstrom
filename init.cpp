
#include <math.h>
#include <signal.h>
#include <stdlib.h>

#include "Maelstrom_Globals.h"
#include "Maelstrom_icon.xpm"
#include "colortable.h"


// Global variables set in this file...

SoundClient *sound;
FontServ    *fontserv;
FrameBuf    *win;

int     gLastHigh;
unsigned long     gLastDrawn;
int     gNumSprites;
Rect	gScrnRect;
Rect	gClipRect;
int	gStatusLine;
int	gTop, gLeft, gBottom, gRight;
MPoint	gShotOrigins[SHIP_FRAMES];
MPoint	gThrustOrigins[SHIP_FRAMES];
MPoint	gVelocityTable[SHIP_FRAMES];
StarPtr	gTheStars[MAX_STARS];
unsigned char *gStarColors;

/* -- The blit'ers we use */
BlitPtr		gRock1R, gRock2R, gRock3R, gDamagedShip;
BlitPtr		gRock1L, gRock2L, gRock3L, gShipExplosion;
BlitPtr		gPlayerShip, gExplosion, gNova, gEnemyShip, gEnemyShip2;
BlitPtr		gMult[4], gSteelRoidL;
BlitPtr		gSteelRoidR, gPrize, gBonusBlit, gPointBlit;
BlitPtr		gVortexBlit, gMineBlitL, gMineBlitR, gShieldBlit;
BlitPtr		gThrust1, gThrust2, gShrapnel1, gShrapnel2;

/* -- The prize CICN's */

CIconPtr gAutoFireIcon, gAirBrakesIcon, gMult2Icon, gMult3Icon;
CIconPtr gMult4Icon, gMult5Icon, gLuckOfTheIrishIcon, gLongFireIcon;
CIconPtr gTripleFireIcon, gKeyIcon, gShieldIcon;

// Local functions used in this file.
static int LoadSounds(void);
static void DrawLoadBar(int first);
static int InitSprites(void);
static int LoadBlits(void);
static int LoadCICNS(void);
static void BackwardsSprite(BlitPtr *theBlit, BlitPtr oldBlit);
static int LoadSprite(BlitPtr *theBlit, int baseID, int numFrames);
static int LoadSmallSprite(BlitPtr *theBlit, int baseID, int numFrames);

/* ----------------------------------------------------------------- */
/* -- Put up our intro splash screen */

void DoIntroScreen(void)
{
	MFont *geneva;
	BitMap *text;
	struct Title intro;
	unsigned short Yoff, Xoff;
	unsigned long  clr, ltClr, ltrClr;

	if ( Load_Title(&intro, 130) < 0 ) {
		error("Can't load intro title!\n");
		exit(255);
	}
	Xoff = ((SCREEN_WIDTH-intro.width)/2); 
	Yoff = ((SCREEN_HEIGHT-intro.height)/2); 

	// -- Draw a border
	ltClr = win->Map_Color(40000, 40000, 0xFFFF);
	ltrClr = win->Map_Color(50000, 50000, 0xFFFF);
	clr = win->Map_Color(30000, 30000, 0xFFFF);

	win->Clear();
	win->DrawRectangle(Xoff-1, Yoff-1, intro.width+2, intro.height+2, clr);
	win->DrawRectangle(Xoff-2, Yoff-2, intro.width+4, intro.height+4, clr);
	win->DrawRectangle(Xoff-3, Yoff-3, 
				intro.width+6, intro.height+6, ltClr);
	win->DrawRectangle(Xoff-4, Yoff-4, 
				intro.width+8, intro.height+8, ltClr);
	win->DrawRectangle(Xoff-5, Yoff-5, 
				intro.width+10, intro.height+10, ltrClr);
	win->DrawRectangle(Xoff-6, Yoff-6, 
				intro.width+12, intro.height+12, ltClr);
	win->DrawRectangle(Xoff-7, Yoff-7, 
				intro.width+14, intro.height+14, clr);
	win->Blit_Title(Xoff, Yoff, intro.width, intro.height, intro.data);
	Free_Title(&intro);

/* -- Draw the loading message */

	clr = win->Map_Color(0xFFFF, 0xFFFF, 0x0000);
	if ( (geneva = fontserv->New_Font("Geneva", 9)) == NULL ) {
		error("Can't use Geneva font! -- Exiting.\n");
		exit(255);
	}
	text = fontserv->Text_to_BitMap("Loading...", geneva, STYLE_BOLD);
	
	Xoff = (SCREEN_WIDTH - text->width) / 2;
	DrawText(Xoff, Yoff+intro.height+20, text, clr);
	fontserv->Free_Text(text);
	fontserv->Free_Font(geneva);

	win->Refresh();
}	// -- DoIntroScreen


/* ----------------------------------------------------------------- */
/* -- Draw a loading status bar */

#define	MAX_BAR	26

static void DrawLoadBar(int first)
{
	static int    stage;
	static int    fact;

	unsigned long black, clr;

	if (first) {
		stage = 1;
	
		black = win->Map_Color(0x0000, 0x0000, 0x0000);
		win->DrawRectangle((SCREEN_WIDTH-200)/2, 
				   ((SCREEN_HEIGHT-10)/2)+185, 200, 10, black);
		clr = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
		win->DrawRectangle(((SCREEN_WIDTH-200)/2)+1, 
				   ((SCREEN_HEIGHT-10)/2)+185+1, 
							200-2, 10-2, clr);
		clr = win->Map_Color(0x8FFF, 0x8FFF, 0xFFFF);
		win->FillRectangle(((SCREEN_WIDTH-200)/2)+1+1, 
				   ((SCREEN_HEIGHT-10)/2)+185+1+1, 
							200-2-2, 10-2-2, clr);
		win->Refresh();
	}
	clr = win->Map_Color(0x6FFF, 0x6FFF, 0xFFFF);
	fact = ((200-2-2) * stage) / MAX_BAR;
	win->FillRectangle(((SCREEN_WIDTH-200)/2)+1+1, 
				   ((SCREEN_HEIGHT-10)/2)+185+1+1, 
							fact, 10-2-2, clr);
	win->RefreshArea(((SCREEN_WIDTH-200)/2)+1+1, 
			((SCREEN_HEIGHT-10)/2)+185+1+1, fact+1, 10-2-2+1);
	win->Flush(0);
	stage++;
}	/* -- DrawLoadBar */


/* ----------------------------------------------------------------- */
/* -- Initialize the stars */

static void InitStars(void)
{
	int	index;
	unsigned char StarColors[20];

	/* Map star pixel colors to new colormap */
	StarColors[0] = 0xEB;
	StarColors[1] = 0xEC;
	StarColors[2] = 0xED;
	StarColors[3] = 0xEE;
	StarColors[4] = 0xEF;
	StarColors[5] = 0xD9;
	StarColors[6] = 0xDA;
	StarColors[7] = 0xDB;
	StarColors[8] = 0xDC;
	StarColors[9] = 0xDD;
	StarColors[10] = 0xE4;
	StarColors[11] = 0xE5;
	StarColors[12] = 0xE6;
	StarColors[13] = 0xE7;
	StarColors[14] = 0xE8;
	StarColors[15] = 0xF7;
	StarColors[16] = 0xF8;
	StarColors[17] = 0xF9;
	StarColors[18] = 0xFA;
	StarColors[19] = 0xFB;
	win->ReColor(StarColors, &gStarColors, 20);

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
	int	       xx = 30;

	/* Remap the shot colors */
	win->ReColor(gPlayerShotColors,&gPlayerShotColors,SHOT_SIZE*SHOT_SIZE);
	win->ReColor(gEnemyShotColors, &gEnemyShotColors, SHOT_SIZE*SHOT_SIZE);

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
#define PRECOMPUTE_VELTABLE
#ifdef PRECOMPUTE_VELTABLE
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
#else
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
#endif /* PRECOMPUTE_VELTABLE */
}	/* -- BuildVelocityTable */

/* ----------------------------------------------------------------- */
/* -- Open/Close the Macintosh Resource files */

static Mac_Resource *spriteres;
static int OpenFiles(void)
{
	spriteres = new Mac_Resource(file2libpath(SPRITES_NAME));
	return(0);
}
static void CloseFiles(void)
{
	delete spriteres;
}

/* ----------------------------------------------------------------- */
/* -- Perform some initializations and report failure if we choke */
int DoInitializations(int fullscreen, int private_cmap, int dofade)
{
	// -- Initialize some variables
	gLastHigh = -1;

	// -- Create our scores file
	LoadScores();

	// -- Open up the resource files we need
	if ( OpenFiles() < 0 )
		return(-1);

	/* Initialize the main system modules
	   They are loaded in order of failure importance
	 */
	/* Load the Font Server */
	fontserv = new FontServ(file2libpath("Maelstrom Fonts"));

#ifdef __WIN95__
	/* DirectSound requires a window for the initialization.
	   Create our own, and provide it for the sound module.
	 */
#ifdef USE_MGL
	win = new MGL_FrameBuf(SCREEN_WIDTH, SCREEN_HEIGHT, 1, fullscreen);
#else
	win = new DX_FrameBuf(SCREEN_WIDTH, SCREEN_HEIGHT, 1, fullscreen);
#endif /* SciTech MGL API */
#endif /* Win95 */

	/* Load the Sound Server and initialize sound */
	sound = new SoundClient();
	(void) sound->SetVolume(gSoundLevel);

	/* Load sounds here, before we try to create the window */
	if ( LoadSounds() < 0 )
		return(-1);

	/* Now create the display (what a mess) */
#ifndef __WIN95__
	if ( getenv("DISPLAY") ) {
#ifdef USE_DGA
		if ( fullscreen )
			win = new DGA_FrameBuf(SCREEN_WIDTH, SCREEN_HEIGHT, 1);
		else
#endif
		win = new X11_FrameBuf(SCREEN_WIDTH, SCREEN_HEIGHT, 1,
				fullscreen, "Maelstrom", Maelstrom_icon);
#if defined(USE_SVGALIB) && defined(linux)
	} else if ( On_Console() ) {
		win = new VGA_FrameBuf(SCREEN_WIDTH, SCREEN_HEIGHT, 1);
	} else {
		error("Not running under X11 or a Linux console.\n");
#else
	} else {
		error("Not running under X11.\n");
#endif
		return(-1);
	}
#endif /* Win95 */
	win->SetFade(dofade);
	win->Hide_Cursor();

	/* Make sure we clean up properly at exit */
	atexit(CleanUp);
#ifdef SIGHUP
	signal(SIGHUP, Killed);
#endif
#ifdef SIGINT
	signal(SIGINT, Killed);
#endif
#ifdef SIGQUIT
	signal(SIGQUIT, Killed);
#endif
#ifdef SIGILL
	signal(SIGILL, Killed);
#endif
#ifdef SIGTRAP
	signal(SIGTRAP, Killed);
#endif
#ifdef SIGPIPE
	signal(SIGPIPE, Killed);
#endif
#ifdef SIGBUS
	signal(SIGBUS, Killed);
#endif
#ifdef SIGSEGV
	signal(SIGSEGV, Killed);
#endif
#ifdef SIGTERM
	signal(SIGTERM, Killed);
#endif
#ifdef SIGCHLD
	signal(SIGCHLD, ReapChild);
#endif

	// -- Load in the palette we want to use
	int GammaLevel = gGammaCorrect;
#ifdef DEBUG
error("Gamma correction level %d\n", GammaLevel);
#endif
#ifdef INTERLACE_DOUBLED
	/* Increase gamma to compensate for losing every other scanline */
	if ( GammaLevel < MAX_GAMMA )
		++GammaLevel;
	/* Twice */
	if ( GammaLevel < MAX_GAMMA )
		++GammaLevel;
#endif
	if ( private_cmap )
		win->Alloc_Private_Cmap(full_colors[GammaLevel]);
	else
		win->Alloc_Cmap(full_colors[GammaLevel]);

	/* -- We want to access the FULL screen! */
	SetRect(&gScrnRect, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	gStatusLine = (gScrnRect.bottom - gScrnRect.top - STATUS_HEIGHT);
	gScrnRect.bottom -= STATUS_HEIGHT;
	gTop = 0;
	gLeft = 0;
	gBottom = gScrnRect.bottom - gScrnRect.top;
	gRight = gScrnRect.right - gScrnRect.left;

	SetRect(&gClipRect, gLeft+SPRITES_WIDTH, gTop+SPRITES_WIDTH,
		gRight-SPRITES_WIDTH, gBottom+STATUS_HEIGHT-SPRITES_WIDTH);
	win->Set_BlitClip(gClipRect.left, gClipRect.top,
				gClipRect.right, gClipRect.bottom);

	/* Do the Ambrosia Splash screen */
	win->Fade(FADE_STEPS);
	DoSplash();
	win->Fade(FADE_STEPS);
	Delay(60*5);
	win->Fade(FADE_STEPS);
	/* -- Throw up our intro screen */
	DoIntroScreen();
	sound->PlaySound(gPrizeAppears, 1, NULL);
	win->Fade(FADE_STEPS);

	/* -- Load in our sprites and other needed resources */
	if ( LoadBlits() < 0 )
		return(-1);

	/* -- Create the shots array */
	InitShots();

	/* -- Initialize the sprite manager - after we load blits and shots! */
	if ( InitSprites() < 0 )
		return(-1);

	/* -- Load in the prize CICN's */
	if ( LoadCICNS() < 0 )
		return(-1);

	/* -- Create the stars array */
	InitStars();

	/* -- Set up the velocity tables */
	BuildVelocityTable();

	/* -- Return OKAY! */
	CloseFiles();
	return(0);
}	/* -- DoInitializations */


/* ----------------------------------------------------------------- */
/* -- Load in the sounds we need */

static int LoadSounds(void)
{
/* -- Load in the sound resources */

	if ( sound->LoadSound(gPrizeAppears) < 0 )
		return(-1);
	if ( sound->LoadSound(gBonusShot) )
		return(-1);
	if ( sound->LoadSound(gShotSound) < 0 )
		return(-1);
	if ( sound->LoadSound(gIdiotSound) < 0 )
		return(-1);
	if ( sound->LoadSound(gMultiplier) < 0 )
		return(-1);
	if ( sound->LoadSound(gShipHitSound) < 0 )
		return(-1);
	if ( sound->LoadSound(gExplosionSound) < 0 )
		return(-1);
	if ( sound->LoadSound(gBoom1) < 0 )
		return(-1);
	if ( sound->LoadSound(gBoom2) < 0 )
		return(-1);
	if ( sound->LoadSound(gMultiplierGone) < 0 )
		return(-1);
	if ( sound->LoadSound(gMultShotSound) < 0 )
		return(-1);
	if ( sound->LoadSound(gSteelHit) < 0 )
		return(-1);
	if ( sound->LoadSound(gBonk) < 0 )
		return(-1);
	if ( sound->LoadSound(gRiff) < 0 )
		return(-1);
	if ( sound->LoadSound(gGotPrize) < 0 )
		return(-1);
	if ( sound->LoadSound(gGameOver) < 0 )
		return(-1);
	if ( sound->LoadSound(gNewLife) < 0 )
		return(-1);
	if ( sound->LoadSound(gBonusAppears) < 0 )
		return(-1);
	if ( sound->LoadSound(gNoBonus) < 0 )
		return(-1);
	if ( sound->LoadSound(gGravAppears) < 0 )
		return(-1);
	if ( sound->LoadSound(gHomingAppears) < 0 )
		return(-1);
	if ( sound->LoadSound(gShieldOnSound) < 0 )
		return(-1);
	if ( sound->LoadSound(gNoShieldSound) < 0 )
		return(-1);
	if ( sound->LoadSound(gNovaAppears) < 0 )
		return(-1);
	if ( sound->LoadSound(gNovaBoom) < 0 )
		return(-1);
	if ( sound->LoadSound(gLuckySound) < 0 )
		return(-1);
	if ( sound->LoadSound(gDamagedAppears) < 0 )
		return(-1);
	if ( sound->LoadSound(gSavedShipSound) < 0 )
		return(-1);
	if ( sound->LoadSound(gFunk) < 0 )
		return(-1);
	if ( sound->LoadSound(gEnemyAppears) < 0 )
		return(-1);
	if ( sound->LoadSound(gPrettyGood) < 0 )
		return(-1);
	if ( sound->LoadSound(gThrusterSound) < 0 )
		return(-1);
	if ( sound->LoadSound(gEnemyFire) < 0 )
		return(-1);
	if ( sound->LoadSound(gFreezeSound) < 0 )
		return(-1);
	if ( sound->LoadSound(gPauseSound) < 0 )
		return(-1);
	return(0);
}	/* -- sound->LoadSounds */


/* ----------------------------------------------------------------- */
/* -- Load in the blits */

static int LoadBlits(void)
{
	DrawLoadBar(1);

/* -- Load in the thrusters */

	if ( LoadSmallSprite(&gThrust1, 400, SHIP_FRAMES) < 0 )
		return(-1);
	DrawLoadBar(0);

	if ( LoadSmallSprite(&gThrust2, 500, SHIP_FRAMES) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the player's ship */

	if ( LoadSprite(&gPlayerShip, 200, SHIP_FRAMES) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the large rock */

	if ( LoadSprite(&gRock1R, 500, 60) < 0 )
		return(-1);
	BackwardsSprite(&gRock1L, gRock1R);
	DrawLoadBar(0);

/* -- Load in the medium rock */

	if ( LoadSprite(&gRock2R, 400, 40) < 0 )
		return(-1);
	BackwardsSprite(&gRock2L, gRock2R);
	DrawLoadBar(0);

/* -- Load in the small rock */

	if ( LoadSmallSprite(&gRock3R, 300, 20) < 0 )
		return(-1);
	BackwardsSprite(&gRock3L, gRock3R);
	DrawLoadBar(0);

/* -- Load in the explosion */

	if ( LoadSprite(&gExplosion, 600, 12) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the 2x multiplier */

	if ( LoadSprite(&gMult[0], 2000, 1) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the 3x multiplier */

	if ( LoadSprite(&gMult[1], 2002, 1) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the 4x multiplier */

	if ( LoadSprite(&gMult[2], 2004, 1) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the 5x multiplier */

	if ( LoadSprite(&gMult[3], 2006, 1) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the steel asteroid */

	if ( LoadSprite(&gSteelRoidL, 700, 40) < 0 )
		return(-1);
	BackwardsSprite(&gSteelRoidR, gSteelRoidL);
	DrawLoadBar(0);

/* -- Load in the prize */

	if ( LoadSprite(&gPrize, 800, 30) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the bonus */

	if ( LoadSprite(&gBonusBlit, 900, 10) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the bonus */

	if ( LoadSprite(&gPointBlit, 1000, 6) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the vortex */

	if ( LoadSprite(&gVortexBlit, 1100, 10) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the homing mine */

	if ( LoadSprite(&gMineBlitR, 1200, 40) < 0 )
		return(-1);
	BackwardsSprite(&gMineBlitL, gMineBlitR);
	DrawLoadBar(0);

/* -- Load in the shield */

	if ( LoadSprite(&gShieldBlit, 1300, 2) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the nova */

	if ( LoadSprite(&gNova, 1400, 18) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the ship explosion */

	if ( LoadSprite(&gShipExplosion, 1500, 21) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the shrapnel */

	if ( LoadSprite(&gShrapnel1, 1800, 50) < 0 )
		return(-1);
	DrawLoadBar(0);

	if ( LoadSprite(&gShrapnel2, 1900, 42) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the damaged ship */

	if ( LoadSprite(&gDamagedShip, 1600, 10) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the enemy ship */

	if ( LoadSprite(&gEnemyShip, 1700, 40) < 0 )
		return(-1);
	DrawLoadBar(0);

/* -- Load in the enemy ship */

	if ( LoadSprite(&gEnemyShip2, 2100, 40) < 0 )
		return(-1);
	DrawLoadBar(0);

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
	int	index, nFrames;

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
#ifdef IMAGES_NEEDED
		aBlit->spriteImages[index] = 
				oldBlit->spriteImages[nFrames - index - 1];
#endif
		aBlit->imageMasks[index] = 
				oldBlit->imageMasks[nFrames - index - 1];
		aBlit->sprite[index] = oldBlit->sprite[nFrames - index - 1];
	}
	(*theBlit) = aBlit;
}	/* -- BackwardsSprite */


/* ----------------------------------------------------------------- */
/* -- Load in the sprites we use */

static int LoadSprite(BlitPtr *theBlit, int baseID, int numFrames)
{
	struct Mac_ResData D;
	int	index;
	BlitPtr	aBlit;
	int	top, left, bottom, right;
	int	row, col, offset;
	unsigned char *mask, *pdata;

	aBlit = new Blit;
	aBlit->numFrames = numFrames;
	aBlit->isSmall = 0;

	left = 32;
	right = 0;
	top = 32;
	bottom = 0;

	/* -- Load in the image data */
	for (index = 0; index < numFrames; index++) {
		if ( spriteres->get_resource("icl8", baseID+index, &D) < 0 ) {
			error(
	"LoadSprite(%d+%d): Couldn't load icl8 resource!\n", baseID, index);
			return(-1);
		}
		win->ReColor(D.data, &aBlit->spriteImages[index], D.length);
		delete[] D.data;

		if ( spriteres->get_resource("ICN#", baseID+index, &D) < 0 ) {
			error(
	"LoadSprite(%d+%d): Couldn't load ICN# resource!\n", baseID, index);
			return(-1);
		}

		/* -- Figure out the hit rectangle */
		mask = D.data+128;
		/* -- Do the top/left first */
		for ( row=0; row<32; ++row ) {
			for ( col=0; col<32; ++col ) {
				offset = (row*32)+col;
				if ((mask[offset/8]>>(7-(offset%8))) & 0x01){
					if ( row < top )
						top = row;
					if ( col < left )
						left = col;
				}
			}
		}
		for ( row=31; row>top; --row ) {
			for ( col=31; col>left; --col ) {
				offset = (row*32)+col;
				if ((mask[offset/8]>>(7-(offset%8))) & 0x01){
					if ( row > bottom )
						bottom = row;
					if ( col > right )
						right = col;
				}
			}
		}
		SetRect(&aBlit->hitRect, left, top, right, bottom);
				
		aBlit->sprite[index] = win->Compile_Sprite(32, 32, 
					aBlit->spriteImages[index], mask);
#ifndef IMAGES_NEEDED
		win->FreeArt(aBlit->spriteImages[index]);
#endif
		/* Create the bytemask */
		aBlit->imageMasks[index] = new unsigned char[(D.length-128)*8];
		for ( offset=0; offset<((D.length-128)*8); ++offset ) {
			aBlit->imageMasks[index][offset] = 
				((mask[offset/8]>>(7-(offset%8)))&0x01);
		}
	}
	(*theBlit) = aBlit;
	return(0);
}	/* -- LoadSprite */


/* ----------------------------------------------------------------- */
/* -- Load in the prize CICN's */

static int LoadCICNS(void)
{
	if ( (gAutoFireIcon = GetCIcon(128)) == NULL )
		return(-1);
	if ( (gAirBrakesIcon = GetCIcon(129)) == NULL )
		return(-1);
	if ( (gMult2Icon = GetCIcon(130)) == NULL )
		return(-1);
	if ( (gMult3Icon = GetCIcon(131)) == NULL )
		return(-1);
	if ( (gMult4Icon = GetCIcon(132)) == NULL )
		return(-1);
	if ( (gMult5Icon = GetCIcon(134)) == NULL )
		return(-1);
	if ( (gLuckOfTheIrishIcon = GetCIcon(133)) == NULL )
		return(-1);
	if ( (gTripleFireIcon = GetCIcon(135)) == NULL )
		return(-1);
	if ( (gLongFireIcon = GetCIcon(136)) == NULL )
		return(-1);
	if ( (gShieldIcon = GetCIcon(137)) == NULL )
		return(-1);
	if ( (gKeyIcon = GetCIcon(100)) == NULL )
		return(-1);
	return(0);
}	/* -- LoadCICNS */


/* ----------------------------------------------------------------- */
/* -- Load in the sprites we use */

static int LoadSmallSprite(BlitPtr *theBlit, int baseID, int numFrames)
{
	struct Mac_ResData D;
	int	index;
	BlitPtr	aBlit;
	int	top, left, bottom, right;
	int	row, col, offset;
	unsigned char *mask, *pdata;

	aBlit = new Blit;
	aBlit->numFrames = numFrames;
	aBlit->isSmall = 1;

	left = 16;
	right = 0;
	top = 16;
	bottom = 0;

	/* -- Load in the image data */

	for (index = 0; index < numFrames; index++) {
		if ( spriteres->get_resource("ics8", baseID+index, &D) < 0 ) {
			error(
	"LoadSmallSprite(%d+%d): Couldn't load ics8 resource!\n", baseID,index);
			return(-1);
		}
		win->ReColor(D.data, &aBlit->spriteImages[index], D.length);
		delete[] D.data;

		if ( spriteres->get_resource("ics#", baseID+index, &D) < 0 ) {
			error(
	"LoadSmallSprite(%d+%d): Couldn't load ics# resource!\n", baseID,index);
			return(-1);
		}

		/* -- Figure out the hit rectangle */
		mask = D.data+32;
		/* -- Do the top/left first */
		for ( row=0; row<16; ++row ) {
			for ( col=0; col<16; ++col ) {
				offset = (row*16)+col;
				if ((mask[offset/8]>>(7-(offset%8))) & 0x01){
					if ( row < top )
						top = row;
					if ( col < left )
						left = col;
				}
			}
		}
		for ( row=15; row>top; --row ) {
			for ( col=15; col>left; --col ) {
				offset = (row*16)+col;
				if ((mask[offset/8]>>(7-(offset%8))) & 0x01){
					if ( row > bottom )
						bottom = row;
					if ( col > right )
						right = col;
				}
			}
		}
		SetRect(&aBlit->hitRect, left, top, right, bottom);

		aBlit->sprite[index] = win->Compile_Sprite(16, 16, 
					aBlit->spriteImages[index], mask);
#ifndef IMAGES_NEEDED
		win->FreeArt(aBlit->spriteImages[index]);
#endif
		/* Create the bytemask */
		aBlit->imageMasks[index] = new unsigned char[(D.length-32)*8];
		for ( offset=0; offset<((D.length-32)*8); ++offset ) {
			aBlit->imageMasks[index][offset] = 
				((mask[offset/8]>>(7-(offset%8)))&0x01);
		}
	}
	(*theBlit) = aBlit;
	return(0);
}	/* -- LoadSmallSprite */


/* ----------------------------------------------------------------- */
/* -- Set a star */

void SetStar(int which)
{
	int bpp = win->DisplayBPP();
	gTheStars[which]->xCoord = FastRandom(SCREEN_WIDTH);
	gTheStars[which]->yCoord = FastRandom(SCREEN_HEIGHT-STATUS_HEIGHT);
	switch (bpp) {
		case 1:
			gTheStars[which]->color = gStarColors[FastRandom(20)];
			break;
		case 2:
			gTheStars[which]->color = 
				*((short *)&gStarColors[FastRandom(20)*2]);
			break;
		case 3:
		case 4:
			gTheStars[which]->color = 
				*((long *)&gStarColors[FastRandom(20)*4]);
			break;
	}
}	/* -- SetStar */

