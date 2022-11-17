
/* 
   This file handles the cheat dialogs and the high score file
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "bitesex.h"
#include "Maelstrom_Globals.h"
#include "dialog.h"

#define MAELSTROM_SCORES	"Maelstrom-Scores"
#define NUM_SCORES		10		// Do not change this!

/* Everyone can write to scores file if defined to 0 */
#define SCORES_PERMMASK		0

#define CLR_DIALOG_WIDTH	281
#define CLR_DIALOG_HEIGHT	111

Scores	hScores[NUM_SCORES];
Bool   gNetScores = 0;

#if BYTE_ORDER == BIG_ENDIAN
static inline unsigned long fliplong(unsigned long x)
{
	return((x<<24)|((x&0xff00)<<8)|((x>>8)&0xff00)|(x>>24));
}
#else
#define fliplong(x)	(x)
#endif

extern int NetLoadScores(void);		/* From netscore.cc */

void LoadScores(void)
{
	char *scoresfile;
	FILE *scores_fp;
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
	scoresfile = file2libpath(MAELSTROM_SCORES);
	memset(&hScores, 0, sizeof(hScores));
	if ( (scores_fp=fopen(scoresfile, "r")) == NULL )
		return;
	(void) fread(&hScores, sizeof(hScores), 1, scores_fp);
	fclose(scores_fp);

	for ( i=0; i<NUM_SCORES; ++i ) {
		hScores[i].wave = fliplong(hScores[i].wave);
		hScores[i].score = fliplong(hScores[i].score);
	}
}

void SaveScores(void)
{
	char *scoresfile;
	FILE *scores_fp;
	int   omask, i;

	/* Don't save network scores */
	if ( gNetScores )
		return;

	scoresfile = file2libpath(MAELSTROM_SCORES);
#ifndef __WIN95__
	omask=umask(SCORES_PERMMASK);
#endif
	scores_fp=fopen(scoresfile, "w");
	umask(omask);
	if ( scores_fp == NULL ) {
		error("Warning: Couldn't save scores to %s: ", scoresfile);
		error("\n\t\t");
		perror("");
		return;
	}
	for ( i=0; i<NUM_SCORES; ++i ) {
		hScores[i].wave = fliplong(hScores[i].wave);
		hScores[i].score = fliplong(hScores[i].score);
	}
	(void) fwrite(&hScores, sizeof(hScores), 1, scores_fp);
	fclose(scores_fp);

	for ( i=0; i<NUM_SCORES; ++i ) {
		hScores[i].wave = fliplong(hScores[i].wave);
		hScores[i].score = fliplong(hScores[i].score);
	}
}

/* Just show the high scores */
void PrintHighScores(void)
{
	int i;

	LoadScores();
	/* FIXME! -- Put all lines into a single formatted message */
	printf("Name			Score	Wave\n");
	for ( i=0; i<NUM_SCORES; ++i ) {
		printf("%-20s	%-3.1ld	%d\n", hScores[i].name,
					hScores[i].score, hScores[i].wave);
	}
}


static int     do_clear;

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
	MFont          *chicago;
	Maclike_Dialog *dialog;
	struct Title    splash;
	int             i, X, Y;
	Mac_Button     *clear;
	Mac_Button     *cancel;
	unsigned long   black, white;

	/* Set up all the components of the dialog box */
#ifdef CENTER_DIALOG
	X=(SCREEN_WIDTH-CLR_DIALOG_WIDTH)/2;
	Y=(SCREEN_HEIGHT-CLR_DIALOG_HEIGHT)/2;
#else	/* The way it is on the original Maelstrom */
	X=179;
	Y=89;
