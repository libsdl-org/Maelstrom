
#include "Maelstrom_Globals.h"
#include "object.h"
#include "player.h"
#include "netplay.h"
#include "make.h"
#include "load.h"


#ifdef MOVIE_SUPPORT
extern int gMovie;
static SDL_Rect gMovieRect;
int SelectMovieRect(void)
{
	SDL_Event event;
	SDL_Surface *saved;
	Uint32 white;
	int center_x, center_y;
	int width, height;

	/* Wait for initial button press */
	screen->ShowCursor();
	center_x = 0;
	center_y = 0;
	while ( ! center_x && ! center_y ) {
		screen->WaitEvent(&event);

		/* Check for escape key */
		if ( (event.type == SDL_KEYEVENT) && 
				(event.key.state == SDL_PRESSED) &&
				(event.key.keysym.sym == SDL_ESCAPE) ) {
			screen->HideCursor();
			return(0);
		}

		/* Wait for button press */
		if ( (event.type == SDL_MOUSEBUTTONEVENT) && 
				(event.button.state == SDL_PRESSED) ) {
			center_x = event.button.x;
			center_y = event.button.y;
			break;
		}
	}

	/* Save the screen */
	white = screen->MapRGB(0xFFFF, 0xFFFF, 0xFFFF);
	saved = screen->GrabArea(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	/* As the box moves... */
	width = height = 0;
	while ( 1 ) {
		win->GetEvent(&event);

		/* Check for escape key */
		if ( (event.type == SDL_KEYEVENT) && 
				(event.key.state == SDL_PRESSED) &&
				(event.key.keysym.sym == SDL_ESCAPE) ) {
			screen->QueueBlit(0, 0, saved, NOCLIP);
			screen->Update();
			screen->FreeImage(saved);
			win->HideCursor();
			return(0);
		}

		/* Check for ending button press */
		if ( event.type == ButtonPress ) {
			gMovieRect.x = center_x - width;
			gMovieRect.y = center_y - height;
			gMovieRect.w = 2*width;
			gMovieRect.h = 2*height;
			screen->QueueBlit(0, 0, saved, NOCLIP);
			screen->Update();
			screen->FreeImage(saved);
			win->HideCursor();
			return(1);
		}

		if ( event.type == MotionNotify ) {
			screen->QueueBlit(0, 0, saved, NOCLIP);
			screen->Update();
			width = abs(event.motion.x - center_x);
			height = abs(event.motion.y - center_y);
			screen->DrawRect(center_x-width, center_y-height,
						2*width, 2*height, white);
			screen->Update();
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
static Uint32 ourGrey, ourWhite, ourBlack;
static int text_height;

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
	static int nextDraw;
	static int lastDisplayed;
	int		Score;
	static int	lastScore, lastScores[MAX_PLAYERS];
	static int	lastWave;
	int		Lives;
	static	int	lastLives;
	static int	lastLife[MAX_PLAYERS];
	int		Bonus;
	static int	lastBonus;
	int		Frags;
	static int	lastFrags;
	int		AutoFire;
	static int	lastGun;
	int		AirBrakes;
	static int	lastBrakes;
	int		ShieldLevel;
	static int	lastShield;
	int		MultFactor;
	static int	lastMult;
	int		LongFire;
	static int	lastLong;
	int		TripleFire;
	static int	lastTriple;
	int		LuckOfTheIrish;
	static int	lastLuck;
	static int	fragoff;
	static int score_width, wave_width;
	static int lives_width, bonus_width;
	static int frags_width;
	int i;
	char numbuf[128];

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
	
		score_width = 0;
		wave_width = 0;
		lives_width = 0;
		bonus_width = 0;
		frags_width = 0;
	
/* -- Draw the status display */

		screen->DrawLine(0, gStatusLine, SCREEN_WIDTH-1, 
							gStatusLine, ourWhite);
		x = 3;
		i = DrawText(x, gStatusLine+11, "Score:", geneva, STYLE_BOLD,
						30000>>8, 30000>>8, 0xFF);
		x += (i+70);
		i = DrawText(x, gStatusLine+11, "Shield:", geneva, STYLE_BOLD,
						30000>>8, 30000>>8, 0xFF);
		x += (i+70);
		i = DrawText(x, gStatusLine+11, "Wave:", geneva, STYLE_BOLD,
						30000>>8, 30000>>8, 0xFF);
		x += (i+30);
		i = DrawText(x, gStatusLine+11, "Lives:", geneva, STYLE_BOLD,
						30000>>8, 30000>>8, 0xFF);
		x += (i+30);
		DrawText(x, gStatusLine+11, "Bonus:", geneva, STYLE_BOLD,
						30000>>8, 30000>>8, 0xFF);
		/* Heh, DOOM style frag count */
		if ( gNumPlayers > 1 ) {
			x = 530;
			i = DrawText(x, gStatusLine+11, "Frags:", geneva,
					STYLE_BOLD, 30000>>8, 30000>>8, 0xFF);
			fragoff = x+i+4;
		}
	}

	if ( ForceDraw || (--nextDraw == 0) ) {
		nextDraw = DISPLAY_DELAY+1;
		/* -- Do incremental updates */
	
		if ( (gNumPlayers > 1) && (lastDisplayed != gDisplayed) ) {
			char caption[BUFSIZ];

			lastDisplayed = gDisplayed;
			screen->FillRect(0, 0, SCREEN_WIDTH, 12, ourBlack);
			sprintf(caption,
				"You are player %d --- displaying player %d",
						gOurPlayer+1, gDisplayed+1);
			DrawText(SPRITES_WIDTH, 11, caption, geneva,
					STYLE_BOLD, 30000>>8, 30000>>8, 0xFF);

			/* Fill in the color by the frag count */
			screen->FillRect(518, gStatusLine+4, 4, 8,
							TheShip->Color());
		}

		ShieldLevel = TheShip->GetShieldLevel();
		if (lastShield != ShieldLevel) {
			int	fact;

			lastShield = ShieldLevel;
			screen->DrawRect(152, gStatusLine+4, SHIELD_WIDTH, 
								8, ourWhite);
			fact = ((SHIELD_WIDTH - 2) * ShieldLevel) / MAX_SHIELD;
			screen->FillRect(152+1,gStatusLine+4+1, fact, 6,
								ourGrey);
			screen->FillRect(152+1+fact, gStatusLine+4+1,
					SHIELD_WIDTH-2-fact, 6, ourBlack);
		}
		
		MultFactor = TheShip->GetBonusMult();
		if (lastMult != MultFactor) {
			lastMult = MultFactor;
		
			switch (MultFactor) {
				case 1:	screen->FillRect(424,
						gStatusLine+4, 8, 8, ourBlack);
					break;
				case 2:	screen->QueueBlit(424, gStatusLine+4,
							gMult2Icon, NOCLIP);
					break;
				case 3:	screen->QueueBlit(424, gStatusLine+4,
							gMult3Icon, NOCLIP);
					break;
				case 4:	screen->QueueBlit(424, gStatusLine+4,
							gMult4Icon, NOCLIP);
					break;
				case 5:	screen->QueueBlit(424, gStatusLine+4,
							gMult5Icon, NOCLIP);
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
				screen->QueueBlit(438, gStatusLine+4,
						gAutoFireIcon, NOCLIP);
			} else {
				screen->FillRect(438, gStatusLine+4, 8, 8,
								ourBlack);
			}
		}
	
		AirBrakes = TheShip->GetSpecial(AIR_BRAKES);
		if (lastBrakes != AirBrakes) {
			lastBrakes = AirBrakes;

			if ( AirBrakes > 0 ) {
				screen->QueueBlit(454, gStatusLine+4,
						gAirBrakesIcon, NOCLIP);
			} else {
				screen->FillRect(454, gStatusLine+4, 8, 8,
								ourBlack);
			}
		}
	
		LuckOfTheIrish = TheShip->GetSpecial(LUCKY_IRISH);
		if (lastLuck != LuckOfTheIrish) {
			lastLuck = LuckOfTheIrish;

			if ( LuckOfTheIrish > 0 ) {
				screen->QueueBlit(470, gStatusLine+4, 
						gLuckOfTheIrishIcon, NOCLIP);
			} else {
				screen->FillRect(470, gStatusLine+4, 8, 8,
								ourBlack);
			}
		}
	
		TripleFire = TheShip->GetSpecial(TRIPLE_FIRE);
		if (lastTriple != TripleFire) {
			lastTriple = TripleFire;

			if ( TripleFire > 0 ) {
				screen->QueueBlit(486, gStatusLine+4,
						gTripleFireIcon, NOCLIP);
			} else {
				screen->FillRect(486, gStatusLine+4, 8, 8,
								ourBlack);
			}
		}
	
		LongFire = TheShip->GetSpecial(LONG_RANGE);
		if (lastLong != LongFire) {
			lastLong = LongFire;

			if ( LongFire > 0 ) {
				screen->QueueBlit(502, gStatusLine+4,
						gLongFireIcon, NOCLIP);
			} else {
				screen->FillRect(502, gStatusLine+4, 8, 8,
								ourBlack);
			}
		}
	
		/* Check for everyone else's new lives */
		OBJ_LOOP(i, gNumPlayers) {
			Score = gPlayers[i]->GetScore();
	
			if ( (i == gDisplayed) && (Score != lastScore) ) {
				/* -- Erase old and draw new score */
				screen->FillRect(45, gStatusLine+1,
					score_width, text_height, ourBlack);
				sprintf(numbuf, "%d", Score);
				score_width = DrawText(45, gStatusLine+11, 
						numbuf, geneva, STYLE_BOLD,
							0xFF, 0xFF, 0xFF);
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
					sound->PlaySound(gNewLife, 5);
			}
		}
	
		if (lastWave != gWave) {
			screen->FillRect(255, gStatusLine+1,
					wave_width, text_height, ourBlack);
			sprintf(numbuf, "%d", gWave);
			wave_width = DrawText(255, gStatusLine+11, 
					numbuf, geneva, STYLE_BOLD,
							0xFF, 0xFF, 0xFF);
			lastWave = gWave;
		}
	
		Lives = TheShip->GetLives();
		if (lastLives != Lives) {
			screen->FillRect(319, gStatusLine+1,
					lives_width, text_height, ourBlack);
			sprintf(numbuf, "%-3.1d", Lives);
			lives_width = DrawText(319, gStatusLine+11,
					numbuf, geneva, STYLE_BOLD,
							0xFF, 0xFF, 0xFF);
			lastLives = Lives;
		}
	
		Bonus = TheShip->GetBonus();
		if (lastBonus != Bonus) {
			screen->FillRect(384, gStatusLine+1,
					bonus_width, text_height, ourBlack);
			sprintf(numbuf, "%-7.1d", Bonus);
			bonus_width = DrawText(384, gStatusLine+11,
					numbuf, geneva, STYLE_BOLD,
							0xFF, 0xFF, 0xFF);
			lastBonus = Bonus;
		}

		if ( gNumPlayers > 1 ) {
			Frags = TheShip->GetFrags();
			if (lastFrags != Frags) {
				screen->FillRect(fragoff, gStatusLine+1,
					frags_width, text_height, ourBlack);
				sprintf(numbuf, "%-3.1d", Frags);
				frags_width = DrawText(fragoff, gStatusLine+11,
						numbuf, geneva, STYLE_BOLD,
							0xFF, 0xFF, 0xFF);
				lastFrags = Frags;
			}
		}
	}
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

	/* Load the font and colors we use everywhere */
	if ( (geneva = fontserv->NewFont("Geneva", 9)) == NULL ) {
		error("Can't use Geneva font! -- Exiting.\n");
		exit(255);
	}
	text_height = fontserv->TextHeight(geneva);
	ourGrey = screen->MapRGB(30000>>8, 30000>>8, 0xFF);
	ourWhite = screen->MapRGB(0xFF, 0xFF, 0xFF);
	ourBlack = screen->MapRGB(0x00, 0x00, 0x00);

	/* Fade into game mode */
	screen->Fade();
	screen->HideCursor();

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
	while ( (RunFrame() > 0) && gGameOn )
		DoHouseKeeping();
	
/* -- Do the game over stuff */

	DoGameOver();
	screen->ShowCursor();
	delete geneva;
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
	screen->Clear();

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
	screen->Update();

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
	screen->FocusBG();
	for ( index=0; index<MAX_STARS; ++index ) {
		screen->DrawPoint(gTheStars[index]->xCoord, 
			gTheStars[index]->yCoord, gTheStars[index]->color);
	}
	screen->Update(1);
	screen->FocusFG();
	screen->Fade();
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
	return(((struct FinalScore *)B)->Score-((struct FinalScore *)A)->Score);
}
static int cmp_byfrags(const void *A, const void *B)
{
	return(((struct FinalScore *)B)->Frags-((struct FinalScore *)A)->Frags);
}

static void DoGameOver(void)
{
	SDL_Event event;
	SDL_Surface *gameover;
	MFont *newyork;
	int newyork_height, w, x;
	int which = -1, i;
	char handle[20];
	Uint8 key;
	int chars_in_handle = 0;
	Bool done = false;

	/* Get the final scoring */
	struct FinalScore *final = new struct FinalScore[gNumPlayers];
	for ( i=0; i<gNumPlayers; ++i ) {
		final[i].Player = i+1;
		final[i].Score = gPlayers[i]->GetScore();
		final[i].Frags = gPlayers[i]->GetFrags();
	}
#ifndef macintosh
	if ( gDeathMatch )
		qsort(final,gNumPlayers,sizeof(struct FinalScore),cmp_byfrags);
	else
		qsort(final,gNumPlayers,sizeof(struct FinalScore),cmp_byscore);
#endif

	screen->Fade();
	sound->HaltSound();

	/* -- Kill any existing sprites */
	while (gNumSprites > 0)
		delete gSprites[gNumSprites-1];

	/* -- Clear the screen */
	screen->FillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ourBlack);

	/* -- Draw the game over picture */
	gameover = Load_Title(screen, 128);
	if ( gameover == NULL ) {
		error("Can't load 'gameover' title!\n");
		exit(255);
	}
	screen->QueueBlit((SCREEN_WIDTH-gameover->w)/2,
			((SCREEN_HEIGHT-gameover->h)/2)-80, gameover, NOCLIP);
	screen->FreeImage(gameover);

	/* Show the player ranking */
	if ( gNumPlayers > 1 ) {
		if ( (newyork = fontserv->NewFont("New York", 18)) == NULL ) {
                        error("Can't use New York font! -- Exiting.\n");
                        exit(255);
                }
		newyork_height = fontserv->TextHeight(newyork);
		for ( i=0; i<gNumPlayers; ++i ) {
			char buffer[BUFSIZ], num1[12], num2[12];

			sprintf(num1, "%7.1d", final[i].Score);
			sprintf(num2, "%3.1d", final[i].Frags);
			sprintf(buffer, "Player %d: %-.7s Points, %-.3s Frags",
						final[i].Player, num1, num2);
			DrawText(160, 380+i*newyork_height, buffer,
				newyork, STYLE_NORM, 30000>>8, 30000>>8, 0xFF);
		}
		delete newyork;
	}
	screen->Update();

	/* -- Play the game over sound */
	sound->PlaySound(gGameOver, 5);
	screen->Fade();

	while( sound->Playing() )
		Delay(SOUND_DELAY);

	/* -- See if they got a high score */
	LoadScores();
	for ( i = 0; i<10; ++i ) {
		if ( OurShip->GetScore() > hScores[i].score ) {
			which = i;
			break;
		}
	}

	/* -- They got a high score! */
	gLastHigh = which;

	if ((which != -1) && (gStartLevel == 1) && (gStartLives == 3) &&
					(gNumPlayers == 1) && !gDeathMatch ) {
		sound->PlaySound(gBonusShot, 5);
		for ( i = 8; i >= which ; --i ) {
			hScores[i + 1].score = hScores[i].score;
			hScores[i + 1].wave = hScores[i].wave;
			strcpy(hScores[i+1].name, hScores[i].name);
		}

		/* -- Draw the "Enter your name" string */
		if ( (newyork = fontserv->NewFont("New York", 18)) == NULL ) {
                        error("Can't use New York font! -- Exiting.\n");
                        exit(255);
                }
		newyork_height = fontserv->TextHeight(newyork);
		x = (SCREEN_WIDTH-(fontserv->TextWidth("Enter your name: ",
						newyork, STYLE_NORM)*2))/2;
		x += DrawText(x, 300, "Enter your name: ",
				newyork, STYLE_NORM, 30000>>8, 30000>>8, 0xFF);
		screen->Update();

		/* -- Let them enter their name */
		w = 0;
		chars_in_handle = 0;

		while ( screen->PollEvent(&event) ) /* Loop, flushing events */;
		SDL_EnableUNICODE(1);
		while ( !done ) {
			screen->WaitEvent(&event);

			/* -- Handle key down's (no UNICODE support) */
			if ( event.type == SDL_KEYDOWN ) {
				key = (Uint8)event.key.keysym.unicode;
				switch ( key  ) {
					case '\0':	// Ignore NUL char
					case '\033':	// Ignore ESC char
					case '\t':	// Ignore TAB too.
						continue;
					case '\003':
					case '\r':
					case '\n':
						done = true;
						continue;
					case 127:
					case '\b':
						if ( chars_in_handle ) {
							sound->PlaySound(gExplosionSound, 5);
							--chars_in_handle;
						}
						break;
					default:
						if ( chars_in_handle < 15 ) {
							sound->PlaySound(gShotSound, 5);
							handle[chars_in_handle++] = (char)key;
						} else
							sound->PlaySound(gBonk, 5);
						break;
				}
				screen->FillRect(x, 300-newyork_height+2,
						w, newyork_height, ourBlack);

				handle[chars_in_handle] = '\0';
				w = DrawText(x, 300, handle,
					newyork, STYLE_NORM, 0xFF, 0xFF, 0xFF);
				screen->Update();
			}
		}
		delete newyork;
		SDL_EnableUNICODE(0);

		/* In case the user just pressed <Return> */
		handle[chars_in_handle] = '\0';

		hScores[which].wave = gWave;
		hScores[which].score = OurShip->GetScore();
		strcpy(hScores[which].name, handle);

		sound->HaltSound();
		sound->PlaySound(gGotPrize, 6);
		if ( gNetScores )	// All time high!
			RegisterHighScore(hScores[which]);
		else
			SaveScores();
	} else
	if ( gNumPlayers > 1 )	/* Let them watch their ranking */
		SDL_Delay(3000);

	while ( sound->Playing() )
		Delay(SOUND_DELAY);
	HandleEvents(0);

	screen->Fade();
	gUpdateBuffer = true;
}	/* -- DoGameOver */


/* ----------------------------------------------------------------- */
/* -- Do the bonus display */

static void DoBonus(void)
{
	int i, x, sw, xs, xt;
	int bonus_width;
	int score_width;
	char numbuf[128];

	DrawStatus(false, true);
	screen->Update();

	/* -- Now do the bonus */
	sound->HaltSound();
	sound->PlaySound(gRiff, 6);

	/* Fade out */
	screen->Fade();

	/* -- Clear the screen */
	screen->FillRect(0, 0, SCREEN_WIDTH, gStatusLine-1, ourBlack);
	

	/* -- Draw the wave completed message */
	sprintf(numbuf, "Wave %d completed.", gWave);
	sw = fontserv->TextWidth(numbuf, geneva, STYLE_BOLD);
	x = (SCREEN_WIDTH - sw) / 2;
	DrawText(x,  150, numbuf, geneva, STYLE_BOLD, 0xFF, 0xFF, 0x00);

	/* -- Draw the bonus */
	sw = fontserv->TextWidth("Bonus Score:     ", geneva, STYLE_BOLD);
	x = ((SCREEN_WIDTH - sw) / 2) - 20;
	DrawText(x, 200, "Bonus Score:     ", geneva, STYLE_BOLD,
						30000>>8, 30000>>8, 0xFF);
	xt = x+sw;

	/* -- Draw the score */
	sw = fontserv->TextWidth("Score:     ", geneva, STYLE_BOLD);
	x = ((SCREEN_WIDTH - sw) / 2) - 3;
	DrawText(x, 220, "Score:     ", geneva, STYLE_BOLD,
						30000>>8, 30000>>8, 0xFF);
	xs = x+sw;
	screen->Update();

	/* Fade in */
	screen->Fade();
	while ( sound->Playing() )
		Delay(SOUND_DELAY);

	/* -- Count the score down */
	x = xs;

	OBJ_LOOP(i, gNumPlayers) {
		if ( i != gOurPlayer ) {
			gPlayers[i]->MultBonus();
			continue;
		}

		if (OurShip->GetBonusMult() != 1) {
			SDL_Surface *sprite;

			sprintf(numbuf, "%-5.1d", OurShip->GetBonus());
			DrawText(x, 200, numbuf, geneva, STYLE_BOLD,
							0xFF, 0xFF, 0xFF);
			x += 75;
			OurShip->MultBonus();
			Delay(SOUND_DELAY);
			sound->PlaySound(gMultiplier, 5);
			sprite = gMult[OurShip->GetBonusMult()-2]->sprite[0];
			screen->QueueBlit(xs+34, 180, sprite);
			screen->Update();
			Delay(60);
		}
	}
	Delay(SOUND_DELAY);
	sound->PlaySound(gFunk, 5);

	sprintf(numbuf, "%-5.1d", OurShip->GetBonus());
	bonus_width = DrawText(x, 200, numbuf, geneva, STYLE_BOLD,
							0xFF, 0xFF, 0xFF);
	sprintf(numbuf, "%-5.1d", OurShip->GetScore());
	score_width = DrawText(xt, 220, numbuf, geneva, STYLE_BOLD,
							0xFF, 0xFF, 0xFF);
	screen->Update();
	Delay(60);

	/* -- Praise them or taunt them as the case may be */
	if (OurShip->GetBonus() == 0) {
		Delay(SOUND_DELAY);
		sound->PlaySound(gNoBonus, 5);
	}
	if (OurShip->GetBonus() > 10000) {
		Delay(SOUND_DELAY);
		sound->PlaySound(gPrettyGood, 5);
	}
	while ( sound->Playing() )
		Delay(SOUND_DELAY);

	/* -- Count the score down */
	OBJ_LOOP(i, gNumPlayers) {
		if ( i != gOurPlayer ) {
			while ( gPlayers[i]->GetBonus() > 500 ) {
				gPlayers[i]->IncrScore(500);
				gPlayers[i]->IncrBonus(-500);
			}
			continue;
		}

		while (OurShip->GetBonus() > 0) {
			while ( sound->Playing() )
				Delay(SOUND_DELAY);

			sound->PlaySound(gBonk, 5);
			if ( OurShip->GetBonus() >= 500 ) {
				OurShip->IncrScore(500);
				OurShip->IncrBonus(-500);
			} else {
				OurShip->IncrScore(OurShip->GetBonus());
				OurShip->IncrBonus(-OurShip->GetBonus());
			}
	
			screen->FillRect(x, 200-text_height+2,
					bonus_width, text_height, ourBlack);
			sprintf(numbuf, "%-5.1d", OurShip->GetBonus());
			bonus_width = DrawText(x, 200, numbuf,
					geneva, STYLE_BOLD, 0xFF, 0xFF, 0xFF);
			screen->FillRect(xt, 220-text_height+2,
					score_width, text_height, ourBlack);
			sprintf(numbuf, "%-5.1d", OurShip->GetScore());
			score_width = DrawText(xt, 220, numbuf,
					geneva, STYLE_BOLD, 0xFF, 0xFF, 0xFF);

			DrawStatus(false, true);
			screen->Update();
		}
	}
	while ( sound->Playing() )
		Delay(SOUND_DELAY);
	HandleEvents(10);

	/* -- Draw the "next wave" message */
	sprintf(numbuf, "Prepare for Wave %d...", gWave+1);
	sw = fontserv->TextWidth(numbuf, geneva, STYLE_BOLD);
	x = (SCREEN_WIDTH - sw)/2;
	DrawText(x, 259, numbuf, geneva, STYLE_BOLD, 0xFF, 0xFF, 0x00);
	screen->Update();
	HandleEvents(100);

	screen->Fade();
}	/* -- DoBonus */


/* ----------------------------------------------------------------- */
/* -- Flash the stars on the screen */

static void TwinkleStars(void)
{
	int theStar;

	theStar = FastRandom(MAX_STARS);

	/* -- Draw the star */
	screen->FocusBG();
	screen->DrawPoint(gTheStars[theStar]->xCoord, 
					gTheStars[theStar]->yCoord, ourBlack);
	SetStar(theStar);
	screen->DrawPoint(gTheStars[theStar]->xCoord, 
			gTheStars[theStar]->yCoord, gTheStars[theStar]->color);
	screen->Update(1);
	screen->FocusFG();
}	/* -- TwinkleStars */

