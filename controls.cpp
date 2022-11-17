
/* This file handles the controls configuration and updating the keystrokes 
*/

#include <string.h>
#include <ctype.h>

#include "Maelstrom_Globals.h"
#include "load.h"
#include "dialog.h"
//#include "keyboard.h"
//#include "joystick.h"

#define MAELSTROM_DATA	".Maelstrom-data"


/* Savable and configurable controls/data */

/* Pause        Shield     Thrust  TurnR      TurnL     Fire     Quit  */
#ifdef FAITHFUL_SPECS
Controls controls =
{ SDLK_CAPSLOCK,SDLK_SPACE,SDLK_UP,SDLK_RIGHT,SDLK_LEFT,SDLK_TAB,SDLK_ESCAPE };
#else
Controls controls =
   { SDLK_PAUSE,SDLK_SPACE,SDLK_UP,SDLK_RIGHT,SDLK_LEFT,SDLK_TAB,SDLK_ESCAPE };
#endif

#ifdef MOVIE_SUPPORT
int	gMovie = 0;
#endif
Uint8 gSoundLevel = 4;
Uint8 gGammaCorrect = 3;
#ifdef USE_JOYSTICK
Joystick *Jstick = NULL;
#endif


/* Map a keycode to a key name */
void KeyName(SDLKey keycode, char *namebuf)
{
	char *name, ch;
	int starting;

	/* Get the name of the key */
	name = SDL_GetKeyName(keycode);

	/* Add "arrow" to the arrow keys */
	if ( strcmp(name, "up") == 0 ) {
		name = "up arrow";
	} else
	if ( strcmp(name, "down") == 0 ) {
		name = "down arrow";
	} else
	if ( strcmp(name, "right") == 0 ) {
		name = "right arrow";
	} else
	if ( strcmp(name, "left") == 0 ) {
		name = "left arrow";
	}
	/* Make the key names uppercased */
	for ( starting = 1; *name; ++name ) {
		ch = *name;
		if ( starting ) {
			if ( islower(ch) )
				ch = toupper(ch);
			starting = 0;
		} else {
			if ( ch == ' ' )
				starting = 1;
		}
		*namebuf++ = ch;
	}
	*namebuf = '\0';
}

static FILE *OpenData(char *mode, char **fname)
{
	static char datafile[BUFSIZ];
	char *home;
	FILE *data;

#ifdef WIN32
	home = ".";
#else
	if ( (home=getenv("HOME")) == NULL )
		home="";
#endif

	if ( fname ) {
		*fname = datafile;
	}
	sprintf(datafile,  "%s/%s", home, MAELSTROM_DATA);
	if ( (data=fopen(datafile, mode)) == NULL )
		return(NULL);
	return(data);
}

void LoadControls(void)
{
	char  buffer[BUFSIZ], *datafile;
	FILE *data;

	/* Open our control data file */
	data = OpenData("r", &datafile);
	if ( data == NULL ) {
		return;
	}

	/* Read the controls */
	if ( (fread(buffer, 1, 5, data) != 5) || strncmp(buffer, "MAEL3", 5) ) {
		error(
		"Warning: Data file '%s' is corrupt! (will fix)\n", datafile);
		fclose(data);
		return;
	}
	fread(&gSoundLevel, sizeof(gSoundLevel), 1, data);
	fread(&controls, sizeof(controls), 1, data);
	fread(&gGammaCorrect, sizeof(gGammaCorrect), 1, data);
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
		error("Warning: Couldn't save controls to %s\n", datafile);
		return;
	}

	fwrite("MAEL3", 1, 5, data);
	fwrite(&gSoundLevel, sizeof(gSoundLevel), 1, data);
	fwrite(&controls, sizeof(controls), 1, data);
	fwrite(&gGammaCorrect, sizeof(gGammaCorrect), 1, data);
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
	SDLKey *control;
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

static int X=0;
static int Y=0;
static MFont *chicago;
static SDL_Surface *keynames[NUM_CTLS];
static int currentbox, valid;

