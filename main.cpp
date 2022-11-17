/* ------------------------------------------------------------- */
/* 								 */
/* Maelstrom							 */
/* By Andrew Welch						 */
/* 								 */
/* Ported to Linux  (Spring 1995)				 */
/* Ported to Win95  (Fall   1996) -- not releasable		 */
/* By Sam Lantinga  (slouken@devolution.com)			 */
/* 								 */
/* ------------------------------------------------------------- */


#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#include "Maelstrom_Globals.h"
#include "buttons.h"
#include "checksum.h"

#if defined(MEM_DEBUG) && !defined(HEAPAGNT)
#include "newmem.h"
#endif /* MEM_DEBUG */

#ifdef __ultrix
#undef SIG_IGN
#define SIG_IGN  ((void (*)(int))(1))
#endif

static char *Version =
"Maelstrom v1.4.3 (Linux version 2.0.7) -- 4/26/99 by Sam Lantinga\n";

// Global variables set in this file...
int	gStartLives;
int	gStartLevel;
Bool	gUpdateBuffer;
Bool	gRunning;
Bool	gFadeBack;
int	gNoDelay;

// Local variables in this file...
static Buttons buttons;

// Global functions in this file...
void DrawMainScreen(void);

// Local functions in this file...
static void DrawSoundLevel(void);
static void DrawKey(MPoint *pt, char *ch, char *str, void (*callback)(void));

// Main Menu actions:
static void RunDoAbout(void)
{
	gNoDelay = 0;
	Delay(SOUND_DELAY);
	sound->PlaySound(gNovaAppears, 5, NULL);
	DoAbout();
}
static void RunConfigureControls(void)
{
	Delay(SOUND_DELAY);
	sound->PlaySound(gHomingAppears, 5, NULL);
	ConfigureControls();
}
static void RunPlayGame(void)
{
	gStartLives = 3;
	gStartLevel = 1;
	gNoDelay = 0;
	sound->PlaySound(gNewLife, 5, NULL);
	Delay(SOUND_DELAY);
	NewGame();
	Message(NULL);		/* Clear any messages */
}
static void RunQuitGame(void)
{
	Delay(SOUND_DELAY);
	sound->PlaySound(gMultiplierGone, 5, NULL);
	while(sound->IsSoundPlaying(0))
		Delay(SOUND_DELAY);
	gRunning = false;
}
static void IncrementSound(void)
{
	if ( gSoundLevel < 8 ) {
		if ( sound->SetVolume(gSoundLevel+1) < 0 )
			return;

		++gSoundLevel;
		sound->PlaySound(gNewLife, 5, NULL);

		/* -- Draw the new sound level */
		DrawSoundLevel();
	}
}
static void DecrementSound(void)
{
	if ( gSoundLevel > 0 ) {
		if ( sound->SetVolume(gSoundLevel-1) < 0 )
			return;

		--gSoundLevel;
		sound->PlaySound(gNewLife, 5, NULL);

		/* -- Draw the new sound level */
		DrawSoundLevel();
	}
}
static void SetSoundLevel(int volume)
{
	/* Make sure the device is working */
	if ( sound->SetVolume(volume) < 0 )
		return;

	/* Set the new sound level! */
	gSoundLevel = volume;
	sound->PlaySound(gNewLife, 5, NULL);

	/* -- Draw the new sound level */
	DrawSoundLevel();
}

static void RunZapScores(void)
{
	Delay(SOUND_DELAY);
	sound->PlaySound(gMultShotSound, 5, NULL);
	if ( ZapHighScores() ) {
		Delay(SOUND_DELAY);
		sound->PlaySound(gExplosionSound, 5, NULL);
		gUpdateBuffer = true;
	}
}

