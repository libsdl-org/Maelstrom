
#include "Maelstrom_Globals.h"
#include "globals.h"
#include "blit.h"
#include "make.h"

#ifdef __WIN95__
/* The Windows 95 version is not allowed for release by Andrew Welch! */
#define CRIPPLED_DEMO
#endif

// Global variables set in this file...
int	gNumRoids;
ULONG	gBoomDelay;
ULONG	gLastBoom;
int	gBoomPhase;
int	gThrustOn;
int	gNumRocks;
int	gLives;
ULONG	gLastStar;
long	gBonus;
ULONG	gLastBonus;
ULONG	gWaveStarted;
ULONG	gWhenDone;
ULONG	gOurTicks;
int     gGameOn;

int	gMultiplierShown;
ULONG	gMultTime;
ULONG	gWhenMult;
int	gMultFactor;

int	gPrizeShown;
ULONG	gWhenPrize;

int	gBonusShown;
ULONG	gWhenBonus;

ULONG	gWhenHoming;
ULONG	gWhenGrav;
ULONG	gWhenDamaged;

int	gAirBrakes;
int	gAutoFire;
int	gLuckOfTheIrish;
int	gLongFire;
int	gTripleFire;

int	gShieldOn;
int	gShieldLevel;
int	gShaking;
ULONG	gWhenNova;
ULONG	gShakeTime;
int	gNovaBlast;
int	gNovaFlag;
ULONG	gShieldTime;
ULONG	gFreezeTime;
ULONG	gEnemyFireTime;

int	gWave;
int	gEnemySprite;
ULONG	gWhenEnemy;

// Local global variables;
static MFont *geneva=NULL;

// Local functions used in the game module of Maelstrom
static void DoHouseKeeping(void);
static void NextWave(void);
static void HandleDeath(void);
static void DoGameOver(void);
static void DoBonus(void);
static void TwinkleStars(void);

/* ----------------------------------------------------------------- */
/* -- Draw the status display */