static int OK_callback(void) {
	valid = 1;
	return(1);
}
static int Cancel_callback(void) {
	valid = 0;
	return(1);
}
static void BoxKeyPress(SDL_keysym key, int *doneflag)
{
	SDL_Color black = { 0x00, 0x00, 0x00, 0 };
	SDL_Color white = { 0xFF, 0xFF, 0xFF, 0 };
	int i;
	char keyname[128];

	if ( key.sym == *checkboxes[currentbox].control )
		return;

	/* Make sure the key isn't in use! */
	for ( i=0; i<NUM_CTLS; ++i ) {
		if ( key.sym == *checkboxes[i].control ) {
			key.sym = (SDLKey)*checkboxes[currentbox].control;

			/* Clear the current text */
			fontserv->InvertText(keynames[currentbox]);
			screen->QueueBlit(
				X+96+(BOX_WIDTH-keynames[currentbox]->w)/2, 
				Y+75+SP+checkboxes[currentbox].yoffset,
						keynames[currentbox], NOCLIP);
			screen->Update();
			fontserv->FreeText(keynames[currentbox]);

			/* Blit the new message */
			strcpy(keyname, "That key is in use!");
			keynames[currentbox] = fontserv->TextImage(keyname,
					chicago, STYLE_NORM, black, white);
			screen->QueueBlit(
				X+96+(BOX_WIDTH-keynames[currentbox]->w)/2, 
				Y+75+SP+checkboxes[currentbox].yoffset,
					keynames[currentbox], NOCLIP);
			screen->Update();
			SDL_Delay(1000);
			break;
		}
	}

	/* Clear the current text */
	fontserv->InvertText(keynames[currentbox]);
	screen->QueueBlit(X+96+(BOX_WIDTH-keynames[currentbox]->w)/2, 
				Y+75+SP+checkboxes[currentbox].yoffset,
						keynames[currentbox], NOCLIP);
	screen->Update();
	fontserv->FreeText(keynames[currentbox]);

	/* Display the new key */
	*checkboxes[currentbox].control = key.sym;
	KeyName(*checkboxes[currentbox].control, keyname);
	keynames[currentbox] = fontserv->TextImage(keyname, chicago, STYLE_NORM,
								black, white);
	screen->QueueBlit(X+96+(BOX_WIDTH-keynames[currentbox]->w)/2, 
				Y+75+SP+checkboxes[currentbox].yoffset,
						keynames[currentbox], NOCLIP);
	screen->Update();
}

