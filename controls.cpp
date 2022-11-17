
/* This file handles the controls configuration and updating the keystrokes 
*/

#include <string.h>

#include "Maelstrom_Globals.h"
#include "dialog.h"
#include "keyboard.h"
#include "joystick.h"

#define MAELSTROM_DATA	".Maelstrom-data"


/* Savable and configurable controls/data */

       /* Pause         Shield    Thrust TurnR     TurnL    Fire    Quit     */
#ifdef FAITHFUL_SPECS
Controls controls =
	{ XK_Caps_Lock, XK_space, XK_Up, XK_Right, XK_Left, XK_Tab, XK_Escape };
#else
Controls controls =
	{ XK_Pause, XK_space, XK_Up, XK_Right, XK_Left, XK_Tab, XK_Escape };
#endif

#ifdef MOVIE_SUPPORT
int	gMovie = 0;
#endif
int     gSoundLevel = 4;
int	gGammaCorrect = 0;
#ifdef USE_JOYSTICK
Joystick *Jstick = NULL;
#endif


/* Map a keycode to a key name */
char *KeyName(unsigned short keycode)
{
	int i;

	/* Check normal keys */
	for ( i=0; keycodes[i].name; ++i ) {
		if ( keycode == keycodes[i].key )
			return(keycodes[i].name);
	}
	/* Check shifted keys */
	for ( i=0; keycodes[i].name; ++i ) {
		if ( keycode == keycodes[i].shift )
			return(keycodes[i].name);
	}
	return(NULL);
}

static FILE *OpenData(char *mode, char **fname)
{
	static char datafile[BUFSIZ];
	char *home;
	FILE *data;

#ifdef __WIN95__
	home = ".";
#else
	if ( (home=getenv("HOME")) == NULL )
		home="";
#endif

	if ( fname )
		*fname = datafile;
#ifdef SYSTEM
	sprintf(datafile,  "%s/%s-%s", home, MAELSTROM_DATA, SYSTEM);
	if ( (data=fopen(datafile, mode)) == NULL ) {
		sprintf(datafile,  "%s/%s", home, MAELSTROM_DATA);
		if ( (data=fopen(datafile, mode)) == NULL )
			return(NULL);
	}
#else
	sprintf(datafile,  "%s/%s", home, MAELSTROM_DATA);
	if ( (data=fopen(datafile, mode)) == NULL )
		return(NULL);
#endif
	return(data);
}

void LoadControls(void)
{
	char  buffer[BUFSIZ], *datafile;
	FILE *data;

	/* Open our control data file */
	if ( (data=OpenData("r", &datafile)) == NULL )
		return;

	if ( (fread(buffer, 1, 4, data) != 4) || strncmp(buffer, "MAEL", 4) ) {
		error(
		"Warning: Data file '%s' is corrupt! (will fix)\n", datafile);
		fclose(data);
		return;
	}
	(void) fread(&gSoundLevel, sizeof(gSoundLevel), 1, data);
	(void) fread(&controls, sizeof(controls), 1, data);
	if ( ! fread(&gGammaCorrect, sizeof(gGammaCorrect), 1, data) )
		gGammaCorrect = 0;
#ifdef USE_JOYSTICK
#define JOYSTICK_MAGIC	(('J'+'O'+'Y')&0xFF)
	if ( ! Jstick ) {
		char *joystick = NULL;
		if ( (getc(data) == JOYSTICK_MAGIC) &&
					fgets(buffer, BUFSIZ-1, data) ) {
			struct joy_sensitivity calib;

			buffer[strlen(buffer)-1] = '\0';
			if ( strlen(buffer) )
				joystick = buffer;

			if ( fread(&calib, sizeof(calib), 1, data) )
				Jstick = new Joystick(joystick, &calib);
			else
				Jstick = new Joystick(joystick, NULL);
		} else
			Jstick = new Joystick(joystick, NULL);
	}
#endif
	fclose(data);
}

