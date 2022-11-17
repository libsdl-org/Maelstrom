
/* 
   This file handles the cheat dialogs and the high score file
*/

#ifdef unix
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <stdio.h>

#include "SDL_endian.h"

#include "Maelstrom_Globals.h"
#include "load.h"
#include "dialog.h"

#define MAELSTROM_SCORES	"Maelstrom-Scores"
#define NUM_SCORES		10		// Do not change this!

/* Everyone can write to scores file if defined to 0 */
#define SCORES_PERMMASK		022

#define CLR_DIALOG_WIDTH	281
#define CLR_DIALOG_HEIGHT	111

Bool gNetScores = 0;
Scores hScores[NUM_SCORES];

void LoadScores(void)
{
	SavePath path;
	SDL_RWops *scores_src;
	int i;

	/* Try to load network scores, if we can */
	if ( gNetScores ) {
		if ( NetLoadScores() == 0 )
			return;
		else {
			mesg("Using local score file\n\n");
			gNetScores = 0;
		}
	}
	memset(&hScores, 0, sizeof(hScores));

	scores_src = SDL_RWFromFile(path.Path(MAELSTROM_SCORES), "rb");
	if ( scores_src != NULL ) {
		for ( i=0; i<NUM_SCORES; ++i ) {
			SDL_RWread(scores_src, hScores[i].name,
			           sizeof(hScores[i].name), 1);
			hScores[i].wave = SDL_ReadBE32(scores_src);
			hScores[i].score = SDL_ReadBE32(scores_src);
		}
		SDL_RWclose(scores_src);
	}
}

void SaveScores(void)
{
	SavePath path;
	SDL_RWops *scores_src;
	int i;
#ifdef unix
	int omask;
#endif

	/* Don't save network scores */
	if ( gNetScores )
		return;

#ifdef unix
	omask=umask(SCORES_PERMMASK);
#endif
	scores_src = SDL_RWFromFile(path.Path(MAELSTROM_SCORES), "wb");
	if ( scores_src != NULL ) {
		for ( i=0; i<NUM_SCORES; ++i ) {
			SDL_RWwrite(scores_src, hScores[i].name,
			            sizeof(hScores[i].name), 1);
			SDL_WriteBE32(scores_src, hScores[i].wave);
			SDL_WriteBE32(scores_src, hScores[i].score);
		}
		SDL_RWclose(scores_src);
	} else {
		error("Warning: Couldn't save scores to %s\n",
						path.Path(MAELSTROM_SCORES));
	}
#ifdef unix
	umask(omask);
#endif
}

/* Just show the high scores */
void PrintHighScores(void)
{
	int i;

	LoadScores();
	/* FIXME! -- Put all lines into a single formatted message */
	printf("Name			Score	Wave\n");
	for ( i=0; i<NUM_SCORES; ++i ) {
		printf("%-20s	%-3.1u	%u\n", hScores[i].name,
					hScores[i].score, hScores[i].wave);
	}
}

static int do_clear;

static int Clear_callback(void) {
	do_clear = 1;
	return(1);
}
static int Cancel_callback(void) {
	do_clear = 0;
	return(1);
}

int ZapHighScores(void)
{
	MFont *chicago;
	Maclike_Dialog *dialog;
	int X, Y;
	SDL_Surface *splash;
	Mac_Button *clear;
	Mac_Button *cancel;

	/* Set up all the components of the dialog box */
#ifdef CENTER_DIALOG
	X=(SCREEN_WIDTH-CLR_DIALOG_WIDTH)/2;
	Y=(SCREEN_HEIGHT-CLR_DIALOG_HEIGHT)/2;
#else	/* The way it is on the original Maelstrom */
	X=179;
	Y=89;
#endif
	if ( (chicago = fontserv->NewFont("Chicago", 12)) == NULL ) {
		error("Can't use Chicago font!\n");
		return(0);
	}
	if ( (splash = Load_Title(screen, 102)) == NULL ) {
		error("Can't load score zapping splash!\n");
		delete chicago;
		return(0);
	}
	dialog = new Maclike_Dialog(X, Y, CLR_DIALOG_WIDTH, CLR_DIALOG_HEIGHT,
								screen);
	dialog->Add_Image(splash, 4, 4);
	do_clear = 0;
	clear = new Mac_Button(99, 74, BUTTON_WIDTH, BUTTON_HEIGHT,
				"Clear", chicago, fontserv, Clear_callback);
	dialog->Add_Dialog(clear);
	cancel = new Mac_DefaultButton(99+BUTTON_WIDTH+14, 74, 
				BUTTON_WIDTH, BUTTON_HEIGHT,
				"Cancel", chicago, fontserv, Cancel_callback);
	dialog->Add_Dialog(cancel);

	/* Run the dialog box */
	dialog->Run();

	/* Clean up and return */
	screen->FreeImage(splash);
	delete chicago;
	delete dialog;
	if ( do_clear ) {
		memset(hScores, 0, sizeof(hScores));
		SaveScores();
		gLastHigh = -1;
	}
	return(do_clear);
}