void DrawStatus(Bool first)
{
	static	long	lastScore;
	static	long	lastWave;
	static	int	lastLives;
	static	long	lastLife;
	static	long	lastBonus;
	static	int	lastGun;
	static	int	lastBrakes;
	static	int	lastShield;
	static	int	lastMult;
	static	int	lastLong;
	static	int	lastTriple;
	static	int	lastThrusters;
	static	unsigned long ourGrey, ourWhite, ourBlack;
	static	ULONG	lastDrawn;
	static  BitMap	*scoretext=NULL;
	static  BitMap	*wavetext=NULL;
	static  BitMap	*livestext=NULL;
	static  BitMap	*bonustext=NULL;
	BitMap              *text;
	char                 numbuf[128];

	if (first || gRefreshDisplay) {
		int x;

		gRefreshDisplay = 0;
		lastDrawn = gOurTicks - DISPLAY_DELAY;
		lastScore = -1;
		lastWave = -1;
		lastShield = -1;
		lastLives = -1;
		lastBonus = -1;
		lastGun = -1;
		lastBrakes = -1;
		lastMult = -1;
		lastThrusters = -1;
		if (gWave == 1)
			lastLife = 0;
		lastLong = -1;
		lastTriple = -1;
	
		if ( ! geneva &&
			((geneva = fontserv->New_Font("Geneva", 9)) == NULL) ) {
			error("Can't use Geneva font! -- Exiting.\n");
			exit(255);
		}
	
/* -- Set up our colors */

		ourGrey = win->Map_Color(30000, 30000, 0xFFFF);
		ourWhite = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
		ourBlack = win->Map_Color(0x0000, 0x0000, 0x0000);

		win->DrawLine(0, gStatusLine, SCREEN_WIDTH-1, 
							gStatusLine, ourWhite);
	
/* -- Draw the status display */

		text = fontserv->Text_to_BitMap("Score:", geneva, STYLE_BOLD);
		x = 3;
		DrawText(x, gStatusLine+11, text, ourGrey);
		fontserv->Free_Text(text);
		x += (fontserv->TextWidth("Score:", geneva, STYLE_BOLD)+70);
		text = fontserv->Text_to_BitMap("Shield:", geneva, STYLE_BOLD);
		DrawText(x, gStatusLine+11, text, ourGrey);
		fontserv->Free_Text(text);
		x += (fontserv->TextWidth("Shield:", geneva, STYLE_BOLD)+70);
		text = fontserv->Text_to_BitMap("Wave:", geneva, STYLE_BOLD);
		DrawText(x, gStatusLine+11, text, ourGrey);
		fontserv->Free_Text(text);
		x += (fontserv->TextWidth("Wave:", geneva, STYLE_BOLD)+30);
		text = fontserv->Text_to_BitMap("Lives:", geneva, STYLE_BOLD);
		DrawText(x, gStatusLine+11, text, ourGrey);
		fontserv->Free_Text(text);
		x += (fontserv->TextWidth("Lives:", geneva, STYLE_BOLD)+30);
		text = fontserv->Text_to_BitMap("Bonus:", geneva, STYLE_BOLD);
		DrawText(x, gStatusLine+11, text, ourGrey);
		fontserv->Free_Text(text);
	}

	if (((gOurTicks - lastDrawn) > DISPLAY_DELAY) || (gGameOn == 0)) {
		
		/* -- Do incremental updates */
	
		if (lastShield != gShieldLevel) {
			int	fact;

			if (gShieldLevel > MAX_SHIELD)
				gShieldLevel = MAX_SHIELD;
			
			lastShield = gShieldLevel;
			win->DrawRectangle(152, gStatusLine+4, SHIELD_WIDTH, 
								8, ourWhite);
			fact = ((SHIELD_WIDTH - 2) * gShieldLevel) / MAX_SHIELD;
			win->FillRectangle(152+1,gStatusLine+4+1, fact, 6,
								ourGrey);
			win->FillRectangle(152+1+fact, gStatusLine+4+1,
						SHIELD_WIDTH-2-fact, 6,
								ourBlack);
		}
		
		if (lastMult != gMultFactor) {
			lastMult = gMultFactor;
		
			switch (gMultFactor) {
				case 1:	win->FillRectangle(424,
						gStatusLine+4, 8, 8, ourBlack);
					break;
				case 2:	BlitCIcon(424, 
						gStatusLine+4, gMult2Icon);
					break;
				case 3:	BlitCIcon(424, 
						gStatusLine+4, gMult3Icon);
					break;
				case 4:	BlitCIcon(424, 
						gStatusLine+4, gMult4Icon);
					break;
				case 5:	BlitCIcon(424, 
						gStatusLine+4, gMult5Icon);
					break;
				default:  /* WHAT? */
					break;
			}
		}
	
		/* -- Do incremental updates */
	
		if (lastGun != gAutoFire) {
			lastGun = gAutoFire;

			if (gAutoFire == 1) {
				BlitCIcon(438, gStatusLine+4, gAutoFireIcon);
			} else {
				win->FillRectangle(438,
						gStatusLine+4, 8, 8, ourBlack);
			}
		}
	
		if (lastBrakes != gAirBrakes) {
			lastBrakes = gAirBrakes;

			if (gAirBrakes == 1) {
				BlitCIcon(454, gStatusLine+4, gAirBrakesIcon);
			} else {
				win->FillRectangle(454,
						gStatusLine+4, 8, 8, ourBlack);
			}
		}
	
		if (lastThrusters != gLuckOfTheIrish) {
			lastThrusters = gLuckOfTheIrish;

			if (gLuckOfTheIrish == 1) {
				BlitCIcon(470, gStatusLine+4, 
							gLuckOfTheIrishIcon);
			} else {
				win->FillRectangle(470,
						gStatusLine+4, 8, 8, ourBlack);
			}
		}
	
		if (lastTriple != gTripleFire) {
			lastTriple = gTripleFire;

			if (gTripleFire == 1) {
				BlitCIcon(486, gStatusLine+4, gTripleFireIcon);
			} else {
				win->FillRectangle(486,
						gStatusLine+4, 8, 8, ourBlack);
			}
		}
	
		if (lastLong != gLongFire) {
			lastLong = gLongFire;

			if (gLongFire == 1) {
				BlitCIcon(502, gStatusLine+4, gLongFireIcon);
			} else {
				win->FillRectangle(502,
						gStatusLine+4, 8, 8, ourBlack);
			}
		}
	
		if (lastScore != gScore) {
			/* -- See if they got a new life */
			if ((gScore - lastLife) >= NEW_LIFE) {
				gLives++;
				lastLife = (gScore / NEW_LIFE) * NEW_LIFE;
				sound->PlaySound(gNewLife, 5, NULL);
			}
	
			/* -- Draw the score */
			if ( scoretext ) {
				UnDrawText(45, gStatusLine+11, scoretext);
				fontserv->Free_Text(scoretext);
			}
			lastScore = gScore;
			sprintf(numbuf, "%ld", gScore);
			scoretext = fontserv->Text_to_BitMap(numbuf, geneva, 
								STYLE_BOLD);
			DrawText(45, gStatusLine+11, scoretext, ourWhite);
		}
	
		if (lastWave != gWave) {
			if ( wavetext ) {
				UnDrawText(255, gStatusLine+11, wavetext);
				fontserv->Free_Text(wavetext);
			}
			lastWave = gWave;
			sprintf(numbuf, "%d", gWave);
			wavetext = fontserv->Text_to_BitMap(numbuf, geneva, 
								STYLE_BOLD);
			DrawText(255, gStatusLine+11, wavetext, ourWhite);
		}
	
		if (lastLives != gLives) {
			if ( livestext ) {
				UnDrawText(319, gStatusLine+11, livestext);
				fontserv->Free_Text(livestext);
			}
			lastLives = gLives;
			sprintf(numbuf, "%-3.1d", gLives);
			livestext = fontserv->Text_to_BitMap(numbuf, geneva, 
								STYLE_BOLD);
			DrawText(319, gStatusLine+11, livestext, ourWhite);
		}
	
		if (lastBonus != gBonus) {
			if ( bonustext ) {
				UnDrawText(384, gStatusLine+11, bonustext);
				fontserv->Free_Text(bonustext);
			}
			lastBonus = gBonus;
			sprintf(numbuf, "%-7.1ld", gBonus);
			bonustext = fontserv->Text_to_BitMap(numbuf, geneva, 
								STYLE_BOLD);
			DrawText(384, gStatusLine+11, bonustext, ourWhite);
		}
	
		lastDrawn = gOurTicks;
	}
	win->RefreshArea(0, gStatusLine, 
				gScrnRect.right-gScrnRect.left, STATUS_HEIGHT);
}	/* -- DrawStatus */