void CalibrateJoystick(char *joystick)
{
#ifdef USE_JOYSTICK
	char  buffer[BUFSIZ];
	FILE *data;
	struct joy_sensitivity calib;

	/* Did the normal initialization fail?  No init data? :) */
	if ( ! Jstick )
		Jstick = new Joystick(NULL, NULL);

	/* Try to pull the joystick device out of the data file, if needed */
	if ( ! joystick && ((data=OpenData("r", NULL)) != NULL) ) {
		if ( (fseek(data, 4+sizeof(gSoundLevel)+sizeof(controls)+
				sizeof(gGammaCorrect), SEEK_SET) == 0) && 
			(getc(data) == JOYSTICK_MAGIC) &&
					fgets(buffer, BUFSIZ-1, data) ) {
			buffer[strlen(buffer)-1] = '\0';
			if ( strlen(buffer) )
				joystick = buffer;
		}
		fclose(data);
	}
	if ( Jstick->Calibrate(joystick, &calib) == 0 ) {
		/* Save the joystick calibration information */
		SaveControls();
		if ( (data=OpenData("r+", NULL)) == NULL )
			return;
		
		if ( fseek(data, 4+sizeof(gSoundLevel)+sizeof(controls)+
				sizeof(gGammaCorrect), SEEK_SET) == 0 ) {
			putc(JOYSTICK_MAGIC, data);
			fprintf(data, "%s\n", joystick ? joystick : "");
			fwrite(&calib, sizeof(calib), 1, data);
		}
		fclose(data);
	}
	return;
#else
	Unused(joystick);
#endif
}

void SaveControls(void)
{
	char  *datafile, *newmode;
	FILE *data;

	/* Don't clobber existing joystick data */
	if ( (data=OpenData("r", NULL)) != NULL ) {
		newmode = "r+";
		fclose(data);
	} else
		newmode = "w";

	if ( (data=OpenData(newmode, &datafile)) == NULL ) {
		error("Warning: Couldn't save controls to %s: ", datafile);
		error("\n\t\t");
		perror("");
		return;
	}

	(void) fwrite("MAEL", 1, 4, data);
	(void) fwrite(&gSoundLevel, sizeof(gSoundLevel), 1, data);
	(void) fwrite(&controls, sizeof(controls), 1, data);
	(void) fwrite(&gGammaCorrect, sizeof(gGammaCorrect), 1, data);
	fclose(data);
}

#define FIRE_CTL	0
#define THRUST_CTL	1
#define SHIELD_CTL	2
#define TURNR_CTL	3
#define TURNL_CTL	4
#define PAUSE_CTL	5
#define QUIT_CTL	6
#define NUM_CTLS	7

#define SP		3

#define CTL_DIALOG_WIDTH	482
#define CTL_DIALOG_HEIGHT	300

Controls newcontrols;
static struct {
	char *label;
	int  yoffset;
	unsigned short *control;
} checkboxes[] = {
	{ "Fire",	0*BOX_HEIGHT+0*SP, &newcontrols.gFireControl },
	{ "Thrust",	1*BOX_HEIGHT+1*SP, &newcontrols.gThrustControl },
	{ "Shield",	2*BOX_HEIGHT+2*SP, &newcontrols.gShieldControl },
	{ "Turn Clockwise", 3*BOX_HEIGHT+3*SP, &newcontrols.gTurnRControl },
	{ "Turn Counter-Clockwise",
			4*BOX_HEIGHT+4*SP, &newcontrols.gTurnLControl },
	{ "Pause",	5*BOX_HEIGHT+5*SP, &newcontrols.gPauseControl },
	{ "Abort Game",	6*BOX_HEIGHT+6*SP, &newcontrols.gQuitControl },
	};

static int     X=0;
static int     Y=0;
static MFont  *chicago;
static BitMap *keynames[NUM_CTLS];
static int     currentbox, valid;
static unsigned long black, white;

