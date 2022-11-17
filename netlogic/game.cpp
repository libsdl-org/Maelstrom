
#include "Maelstrom_Globals.h"
#include "object.h"
#include "player.h"
#include "netplay.h"
#include "make.h"

#ifdef __WIN95__
/* The Windows 95 version is not allowed for release by Andrew Welch! */
//#define CRIPPLED_DEMO
#endif


#ifdef MOVIE_SUPPORT
extern int  gMovie;
static Rect gMovieRect;
int SelectMovieRect(void)
{
	XEvent	event;
	char    buf[127];
	KeySym  key;
	unsigned char *saved;
	unsigned long  white;
	int center_x, center_y, havecenter;
	int width, height;

	/* Wait for initial button press */
	win->Show_Cursor();
	while ( 1 ) {
		win->GetEvent(&event);

		/* Check for escape key */
		if ( event.type == KeyPress ) {
			win->KeyToAscii(&event, buf, 127, &key);
			if ( key == XK_Escape ) {
				win->Hide_Cursor();
				return(0);
			}
			continue;
		}

		/* Wait for button press */
		if ( event.type == ButtonPress ) {
			center_x = event.xbutton.x;
			center_y = event.xbutton.y;
			break;
		}
	}

	/* Save the screen */
	white = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
	saved = win->Grab_Area(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1);

	/* As the box moves... */
	width = height = 0;
	while ( 1 ) {
		win->GetEvent(&event);

		/* Check for escape key */
		if ( event.type == KeyPress ) {
			win->KeyToAscii(&event, buf, 127, &key);
			if ( key == XK_Escape ) {
				win->Set_Area(0, 0,
					SCREEN_WIDTH-1, SCREEN_HEIGHT-1, saved);
				delete[] saved;
				win->Hide_Cursor();
				win->Refresh();
				return(0);
			}
			continue;
		}

		/* Check for ending button press */
		if ( event.type == ButtonPress ) {
			gMovieRect.left = center_x - width;
			gMovieRect.right = center_x + width;
			gMovieRect.top = center_y - height;
			gMovieRect.bottom = center_y + height;
			win->Set_Area(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1,
									saved);
			delete[] saved;
			win->Hide_Cursor();
			win->Refresh();
			return(1);
		}

		if ( event.type == MotionNotify ) {
			win->Set_Area(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1,
									saved);
			width = abs(event.xmotion.x - center_x);
			height = abs(event.xmotion.y - center_y);
			win->DrawRectangle(center_x-width, center_y-height,
						2*width, 2*height, white);
			win->Refresh();
		}
	}
	/* NEVERREACHED */

}
#endif
extern int RunFrame(void);	/* The heart of blit.cc */

// Global variables set in this file...
int	gGameOn;
int	gPaused;
int	gWave;
int	gBoomDelay;
int	gNextBoom;
int	gBoomPhase;
int	gNumRocks;
int	gLastStar;
int	gWhenDone;
int	gDisplayed;

int	gMultiplierShown;
int	gPrizeShown;
int	gBonusShown;
int	gWhenHoming;
int	gWhenGrav;
int	gWhenDamaged;
int	gWhenNova;
int	gShakeTime;
int	gFreezeTime;
Object *gEnemySprite;
int	gWhenEnemy;

// Local global variables;
static MFont *geneva=NULL;

// Local functions used in the game module of Maelstrom
static void DoHouseKeeping(void);
static void NextWave(void);
static void DoGameOver(void);
static void DoBonus(void);
static void TwinkleStars(void);

/* ----------------------------------------------------------------- */
/* -- Draw the status display */