/* ----------------------------------------------------------------- */
/* -- Start a new game */

void NewGame(void)
{
#ifdef CRIPPLED_DEMO
	mesg("Warning!!! This is DEMO code only!  DO NOT Release!!!!\n");
#endif
	win->Fade(FADE_STEPS);
	win->Hide_Cursor();

	/* -- Initialize some game variables to zero */

	pressed.gQuitControl = 0;
	gScore = 0L;
	gWave = gStartLevel - 1;
	gLives = gStartLives - 1;
	gGameOn = 1;
	gLastStar = 0L;
	gOurTicks = Ticks();

	gLastBonus = gOurTicks;

	gShieldOn = 0;
	gShieldLevel = INITIAL_SHIELD;

	gShieldTime = 0L;

/* -- Set all the prizes to 0 */

	gAirBrakes = 0;
	gAutoFire = 0;
	gLuckOfTheIrish = 0;
	gLongFire = 0;
	gTripleFire = 0;

	NextWave();

	/* Play the game, dammit! */
	while (gGameOn == 1) {
		CompositeFrame();
		BlastFrame();
		DoHouseKeeping();
	}
	
/* -- Do the game over stuff */

	DoGameOver();
	win->Show_Cursor();
}	/* -- NewGame */


/* ----------------------------------------------------------------- */
/* -- Do some housekeeping! */