static int OK_callback(void) {
	valid = 1;
	return(1);
}
static int Cancel_callback(void) {
	valid = 0;
	return(1);
}
static void BoxKeyPress(KeySym key, int *doneflag)
{
	char *keyname;
	int   i;

	Unused(doneflag);		/* Callback doesn't set doneflag */

	if ( key == *checkboxes[currentbox].control )
		return;

	/* Make sure the key isn't in use! */
	for ( i=0; i<NUM_CTLS; ++i ) {
		if ( key == *checkboxes[i].control ) {
			key = *checkboxes[currentbox].control;

			/* Clear the current text */
			win->Blit_BitMap(
				X+96+(BOX_WIDTH-keynames[currentbox]->width)/2, 
				Y+75+SP+checkboxes[currentbox].yoffset,
				keynames[currentbox]->width, 
				keynames[currentbox]->height,
					keynames[currentbox]->bits, white);
			fontserv->Free_Text(keynames[currentbox]);

			/* Blit the new message */
			keyname = "That key is in use!";
			keynames[currentbox] = fontserv->Text_to_BitMap(keyname,
							chicago, STYLE_NORM);
			win->Blit_BitMap(
				X+96+(BOX_WIDTH-keynames[currentbox]->width)/2, 
				Y+75+SP+checkboxes[currentbox].yoffset,
				keynames[currentbox]->width, 
				keynames[currentbox]->height,
					keynames[currentbox]->bits, black);
			win->Refresh();
			win->Flush(1);
			sleep(1);
			break;
		}
	}

	/* Clear the current text */
	win->Blit_BitMap(X+96+(BOX_WIDTH-keynames[currentbox]->width)/2, 
		Y+75+SP+checkboxes[currentbox].yoffset,
		keynames[currentbox]->width, keynames[currentbox]->height,
					 keynames[currentbox]->bits, white);
	fontserv->Free_Text(keynames[currentbox]);

	/* Display the new key */
	*checkboxes[currentbox].control = key;
	if (!(keyname=KeyName(*checkboxes[currentbox].control)))
		keyname = "Unknown Key";
	keynames[currentbox] = fontserv->Text_to_BitMap(keyname,
							chicago, STYLE_NORM);
	win->Blit_BitMap(X+96+(BOX_WIDTH-keynames[currentbox]->width)/2, 
		Y+75+SP+checkboxes[currentbox].yoffset,
		keynames[currentbox]->width, keynames[currentbox]->height,
					 keynames[currentbox]->bits, black);
	win->Refresh();
}

