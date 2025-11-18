/*
    Maelstrom: Open Source version of the classic game by Ambrosia Software
    Copyright (C) 1997-2011  Sam Lantinga

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifndef _controls_h
#define _controls_h

#include "../screenlib/UIDialog.h"

#if defined(__IPHONEOS__) || defined(__ANDROID__)
#define USE_TOUCHCONTROL
#endif

// Functions from controls.cc
#ifdef USE_JOYSTICK
extern void	CalibrateJoystick(char *joystick);
#endif
extern void	LoadControls(void);
extern void	SaveControls(void);
extern void	InitPlayerControls(void);
extern void	QuitPlayerControls(void);
extern void	HandleEvents(int timeout);
extern int	DropEvents(void);

/* Generic key control definitions */
#define THRUST_KEY	0x01
#define RIGHT_KEY	0x02
#define LEFT_KEY	0x03
#define SHIELD_KEY	0x04
#define FIRE_KEY	0x05
#define PAUSE_KEY	0x06
#define ABORT_KEY	0x07

/* The controls structure */
class Controls
{
public:
	Controls();

	void Bind(Prefs *prefs);

public:
	PrefsVariable<SDL_Keycode> gPauseControl;
	PrefsVariable<SDL_Keycode> gShieldControl;
	PrefsVariable<SDL_Keycode> gThrustControl;
	PrefsVariable<SDL_Keycode> gTurnRControl;
	PrefsVariable<SDL_Keycode> gTurnLControl;
	PrefsVariable<SDL_Keycode> gFireControl;
	PrefsVariable<SDL_Keycode> gQuitControl;
};


class UIElement;
class UIElementRadioGroup;

class ControlsDialogDelegate : public UIDialogDelegate
{
public:
	ControlsDialogDelegate(UIPanel *panel) : UIDialogDelegate(panel) { }

	virtual bool OnLoad();
	virtual void OnShow();
	virtual void OnHide();
	virtual void OnTick();
	virtual bool HandleEvent(const SDL_Event &event);

protected:
	void ShowKeyLabel(int index);
	void ShowKeyLabels() {
		for (int i = 0; i < NUM_CTLS; ++i) {
			ShowKeyLabel(i);
		}
	}

	SDL_Keycode GetKeycode(int index);
	void SetKeycode(int index, SDL_Keycode keycode);

protected:
	enum {
		FIRE_CTL,
		THRUST_CTL,
		SHIELD_CTL,
		TURNR_CTL,
		TURNL_CTL,
		PAUSE_CTL,
		QUIT_CTL,
		NUM_CTLS
	};

	Controls m_controls;
	UIElement *m_controlKeys[NUM_CTLS];
	UIElementRadioGroup *m_radioGroup;
	Uint32 m_keyinuseTimers[NUM_CTLS];
};

#endif /* _controls_h */