static void DoHouseKeeping(void)
{
	/* -- Check the shield time */
	if (gShieldTime != 0L) {
		if ((gOurTicks - gShieldTime) > SAFE_TIME)
			gShieldTime = 0L;
	}
	
	/* -- Check the freeze time */
	if (gFreezeTime != 0L) {
		if ((gOurTicks - gFreezeTime) > FREEZE_DURATION)
			gFreezeTime = 0L;
	}
	
	/* -- Maybe throw a multiplier up on the screen */
	if (gMultiplierShown == 0) {
		if ((gOurTicks - gWaveStarted) > gWhenMult)
			MakeMultiplier();
	}
	
	/* -- Maybe throw a prize(!) up on the screen */
	if (gPrizeShown == 0) {
		if ((gOurTicks - gWaveStarted) > gWhenPrize)
			MakePrize();
	}
	
	/* -- Maybe throw a bonus up on the screen */
	if (gBonusShown == 0) {
		if ((gOurTicks - gWaveStarted) > gWhenBonus)
			MakeBonus();
	}

	/* -- Maybe make a nasty enemy fighter? */
	if (gWhenEnemy != 0L) {
		if ((gOurTicks - gWaveStarted) > gWhenEnemy)
			MakeEnemy();
	}

	/* -- Maybe create a transcenfugal vortex */
	if (gWhenGrav != 0L) {
		if ((gOurTicks - gWaveStarted) > gWhenGrav)
			MakeGravity();
	}
	
	/* -- Maybe create a recified space vehicle */
	if (gWhenDamaged != 0L) {
		if ((gOurTicks - gWaveStarted) > gWhenDamaged)
			MakeDamagedShip();
	}
	
	/* -- Maybe create a autonominous tracking device */
	if (gWhenHoming != 0L) {
		if ((gOurTicks - gWaveStarted) > gWhenHoming)
			MakeHoming();
	}
	
	/* -- Maybe make a supercranial destruction thang */
	if (gWhenNova != 0L) {
		if ((gOurTicks - gWaveStarted) > gWhenNova)
			MakeNova();
	}

	/* -- Handle the shimmy and shake */
	if (gShaking == 1) {
		if ((gOurTicks - gShakeTime) > SHAKE_DURATION) {
			int		xVel, rx, index;
		
			gShaking = 0;
			for (index = 0; index < gNumSprites; index++) {
				if (gSprites[index]->spriteType == PLAYER_SHIP){
					gSprites[index]->xVel = 0;
					gSprites[index]->yVel = 0;
				}
				
				if (gSprites[index]->spriteType == EXPLOSION) {
					gSprites[index]->xVel = 0;
					gSprites[index]->yVel = 0;
				}
				
				if (gSprites[index]->spriteType == MULTIPLIER) {
					gSprites[index]->xVel = 0;
					gSprites[index]->yVel = 0;
				}
				
				if (gSprites[index]->spriteType == BONUS) {
					if (gSprites[index]->theBlit == 
								gPointBlit) {
						gSprites[index]->xVel = 0;
						gSprites[index]->yVel = 0;
					} else {
						xVel = 0;
						rx = (VEL_FACTOR + 
						      (gWave/6))*(SCALE_FACTOR);
						while (xVel == 0)
							xVel = FastRandom(rx/2);
						xVel += (3 * SCALE_FACTOR);
						gSprites[index]->xVel = xVel;
						gSprites[index]->yVel = xVel;
					}
				}
				
				if (gSprites[index]->spriteType == GRAVITY) {
					gSprites[index]->xVel = 0;
					gSprites[index]->yVel = 0;
				}
				
			}
		}
	}
	
	/* -- Decrement the bonus */
	if ((gOurTicks - gLastBonus) > BONUS_DELAY) {
		gBonus -= 10L;
		if (gBonus < 0L)
			gBonus = 0L;
		
		gLastBonus = gOurTicks;

		/* -- The game is paused */
		if ( pressed.gPauseControl ) {
			sound->PlaySound(gPauseSound, 5, NULL);
			do {
				/* Give up the CPU for a frame duration */
				Delay(FRAME_DELAY);

				/* Look for Pause key-release */
				HandleEvents(-1);
			} while ( pressed.gPauseControl );
			/* -- Restore the game so we can keep going! */
			/* Nothing to do. :) */
		}
	}
	
	/* -- Maybe create a new star ? */
	if ((gOurTicks - gLastStar) > STAR_DELAY)
		TwinkleStars();
	
	/* -- If they died, time for a little reincarnation! */
	if (gSprites[gShipSprite]->spriteTag == SHIP_DEAD) {
		gThrustOn = 0;
		if ((gOurTicks - gDeadTime) > DEAD_DELAY) {
#ifdef CRIPPLED_DEMO
			gGameOn = 0;
#else
			if (gLives == 0)
				gGameOn = 0;
			else
				HandleDeath();
#endif
		}
	}
	
	/* -- Housekeping */
	if (gLives != -1) {
		DrawStatus(false);
	
		/* -- Time for the next wave? */
		if (gNumRocks <= 0) {
			if (gWhenDone == 0L) {
				gWhenDone = gOurTicks;
				gThrustOn = 0;
			}
		}
	
		if (((gOurTicks-gWhenDone) > DEAD_DELAY) && (gWhenDone != 0L))
			NextWave();
	}
}	/* -- DoHouseKeeping */


/* ----------------------------------------------------------------- */
/* -- Start the next wave! */