#define LVL_DIALOG_WIDTH	346
#define LVL_DIALOG_HEIGHT	136

static int     do_level;

static int Level_callback(void) {
	do_level = 1;
	return(1);
}
static int Cancel2_callback(void) {
	do_level = 0;
	return(1);
}

int GetStartLevel(void)
{
	static const char    *Ltext1 = 
			"Enter the level to start from (1-40).  This";
	static const char    *Ltext2 = 
			"disqualifies you from a high score...";
	static const char    *Ltext3 = "Level:";
	static const char    *Ltext4 = "Lives:";
	MFont *chicago;
	Maclike_Dialog *dialog;
	SDL_Surface *splash;
	SDL_Surface *text1, *text2, *text3, *text4;
	static const char *turbotext = "Turbofunk On";
	int x, y, X, Y;
	Mac_Button *doit;
	Mac_Button *cancel;
	Mac_NumericEntry *numeric_entry;
	Mac_CheckBox *checkbox;
	int startlevel=10, startlives=5, turbofunk=0;

	/* Set up all the components of the dialog box */
	if ( (chicago = fontserv->NewFont("Chicago", 12)) == NULL ) {
		error("Can't use Chicago font!\n");
		return(0);
	}
	if ( (splash = GetCIcon(screen, 103)) == NULL ) {
		error("Can't load alien level splash!\n");
		delete chicago;
		return(0);
	}
	X=(SCREEN_WIDTH-LVL_DIALOG_WIDTH)/2;
	Y=(SCREEN_HEIGHT-LVL_DIALOG_HEIGHT)/2;
	dialog = new Maclike_Dialog(X, Y, LVL_DIALOG_WIDTH, LVL_DIALOG_HEIGHT,
								screen);
	x = y = 14;
	dialog->Add_Image(splash, x, y);
	x += (splash->w+14);
	text1 = fontserv->TextImage(Ltext1,chicago,STYLE_NORM,0x00,0x00,0x00);
	dialog->Add_Image(text1, x, y);
	y += (text1->h+2);
	text2 = fontserv->TextImage(Ltext2, chicago, STYLE_NORM,
							0x00, 0x00, 0x00);
	dialog->Add_Image(text2, x, y);
	do_level = 0;
	cancel = new Mac_Button(166, 96, 73, BUTTON_HEIGHT,
				"Cancel", chicago, fontserv, Cancel2_callback);
	dialog->Add_Dialog(cancel);
	doit = new Mac_DefaultButton(166+73+14, 96, BUTTON_WIDTH, BUTTON_HEIGHT,
				"Do it!", chicago, fontserv, Level_callback);
	dialog->Add_Dialog(doit);
	numeric_entry = new Mac_NumericEntry(X, Y, chicago, fontserv);
	numeric_entry->Add_Entry(78, 60, 3, 1, &startlevel);
	text3 = fontserv->TextImage(Ltext3,chicago,STYLE_NORM,0x00,0x00,0x00);
	dialog->Add_Image(text3, 78-text3->w-2, 60+3);
	numeric_entry->Add_Entry(78, 86, 3, 0, &startlives);
	text4 = fontserv->TextImage(Ltext4,chicago,STYLE_NORM,0x00,0x00,0x00);
	dialog->Add_Image(text4, 78-text3->w-2, 86+3);
	dialog->Add_Dialog(numeric_entry);
	checkbox = new Mac_CheckBox(&turbofunk, 136, 64, turbotext,
						chicago, fontserv);
	dialog->Add_Dialog(checkbox);

	/* Run the dialog box */
	dialog->Run(EXPAND_STEPS);

	/* Clean up and return */
	screen->FreeImage(splash);
	fontserv->FreeText(text1);
	fontserv->FreeText(text2);
	fontserv->FreeText(text3);
	fontserv->FreeText(text4);
	delete chicago;
	delete dialog;
	if ( do_level ) {
		if ( ! startlives || (startlives > 40) )
			startlives = 3;
		gStartLives = startlives;
		if ( startlevel > 40 )
			startlevel = 0;
		gStartLevel = startlevel;
		gNoDelay = turbofunk;
		return(gStartLevel);
	}
	return(0);
}