/* ----------------------------------------------------------------- */
/* -- Run a graphics speed test.                                     */
static void RunSpeedTest(void)
{
	const int test_reps = 100;	/* How many full cycles to run */

	struct timeval then, now;
	int i, frame, x=((640/2)-16), y=((480/2)-16), onscreen=0;

	win->Clear();
	gettimeofday(&then, NULL);
	for ( i=0; i<test_reps; ++i ) {
		for ( frame=0; frame<SHIP_FRAMES; ++frame ) {
			if ( onscreen ) {
				if ( frame ) 
					win->UnBlit_CSprite(x, y,
						gPlayerShip->sprite[frame-1]);
				else
					win->UnBlit_CSprite(x, y,
					gPlayerShip->sprite[SHIP_FRAMES-1]);
			} else {
				onscreen = 1;
			}
			win->Blit_CSprite(x, y, gPlayerShip->sprite[frame]);
			win->Flush(1);
		}
	}
	gettimeofday(&now, NULL);
	now.tv_sec -= then.tv_sec;
	now.tv_usec -= then.tv_usec;
	mesg("Graphics speed test took %d microseconds per cycle.\r\n",
			(((now.tv_sec*1000000)+now.tv_usec)/test_reps));
}

/* ----------------------------------------------------------------- */
/* -- Print a Usage message and quit.
      In several places we depend on this function exiting.
 */
static char *progname;
void PukeUsage(void)
{
	error("\nUsage: %s [-netscores] -printscores\n", progname);
	error("or\n");
	error("Usage: %s <options>\n\n", progname);
	error("Where <options> can be any of:\n\n"
#ifdef USE_JOYSTICK
"	-calibrate [device]	# Calibrate your joystick before playing\n"
#endif
#ifndef FORCE_XSHM
"	-display <host:0>	# Run Maelstrom on a remote display\n"
#endif
"	-fullscreen		# Run Maelstrom in full-screen mode\n"
"	-privatecmap		# Run Maelstrom with a private colormap\n"
"	-gamma [0-8]		# Set the gamma correction under X11\n"
"	-volume [0-8]		# Set the sound volume\n"
"	-realfade		# Really fade the display\n"
"	-nofade			# Don't fade the display\n"
"	-netscores		# Use the world-wide network score server\n"
	);
	LogicUsage();
	error("\n");
	exit(1);
}

/* ----------------------------------------------------------------- */
/* -- Timer initialization */
unsigned long started;			/* Used by the Ticks() function */
void InitTimer(void)
{
	struct timeval tv;

	/* Set started to the current time (in seconds) */
	gettimeofday(&tv, NULL);
	started = tv.tv_sec;

	/* Seed the random number generator */
	SeedRandom(0L);
}

