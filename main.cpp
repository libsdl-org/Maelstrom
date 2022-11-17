/* ------------------------------------------------------------- */
/* 								 */
/* Maelstrom							 */
/* By Andrew Welch						 */
/* 								 */
/* Ported to Linux  (Spring 1995)				 */
/* Ported to Win95  (Fall   1996) -- not releasable		 */
/* Ported to SDL    (Fall   1997)                                */
/* By Sam Lantinga  (slouken@libsdl.org)			 */
/* 								 */
/* ------------------------------------------------------------- */

#include "Maelstrom_Globals.h"
#include "buttonlist.h"
#include "load.h"
#include "fastrand.h"
#include "checksum.h"

/* External functions used in this file */
extern int DoInitializations(Uint32 video_flags);		/* init.cc */

static const char *Version =
"Maelstrom v1.4.3 (GPL version 3.0.7) -- 02/01/2021 by Sam Lantinga\n";

// Global variables set in this file...
int	gStartLives;
int	gStartLevel;
Bool	gUpdateBuffer;
Bool	gRunning;
int	gNoDelay;

// Local variables in this file...
static ButtonList buttons;

// Local functions in this file...
static void DrawMainScreen(void);
static void DrawSoundLevel(void);
static void DrawKey(MPoint *pt, const char *ch, const char *str, void (*callback)(void));

// Main Menu actions:
static void RunDoAbout(void)
{
	gNoDelay = 0;
	Delay(SOUND_DELAY);
	sound->PlaySound(gNovaAppears, 5);
	DoAbout();
}
static void RunConfigureControls(void)
{
	Delay(SOUND_DELAY);
	sound->PlaySound(gHomingAppears, 5);
	ConfigureControls();
}
static void RunPlayGame(void)
{
	gStartLives = 3;
	gStartLevel = 1;
	gNoDelay = 0;
	sound->PlaySound(gNewLife, 5);
	Delay(SOUND_DELAY);
	NewGame();
	Message(NULL);		/* Clear any messages */
}
static void RunQuitGame(void)
{
	Delay(SOUND_DELAY);
	sound->PlaySound(gMultiplierGone, 5);
	while ( sound->Playing() )
		Delay(SOUND_DELAY);
	gRunning = false;
}
static void IncrementSound(void)
{
	if ( gSoundLevel < 8 ) {
		sound->Volume(++gSoundLevel);
		sound->PlaySound(gNewLife, 5);

		/* -- Draw the new sound level */
		DrawSoundLevel();
	}
}
static void DecrementSound(void)
{
	if ( gSoundLevel > 0 ) {
		sound->Volume(--gSoundLevel);
		sound->PlaySound(gNewLife, 5);

		/* -- Draw the new sound level */
		DrawSoundLevel();
	}
}
static void SetSoundLevel(int volume)
{
	/* Make sure the device is working */
	sound->Volume(volume);

	/* Set the new sound level! */
	gSoundLevel = volume;
	sound->PlaySound(gNewLife, 5);

	/* -- Draw the new sound level */
	DrawSoundLevel();
}

static void RunZapScores(void)
{
	Delay(SOUND_DELAY);
	sound->PlaySound(gMultShotSound, 5);
	if ( ZapHighScores() ) {
		/* Fade the screen and redisplay scores */
		screen->Fade();
		Delay(SOUND_DELAY);
		sound->PlaySound(gExplosionSound, 5);
		gUpdateBuffer = true;
	}
}

/* ----------------------------------------------------------------- */
/* -- Run a graphics speed test.                                     */
static void RunSpeedTest(void)
{
	const int test_reps = 100;	/* How many full cycles to run */

	Uint32 then, now;
	int i, frame, x=((640/2)-16), y=((480/2)-16), onscreen=0;

	screen->Clear();
	then = SDL_GetTicks();
	for ( i=0; i<test_reps; ++i ) {
		for ( frame=0; frame<SHIP_FRAMES; ++frame ) {
			if ( onscreen ) {
				screen->Clear(x, y, 32, 32);
			} else {
				onscreen = 1;
			}
			screen->QueueBlit(x, y, gPlayerShip->sprite[frame]);
			screen->Update();
		}
	}
	now = SDL_GetTicks();
	mesg("Graphics speed test took %d microseconds per cycle.\r\n",
						((now-then)/test_reps));
}

/* ----------------------------------------------------------------- */
/* -- Print a Usage message and quit.
      In several places we depend on this function exiting.
 */
