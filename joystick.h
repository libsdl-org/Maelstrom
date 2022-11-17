
/* Hum de dum, joysticks.... :) */

#ifdef USE_JOYSTICK
#ifndef linux
#error Joystick support is only available for Linux!
#endif

/* We assume we are using Joystick 1 on a Linux box, for now */

#include <stdio.h>
#include <sys/fcntl.h>
#include <math.h>				/* For abs() definition */

#include "linux/joystick.h"

#ifndef _PATH_DEV_JOYSTICK
#define _PATH_DEV_JOYSTICK	"/dev/js0"
#endif

extern void select_usleep(unsigned long);	/* From shared.cc */

struct joy_sensitivity {
	int min_x;
	int min_y;
	int x_middle;
	int y_middle;
	int max_x;
	int max_y;
};

/* Button encoding: 8 bits state in high byte, 8 bits for buttons in low byte */
#define BUTTON_1	0x01
#define BUTTON_2	0x02
#define PRESSED(state,button)	( (state)->buttons&(button<<8) )

typedef struct joystate {
	int buttons;			/* Joystick button state */
	int x_axis;			/* Percentage of x tilt */
	int y_axis;			/* Percentage of y tilt */
} joystate;

class Joystick {

public:
	Joystick(char *JoyDev, struct joy_sensitivity *Calib) {
		joyfd = -1;

		if ( ! JoyDev )
			JoyDev = _PATH_DEV_JOYSTICK;
		(void) OpenJoystick(JoyDev, 0);

		if ( ! Calib ) {	/* Do precomputed calibration */
			/* Just a guess */
			calib.min_x = 15;
			calib.min_y = 25;
			calib.max_x = 1920;
			calib.max_y = 1950;
			calib.x_middle = 750;
			calib.min_x -= calib.x_middle;	// Normalize to center
			calib.max_x -= calib.x_middle;	// Normalize to center
			calib.y_middle = 600;
			calib.min_y -= calib.y_middle;	// Normalize to center
			calib.max_y -= calib.y_middle;	// Normalize to center
		} else
			calib = *Calib;

		jstate.buttons = 0;
		jstate.x_axis = 0;
		jstate.y_axis = 0;
	}

	~Joystick() {
		if ( joyfd >= 0 )
			close(joyfd);
	}


	int Calibrate(char *JoyDev, struct joy_sensitivity *Calib) {
		joystate js;

		if ( JoyDev ) {
			if ( OpenJoystick(JoyDev, 1) < 0 )
				return(-1);
		}
		/* Calibrate the suckah. :) */
		printf(
		"Move joystick to the upper left and press any button!\n");
		WaitButton(&js);
		calib.min_x = js.x_axis;
		calib.min_y = js.y_axis;
		printf(
		"Move joystick to the lower right and press any button!\n");
		WaitButton(&js);
		calib.max_x = js.x_axis;
		calib.max_y = js.y_axis;
		printf(
		"Move joystick to the middle and press any button!\n");
		WaitButton(&js);
		calib.x_middle = js.x_axis;
		calib.min_x -= calib.x_middle;	// Normalize to center
		calib.max_x -= calib.x_middle;	// Normalize to center
		calib.y_middle = js.y_axis;
		calib.min_y -= calib.y_middle;	// Normalize to center
		calib.max_y -= calib.y_middle;	// Normalize to center
error("Calibrated at:\n");
error("		-Y: %d\n", calib.min_y);
error("	  -X: %d    +X: %d\n", calib.min_x, calib.max_x);
error("		+Y: %d\n", calib.max_y);

		if ( Calib )
			*Calib = calib;
		return(0);
	}