/* ----------------------------------------------------------------- */
/* -- Blitter main program */
int main(int argc, char *argv[])
{
	XEvent	event;
	char    buf[128];
	KeySym  key;
	int fullscreen = 0;
	int private_cmap = 0;
	int dofade = FADE_FAKE;
	int doprinthigh = 0;
	int speedtest = 0;		// Testing flag...
	int dispinfo = 0;		// Display info flag...

#ifndef __WIN95__
	/* Actually, the first thing we do is calculate our checksum */
	(void) checksum();

	/* The first thing we do is get rid of any suid permissions */
	if ( On_Console() )
		vga_init();
	else {
#ifdef USE_DGA				/* Insecure!! (buffer overflow?) */
		seteuid(getuid());
#else
		setuid(getuid());
#endif
	}
#endif /* ! Win95 */

	/* Initialize the timer and controls */
	InitTimer();
	LoadControls();

	/* Initialize game logic data structures */
	InitLogicData();

	/* Parse command line arguments */
	for ( progname=argv[0]; --argc; ++argv ) {
		if ( strcmp(argv[1], "-fullscreen") == 0 )
			fullscreen = 1;
		else if ( strcmp(argv[1], "-privatecmap") == 0 )
			private_cmap = 1;
		else if ( strcmp(argv[1], "-gamma") == 0 ) {
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
#ifdef USE_JOYSTICK
		else if ( strcmp(argv[1], "-calibrate") == 0 ) {
			if ( argv[2] && (argv[2][0] != '-') ) {
				CalibrateJoystick(argv[2]);
				++argv;
				--argc;
			} else
				CalibrateJoystick(NULL);
			exit(0);
		}
#endif
		else if ( strcmp(argv[1], "-nofade") == 0 )
			dofade = FADE_NONE;
		else if ( strcmp(argv[1], "-realfade") == 0 )
			dofade = FADE_REAL;
		else if ( strcmp(argv[1], "-speedtest") == 0 )
			speedtest = 1;
#ifndef FORCE_XSHM
		else if ( strcmp(argv[1], "-display") == 0 ) {
			char *ptr, *display="DISPLAY=";
			if ( ! argv[2] ) {
				error(
			"The '-display' option requires an argument!\n");
				PukeUsage();
			}
			if ( (ptr=(char *)malloc(
				strlen(display)+strlen(argv[2])+1)) == NULL ) {
				perror("malloc() error");
				exit(3);
			}
			sprintf(ptr, "%s%s", display, argv[2]);
			(void) putenv(ptr);
			++argv;
			--argc;
		}
#endif /* Not FORCE_XSHM */

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
		else if ( LogicParseArgs(&argv, &argc) == 0 ) {
			/* LogicParseArgs() took care of everything */;
		} else if ( strcmp(argv[1], "-version") == 0 ) {
			error("%s", Version);
			exit(0);
		} else if ( strcmp(argv[1], "-displaytype") == 0 ) {
			dispinfo = 1;
		} else {
			PukeUsage();
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
	if (DoInitializations(fullscreen, private_cmap, dofade)<0) {
		error("Couldn't initialize! -- Exiting.\n");
		exit(1);
	}
	if ( dispinfo ) {
		mesg("Current Graphics display = %s\n", win->DisplayType());
	}
	if ( speedtest ) {
		RunSpeedTest();
		exit(0);
	}
	sound->PlaySound(gNovaBoom, 5, NULL);
	win->Fade(FADE_STEPS);
	Delay(SOUND_DELAY);
	gFadeBack = true;
	while(sound->IsSoundPlaying(0))
		Delay(SOUND_DELAY);
	DrawMainScreen();
	win->Show_Cursor();

	gRunning = true;
	while ( gRunning ) {
		
		/* Update the screen if necessary */
		if ( gUpdateBuffer )
			DrawMainScreen();

		/* -- Get an event */
		win->GetEvent(&event);

		/* -- Handle it! */
		if ( event.type == KeyPress ) {
			win->KeyToAscii(&event, buf, 127, &key);
			switch (key) {
					
				/* -- About the game...*/
				case XK_A:
				case XK_a:
					RunDoAbout();
					break;

				/* -- Configure the controls */
				case XK_C:
				case XK_c:
					RunConfigureControls();
					break;

				/* -- Start the game */
				case XK_P: 
				case XK_p:
					RunPlayGame();
					break;

				/* -- Start the game */
				case XK_L:
				case XK_l:
					Delay(SOUND_DELAY);
					sound->PlaySound(gLuckySound, 5, NULL);
					gStartLevel = GetStartLevel();
					if ( gStartLevel > 0 ) {
						Delay(SOUND_DELAY);
						sound->PlaySound(gNewLife, 
								5, NULL);
						Delay(SOUND_DELAY);
						NewGame();
					}
					break;

				/* -- Let them leave */
				case XK_Q:
				case XK_q:
					RunQuitGame();
					break;

				/* -- Set the volume */
				/* (XK_0 - XK_8 aren't contiguous) */
				case XK_0:
					SetSoundLevel(0);
					break;
				case XK_1:
					SetSoundLevel(1);
					break;
				case XK_2:
					SetSoundLevel(2);
					break;
				case XK_3:
					SetSoundLevel(3);
					break;
				case XK_4:
					SetSoundLevel(4);
					break;
				case XK_5:
					SetSoundLevel(5);
					break;
				case XK_6:
					SetSoundLevel(6);
					break;
				case XK_7:
					SetSoundLevel(7);
					break;
				case XK_8:
					SetSoundLevel(8);
					break;

				/* -- Give 'em a little taste of the peppers */
				case XK_X:
				case XK_x:
					Delay(SOUND_DELAY);
					sound->PlaySound(gEnemyAppears,5,NULL);
					ShowDawn();
					break;

				/* -- Zap the high scores */
				case XK_Z:
				case XK_z:
					RunZapScores();
					break;
						
				/* -- Create a screen dump of high scores */
				case XK_F3:
				{
					Rect area = {48, 64, 432, 362};
					win->ScreenDump("ScoreDump", &area);
				}
					break;

				// Ignore Shift, Ctrl, Alt keys
				case XK_Shift_L:
#if XK_Shift_L != XK_Shift_R
				case XK_Shift_R:
#endif
				case XK_Control_L:
#if XK_Control_L != XK_Control_R
				case XK_Control_R:
#endif
				case XK_Alt_L:
#if XK_Alt_L != XK_Alt_R
				case XK_Alt_R:
#endif
					break;

				// Dink! :-)
				default:
					Delay(SOUND_DELAY);
					sound->PlaySound(gSteelHit, 5, NULL);
					break;
			}
		}

		/* -- Handle mouse clicks */
		if ( event.type == ButtonPress ) {
			buttons.Activate_Button(event.xbutton.x, 
					event.xbutton.y, event.xbutton.button);
		}

		/* -- Handle screen blanking */
		if ( event.type == Expose ) {
			gUpdateBuffer = 1;
		}
	}
	win->Fade(FADE_STEPS);
	Delay(60);
	Quit(0);
}	/* -- main */

/* ----------------------------------------------------------------- */
/* -- Handle mouse clicks */

void HandleMouse(XEvent *event)
{
	Unused(event);		/* Change this if we ever do something */
	return;
}

/* ----------------------------------------------------------------- */
/* -- Clean up and quit */

void CleanUp(void)
{
	/* We don't need to hear our child die */
#ifdef SIGCHLD
	signal(SIGCHLD, SIG_IGN);
#endif
#ifdef SIGIO
	signal(SIGIO, SIG_IGN);
#endif
	delete sound;

	/* Shut down the game logic */
	HaltLogic();

	/* Clear the display */
	win->Flush(1);
	delete win;
	delete fontserv;

	/* The scores should be saved when they are modified */
	SaveControls();
}	/* -- CleanUp */

void Quit(int status)
{
	exit(status);
}

#ifdef SIGCHLD
/* The status of the child changed... */
void ReapChild(int sig)
{
	int status;

	if ( waitpid(-1, &status, WNOHANG) > 0 ) {
		error("Maelstrom: Lost sound server! -- Exiting.\r\n");
		Quit(sig);
	}
	signal(sig, ReapChild);
}
#endif /* SIGCHLD */

#ifdef _INCLUDE_HPUX_SOURCE
/* These signals are wrong... but hey. :) */
static char *sig_list[NSIG] = {
	"", "HUP", "USR1", "INT", "USR2", "QUIT", "CHLD", "ILL",
	"PWR", "TRAP", "VTALRM", "ABRT", "PROF", "EMT", "IO", "FPE",
	"WINCH", "KILL", "STOP", "BUS", "TSTP", "SEGV", "CONT",
	"SYS", "TTIN", "PIPE", "TTOU", "ALRM", "URG", "TERM", "LOST"
	};
#else
static char *sig_list[NSIG] = {
	"", "HUP", "INT", "QUIT", "ILL", "TRAP", "IOT", "BUS",
	"FPE", "KILL", "USR1", "SEGV", "USR2", "PIPE", "ALRM",
	"TERM", "STKFLT", "CHLD", "CONT", "STOP", "TSTP", "TTIN",
	"TTOU",
#if NSIG > 23
	"URG", "XCPU", "XFSZ", "VTALRM", "PROF", "WINCH", "IO",
   "PWR", "UNUSED"
#endif /* NSIG */
	};
#endif /* ! HPUX */


void Killed(int sig)
{
	error("Killed by signal %d (SIG%s)\r\n", sig, sig_list[sig]);
	Quit(sig);
}

void DrawText(int x, int y, BitMap *text, unsigned long color)
{
	win->Blit_BitMap(x, y-text->height+2, 
				text->width, text->height, text->bits, color);
}
void UnDrawText(int x, int y, BitMap *text)
{
	win->UnBlit_BitMap(x, y-text->height+2, 
				text->width, text->height, text->bits);
}


/* ----------------------------------------------------------------- */
/* -- Draw the current sound volume */
static void DrawSoundLevel(void)
{
	static int           need_init=1;
	static MFont        *geneva;
	static BitMap       *text;
	static unsigned long clr;
	char                 buf[12];
	static int           xOff, yOff;

	if ( need_init ) {
		if ( (geneva = fontserv->New_Font("Geneva", 9)) == NULL ) {
			error("Can't use Geneva font! -- Exiting.\n");
			exit(255);
		}
		xOff = (SCREEN_WIDTH - 512) / 2;
		yOff = (SCREEN_HEIGHT - 384) / 2;
		clr = win->Map_Color(30000, 30000, 0xFFFF);
		need_init = 0;
	} else {
		UnDrawText(xOff+309-7, yOff+240-6, text);
		fontserv->Free_Text(text);
	}
	sprintf(buf, "%d", gSoundLevel);
	text = fontserv->Text_to_BitMap(buf, geneva, STYLE_BOLD);
	DrawText(xOff+309-7, yOff+240-6, text, clr);
}	/* -- DrawSoundLevel */


/* ----------------------------------------------------------------- */
/* -- Draw the main screen */

void DrawMainScreen(void)
{
	struct Title  title;
	MFont        *font, *bigfont;
	BitMap       *text;
	MPoint         pt;
	int	      width, height;
	int           xOff, yOff, botDiv, rightDiv;
	int           index, sRt, wRt, sw;
	unsigned long clr, ltClr, ltrClr;
	char          buffer[128];
	int           offset;

	gUpdateBuffer = false;
	buttons.Delete_Buttons();

	width = 512;
	height = 384;
	xOff = (SCREEN_WIDTH - width) / 2;
	yOff = (SCREEN_HEIGHT - height) / 2;

	/* -- Black the screen out */
	win->Clear();

	/* -- Draw the screen frame */
	clr = win->Map_Color(30000, 30000, 0xFFFF);
	ltClr = win->Map_Color(40000, 40000, 0xFFFF);
	ltrClr = win->Map_Color(50000, 50000, 0xFFFF);

	if ( Load_Title(&title, 129) < 0 ) {
		error("Can't load 'title' title! (ID=%d)\n", 129);
		exit(255);
        }

	win->DrawRectangle(xOff-1, yOff-1, width+2, height+2, clr);
	win->DrawRectangle(xOff-2, yOff-2, width+4, height+4, clr);
	win->DrawRectangle(xOff-3, yOff-3, width+6, height+6, ltClr);
	win->DrawRectangle(xOff-4, yOff-4, width+8, height+8, ltClr);
	win->DrawRectangle(xOff-5, yOff-5, width+10, height+10, ltrClr);
	win->DrawRectangle(xOff-6, yOff-6, width+12, height+12, ltClr);
	win->DrawRectangle(xOff-7, yOff-7, width+14, height+14, clr);

	/* -- Draw the title picture */
	win->Blit_Title(xOff+5, yOff+5, title.width, title.height, title.data);
	Free_Title(&title);

	/* -- Draw the dividers */
	botDiv = yOff + 5 + title.height + 5;
	rightDiv = xOff + 5 + title.width + 5;
	win->DrawLine(rightDiv, yOff, rightDiv, yOff+height, ltClr);
	win->DrawLine(xOff, botDiv, rightDiv, botDiv, ltClr);
	win->DrawLine(rightDiv, 263+yOff, xOff+width, 263+yOff, ltClr);

	/* -- Draw the high scores */

	/* -- First the headings  -- fontserv() isn't elegant, but hey.. */
	if ( (bigfont = fontserv->New_Font("New York", 18)) == NULL ) {
		error("Can't use New York(18) font! -- Exiting.\n");
		exit(255);
	}
	clr = win->Map_Color(0xFFFF, 0xFFFF, 0x0000);
	text = fontserv->Text_to_BitMap("Name", bigfont, STYLE_ULINE);
	DrawText(xOff+5, botDiv+22, text, clr);
	fontserv->Free_Text(text);
	text = fontserv->Text_to_BitMap("Score", bigfont, STYLE_ULINE);
	sRt = (xOff+185+text->width);
	DrawText(xOff+185, botDiv+22, text, clr);
	fontserv->Free_Text(text);
	text = fontserv->Text_to_BitMap("Wave", bigfont, STYLE_ULINE);
	wRt = (xOff+245+text->width-10);
	DrawText(xOff+245, botDiv+22, text, clr);
	fontserv->Free_Text(text);

	/* -- Now the scores */
	LoadScores();
	if ( (font = fontserv->New_Font("New York", 14)) == NULL ) {
		error("Can't use New York(14) font! -- Exiting.\n");
		exit(255);
	}
	clr = win->Map_Color(0xFFFF, 0xFFFF, 0x0000);

	for (index = 0; index < 10; index++) {
		if ( gLastHigh == index )
			clr = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
		else
			clr = win->Map_Color(30000, 30000, 30000);
		
		text = fontserv->Text_to_BitMap(hScores[index].name, 
							font, STYLE_BOLD);
		DrawText(xOff+5, botDiv+42+(index*18), text, clr);
		fontserv->Free_Text(text);

		sprintf(buffer, "%ld", hScores[index].score);
		text = fontserv->Text_to_BitMap(buffer, font, STYLE_BOLD);
		sw = text->width;
		DrawText(sRt-sw, botDiv+42+(index*18), text, clr);
		fontserv->Free_Text(text);

		sprintf(buffer, "%d", hScores[index].wave);
		text = fontserv->Text_to_BitMap(buffer, font, STYLE_BOLD);
		sw = text->width;
		DrawText(wRt-sw, botDiv+42+(index*18), text, clr);
		fontserv->Free_Text(text);
	}
	fontserv->Free_Font(font);

	clr = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
	text = fontserv->Text_to_BitMap("Last Score: ", bigfont, STYLE_NORM);
	DrawText(xOff+5, botDiv+46+(10*18)+3, text, clr);
	fontserv->Free_Text(text);
	sprintf(buffer, "%d", GetScore());
	text = fontserv->Text_to_BitMap(buffer, bigfont, STYLE_NORM);
	DrawText(xOff+5+
		fontserv->TextWidth("Last Score: ", bigfont, STYLE_NORM),
					botDiv+46+(index*18)+3, text, clr);
	fontserv->Free_Text(text);
	fontserv->Free_Font(bigfont);

	/* -- Draw the Instructions */
	clr = win->Map_Color(0xFFFF, 0xFFFF, 0x0000);
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

#ifdef USE_REGISTRATION
	pt.h = rightDiv + 10;
	pt.v += offset;
	DrawKey(&pt, "R", " Print registration form");
#else
	pt.v += offset;
#endif /* USE_REGISTRATION */

	pt.h = rightDiv + 10;
	pt.v += offset;
	DrawKey(&pt, "Q", " Quit Maelstrom", RunQuitGame);

	pt.h = rightDiv + 10;
	pt.v += offset;
	DrawKey(&pt, "0", " ", DecrementSound);

	if ( (font = fontserv->New_Font("Geneva", 9)) == NULL ) {
		error("Can't use Geneva font! -- Exiting.\n");
		exit(255);
	}
	text = fontserv->Text_to_BitMap("-", font, STYLE_NORM);
	DrawText(pt.h+gKeyIcon->width+3, pt.v+19, text, clr);
	fontserv->Free_Text(text);

	pt.h = rightDiv + 50;
	DrawKey(&pt, "8", " Set Sound Volume", IncrementSound);

/* -- Draw the credits */

	text = fontserv->Text_to_BitMap("Port to Linux by Sam Lantinga", 
							font, STYLE_BOLD);
	DrawText(xOff+5+68, yOff+5+127, text, clr);
	fontserv->Free_Text(text);

	clr = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
	text = fontserv->Text_to_BitMap("©1992-4 Ambrosia Software, Inc.", 
							font, STYLE_BOLD);
	DrawText(rightDiv+10, yOff+259, text, clr);
	fontserv->Free_Text(text);

/* -- Draw the version number */

	text = fontserv->Text_to_BitMap(VERSION_STRING, font, STYLE_NORM);
	DrawText(xOff+20, yOff+151, text, clr);
	fontserv->Free_Text(text);
	fontserv->Free_Font(font);

	DrawSoundLevel();
	win->Refresh();
	if ( gFadeBack ) {
		win->Fade(FADE_STEPS);
		gFadeBack = false;
	}
}	/* -- DrawMainScreen */



/* ----------------------------------------------------------------- */
/* -- Draw the key and its function */

static void DrawKey(MPoint *pt, char *ch, char *str, void (*callback)(void))
{
	MFont        *geneva;
	BitMap       *text;
	unsigned long c;

	if ( (geneva = fontserv->New_Font("Geneva", 9)) == NULL ) {
		error("Can't use Geneva font! -- Exiting.\n");
		exit(255);
	}
	win->Blit_Icon(pt->h, pt->v, gKeyIcon->width, gKeyIcon->height,
					gKeyIcon->pixels, gKeyIcon->mask);
	buttons.Add_Button(pt->h, pt->v, gKeyIcon->width, gKeyIcon->height,
								callback);

	text = fontserv->Text_to_BitMap(ch, geneva, STYLE_BOLD);
	c = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
	DrawText(pt->h+14, pt->v+20, text, c);
	c = win->Map_Color(0x0000, 0x0000, 0x0000);
	DrawText(pt->h+13, pt->v+19, text, c);
	fontserv->Free_Text(text);

	text = fontserv->Text_to_BitMap(str, geneva, STYLE_BOLD);
	c = win->Map_Color(0xFFFF, 0xFFFF, 0x0000);
	DrawText(pt->h+gKeyIcon->width+3, pt->v+19, text, c);
	fontserv->Free_Text(text);
	fontserv->Free_Font(geneva);
}	/* -- DrawKey */


void DoSplash(void)
{
	struct Title splash;

	win->Clear();

	if ( Load_Title(&splash, 999) < 0 ) {
		error("Can't load Ambrosia splash title! (ID=%d)\n", 999);
		return;
        }
	win->Blit_Title((SCREEN_WIDTH-splash.width)/2,
			(SCREEN_HEIGHT-splash.height)/2, splash.width, 
						splash.height, splash.data);
	Free_Title(&splash);
}

void Message(char *message)
{
	static MFont *font;
	static int xOff;
	static char *last_message;
	static unsigned long blk, clr;
	BitMap *text;

	if ( ! last_message ) { 	/* Initialize everything */
		/* This was taken from the DrawMainScreen function */
		xOff = (SCREEN_WIDTH - 512) / 2;

		if ( (font = fontserv->New_Font("New York", 14)) == NULL ) {
			error("Can't use New York(14) font! -- Exiting.\n");
			exit(255);
		}
		blk = win->Map_Color(0x0000, 0x0000, 0x0000);
		clr = win->Map_Color(0xCCCC, 0xCCCC, 0xCCCC);
	} else {
		text = fontserv->Text_to_BitMap(last_message, font,
								STYLE_BOLD);
		DrawText(xOff, 25, text, blk);
		fontserv->Free_Text(text);
		delete[] last_message;
	}
	if ( message ) {
		text = fontserv->Text_to_BitMap(message, font, STYLE_BOLD);
		DrawText(xOff, 25, text, clr);
		fontserv->Free_Text(text);
		last_message = new char[strlen(message)+1];
		strcpy(last_message, message);
	} else {
		last_message = new char[1];
		last_message[0] = '\0';
	}
	win->Flush(1);
}