void ConfigureControls(void)
{
#ifdef FAITHFUL_SPECS
	static char    *C_text1 = 
		"While playing Maelstrom, CAPS LOCK pauses the game and";
	static char    *C_text2 = 
		"ESC aborts the game.";
	BitMap         *text1, *text2;
#endif
	Maclike_Dialog *dialog;
	struct Title    splash;
	BitMap         *labels[NUM_CTLS];
	int             i;
	char           *keyname;
	Mac_Button     *okay;
	Mac_Button     *cancel;
	Radio_List     *radiobuttons;
	Dialog         *boxes;


	/* Set up all the components of the dialog box */
	X=(SCREEN_WIDTH-CTL_DIALOG_WIDTH)/2;
	Y=(SCREEN_HEIGHT-CTL_DIALOG_HEIGHT)/2;
	black = win->Map_Color(0x0000, 0x0000, 0x0000);
	white = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
	dialog = new Maclike_Dialog(win, X, Y, 
				CTL_DIALOG_WIDTH, CTL_DIALOG_HEIGHT, white);
	if ( (chicago = fontserv->New_Font("Chicago", 12)) == NULL ) {
		error("Can't use Chicago font!\n");
		return;
	}
	if ( Load_Title(&splash, 100) < 0 ) {
		error("Can't load configuration splash!\n");
		return;
	}
	dialog->Add_Title(&splash, X+8, Y+8);
#ifdef FAITHFUL_SPECS
	text1 = fontserv->Text_to_BitMap(C_text1, chicago, STYLE_NORM);
	text2 = fontserv->Text_to_BitMap(C_text2, chicago, STYLE_NORM);
	dialog->Add_BitMap(text1, X+70, Y+220, black);
	dialog->Add_BitMap(text2, X+70, Y+236, black);
#endif
	valid = 0;
	cancel = new Mac_Button(win, X+295, Y+269, BUTTON_WIDTH, BUTTON_HEIGHT,
			"Cancel", chicago, fontserv, black, white, 
							Cancel_callback);
	dialog->Add_Dialog(cancel);
	okay = new Mac_Button(win, X+295+BUTTON_WIDTH+26, Y+269, 
			BUTTON_WIDTH, BUTTON_HEIGHT, "OK", chicago, 
					fontserv, black, white, OK_callback);
	dialog->Add_Dialog(okay);
	memcpy(&newcontrols, &controls, sizeof(controls));
	radiobuttons = new Radio_List(win, X+266, Y+75, NUM_CTLS);
	boxes = new Dialog(win, X+96, Y+75);
	currentbox = FIRE_CTL;
	for ( i=0; i<NUM_CTLS; ++i ) {
		labels[i] = fontserv->Text_to_BitMap(checkboxes[i].label,
							chicago, STYLE_NORM);
		if ( (keyname=KeyName(*checkboxes[i].control)) == NULL )
			keyname = "Unknown key";
		keynames[i] = fontserv->Text_to_BitMap(keyname,
							chicago, STYLE_NORM);
		/* Only display "Fire" through "Turn Counter-Clockwise" */
#ifdef FAITHFUL_SPECS
		if ( i <  PAUSE_CTL ) {
#else
		if ( i <  NUM_CTLS ) {
#endif
			radiobuttons->Add_Radio(X+267, 
				Y+75+checkboxes[i].yoffset,
						i == FIRE_CTL, labels[i]);
			win->DrawRectangle(X+96, Y+75+checkboxes[i].yoffset,
						BOX_WIDTH,BOX_HEIGHT,black);
			win->Blit_BitMap(
				X+96+(BOX_WIDTH-keynames[i]->width)/2, 
				Y+75+SP+checkboxes[i].yoffset,
				keynames[i]->width, keynames[i]->height,
						keynames[i]->bits, black);
		}
	}
	radiobuttons->Set_RadioVar(&currentbox);
	dialog->Add_Dialog(radiobuttons);
	boxes->SetKeyPress(BoxKeyPress);
	dialog->Add_Dialog(boxes);
	win->Refresh();

	/* Run the dialog box */
	dialog->Run();

	/* Clean up and return */
	Free_Title(&splash);
	delete cancel;
	delete okay;
	delete radiobuttons;
	delete dialog;
#ifdef FAITHFUL_SPECS
	fontserv->Free_Text(text1);
	fontserv->Free_Text(text2);
#endif
	for ( i=0; i<NUM_CTLS; ++i ) {
		fontserv->Free_Text(labels[i]);
		fontserv->Free_Text(keynames[i]);
	}
	fontserv->Free_Font(chicago);
	if ( valid ) {
		memcpy(&controls, &newcontrols, sizeof(controls));
		SaveControls();
	}
	return;
}

/* Wait for next event; timeout is specified in 1/60 of a second */
int PollEvent(XEvent *event, int timeout)
{
	do { 
		if ( win->NumEvents() ) {
			win->GetEvent(event);
			return(1);
		}
		if ( timeout ) {
			/* Delay 1/60 of a second... */
			Delay(1);
		}
	} while ( timeout-- );
	return(0);
}

int gRefreshDisplay;
static void HandleEvent(int numevents)
{
	char   buf[128];
	KeySym key;
	XEvent event;

	while ( numevents-- ) {
		win->GetEvent(&event);

		switch (event.type) {
			/* -- Handle unknown refresh */
			case Expose:
				gRefreshDisplay = 1;
				break;
			/* -- Handle mouse clicks */
			case ButtonPress:
				HandleMouse(&event);
				break;
			/* -- Handle key presses/releases */
			case KeyPress:
//error("Down!\n");
				/* Clear state info */
				event.xkey.state = 0;
				win->KeyToAscii(&event, buf, 127, &key);
				/* Check for various control keys */
				if ( key == controls.gFireControl )
					SetControl(FIRE_KEY, 1);
				else if ( key == controls.gTurnRControl )
					SetControl(RIGHT_KEY, 1);
				else if ( key == controls.gTurnLControl )
					SetControl(LEFT_KEY, 1);
				else if ( key == controls.gShieldControl )
					SetControl(SHIELD_KEY, 1);
				else if ( key == controls.gThrustControl )
					SetControl(THRUST_KEY, 1);
				else if ( key == controls.gPauseControl )
					SetControl(PAUSE_KEY, 1);
				else if ( key == controls.gQuitControl )
					SetControl(ABORT_KEY, 1);
				else if ( SpecialKey(key) == 0 )
					/* The key has been handled */;
				else if ( key == XK_F3 ) {
					/* Special key --
						Do a screen dump here.
					 */
					win->ScreenDump("ScreenShot", NULL);
// DEBUG
				} else if ( key == XK_F4 ) {
					win->DebugArt();
#ifdef MOVIE_SUPPORT
				} else if ( key == XK_F5 ) {
					/* Special key --
						Toggle movie function.
					 */
					extern int SelectMovieRect(void);
					if ( ! gMovie )
						gMovie = SelectMovieRect();
					else
						gMovie = 0;
mesg("Movie is %s...\n", gMovie ? "started" : "stopped");
#endif
				}
				break;
			case KeyRelease:
//error("Up!\n");
				/* Clear state info */
				event.xkey.state = 0;
				win->KeyToAscii(&event, buf, 127, &key);
				/* Update control key status */
				if ( key == controls.gFireControl )
					SetControl(FIRE_KEY, 0);
				else if ( key == controls.gTurnRControl )
					SetControl(RIGHT_KEY, 0);
				else if ( key == controls.gTurnLControl )
					SetControl(LEFT_KEY, 0);
				else if ( key == controls.gShieldControl )
					SetControl(SHIELD_KEY, 0);
				else if ( key == controls.gThrustControl )
					SetControl(THRUST_KEY, 0);
				break;
		}
	}
}

#ifdef USE_JOYSTICK
void HandleJoystick(void)
{
	const  int threshold = 20;	/* 20 percent movement threshold */
	static int old_x=0;
	static int old_y=0;
	joystate   js;

	if ( Jstick->Event(&js) < 0 )
		return;

	/* Check the button keys */
	/* If you want to change joystick button mappings, do it here. */
	if ( js.buttons&BUTTON_1 ) {
		if ( PRESSED(&js, BUTTON_1) )
			SetControl(FIRE_KEY, 1);
		else
			SetControl(FIRE_KEY, 0);
	}
	if ( js.buttons&BUTTON_2 ) {
		if ( PRESSED(&js, BUTTON_2) )
			SetControl(SHIELD_KEY, 1);
		else
			SetControl(SHIELD_KEY, 0);
	}

	/* Check for thrusting */
	if ( (old_y < -threshold) && (js.y_axis > -threshold) ) {
		SetControl(THRUST_KEY, 0);
	} else if ( (old_y > -threshold) && (js.y_axis < -threshold) ) {
		SetControl(THRUST_KEY, 1);
	}
	old_y = js.y_axis;

//mesg("X axis = %d\n", js.x_axis);
	/* Check for rotation */
	if ( (old_x > threshold) && (js.x_axis < threshold) ) { /* Center */
		SetControl(RIGHT_KEY, 0);
		if ( js.x_axis < -threshold )	/* Heading left */
			SetControl(LEFT_KEY, 1);
	} else
	if ( (old_x < -threshold) && (js.x_axis > -threshold) ) { /* Center */
		SetControl(LEFT_KEY, 0);
		if ( js.x_axis > threshold )	/* Heading right */
			SetControl(RIGHT_KEY, 1);
	} else
	if ( js.x_axis > threshold ) {		/* Heading right */
		SetControl(RIGHT_KEY, 1);
	} else
	if ( js.x_axis < -threshold ) {		/* Heading left */
		SetControl(LEFT_KEY, 1);
	}
	old_x = js.x_axis;
}
#endif /* USE_JOYSTICK */


/* This function gives a good way to delay a specified amount of time
   while handling keyboard/joystick events, or just to poll for events.
*/
void HandleEvents(int timeout)
{
	int numevents;

	do { 
#ifdef USE_JOYSTICK
		HandleJoystick();
#endif
		if ( (numevents=win->NumEvents()) ) {
			HandleEvent(numevents);
			return;
		}
		if ( timeout ) {
			/* Delay 1/60 of a second... */
			Delay(1);
		}
	} while ( timeout-- );
}

#define DAWN_DIALOG_WIDTH	318
#define DAWN_DIALOG_HEIGHT	194

void ShowDawn(void)
{
	static char *D_text[6] = {
		"No eternal reward will forgive us",
		"now",
		    "for",
		        "wasting",
		                "the",
		                    "dawn."
	};
	BitMap         *text[6];
	MFont          *chicago;
	Maclike_Dialog *dialog;
	CIcon          *splash;
	int             i, x, y, X, Y;
	Mac_Button     *def_button;

	/* Set up all the components of the dialog box */
#ifdef CENTER_DIALOG
	X=(SCREEN_WIDTH-DAWN_DIALOG_WIDTH)/2;
	Y=(SCREEN_HEIGHT-DAWN_DIALOG_HEIGHT)/2;
#else	/* The way it is on the original Maelstrom */
	X=160;
	Y=73;
#endif
	black = win->Map_Color(0x0000, 0x0000, 0x0000);
	white = win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
	dialog = new Maclike_Dialog(win, X, Y, 
				DAWN_DIALOG_WIDTH, DAWN_DIALOG_HEIGHT, white);
	if ( (chicago = fontserv->New_Font("Chicago", 12)) == NULL ) {
		error("Can't use Chicago font!\n");
		return;
	}
	if ( (splash=GetCIcon(103)) == NULL ) {
		error("Can't load alien dawn splash!\n");
		return;
	}
	x = y = 23;
	dialog->Add_CIcon(splash, X+x, Y+y);
	x += (splash->width+26);
	text[0] = fontserv->Text_to_BitMap(D_text[0], chicago, STYLE_NORM);
	dialog->Add_BitMap(text[0], X+x, Y+y, black);
	for ( i=1; i<6; ++i ) {
		y += (text[i-1]->height+2);
		text[i] = fontserv->Text_to_BitMap(D_text[i], 
						chicago, STYLE_NORM);
		dialog->Add_BitMap(text[i], X+x, Y+y, black);
		x += (text[i]->width+2);
	}
	def_button = new Mac_DefaultButton(win, X+214, Y+164,
			90, BUTTON_HEIGHT, "OK", chicago, fontserv, 
							black, white, NULL);
	dialog->Add_Dialog(def_button);
	win->Refresh();

	/* Run the dialog box */
	dialog->Run();

	/* Clean up and return */
	FreeCIcon(splash);
	for ( i=0; i<6; ++i )
		fontserv->Free_Text(text[i]);
	delete def_button;
	delete dialog;
	fontserv->Free_Font(chicago);
	return;
}