static void NextWave(void)
{
	int	index, x, y;
	short	temp;

	gEnemySprite = 0;

	/* -- Initialize some variables */
	gEnemyFireTime = 0L;
	gNumRocks = 0;
	gWhenDone = 0L;
	gShaking = 0;
	gNovaBlast = 0;
	gNovaFlag = 0;
	gFreezeTime = 0L;

	if (gWave != (gStartLevel - 1))
		DoBonus();

	gWave++;

	gBonus = INITIAL_BONUS;
	gMultiplierShown = FastRandom(2);
	gWhenMult = (FastRandom(30) * 60L);

	gPrizeShown = FastRandom(2);
	gWhenPrize = (FastRandom(30) * 60L);

	gBonusShown = FastRandom(2);
	gWhenBonus = (FastRandom(30) * 60L);

	if (FastRandom(10 + gWave) > 11)
		gWhenGrav = (FastRandom(60) * 60L);
	else
		gWhenGrav = 0L;

	if (FastRandom(10 + gWave) > 13)
		gWhenNova = (FastRandom(60) * 60L);
	else
		gWhenNova = 0L;

	if (FastRandom(3) == 0)
		gWhenEnemy = (FastRandom(30) * 60L);
	else
		gWhenEnemy = 0L;

	gShakeTime = 0L;

	if (FastRandom(10) == 0)
		gWhenDamaged = (FastRandom(60) * 60L);
	else
		gWhenDamaged = 0L;

	if (FastRandom(10 + gWave) > 12)
		gWhenHoming = (FastRandom(60) * 60L);
	else
		gWhenHoming = 0L;

	gWaveStarted = gOurTicks;
	gMultTime = 0L;
	gMultFactor = 1;

	temp = gWave / 4;
	if (temp < 1)
		temp = 1;

	gNumRoids = FastRandom(temp) + (gWave / 5) + 3;

	/* -- Get rid of any shots */
	for (index = 0; index < MAX_SHOTS; index++)
		gTheShots[index]->shotVis = 0;
	for (index = 0; index < MAX_SHOTS; index++)
		gEnemyShots[index]->shotVis = 0;
	memset(&pressed, 0, sizeof(pressed));

	/* -- Black the screen out and draw the wave */
	win->Clear();
	DrawStatus(true);

	/* -- Kill any existing sprites */
	while (gNumSprites != 0)
		KillSprite(0);

	/* -- Initialize some variables */
	gNumSprites = 0;
	gLastDrawn = 0L;
	gLastKeyPolled = 0L;
	gBoomDelay = 60L;
	gLastBoom = 0L;
	gBoomPhase = 0;
	gThrustOn = 0;

	/* -- Create the ship's sprite */
	x = (gRight / 2) * SCALE_FACTOR;
	y = (gBottom / 2) * SCALE_FACTOR;

	(void) NewSprite(gPlayerShip, NO_PHASE_CHANGE, x, y, 0, 0, PLAYER_SHIP);
	gSprites[gShipSprite]->spriteTag = SHIP_ALIVE;

	/* -- Create some asteroids */
	for (index = 0; index < gNumRoids; index++) {
		int	randval;
	
		x = FastRandom(gRight) * SCALE_FACTOR;
		y = 0;
	
		randval = FastRandom(10);

		/* -- See what kind of asteroid to make */
		if (randval == 0)
			MakeSteelRoid(x, y);
		else
			MakeLargeRock(x, y);
	}

	/* -- Create the star field */
	for ( index=0; index<MAX_STARS; ++index ) {
		win->DrawBGPoint(gTheStars[index]->xCoord, 
			gTheStars[index]->yCoord, gTheStars[index]->color);
		win->RefreshArea(gTheStars[index]->xCoord,
					gTheStars[index]->yCoord, 1, 1);
	}
	win->Fade(FADE_STEPS);
}	/* -- NextWave */


/* ----------------------------------------------------------------- */
/* -- The player died(!) */

