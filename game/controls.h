/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _controls_h
#define _controls_h

#include "../screenlib/UIDialog.h"

// Functions from controls.cc
#ifdef USE_JOYSTICK
extern void	CalibrateJoystick(char *joystick);
#endif
extern void	LoadControls(void);
extern void	SaveControls(void);
extern void	InitPlayerControls(void);
extern void	QuitPlayerControls(void);
extern unsigned int GetNumGamepads();
extern void	HandleEvent(SDL_Event *event);

/* Generic key control definitions */
#define THRUST_KEY	0x01
#define BRAKE_KEY	0x02
#define RIGHT_KEY	0x03
#define LEFT_KEY	0x04
#define SHIELD_KEY	0x05
#define FIRE_KEY	0x06
#define PAUSE_KEY	0x07
#define ABORT_KEY	0x08

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
	PrefsVariable<SDL_Keycode> gBrakeControl;
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
		BRAKE_CTL,
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
	Uint64 m_keyinuseTimers[NUM_CTLS];
};

#endif /* _controls_h */