	int RawEvent(joystate *js, int cookaxis) {
		int i;
		JS_DATA_TYPE jdata;

		if ( joyfd < 0 )
			return(-1);

		if ( read(joyfd, &jdata, JS_RETURN) != JS_RETURN ) {
			perror("Warning: Can't read from joystick device");
			return(-1);
		}

		/* Handle the button state */
		if ( jdata.buttons&BUTTON_1 ) {
			if ( PRESSED(&jstate, BUTTON_1) ) {
				/* Clear alert for button 1 -- no change */
				jstate.buttons &= ~BUTTON_1;
			} else {
				/* Button 1 has been pressed */
				jstate.buttons |= ((BUTTON_1<<8)|BUTTON_1);
//error("Button 1 pressed, state = 0x%.4x\n", jstate.buttons);
			}
		} else { /* Button 1 is not down */
			if ( PRESSED(&jstate, BUTTON_1) ) {
				/* Button 1 has been released */
				jstate.buttons |= BUTTON_1;
				jstate.buttons &= ~(BUTTON_1<<8);
//error("Button 1 released, state = 0x%.4x\n", jstate.buttons);
			} else {
				/* Clear alert for button 1 -- no change */
				jstate.buttons &= ~BUTTON_1;
			}
		}
		if ( jdata.buttons&BUTTON_2 ) {
			if ( PRESSED(&jstate, BUTTON_2) ) {
				/* Clear alert for button 2 -- no change */
				jstate.buttons &= ~BUTTON_2;
			} else {
				/* Button 2 has been pressed */
				jstate.buttons |= ((BUTTON_2<<8)|BUTTON_2);
			}
		} else { /* Button 2 is not down */
			if ( PRESSED(&jstate, BUTTON_2) ) {
				/* Button 2 has been released */
				jstate.buttons |= BUTTON_2;
				jstate.buttons &= ~(BUTTON_2<<8);
			} else {
				/* Clear alert for button 2 -- no change */
				jstate.buttons &= ~BUTTON_2;
			}
		}

		/* See if we need to translate axis to percentages */
		if ( cookaxis )
			CookAxis(&jdata);
		else {
			jstate.x_axis = jdata.x;
			jstate.y_axis = jdata.y;
		}

		/* Copy the current state to our output (slow!) */
		*js = jstate;
		return(0);
	}

	int Event(joystate *js) {
		return(RawEvent(js, 1));
	}

protected:
	int      joyfd;
	joystate jstate;
	struct joy_sensitivity calib;

	int OpenJoystick(char *joydev, int complain) {
		if ( joyfd >= 0 ) {
			close(joyfd);
		}
		if ( ((joyfd=open(joydev, O_RDONLY, 0)) < 0) && complain ) {
			error("Can't open joystick device (%s): ", joydev);
			perror("");
		}
		return(joyfd);
	}

	void CookAxis(JS_DATA_TYPE *jdata) {
//error("J-RAW: x = %d, y = %d\n", jdata->x, jdata->y);
		jstate.x_axis = (jdata->x - calib.x_middle);
		jstate.y_axis = (jdata->y - calib.y_middle);
//error("J-RARE: x = %d, y = %d\n", jstate.x_axis, jstate.y_axis);
		if ( jstate.x_axis < 0 )
			jstate.x_axis = -(jstate.x_axis*100)/calib.min_x;
		else
			jstate.x_axis = (jstate.x_axis*100)/calib.max_x;
		if ( jstate.y_axis < 0 )
			jstate.y_axis = -(jstate.y_axis*100)/calib.min_y;
		else
			jstate.y_axis = (jstate.y_axis*100)/calib.max_y;
//error("J-COOKED: x = %d, y = %d\n", jstate.x_axis, jstate.y_axis);
	}

	void WaitButton(joystate *js) {
		joystate ignored;

		/* Wait for button press */
		do {
			RawEvent(js, 0);
		} while ( ! PRESSED(js, BUTTON_1) && ! PRESSED(js, BUTTON_2) );

		/* Wait for button release */
		do {
			RawEvent(&ignored, 0);
		} while ( PRESSED(&ignored, BUTTON_1) || 
						PRESSED(&ignored, BUTTON_2) );
	}
};

#endif /* USE_JOYSTICK */