static char *progname;
void PrintUsage(void)
{
	error("\nUsage: %s [-netscores] -printscores\n", progname);
	error("or\n");
	error("Usage: %s <options>\n\n", progname);
	error("Where <options> can be any of:\n\n"
"	-windowed		# Run Maelstrom in windowed mode\n"
"	-gamma [0-8]		# Set the gamma correction\n"
"	-volume [0-8]		# Set the sound volume\n"
"	-netscores		# Use the world-wide network score server\n"
	);
	LogicUsage();
	error("\n");
	exit(1);
}

/* ----------------------------------------------------------------- */
/* -- Blitter main program */
int main(int argc, char *argv[])
{
	/* Command line flags */
	int doprinthigh = 0;
	int speedtest = 0;
	Uint32 video_flags = SDL_WINDOW_FULLSCREEN_DESKTOP;

	/* Normal variables */
	SDL_Event event;
	LibPath::SetExePath(argv[0]);

#ifndef __WIN95__
	/* The first thing we do is calculate our checksum */
	(void) checksum();
#endif /* ! Win95 */

	/* Seed the random number generator */
	SeedRandom(0L);
	/* Initialize the controls */
	LoadControls();

	/* Initialize game logic data structures */
	if ( InitLogicData() < 0 ) {
		exit(1);
	}

	/* Parse command line arguments */
	for ( progname=argv[0]; --argc; ++argv ) {
		if ( strcmp(argv[1], "-windowed") == 0 ) {
			video_flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
		} else
		if ( strcmp(argv[1], "-gamma") == 0 ) {
			int gammacorrect;

			if ( ! argv[2] ) {  /* Print the current gamma */
				mesg("Current Gamma correction level: %d\n",
								gGammaCorrect);
				exit(0);
			}
			if ( (gammacorrect=atoi(argv[2])) < 0 || 
							gammacorrect > 8 ) {
				error(
	"Gamma correction value must be between 0 and 8. -- Exiting.\n");
				exit(1);
			}
			/* We need to update the gamma */
			gGammaCorrect = gammacorrect;
			SaveControls();

			++argv;
			--argc;
		}
		else if ( strcmp(argv[1], "-volume") == 0 ) {
			int volume;

			if ( ! argv[2] ) {  /* Print the current volume */
				mesg("Current volume level: %d\n",
								gSoundLevel);
				exit(0);
			}
			if ( (volume=atoi(argv[2])) < 0 || volume > 8 ) {
				error(
	"Volume must be a number between 0 and 8. -- Exiting.\n");
				exit(1);
			}
			/* We need to update the volume */
			gSoundLevel = volume;
			SaveControls();

			++argv;
			--argc;
		}
#define CHECKSUM_DEBUG
#ifdef CHECKSUM_DEBUG
		else if ( strcmp(argv[1], "-checksum") == 0 ) {
			mesg("Checksum = %s\n", get_checksum(NULL, 0));
			exit(0);
		}
#endif /* CHECKSUM_DEBUG */
		else if ( strcmp(argv[1], "-printscores") == 0 )
			doprinthigh = 1;
		else if ( strcmp(argv[1], "-netscores") == 0 )
			gNetScores = 1;
		else if ( strcmp(argv[1], "-speedtest") == 0 )
			speedtest = 1;
		else if ( LogicParseArgs(&argv, &argc) == 0 ) {
			/* LogicParseArgs() took care of everything */;
		} else if ( strcmp(argv[1], "-version") == 0 ) {
			error("%s", Version);
			exit(0);
		} else if ( strncmp(argv[1], "-psn_", 5) == 0 ) {
			/* Running from the Finder on Mac OSX */
		} else {
			PrintUsage();
		}
	}

	/* Do we just want the high scores? */
	if ( doprinthigh ) {
		PrintHighScores();
		exit(0);
	}

	/* Make sure we have a valid player list (netlogic) */
	if ( InitLogic() < 0 )
		exit(1);

	/* Initialize everything. :) */
	if ( DoInitializations(video_flags) < 0 ) {
		/* An error message was already printed */
		exit(1);
	}

	if ( speedtest ) {
		RunSpeedTest();
		exit(0);
	}

	gRunning = true;
	sound->PlaySound(gNovaBoom, 5);
	screen->Fade();		/* Fade-out */
	Delay(SOUND_DELAY);
	gUpdateBuffer = true;
	while ( sound->Playing() )
		Delay(SOUND_DELAY);

	while ( gRunning ) {
		
		/* Update the screen if necessary */
		if ( gUpdateBuffer )
			DrawMainScreen();

		/* -- Get an event */
		screen->WaitEvent(&event);

		/* -- Handle it! */
		if ( event.type == SDL_KEYDOWN ) {
			switch (event.key.keysym.sym) {

				/* -- Toggle fullscreen */
				case SDLK_RETURN:
					if ( event.key.keysym.mod & gToggleFullscreenMod )
						screen->ToggleFullScreen();
					break;

				/* -- About the game...*/
				case SDLK_a:
					RunDoAbout();
					break;

				/* -- Configure the controls */
				case SDLK_c:
					RunConfigureControls();
					break;

				/* -- Start the game */
				case SDLK_p:
					RunPlayGame();
					break;

				/* -- Start the game */
				case SDLK_l:
					Delay(SOUND_DELAY);
					sound->PlaySound(gLuckySound, 5);
					gStartLevel = GetStartLevel();
					if ( gStartLevel > 0 ) {
						Delay(SOUND_DELAY);
						sound->PlaySound(gNewLife, 5);
						Delay(SOUND_DELAY);
						NewGame();
					}
					break;

				/* -- Let them leave */
				case SDLK_q:
					RunQuitGame();
					break;

				/* -- Set the volume */
				/* (SDLK_0 - SDLK_8 are contiguous) */
				case SDLK_0:
				case SDLK_1:
				case SDLK_2:
				case SDLK_3:
				case SDLK_4:
				case SDLK_5:
				case SDLK_6:
				case SDLK_7:
				case SDLK_8:
					SetSoundLevel(event.key.keysym.sym
								- SDLK_0);
					break;

				/* -- Give 'em a little taste of the peppers */
				case SDLK_x:
					Delay(SOUND_DELAY);
					sound->PlaySound(gEnemyAppears, 5);
					ShowDawn();
					break;

				/* -- Zap the high scores */
				case SDLK_z:
					RunZapScores();
					break;
						
				/* -- Create a screen dump of high scores */
				case SDLK_F3:
					screen->ScreenDump("ScoreDump",
							64, 48, 298, 384);
					break;

				// Ignore Shift, Ctrl, Alt keys
				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
				case SDLK_LCTRL:
				case SDLK_RCTRL:
				case SDLK_LALT:
				case SDLK_RALT:
					break;

				// Dink! :-)
				default:
					Delay(SOUND_DELAY);
					sound->PlaySound(gSteelHit, 5);
					break;
			}
		} else
		/* -- Handle mouse clicks */
		if ( event.type == SDL_MOUSEBUTTONDOWN ) {
			buttons.Activate_Button(event.button.x, 
						event.button.y);
		} else
		/* -- Handle window close requests */
		if ( event.type == SDL_QUIT ) {
			RunQuitGame();
		}
	}
	screen->Fade();
	Delay(60);
	return(0);
}	/* -- main */