void DrawStatus(Bool first, Bool ForceDraw)
{
	int             i;
	static	int	nextDraw;
	static  int	lastDisplayed;
	int		Score;
	static	int	lastScore, lastScores[MAX_PLAYERS];
	static	int	lastWave;
	int		Lives;
	static	int	lastLives;
	static	int	lastLife[MAX_PLAYERS];
	int		Bonus;
	static	int	lastBonus;
	int		Frags;
	static int	lastFrags;
	int		AutoFire;
	static	int	lastGun;
	int		AirBrakes;
	static	int	lastBrakes;
	int		ShieldLevel;
	static	int	lastShield;
	int		MultFactor;
	static	int	lastMult;
	int		LongFire;
	static	int	lastLong;
	int		TripleFire;
	static	int	lastTriple;
	int		LuckOfTheIrish;
	static	int	lastLuck;
	static	int	fragoff;
	static	unsigned long ourGrey, ourWhite, ourBlack;
	static  BitMap	*scoretext=NULL;
	static  BitMap	*wavetext=NULL;
	static  BitMap	*livestext=NULL;
	static  BitMap	*bonustext=NULL;
	static  BitMap	*fragstext=NULL;
	static	BitMap	*disptext=NULL;
	BitMap              *text;
	char                 numbuf[128];

	if (first) {
		int x;

		nextDraw = 1;
		lastDisplayed = -1;
		OBJ_LOOP(i, gNumPlayers)
			lastScores[i] = -1;
		lastScore = -1;
		lastWave = -1;
		lastShield = -1;
		lastLives = -1;
		lastBonus = -1;
		lastFrags = -1;
		lastGun = -1;
		lastBrakes = -1;
		lastMult = -1;
		lastLuck = -1;
		if (gWave == 1) {
			OBJ_LOOP(i, gNumPlayers)
				lastLife[i] = 0;
		}
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
		/* Heh, DOOM style frag count */
		if ( gNumPlayers > 1 ) {
			x = 530;
			text = fontserv->Text_to_BitMap("Frags:", geneva,
								STYLE_BOLD);
			DrawText(x, gStatusLine+11, text, ourGrey);
			fontserv->Free_Text(text);
			x += fontserv->TextWidth("Frags:", geneva, STYLE_BOLD);
			fragoff = x+4;
		}
	}

	if ( ForceDraw || (--nextDraw == 0) ) {
		nextDraw = DISPLAY_DELAY+1;
		/* -- Do incremental updates */
	
		if ( (gNumPlayers > 1) && (lastDisplayed != gDisplayed) ) {
			char  buffer[BUFSIZ];

			lastDisplayed = gDisplayed;
			if ( disptext ) {
				UnDrawText(SPRITES_WIDTH, 11, disptext);
				fontserv->Free_Text(disptext);
			}
			sprintf(buffer,
				"You are player %d --- displaying player %d",
						gOurPlayer+1, gDisplayed+1);
			disptext = fontserv->Text_to_BitMap(buffer, geneva,
								STYLE_BOLD);
			DrawText(SPRITES_WIDTH, 11, disptext, ourGrey);

			/* Fill in the color by the frag count */
			win->FillRectangle(518, gStatusLine+4, 4, 8,
							TheShip->Color());
		}

		ShieldLevel = TheShip->GetShieldLevel();
		if (lastShield != ShieldLevel) {
			int	fact;

			lastShield = ShieldLevel;
			win->DrawRectangle(152, gStatusLine+4, SHIELD_WIDTH, 
								8, ourWhite);
			fact = ((SHIELD_WIDTH - 2) * ShieldLevel) / MAX_SHIELD;
			win->FillRectangle(152+1,gStatusLine+4+1, fact, 6,
								ourGrey);
			win->FillRectangle(152+1+fact, gStatusLine+4+1,
						SHIELD_WIDTH-2-fact, 6,
								ourBlack);
		}
		
		MultFactor = TheShip->GetBonusMult();
		if (lastMult != MultFactor) {
			lastMult = MultFactor;
		
			switch (MultFactor) {
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
	
		AutoFire = TheShip->GetSpecial(MACHINE_GUNS);
		if (lastGun != AutoFire) {
			lastGun = AutoFire;

			if ( AutoFire > 0 ) {
				BlitCIcon(438, gStatusLine+4, gAutoFireIcon);
			} else {
				win->FillRectangle(438,
						gStatusLine+4, 8, 8, ourBlack);
			}
		}
	
		AirBrakes = TheShip->GetSpecial(AIR_BRAKES);
		if (lastBrakes != AirBrakes) {
			lastBrakes = AirBrakes;

			if ( AirBrakes > 0 ) {
				BlitCIcon(454, gStatusLine+4, gAirBrakesIcon);
			} else {
				win->FillRectangle(454,
						gStatusLine+4, 8, 8, ourBlack);
			}
		}
	
		LuckOfTheIrish = TheShip->GetSpecial(LUCKY_IRISH);
		if (lastLuck != LuckOfTheIrish) {
			lastLuck = LuckOfTheIrish;

			if ( LuckOfTheIrish > 0 ) {
				BlitCIcon(470, gStatusLine+4, 
							gLuckOfTheIrishIcon);
			} else {
				win->FillRectangle(470,
						gStatusLine+4, 8, 8, ourBlack);
			}
		}
	
		TripleFire = TheShip->GetSpecial(TRIPLE_FIRE);
		if (lastTriple != TripleFire) {
			lastTriple = TripleFire;

			if ( TripleFire > 0 ) {
				BlitCIcon(486, gStatusLine+4, gTripleFireIcon);
			} else {
				win->FillRectangle(486,
						gStatusLine+4, 8, 8, ourBlack);
			}
		}
	
		LongFire = TheShip->GetSpecial(LONG_RANGE);
		if (lastLong != LongFire) {
			lastLong = LongFire;

			if ( LongFire > 0 ) {
				BlitCIcon(502, gStatusLine+4, gLongFireIcon);
			} else {
				win->FillRectangle(502,
						gStatusLine+4, 8, 8, ourBlack);
			}
		}
	
		/* Check for everyone else's new lives */
		OBJ_LOOP(i, gNumPlayers) {
			Score = gPlayers[i]->GetScore();
	
			if ( (i == gDisplayed) && (Score != lastScore) ) {
				/* -- Draw the score */
				if ( scoretext ) {
					UnDrawText(45,gStatusLine+11,scoretext);
					fontserv->Free_Text(scoretext);
				}
				sprintf(numbuf, "%d", Score);
				scoretext = fontserv->Text_to_BitMap(numbuf,
							geneva, STYLE_BOLD);
				DrawText(45,gStatusLine+11,scoretext,ourWhite);
				lastScore = Score;
			}

			if (lastScores[i] == Score)
				continue;

			/* -- See if they got a new life */
			lastScores[i] = Score;
			if ((Score - lastLife[i]) >= NEW_LIFE) {
				gPlayers[i]->IncrLives(1);
				lastLife[i] = (Score / NEW_LIFE) * NEW_LIFE;
				if ( i == gOurPlayer )
					sound->PlaySound(gNewLife, 5, NULL);
			}
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
	
		Lives = TheShip->GetLives();
		if (lastLives != Lives) {
			if ( livestext ) {
				UnDrawText(319, gStatusLine+11, livestext);
				fontserv->Free_Text(livestext);
			}
			lastLives = Lives;
			sprintf(numbuf, "%-3.1d", Lives);
			livestext = fontserv->Text_to_BitMap(numbuf, geneva, 
								STYLE_BOLD);
			DrawText(319, gStatusLine+11, livestext, ourWhite);
		}
	
		Bonus = TheShip->GetBonus();
		if (lastBonus != Bonus) {
			if ( bonustext ) {
				UnDrawText(384, gStatusLine+11, bonustext);
				fontserv->Free_Text(bonustext);
			}
			lastBonus = Bonus;
			sprintf(numbuf, "%-7.1d", Bonus);
			bonustext = fontserv->Text_to_BitMap(numbuf, geneva, 
								STYLE_BOLD);
			DrawText(384, gStatusLine+11, bonustext, ourWhite);
		}

		if ( gNumPlayers > 1 ) {
			Frags = TheShip->GetFrags();
			if (lastFrags != Frags) {
				if ( fragstext ) {
					UnDrawText(fragoff, gStatusLine+11,
								fragstext);
					fontserv->Free_Text(fragstext);
				}
				lastFrags = Frags;
				sprintf(numbuf, "%-3.1d", Frags);
				fragstext = fontserv->Text_to_BitMap(numbuf,
							geneva, STYLE_BOLD);
				DrawText(fragoff, gStatusLine+11, fragstext,
								ourWhite);
			}
		}
	}
	win->RefreshArea(0, gStatusLine, 
				gScrnRect.right-gScrnRect.left, STATUS_HEIGHT);
}	/* -- DrawStatus */


/* ----------------------------------------------------------------- */
/* -- Start a new game */

void NewGame(void)
{
	int i;

	/* Send a "NEW_GAME" packet onto the network */
	if ( gNumPlayers > 1 ) {
		if ( gOurPlayer == 0 ) {
			if ( Send_NewGame(&gStartLevel,&gStartLives,&gNoDelay)
									< 0)
				return;
		} else {
			if ( Await_NewGame(&gStartLevel,&gStartLives,&gNoDelay)
									< 0 )
				return;
		}
	}

#ifdef CRIPPLED_DEMO
	mesg("Warning!!! This is DEMO code only!  DO NOT Release!!!!\n");
#endif
	win->Fade(FADE_STEPS);
	win->Hide_Cursor();

	/* Initialize some game variables */
	gGameOn = 1;
	gPaused = 0;
	gWave = gStartLevel - 1;
	for ( i=gNumPlayers; i--; )
		gPlayers[i]->NewGame(gStartLives);
	gLastStar = STAR_DELAY;
	gLastDrawn = 0L;
	gNumSprites = 0;

	NextWave();

	/* Play the game, dammit! */
	while ((RunFrame() > 0) && gGameOn)
		DoHouseKeeping();
	
/* -- Do the game over stuff */

	DoGameOver();
	win->Show_Cursor();
}	/* -- NewGame */


/* ----------------------------------------------------------------- */
/* -- Do some housekeeping! */

static void DoHouseKeeping(void)
{
	/* Don't do anything if we're paused */
	if ( gPaused ) {
		/* Give up the CPU for a frame duration */
		Delay(FRAME_DELAY);
		return;
	}

#ifdef MOVIE_SUPPORT
	if ( gMovie )
		win->ScreenDump("MovieFrame", &gMovieRect);
#endif
	/* -- Maybe throw a multiplier up on the screen */
	if (gMultiplierShown && (--gMultiplierShown == 0) )
		MakeMultiplier();
	
	/* -- Maybe throw a prize(!) up on the screen */
	if (gPrizeShown && (--gPrizeShown == 0) )
		MakePrize();
	
	/* -- Maybe throw a bonus up on the screen */
	if (gBonusShown && (--gBonusShown == 0) )
		MakeBonus();

	/* -- Maybe make a nasty enemy fighter? */
	if (gWhenEnemy && (--gWhenEnemy == 0) )
		MakeEnemy();

	/* -- Maybe create a transcenfugal vortex */
	if (gWhenGrav && (--gWhenGrav == 0) )
		MakeGravity();
	
	/* -- Maybe create a recified space vehicle */
	if (gWhenDamaged && (--gWhenDamaged == 0) )
		MakeDamagedShip();
	
	/* -- Maybe create a autonominous tracking device */
	if (gWhenHoming && (--gWhenHoming == 0) )
		MakeHoming();
	
	/* -- Maybe make a supercranial destruction thang */
	if (gWhenNova && (--gWhenNova == 0) )
		MakeNova();

	/* -- Maybe create a new star ? */
	if ( --gLastStar == 0 ) {
		gLastStar = STAR_DELAY;
		TwinkleStars();
	}
	
	/* -- Time for the next wave? */
	if (gNumRocks == 0) {
		if ( gWhenDone == 0 )
			gWhenDone = DEAD_DELAY;
		else if ( --gWhenDone == 0 )
			NextWave();
	}
	
#ifdef CRIPPLED_DEMO
	if ( ! TheShip->Alive() )
		gGameOn = 0;
#endif
	/* -- Housekeping */
	DrawStatus(false, false);
}	/* -- DoHouseKeeping */


/* ----------------------------------------------------------------- */
/* -- Start the next wave! */

static void NextWave(void)
{
	int	index, x, y;
	int	NewRoids;
	short	temp;

	gEnemySprite = NULL;

	/* -- Initialize some variables */
	gDisplayed = gOurPlayer;
	gNumRocks = 0;
	gShakeTime = 0;
	gFreezeTime = 0;

	if (gWave != (gStartLevel - 1))
		DoBonus();

	gWave++;

	/* See about the Multiplier */
	if ( FastRandom(2) )
		gMultiplierShown = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gMultiplierShown = 0;

	/* See about the Prize */
	if ( FastRandom(2) )
		gPrizeShown = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gPrizeShown = 0;

	/* See about the Bonus */
	if ( FastRandom(2) )
		gBonusShown = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gBonusShown = 0;

	/* See about the Gravity */
	if (FastRandom(10 + gWave) > 11)
		gWhenGrav = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gWhenGrav = 0;

	/* See about the Nova */
	if (FastRandom(10 + gWave) > 13)
		gWhenNova = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gWhenNova = 0;

	/* See about the Enemy */
	if (FastRandom(3) == 0)
		gWhenEnemy = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gWhenEnemy = 0;

	/* See about the Damaged Ship */
	if (FastRandom(10) == 0)
		gWhenDamaged = ((FastRandom(60) * 60L)/FRAME_DELAY);
	else
		gWhenDamaged = 0;

	/* See about the Homing Mine */
	if (FastRandom(10 + gWave) > 12)
		gWhenHoming = ((FastRandom(60) * 60L)/FRAME_DELAY);
	else
		gWhenHoming = 0;

	temp = gWave / 4;
	if (temp < 1)
		temp = 1;

	NewRoids = FastRandom(temp) + (gWave / 5) + 3;

	/* -- Black the screen out and draw the wave */
	win->Clear();

	/* -- Kill any existing sprites */
	while (gNumSprites > 0)
		delete gSprites[gNumSprites-1];

	/* -- Initialize some variables */
	gLastDrawn = 0L;
	gBoomDelay = (60/FRAME_DELAY);
	gNextBoom = gBoomDelay;
	gBoomPhase = 0;
	gWhenDone = 0;

	/* -- Create the ship's sprite */
	for ( index=gNumPlayers; index--; )
		gPlayers[index]->NewWave();
	DrawStatus(true, false);

	/* -- Create some asteroids */
	for (index = 0; index < NewRoids; index++) {
		int	randval;
	
		x = FastRandom(SCREEN_WIDTH) * SCALE_FACTOR;
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
/* -- Do the game over display */

struct FinalScore {
	int Player;
	int Score;
	int Frags;
	};

static int cmp_byscore(const void *A, const void *B)
{
	return(((struct FinalScore *)B)->Score - ((struct FinalScore *)A)->Score);
}
static int cmp_byfrags(const void *A, const void *B)
{
	return(((struct FinalScore *)B)->Frags - ((struct FinalScore *)A)->Frags);
}

static void DoGameOver(void)
{
	struct	Title gameover;
	MFont        *newyork;
	BitMap       *text;
	int	      xOff, yOff, index, x;
	int	      which = -1, count, i;
	char          handle[20];
	int           chars_in_handle = 0;
	Bool          done = false;
	unsigned long clr;

	/* Get the final scoring */
	struct FinalScore *final = new struct FinalScore[gNumPlayers];
	for ( i=0; i<gNumPlayers; ++i ) {
		final[i].Player = i+1;
		final[i].Score = gPlayers[i]->GetScore();
		final[i].Frags = gPlayers[i]->GetFrags();
	}
	if ( gDeathMatch )
		qsort(final,gNumPlayers,sizeof(struct FinalScore),cmp_byfrags);
	else
		qsort(final,gNumPlayers,sizeof(struct FinalScore),cmp_byscore);

	win->Fade(FADE_STEPS);
	sound->HaltSounds();

	/* -- Kill any existing sprites */
	while (gNumSprites > 0)
		delete gSprites[gNumSprites-1];

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

	/* Show the player ranking */
	if ( gNumPlayers > 1 ) {
		clr = win->Map_Color(30000, 30000, 0xFFFF);
		if ( (newyork = fontserv->New_Font("New York", 18)) == NULL ) {
                        error("Can't use New York font! -- Exiting.\n");
                        exit(255);
                }
		for ( i=0; i<gNumPlayers; ++i ) {
			char buffer[BUFSIZ], num1[12], num2[12];

			sprintf(num1, "%7.1d", final[i].Score);
			sprintf(num2, "%3.1d", final[i].Frags);
			sprintf(buffer, "Player %d: %-.7s Points, %-.3s Frags",
						final[i].Player, num1, num2);
			text = fontserv->Text_to_BitMap(buffer,
							newyork, STYLE_NORM);
			DrawText(160, 380+i*text->height, text, clr);
			fontserv->Free_Text(text);
		}
		fontserv->Free_Font(newyork);
	}
	win->Flush(1);

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
		if ( OurShip->GetScore() > hScores[index].score ) {
			which = index;
			break;
		}
	}

	/* -- They got a high score! */
	gLastHigh = which;

	if ((which != -1) && (gStartLevel == 1) && (gStartLives == 3) &&
					(gNumPlayers == 1) && !gDeathMatch ) {
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
		hScores[which].score = OurShip->GetScore();
		strcpy(hScores[which].name, handle);

		sound->HaltSounds();
		sound->PlaySound(gGotPrize, 6, NULL);
		if ( gNetScores )	// All time high!
			RegisterHighScore(hScores[which]);
		else
			SaveScores();
	} else
	if ( gNumPlayers > 1 )	/* Let them watch their ranking */
		sleep(3);

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
	int           i, x, sw, xs, xt;
	BitMap       *text, *score, *bonus;
	char          numbuf[128];

	DrawStatus(false, true);

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

	OBJ_LOOP(i, gNumPlayers) {
		if ( i != gOurPlayer ) {
			gPlayers[i]->MultBonus();
			continue;
		}

		if (OurShip->GetBonusMult() != 1) {
			CSprite        *sprite;

			sprintf(numbuf, "%-5.1d", OurShip->GetBonus());
			text = fontserv->Text_to_BitMap(numbuf, geneva,
								STYLE_BOLD);
			DrawText(x, 200, text, aWhite);
			fontserv->Free_Text(text);

			x += 75;
			OurShip->MultBonus();
			Delay(SOUND_DELAY);
			sound->PlaySound(gMultiplier, 5, NULL);
			sprite = gMult[OurShip->GetBonusMult()-2]->sprite[0];
			win->Blit_CSprite(xs+34, 180, sprite);
			Delay(60);
		}
	}
	Delay(SOUND_DELAY);
	sound->PlaySound(gFunk, 5, NULL);

	sprintf(numbuf, "%-5.1d", OurShip->GetBonus());
	bonus = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
	DrawText(x, 200, bonus, aWhite);
	sprintf(numbuf, "%-5.1d", OurShip->GetScore());
	score = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
	DrawText(xt, 220, score, aWhite);
	win->Flush(1);
	Delay(60);

	/* -- Praise them or taunt them as the case may be */
	if (OurShip->GetBonus() == 0) {
		Delay(SOUND_DELAY);
		sound->PlaySound(gNoBonus, 5, NULL);
		while (sound->IsSoundPlaying(0))
			Delay(SOUND_DELAY);
	}

	if (OurShip->GetBonus() > 10000) {
		Delay(SOUND_DELAY);
		sound->PlaySound(gPrettyGood, 5, NULL);
		while (sound->IsSoundPlaying(0))
			Delay(SOUND_DELAY);
	}

	/* -- Count the score down */
	OBJ_LOOP(i, gNumPlayers) {
		if ( i != gOurPlayer ) {
			while ( gPlayers[i]->GetBonus() > 500 ) {
				gPlayers[i]->IncrScore(500);
				gPlayers[i]->IncrBonus(-500);
			}
			continue;
		}

		while (OurShip->GetBonus() > 500) {
			HandleEvents(0);
			while (sound->IsSoundPlaying(0))
				Delay(SOUND_DELAY);
			/* Timing hack */
			select_usleep(40000);

			sound->PlaySound(gBonk, 5, NULL);
			OurShip->IncrScore(500);
			OurShip->IncrBonus(-500);
	
			UnDrawText(x, 200, bonus);
			fontserv->Free_Text(bonus);
			sprintf(numbuf, "%-5.1d", OurShip->GetBonus());
			bonus = fontserv->Text_to_BitMap(numbuf, geneva,
								STYLE_BOLD);
			DrawText(x, 200, bonus, aWhite);
			UnDrawText(xt, 220, score);
			fontserv->Free_Text(score);
			sprintf(numbuf, "%-5.1d", OurShip->GetScore());
			score = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
			DrawText(xt, 220, score, aWhite);

			DrawStatus(false, true);
			win->Flush(1);
		}
	}
	
	while (sound->IsSoundPlaying(0))
		Delay(SOUND_DELAY);
	sound->PlaySound(gBonk, 5, NULL);

	OurShip->IncrScore(OurShip->GetBonus());
	OurShip->IncrBonus(-OurShip->GetBonus());
	UnDrawText(x, 200, bonus);
	fontserv->Free_Text(bonus);
	sprintf(numbuf, "%-5.1d", OurShip->GetBonus());	// Duh, this is 0.
	bonus = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
	DrawText(x, 200, bonus, aWhite);
	fontserv->Free_Text(bonus);
	UnDrawText(xt, 220, score);
	fontserv->Free_Text(score);
	sprintf(numbuf, "%-5.1d", OurShip->GetScore());
	score = fontserv->Text_to_BitMap(numbuf, geneva, STYLE_BOLD);
	DrawText(xt, 220, score, aWhite);
	fontserv->Free_Text(score);
	DrawStatus(false, true);
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

	win->Fade(FADE_STEPS);
}	/* -- DoBonus */


/* ----------------------------------------------------------------- */
/* -- Flash the stars on the screen */

static void TwinkleStars(void)
{
	unsigned long black;
	int	      theStar;

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
					gTheStars[theStar]->yCoord, 1, 1);
}	/* -- TwinkleStars */