static void HandleDeath(void)
{
	Bool	coastClear = true;
	Bool	zap = true;

	/* -- Set all the prizes to 0 */
	gFreezeTime = 0L;
	gThrustOn = 0;

	if (gLuckOfTheIrish == 1) {
		if (FastRandom(LUCK_ODDS) == 0) {
			zap = false;
			gLuckOfTheIrish = 0;
		}
	}

	/* -- Zap the prizes if we should */
	if (zap) {
		gAirBrakes = 0;
		gAutoFire = 0;
		gLuckOfTheIrish = 0;
		gLongFire = 0;
		gTripleFire = 0;
	}

	gSprites[gShipSprite]->xCoord = (gRight / 2) * SCALE_FACTOR;
	gSprites[gShipSprite]->yCoord = (gBottom / 2) * SCALE_FACTOR;
	gSprites[gShipSprite]->xVel = 0;
	gSprites[gShipSprite]->yVel = 0;

	/* -- If the coast is clear, give them another chance */
	if (coastClear) {
		gLives--;
		gShieldLevel = INITIAL_SHIELD;
		gShieldOn = 1;
		gShieldTime = gOurTicks;
		gSprites[gShipSprite]->numPhases = gPlayerShip->numFrames;
		gSprites[gShipSprite]->phaseOn = 0;
		gSprites[gShipSprite]->changeCount = 0;
		gSprites[gShipSprite]->phaseChange = NO_PHASE_CHANGE;
		gSprites[gShipSprite]->visible = 1;
		gSprites[gShipSprite]->theBlit = gPlayerShip;
		gSprites[gShipSprite]->spriteTag = SHIP_ALIVE;
		gSprites[gShipSprite]->spriteType = PLAYER_SHIP;
	}
}	/* -- HandleDeath */


/* ----------------------------------------------------------------- */
/* -- Do the game over display */

static void DoGameOver(void)
{
	struct	Title gameover;
	MFont        *newyork;
	BitMap       *text;
	int	      xOff, yOff, index, x;
	int	      which = -1, count;
	char          handle[20];
	int           chars_in_handle = 0;
	Bool          done = false;
	unsigned long clr;

	win->Fade(FADE_STEPS);
	sound->HaltSounds();

	gThrustOn = 0;

	/* -- Kill any existing sprites */
	while (gNumSprites != 0)
		KillSprite(0);

	/* -- Clear the screen */
	win->Clear();

	/* -- Draw the game over picture */
	if ( Load_Title(&gameover, 128) < 0 ) {
		error("Can't load 'gameover' title!\n");
		exit(255);
	}
	xOff = (SCREEN_WIDTH - gameover.width) / 2;
	yOff = (SCREEN_HEIGHT - gameover.height) / 2;
	win->Blit_Title(xOff, yOff-80, gameover.width, gameover.height,
								gameover.data);
	Free_Title(&gameover);

	/* -- Play the game over sound */
	sound->PlaySound(gGameOver, 5, NULL);
	win->Fade(FADE_STEPS);

	while(sound->IsSoundPlaying(0))
		Delay(SOUND_DELAY);
		
#ifdef CRIPPLED_DEMO
	mesg("Thanks for playing the Maelstrom95 DEMO!\n");
	mesg(
	"For comments, questions, please contact slouken@devolution.com\n");
	exit(0);
#else
	/* -- See if they got a high score */
	LoadScores();
	for ( index = 0; index<10; index++ ) {
		if ( gScore > hScores[index].score ) {
			which = index;
			break;
		}
	}

	/* -- They got a high score! */
	gLastHigh = which;

	if ((which != -1) && (gStartLevel == 1) && (gStartLives == 3)) {
		sound->PlaySound(gBonusShot, 5, NULL);
		for ( index = 8; index >= which; index-- ) {
			hScores[index + 1].score = hScores[index].score;
			hScores[index + 1].wave = hScores[index].wave;
			strcpy(hScores[index+1].name, hScores[index].name);
		}

		/* -- Draw the "Enter your name" string */
		clr = win->Map_Color(30000, 30000, 0xFFFF);
		if ( (newyork = fontserv->New_Font("New York", 18)) == NULL ) {
                        error("Can't use New York font! -- Exiting.\n");
                        exit(255);
                }
		text = fontserv->Text_to_BitMap("Enter your name: ", 
							newyork, STYLE_NORM);
		x = (SCREEN_WIDTH-(text->width*2))/2;
		DrawText(x, 300, text, clr);
		x += text->width;
		fontserv->Free_Text(text);

		/* -- Let them enter their name */
		clr = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
		text = fontserv->Text_to_BitMap("", newyork, STYLE_NORM);
		chars_in_handle = 0;

		win->FlushEvents();
		while ( !done ) {
			char    buf[128];
			XEvent	event;
			KeySym  key;
				
			win->GetEvent(&event);

			/* -- Handle key down's */
			if ( event.type == KeyPress ) {
				if ( ! (count=win->KeyToAscii(&event, buf, 127, &key)) ) {
					continue;
				}
				switch ( buf[0]  ) {
					case '\033':	// Ignore ESC char
					case '\t':	// Ignore TAB too.
						continue;
					case '\003':
					case '\r':
						done = true;
						continue;
					case 127:
					case '\b':
						if ( chars_in_handle ) {
							sound->PlaySound(gExplosionSound, 5, NULL);
							--chars_in_handle;
						}
						break;
					default:
						if ( chars_in_handle < 15 ) {
							sound->PlaySound(gShotSound, 5, NULL);
							handle[chars_in_handle++] = buf[0];
						} else
							sound->PlaySound(gBonk, 5, NULL);
						break;
				}
				UnDrawText(x, 300, text);
				fontserv->Free_Text(text);

				handle[chars_in_handle] = '\0';
				text = fontserv->Text_to_BitMap(handle,
							newyork, STYLE_NORM);
				DrawText(x, 300, text, clr);
			}
		}
		fontserv->Free_Font(newyork);

		/* In case the user just pressed <Return> */
		handle[chars_in_handle] = '\0';

		hScores[which].wave = gWave;
		hScores[which].score = gScore;
		strcpy(hScores[which].name, handle);

		sound->HaltSounds();
		sound->PlaySound(gGotPrize, 6, NULL);
		if ( gNetScores )	// All time high!
			RegisterHighScore(hScores[which]);
		else
			SaveScores();
	}

	while (sound->IsSoundPlaying(0))
		Delay(SOUND_DELAY);
	HandleEvents(0);
#endif /* CRIPPLED_DEMO */

	win->Fade(FADE_STEPS);
	gUpdateBuffer = true;
	gFadeBack = true;
}	/* -- DoGameOver */