int DrawText(int x, int y, const char *text, MFont *font, Uint8 style,
					Uint8 R, Uint8 G, Uint8 B)
{
	SDL_Surface *textimage;
	int width;

	textimage = fontserv->TextImage(text, font, style, R, G, B);
	if ( textimage == NULL ) {
		width = 0;
	} else {
		screen->QueueBlit(x, y-textimage->h+2, textimage, NOCLIP);
		width = textimage->w;
		fontserv->FreeText(textimage);
	}
	return(width);
}


/* ----------------------------------------------------------------- */
/* -- Draw the current sound volume */
static void DrawSoundLevel(void)
{
	static int need_init=1;
	static MFont *geneva;
	static char text[12];
	static int xOff, yOff;

	if ( need_init ) {
		if ( (geneva = fontserv->NewFont("Geneva", 9)) == NULL ) {
			error("Can't use Geneva font! -- Exiting.\n");
			exit(255);
		}
		xOff = (SCREEN_WIDTH - 512) / 2;
		yOff = (SCREEN_HEIGHT - 384) / 2;
		need_init = 0;
	} else {
		DrawText(xOff+309-7, yOff+240-6, text, geneva, STYLE_BOLD,
							0x00, 0x00, 0x00);
	}
	SDL_snprintf(text, sizeof(text), "%d", gSoundLevel);
	DrawText(xOff+309-7, yOff+240-6, text, geneva, STYLE_BOLD,
						30000>>8, 30000>>8, 0xFF);
	screen->Update();
}	/* -- DrawSoundLevel */


/* ----------------------------------------------------------------- */
/* -- Draw the main screen */