#endif
	black = win->Map_Color(0x0000, 0x0000, 0x0000);
	white = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
	dialog = new Maclike_Dialog(win, X, Y, 
				CLR_DIALOG_WIDTH, CLR_DIALOG_HEIGHT, white);
	if ( (chicago = fontserv->New_Font("Chicago", 12)) == NULL ) {
		error("Can't use Chicago font!\n");
		return(0);
	}
	if ( Load_Title(&splash, 102) < 0 ) {
		error("Can't load score zapping splash!\n");
		return(0);
	}
	dialog->Add_Title(&splash, X+8, Y+8);
	do_clear = 0;
	clear = new Mac_Button(win, X+103, Y+78, BUTTON_WIDTH, BUTTON_HEIGHT,
			"Clear", chicago, fontserv, black, white, 
							Clear_callback);
	dialog->Add_Dialog(clear);
	cancel = new Mac_DefaultButton(win, X+103+BUTTON_WIDTH+14, Y+78, 
			BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel", 
			chicago, fontserv, black, white, Cancel_callback);
	dialog->Add_Dialog(cancel);
	win->Refresh();

	/* Run the dialog box */
	dialog->Run();

	/* Clean up and return */
	Free_Title(&splash);
	delete clear;
	delete cancel;
	delete dialog;
	fontserv->Free_Font(chicago);
	if ( do_clear ) {
		for ( i=0; i<10; ++i ) {
			hScores[i].wave = 0;
			hScores[i].score = 0L;
			hScores[i].name[0] = '\0';
		}
		SaveScores();
		gLastHigh = -1;

		/* Fade for screen update */
		win->Fade(FADE_STEPS/2);
		gUpdateBuffer = true;
		gFadeBack = true;
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
	static char    *Ltext1 = 
			"Enter the level to start from (1-40).  This";
	static char    *Ltext2 = 
			"disqualifies you from a high score...";
	static char    *Ltext3 = "Level:";
	static char    *Ltext4 = "Lives:";
	unsigned long   black, white;
	MFont          *chicago;
	Maclike_Dialog *dialog;
	CIcon          *splash;
	BitMap         *text1, *text2, *text3, *text4;
	static char    *turbotext = "Turbofunk On";
	int             x, y, X, Y;
	Dialog         *doit;
	Dialog         *cancel;
	Numeric_Entry  *numeric_entry;
	CheckBox       *checkbox;
	int             startlevel=10, startlives=5, turbofunk=0;

	/* Set up all the components of the dialog box */
	X=(SCREEN_WIDTH-LVL_DIALOG_WIDTH)/2;
	Y=(SCREEN_HEIGHT-LVL_DIALOG_HEIGHT)/2;
	black = win->Map_Color(0x0000, 0x0000, 0x0000);
	white = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
	dialog = new Maclike_Dialog(win, X, Y, 
				LVL_DIALOG_WIDTH, LVL_DIALOG_HEIGHT, white);
	if ( (chicago = fontserv->New_Font("Chicago", 12)) == NULL ) {
		error("Can't use Chicago font!\n");
		return(0);
	}
	if ( (splash=GetCIcon(103)) == NULL ) {
		error("Can't load alien level splash!\n");
		return(0);
	}
	x = y = 18;
	dialog->Add_CIcon(splash, X+x, Y+y);
	x += (splash->width+14);
	text1 = fontserv->Text_to_BitMap(Ltext1, chicago, STYLE_NORM);
	dialog->Add_BitMap(text1, X+x, Y+y, black);
	y += (text1->height+2);
	text2 = fontserv->Text_to_BitMap(Ltext2, chicago, STYLE_NORM);
	dialog->Add_BitMap(text2, X+x, Y+y, black);
	do_level = 0;
	cancel = new Mac_Button(win, X+170, Y+100, 73, BUTTON_HEIGHT,
			"Cancel", chicago, fontserv, black, white, 
							Cancel2_callback);
	dialog->Add_Dialog(cancel);
	doit = new Mac_DefaultButton(win, X+170+73+14, Y+100, 
			BUTTON_WIDTH, BUTTON_HEIGHT, "Do it!", 
			chicago, fontserv, black, white, Level_callback);
	dialog->Add_Dialog(doit);
	numeric_entry = new Numeric_Entry(win, X, Y, chicago, fontserv,
							black, white);
	numeric_entry->Add_Entry(X+82, Y+64, 3, 1, &startlevel);
	text3 = fontserv->Text_to_BitMap(Ltext3, chicago, STYLE_NORM);
	dialog->Add_BitMap(text3, X+82-text3->width-2, Y+64+3, black);
	numeric_entry->Add_Entry(X+82, Y+90, 3, 0, &startlives);
	text4 = fontserv->Text_to_BitMap(Ltext4, chicago, STYLE_NORM);
	dialog->Add_BitMap(text4, X+82-text3->width-2, Y+90+3, black);
	dialog->Add_Dialog(numeric_entry);
	checkbox = new CheckBox(win, X+140, Y+68, turbotext, chicago,
					fontserv, black, white, &turbofunk);
	dialog->Add_Dialog(checkbox);
	win->Refresh();

	/* Run the dialog box */
	dialog->Run();

	/* Clean up and return */
	FreeCIcon(splash);
	fontserv->Free_Text(text1);
	fontserv->Free_Text(text2);
	fontserv->Free_Text(text3);
	fontserv->Free_Text(text4);
	delete cancel;
	delete doit;
	delete numeric_entry;
	delete dialog;
	fontserv->Free_Font(chicago);
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