void ConfigureControls(void)
{
#ifdef FAITHFUL_SPECS
	static char *C_text1 = 
		"While playing Maelstrom, CAPS LOCK pauses the game and";
	static char *C_text2 = 
		"ESC aborts the game.";
	SDL_Surface *text1, *text2;
#endif
	Uint32 black;
	int i;
	char keyname[128];
	Maclike_Dialog *dialog;
	SDL_Surface *splash;
	Mac_Button *cancel, *okay;
	Mac_RadioList *radiobuttons;
	Mac_Dialog *boxes;


	/* Set up all the components of the dialog box */
	black = screen->MapRGB(0x00, 0x00, 0x00);
	if ( (chicago = fontserv->NewFont("Chicago", 12)) == NULL ) {
		error("Can't use Chicago font!\n");
		return;
	}
	if ( (splash = Load_Title(screen, 100)) == NULL ) {
		error("Can't load configuration splash!\n");
		delete chicago;
		return;
	}
	X=(SCREEN_WIDTH-CTL_DIALOG_WIDTH)/2;
	Y=(SCREEN_HEIGHT-CTL_DIALOG_HEIGHT)/2;
	dialog = new Maclike_Dialog(X, Y, CTL_DIALOG_WIDTH, CTL_DIALOG_HEIGHT,
									screen);
	dialog->Add_Image(splash, 4, 4);
#ifdef FAITHFUL_SPECS
	text1 = fontserv->TextImage(C_text1,chicago,STYLE_NORM,0x00,0x00,0x00);
	text2 = fontserv->TextImage(C_text2,chicago,STYLE_NORM,0x00,0x00,0x00);
	dialog->Add_Image(text1, 66, 216);
	dialog->Add_Image(text2, 66, 232);
#endif
	valid = 0;
	cancel = new Mac_Button(291, 265, BUTTON_WIDTH, BUTTON_HEIGHT,
				"Cancel", chicago, fontserv, Cancel_callback);
	dialog->Add_Dialog(cancel);
	okay = new Mac_Button(291+BUTTON_WIDTH+26, 265, 
					BUTTON_WIDTH, BUTTON_HEIGHT,
				"OK", chicago, fontserv, OK_callback);
	dialog->Add_Dialog(okay);
	memcpy(&newcontrols, &controls, sizeof(controls));
	radiobuttons = new Mac_RadioList(&currentbox, X+266, Y+75,
							chicago, fontserv);
	currentbox = FIRE_CTL;
	for ( i=0; i<NUM_CTLS; ++i ) {
		KeyName(*checkboxes[i].control, keyname);
		keynames[i] = fontserv->TextImage(keyname, chicago, STYLE_NORM,
							0x00, 0x00, 0x00);
		/* Only display "Fire" through "Turn Counter-Clockwise" */
#ifdef FAITHFUL_SPECS
		if ( i <  PAUSE_CTL ) {
#else
		if ( i <  NUM_CTLS ) {
#endif
			radiobuttons->Add_Radio(263, 71+checkboxes[i].yoffset,
							checkboxes[i].label);
			dialog->Add_Rectangle(92, 71+checkboxes[i].yoffset,
						BOX_WIDTH, BOX_HEIGHT, black);
			dialog->Add_Image(keynames[i],
					92+(BOX_WIDTH-keynames[i]->w)/2, 
						71+SP+checkboxes[i].yoffset);
		}
	}
	dialog->Add_Dialog(radiobuttons);
	boxes = new Mac_Dialog(92, 71);
	boxes->SetKeyPress(BoxKeyPress);
	dialog->Add_Dialog(boxes);

	/* Run the dialog box */
	dialog->Run(EXPAND_STEPS);

	/* Clean up and return */
	screen->FreeImage(splash);
#ifdef FAITHFUL_SPECS
	fontserv->FreeText(text1);
	fontserv->FreeText(text2);
#endif
	for ( i=0; i<NUM_CTLS; ++i ) {
		fontserv->FreeText(keynames[i]);
	}
	delete chicago;
	delete dialog;
	if ( valid ) {
		memcpy(&controls, &newcontrols, sizeof(controls));
		SaveControls();
	}
	return;
}

static void HandleEvent(SDL_Event *event)
{
	SDLKey key;

	switch (event->type) {
		/* -- Handle key presses/releases */
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			key = event->key.keysym.sym;
			if ( event->key.state == SDL_PRESSED ) {
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
				else if ( SpecialKey(event->key.keysym) == 0 )
					/* The key has been handled */;
				else if ( key == SDLK_F3 ) {
					/* Special key --
						Do a screen dump here.
					 */
					screen->ScreenDump("ScreenShot",
								0, 0, 0, 0);
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
			} else {
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
			}
			break;

		case SDL_QUIT:
			SetControl(ABORT_KEY, 1);
			break;
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
	SDL_Event event;

	do { 
#ifdef USE_JOYSTICK
		HandleJoystick();
#endif
		while ( SDL_PollEvent(&event) ) {
			HandleEvent(&event);
		}
		if ( timeout ) {
			/* Delay 1/60 of a second... */
			Delay(1);
		}
	} while ( timeout-- );
}

void DropEvents(void)
{
	SDL_Event event;

	while ( SDL_PollEvent(&event) ) {
		/* Keep going */;
	}
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
	MFont *chicago;
	SDL_Surface *splash, *text[6];
	Maclike_Dialog *dialog;
	Mac_Button *OK;
	int i, x, y, X, Y;

	/* Set up all the components of the dialog box */
#ifdef CENTER_DIALOG
	X=(SCREEN_WIDTH-DAWN_DIALOG_WIDTH)/2;
	Y=(SCREEN_HEIGHT-DAWN_DIALOG_HEIGHT)/2;
#else	/* The way it is on the original Maelstrom */
	X=160;
	Y=73;
#endif
	if ( (chicago = fontserv->NewFont("Chicago", 12)) == NULL ) {
		error("Can't use Chicago font!\n");
		return;
	}
	if ( (splash = GetCIcon(screen, 103)) == NULL ) {
		error("Can't load alien dawn splash!\n");
		return;
	}
	dialog = new Maclike_Dialog(X, Y, DAWN_DIALOG_WIDTH, DAWN_DIALOG_HEIGHT,
									screen);
	x = y = 19;
	dialog->Add_Image(splash, x, y);
	x += (splash->w+26);
	text[0] = fontserv->TextImage(D_text[0], chicago, STYLE_NORM,
							0x00, 0x00, 0x00);
	dialog->Add_Image(text[0], x, y);
	for ( i=1; i<6; ++i ) {
		y += (text[i-1]->h+2);
		text[i] = fontserv->TextImage(D_text[i], chicago, STYLE_NORM,
							0x00, 0x00, 0x00);
		dialog->Add_Image(text[i], x, y);
		x += (text[i]->w+2);
	}
	OK = new Mac_DefaultButton(210, 160, 90, BUTTON_HEIGHT,
						"OK", chicago, fontserv, NULL);
	dialog->Add_Dialog(OK);

	/* Run the dialog box */
	dialog->Run(EXPAND_STEPS);

	/* Clean up and return */
	screen->FreeImage(splash);
	for ( i=0; i<6; ++i )
		fontserv->FreeText(text[i]);
	delete chicago;
	delete dialog;
	return;
}