void DrawMainScreen(void)
{
	SDL_Surface *title;
	MFont  *font, *bigfont;
	MPoint  pt;
	Uint16	width, height;
	Uint16  xOff, yOff, botDiv, rightDiv;
	Uint16  index, sRt, wRt, sw;
	Uint32  clr, ltClr, ltrClr;
	char buffer[128];
	int offset;

	gUpdateBuffer = false;
	buttons.Delete_Buttons();

	width = 512;
	height = 384;
	xOff = (SCREEN_WIDTH - width) / 2;
	yOff = (SCREEN_HEIGHT - height) / 2;

	title = Load_Title(screen, 129);
	if ( title == NULL ) {
		error("Can't load 'title' title! (ID=%d)\n", 129);
		exit(255);
        }

	clr = screen->MapRGB(30000>>8, 30000>>8, 0xFF);
	ltClr = screen->MapRGB(40000>>8, 40000>>8, 0xFF);
	ltrClr = screen->MapRGB(50000>>8, 50000>>8, 0xFF);

	screen->Lock();
	screen->Clear();
	/* -- Draw the screen frame */
	screen->DrawRect(xOff-1, yOff-1, width+2, height+2, clr);
	screen->DrawRect(xOff-2, yOff-2, width+4, height+4, clr);
	screen->DrawRect(xOff-3, yOff-3, width+6, height+6, ltClr);
	screen->DrawRect(xOff-4, yOff-4, width+8, height+8, ltClr);
	screen->DrawRect(xOff-5, yOff-5, width+10, height+10, ltrClr);
	screen->DrawRect(xOff-6, yOff-6, width+12, height+12, ltClr);
	screen->DrawRect(xOff-7, yOff-7, width+14, height+14, clr);
	/* -- Draw the dividers */
	botDiv = yOff + 5 + title->h + 5;
	rightDiv = xOff + 5 + title->w + 5;
	screen->DrawLine(rightDiv, yOff, rightDiv, yOff+height, ltClr);
	screen->DrawLine(xOff, botDiv, rightDiv, botDiv, ltClr);
	screen->DrawLine(rightDiv, 263+yOff, xOff+width, 263+yOff, ltClr);
	/* -- Draw the title image */
	screen->Unlock();
	screen->QueueBlit(xOff+5, yOff+5, title, NOCLIP);
	screen->Update();
	screen->FreeImage(title);


	/* -- Draw the high scores */

	/* -- First the headings  -- fontserv() isn't elegant, but hey.. */
	if ( (bigfont = fontserv->NewFont("New York", 18)) == NULL ) {
		error("Can't use New York (18) font! -- Exiting.\n");
		exit(255);
	}
	DrawText(xOff+5, botDiv+22, "Name", bigfont, STYLE_ULINE,
						0xFF, 0xFF, 0x00);
	sRt = xOff+185;
	DrawText(sRt, botDiv+22, "Score", bigfont, STYLE_ULINE,
						0xFF, 0xFF, 0x00);
	sRt += fontserv->TextWidth("Score", bigfont, STYLE_ULINE);
	wRt = xOff+245;
	DrawText(wRt, botDiv+22, "Wave", bigfont, STYLE_ULINE,
						0xFF, 0xFF, 0x00);
	wRt += fontserv->TextWidth("Wave", bigfont, STYLE_ULINE)-10;

	/* -- Now the scores */
	LoadScores();
	if ( (font = fontserv->NewFont("New York", 14)) == NULL ) {
		error("Can't use New York (14) font! -- Exiting.\n");
		exit(255);
	}

	for (index = 0; index < 10; index++) {
		Uint8 R, G, B;

		if ( gLastHigh == index ) {
			R = 0xFF;
			G = 0xFF;
			B = 0xFF;
		} else {
			R = 30000>>8;
			G = 30000>>8;
			B = 30000>>8;
		}
		DrawText(xOff+5, botDiv+42+(index*18), hScores[index].name,
						font, STYLE_BOLD, R, G, B);
		SDL_snprintf(buffer, sizeof(buffer), "%u", hScores[index].score);
		sw = fontserv->TextWidth(buffer, font, STYLE_BOLD);
		DrawText(sRt-sw, botDiv+42+(index*18), buffer, 
						font, STYLE_BOLD, R, G, B);
		SDL_snprintf(buffer, sizeof(buffer), "%u", hScores[index].wave);
		sw = fontserv->TextWidth(buffer, font, STYLE_BOLD);
		DrawText(wRt-sw, botDiv+42+(index*18), buffer, 
						font, STYLE_BOLD, R, G, B);
	}
	delete font;

	DrawText(xOff+5, botDiv+46+(10*18)+3, "Last Score: ", 
					bigfont, STYLE_NORM, 0xFF, 0xFF, 0xFF);
	SDL_snprintf(buffer, sizeof(buffer), "%d", GetScore());
	sw = fontserv->TextWidth("Last Score: ", bigfont, STYLE_NORM);
	DrawText(xOff+5+sw, botDiv+46+(index*18)+3, buffer, 
					bigfont, STYLE_NORM, 0xFF, 0xFF, 0xFF);
	delete bigfont;

	/* -- Draw the Instructions */
	offset = 34;

	pt.h = rightDiv + 10;
	pt.v = yOff + 10;
	DrawKey(&pt, "P", " Start playing Maelstrom", RunPlayGame);

	pt.h = rightDiv + 10;
	pt.v += offset;
	DrawKey(&pt, "C", " Configure the game controls", RunConfigureControls);

	pt.h = rightDiv + 10;
	pt.v += offset;
	DrawKey(&pt, "Z", " Zap the high scores", RunZapScores);

	pt.h = rightDiv + 10;
	pt.v += offset;
	DrawKey(&pt, "A", " About Maelstrom...", RunDoAbout);

	pt.v += offset;

	pt.h = rightDiv + 10;
	pt.v += offset;
	DrawKey(&pt, "Q", " Quit Maelstrom", RunQuitGame);

	pt.h = rightDiv + 10;
	pt.v += offset;
	DrawKey(&pt, "0", " ", DecrementSound);

	if ( (font = fontserv->NewFont("Geneva", 9)) == NULL ) {
		error("Can't use Geneva font! -- Exiting.\n");
		exit(255);
	}
	DrawText(pt.h+gKeyIcon->w+3, pt.v+19, "-",
				font, STYLE_NORM, 0xFF, 0xFF, 0x00);

	pt.h = rightDiv + 50;
	DrawKey(&pt, "8", " Set Sound Volume", IncrementSound);

/* -- Draw the credits */

	DrawText(xOff+5+68, yOff+5+127, "Port to Linux by Sam Lantinga",
				font, STYLE_BOLD, 0xFF, 0xFF, 0x00);
	DrawText(rightDiv+10, yOff+259, "(C) 1992-4 Ambrosia Software, Inc.",
				font, STYLE_BOLD, 0xFF, 0xFF, 0xFF);

/* -- Draw the version number */

	DrawText(xOff+20, yOff+151, VERSION_STRING,
				font, STYLE_NORM, 0xFF, 0xFF, 0xFF);
	delete font;

	DrawSoundLevel();

	/* Always drawing while faded out -- fade in */
	screen->Update();
	screen->Fade();
}	/* -- DrawMainScreen */