/* ----------------------------------------------------------------- */
/* -- Do the bonus display */

static void DoBonus(void)
{
	unsigned long clr, aWhite, aGray;
	int           x, sw, xs, xt, WasGameOn;
	BitMap       *text, *score, *bonus;
	char          numbuf[128];

	WasGameOn = gGameOn;
	gGameOn = 0;

	DrawStatus(false);

	/* -- Now do the bonus */
	sound->HaltSounds();
	sound->PlaySound(gRiff, 6, NULL);

	/* Fade out */
	win->Fade(FADE_STEPS);

	/* -- Clear the screen */
	clr = win->Map_Color(0x0000, 0x0000, 0x0000);
	win->FillRectangle(0, 0, SCREEN_WIDTH, gStatusLine-1, clr);
	

	/* -- Draw the bonus scores */
	aGray = win->Map_Color(30000, 30000, 0xFFFF);
	aWhite = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
	clr = win->Map_Color(0xFFFF, 0xFFFF, 0x0000);

	sprintf(numbuf, "Wave %d completed.", gWave);
	text = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
	sw = fontserv->TextWidth(numbuf, geneva, STYLE_BOLD);
	x = (SCREEN_WIDTH - sw) / 2;
	DrawText(x,  150, text, clr);
	fontserv->Free_Text(text);

	/* -- Draw the bonus */
	text = fontserv->Text_to_BitMap("Bonus Score:     ",geneva,STYLE_BOLD);
	sw = fontserv->TextWidth("Bonus Score:     ", geneva, STYLE_BOLD);
	x = ((SCREEN_WIDTH - sw) / 2) - 20;
	DrawText(x, 200, text, aGray);
	fontserv->Free_Text(text);
	xt = x+sw;

	/* -- Draw the score */
	text = fontserv->Text_to_BitMap("Score:     ", geneva, STYLE_BOLD);
	sw = fontserv->TextWidth("Score:     ", geneva, STYLE_BOLD);
	x = ((SCREEN_WIDTH - sw) / 2) - 3;
	DrawText(x, 220, text, aGray);
	fontserv->Free_Text(text);
	xs = x+sw;

	/* Fade in */
	win->Fade(FADE_STEPS);

	while (sound->IsSoundPlaying(0))
		Delay(SOUND_DELAY);
	
	/* -- Count the score down */
	x = xs;

	if (gMultFactor != 1) {
		CSprite        *sprite=NULL;

		sprintf(numbuf, "%-5.1ld", gBonus);
		text = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
		DrawText(x, 200, text, aWhite);
		fontserv->Free_Text(text);

		x += 75;
		gBonus *= gMultFactor;
		Delay(SOUND_DELAY);
		sound->PlaySound(gMultiplier, 5, NULL);
		switch (gMultFactor) {
			case 2:
				sprite = gMult[2-2]->sprite[0];
				break;
			case 3:
				sprite = gMult[3-2]->sprite[0];
				break;
			case 4:
				sprite = gMult[4-2]->sprite[0];
				break;
			case 5:
				sprite = gMult[5-2]->sprite[0];
				break;
		}
		if ( sprite ) {
			win->Blit_CSprite(xs+34, 180, sprite);
			Delay(60);
			gMultFactor = 1;
		}
	}

	Delay(SOUND_DELAY);
	sound->PlaySound(gFunk, 5, NULL);

	sprintf(numbuf, "%-5.1ld", gBonus);
	bonus = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
	DrawText(x, 200, bonus, aWhite);
	sprintf(numbuf, "%-5.1ld", gScore);
	score = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
	DrawText(xt, 220, score, aWhite);
	win->Flush(1);
	Delay(60);

	/* -- Praise them or taunt them as the case may be */
	if (gBonus == 0L) {
		Delay(SOUND_DELAY);
		sound->PlaySound(gNoBonus, 5, NULL);
		while (sound->IsSoundPlaying(0))
			Delay(SOUND_DELAY);
	}

	if (gBonus > 10000L) {
		Delay(SOUND_DELAY);
		sound->PlaySound(gPrettyGood, 5, NULL);
		while (sound->IsSoundPlaying(0))
			Delay(SOUND_DELAY);
	}

	/* -- Count the score down */
	while (gBonus > 500L) {
		HandleEvents(0);
		while (sound->IsSoundPlaying(0))
			Delay(SOUND_DELAY);
		/* Timing hack */
		select_usleep(40000);

		sound->PlaySound(gBonk, 5, NULL);
		gScore += 500;
		gBonus -= 500L;
	
		UnDrawText(x, 200, bonus);
		fontserv->Free_Text(bonus);
		sprintf(numbuf, "%-5.1ld", gBonus);
		bonus = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
		DrawText(x, 200, bonus, aWhite);
		UnDrawText(xt, 220, score);
		fontserv->Free_Text(score);
		sprintf(numbuf, "%-5.1ld", gScore);
		score = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
		DrawText(xt, 220, score, aWhite);

		DrawStatus(false);
		win->Flush(1);
	}
	
	while (sound->IsSoundPlaying(0))
		Delay(SOUND_DELAY);
	sound->PlaySound(gBonk, 5, NULL);

	gScore += gBonus;
	gBonus = 0;
	UnDrawText(x, 200, bonus);
	fontserv->Free_Text(bonus);
	sprintf(numbuf, "%-5.1ld", gBonus);
	bonus = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
	DrawText(x, 200, bonus, aWhite);
	fontserv->Free_Text(bonus);
	UnDrawText(xt, 220, score);
	fontserv->Free_Text(score);
	sprintf(numbuf, "%-5.1ld", gScore);
	score = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
	DrawText(xt, 220, score, aWhite);
	fontserv->Free_Text(score);
	DrawStatus(false);
	win->Flush(1);
	HandleEvents(10);

	/* -- Draw the "next wave" message */
	sprintf(numbuf, "Prepare for Wave %d...", gWave+1);
	text = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
	sw = fontserv->TextWidth(numbuf, geneva, STYLE_BOLD);
	x = (SCREEN_WIDTH - sw)/2;
	DrawText(x, 259, text, clr);
	fontserv->Free_Text(text);
	win->Flush(1);
	HandleEvents(100);

	gGameOn = WasGameOn;
	win->Fade(FADE_STEPS);
}	/* -- DoBonus */


/* ----------------------------------------------------------------- */
/* -- Flash the stars on the screen */

static void TwinkleStars(void)
{
	unsigned long black;
	int	      theStar;

	gLastStar = gOurTicks;
	theStar = FastRandom(MAX_STARS);

	/* -- Draw the star */
	black = win->Map_Color(0x0000, 0x0000, 0x0000);
	win->DrawBGPoint(gTheStars[theStar]->xCoord, 
					gTheStars[theStar]->yCoord, black);
	win->RefreshArea(gTheStars[theStar]->xCoord,
					gTheStars[theStar]->yCoord, 1, 1);
	SetStar(theStar);
	win->DrawBGPoint(gTheStars[theStar]->xCoord, 
			gTheStars[theStar]->yCoord, gTheStars[theStar]->color);
	win->RefreshArea(gTheStars[theStar]->xCoord,
					gTheStars[theStar]->yCoord, 2, 2);
}	/* -- TwinkleStars */