/* ----------------------------------------------------------------- */
/* -- Draw the key and its function */

static void DrawKey(MPoint *pt, const char *key, const char *text, void (*callback)(void))
{
	MFont *geneva;

	if ( (geneva = fontserv->NewFont("Geneva", 9)) == NULL ) {
		error("Can't use Geneva font! -- Exiting.\n");
		exit(255);
	}
	screen->QueueBlit(pt->h, pt->v, gKeyIcon);
	screen->Update();

	DrawText(pt->h+14, pt->v+20, key, geneva, STYLE_BOLD, 0xFF, 0xFF, 0xFF);
	DrawText(pt->h+13, pt->v+19, key, geneva, STYLE_BOLD, 0x00, 0x00, 0x00);
	DrawText(pt->h+gKeyIcon->w+3, pt->v+19, text,
					geneva, STYLE_BOLD, 0xFF, 0xFF, 0x00);
	delete geneva;

	buttons.Add_Button(pt->h, pt->v, gKeyIcon->w, gKeyIcon->h, callback);
}	/* -- DrawKey */


void Message(const char *message)
{
	static MFont *font;
	static int xOff;
	static char *last_message;

	if ( ! last_message ) { 	/* Initialize everything */
		/* This was taken from the DrawMainScreen function */
		xOff = (SCREEN_WIDTH - 512) / 2;

		if ( (font = fontserv->NewFont("New York", 14)) == NULL ) {
			error("Can't use New York(14) font! -- Exiting.\n");
			exit(255);
		}
	} else {
		DrawText(xOff, 25, last_message, font, STYLE_BOLD, 0, 0, 0);
		delete[] last_message;
	}
	if ( message ) {
		DrawText(xOff, 25, message, font, STYLE_BOLD, 0xCC,0xCC,0xCC);
		size_t len = strlen(message)+1;
		last_message = new char[len];
		SDL_strlcpy(last_message, message, len);
	} else {
		last_message = new char[1];
		last_message[0] = '\0';
	}
	screen->Update();
}

