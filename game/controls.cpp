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

/* This file handles the controls configuration and updating the keystrokes
*/

#include "Maelstrom_Globals.h"
#include "player.h"
#include "../screenlib/UIDialog.h"
#include "../screenlib/UIElement.h"
#include "../screenlib/UIElementRadio.h"

/* Savable and configurable controls/data */

Controls::Controls() :
	gPauseControl("Controls.PauseControl", SDLK_PAUSE),
	gShieldControl("Controls.ShieldControl", SDLK_SPACE),
	gThrustControl("Controls.ThrustControl", SDLK_UP),
	gBrakeControl("Controls.BrakeControl", SDLK_DOWN),
	gTurnRControl("Controls.TurnRControl", SDLK_RIGHT),
	gTurnLControl("Controls.TurnLControl", SDLK_LEFT),
	gFireControl("Controls.FireControl", SDLK_TAB),
	gQuitControl("Controls.QuitControl", SDLK_ESCAPE)
{
}

void
Controls::Bind(Prefs *prefs)
{
	gPauseControl.Bind(prefs);
	gShieldControl.Bind(prefs);
	gThrustControl.Bind(prefs);
	gBrakeControl.Bind(prefs);
	gTurnRControl.Bind(prefs);
	gTurnLControl.Bind(prefs);
	gFireControl.Bind(prefs);
	gQuitControl.Bind(prefs);
}

Controls controls;
PrefsVariable<int> gSoundLevel("SoundLevel", 4);
PrefsVariable<int> gGammaCorrect("GammaCorrect", 3);


void LoadControls(void)
{
	gSoundLevel.Bind(prefs);
	gGammaCorrect.Bind(prefs);
	controls.Bind(prefs);
}

void SaveControls(void)
{
	prefs->Save();
}

bool
ControlsDialogDelegate::OnLoad()
{
	char name[32];

	for (unsigned int i = 0; i < SDL_arraysize(m_controlKeys); ++i) {
		SDL_snprintf(name, sizeof(name), "control%d_box", 1+i);
		m_controlBoxes[i] = m_panel->GetElement<UIElement>(name);
		if (!m_controlBoxes[i]) {
			error("Warning: Couldn't find control '%s'\n", name);
			return false;
		}
	}

	for (unsigned int i = 0; i < SDL_arraysize(m_controlKeys); ++i) {
		SDL_snprintf(name, sizeof(name), "control%d", 1+i);
		m_controlKeys[i] = m_panel->GetElement<UIElement>(name);
		if (!m_controlKeys[i]) {
			error("Warning: Couldn't find control '%s'\n", name);
			return false;
		}
	}

	m_radioGroup = m_panel->GetElement<UIElementRadioGroup>("controlsRadioGroup");
	if (!m_radioGroup) {
		error("Warning: Couldn't find 'controlsRadioGroup'\n");
		return false;
	}

	for (unsigned int i = 0; i < SDL_arraysize(m_controlLabels); ++i) {
		SDL_snprintf(name, sizeof(name), "label%d", 1+i);
		m_controlLabels[i] = m_panel->GetElement<UIElement>(name);
		if (!m_controlLabels[i]) {
			error("Warning: Couldn't find control '%s'\n", name);
			return false;
		}
	}

	unsigned int num_controls = SDL_arraysize(m_controlKeys);
	if (!gControlBrakes) {
		--num_controls;
		m_controlBoxes[num_controls]->Hide();
		m_controlKeys[num_controls]->Hide();
		m_controlLabels[num_controls]->Hide();
	}

	return true;
}

void
ControlsDialogDelegate::OnShow()
{
	UIElementRadioButton *button;

	button = m_radioGroup->GetRadioButton(1);
	if (button) {
		button->SetChecked(true);
	}

	for (int i = 0; (unsigned)i < SDL_arraysize(m_keyinuseTimers); ++i) {
		m_keyinuseTimers[i] = 0;
	}

	m_controls = controls;

	ShowKeyLabels();
	ShowControlLabels();
}

void
ControlsDialogDelegate::OnHide()
{
	if (m_dialog->GetDialogStatus() > 0) {
		controls = m_controls;
		SaveControls();
	}
}

void
ControlsDialogDelegate::OnTick()
{
	for (int i = 0; (unsigned)i < SDL_arraysize(m_keyinuseTimers); ++i) {
		if (m_keyinuseTimers[i] && (SDL_GetTicks() - m_keyinuseTimers[i]) > 1000) {
			m_keyinuseTimers[i] = 0;
			ShowKeyLabel(i);
		}
	}
}

bool
ControlsDialogDelegate::HandleEvent(const SDL_Event &event)
{
	if (event.type == SDL_EVENT_KEY_DOWN) {
		return true;
	}
	if (event.type == SDL_EVENT_KEY_UP) {
		int index;
		SDL_Keycode key = event.key.key;

		index = m_radioGroup->GetValue() - 1;

		/* See if this key is in use */
		m_keyinuseTimers[index] = 0;
		for (int i = 0; i < NUM_CTLS; ++i) {
			if (i != index && key == GetKeycode(i)) {
				m_keyinuseTimers[index] = SDL_GetTicks();
				break;
			}
		}
		if (!m_keyinuseTimers[index]) {
			SetKeycode(index, key);
		}
		ShowKeyLabel(index);

		return true;
	}
	return false;
}

int
ControlsDialogDelegate::TranslateIndex(int index)
{
	static int s_arrBrakeEnabled[] = {
		FIRE_CTL,
		THRUST_CTL,
		BRAKE_CTL,
		SHIELD_CTL,
		TURNR_CTL,
		TURNL_CTL,
		PAUSE_CTL,
		QUIT_CTL
	};
	static int s_arrBrakeDisabled[] = {
		FIRE_CTL,
		THRUST_CTL,
		SHIELD_CTL,
		TURNR_CTL,
		TURNL_CTL,
		PAUSE_CTL,
		QUIT_CTL
	};

	if (gControlBrakes) {
		if (index < SDL_arraysize(s_arrBrakeEnabled)) {
			return s_arrBrakeEnabled[index];
		}
	} else {
		if (index < SDL_arraysize(s_arrBrakeDisabled)) {
			return s_arrBrakeDisabled[index];
		}
	}
	return NUM_CTLS;
}

void
ControlsDialogDelegate::ShowControlLabel(int index)
{
	const char *text;

	switch (TranslateIndex(index)) {
		case FIRE_CTL:
			text = "Fire";
			break;
		case THRUST_CTL:
			text = "Thrust";
			break;
		case BRAKE_CTL:
			text = "Brake";
			break;
		case SHIELD_CTL:
			text = "Shield";
			break;
		case TURNR_CTL:
			text = "Turn Clockwise";
			break;
		case TURNL_CTL:
			text = "Turn Counter-Clockwise";
			break;
		case PAUSE_CTL:
			text = "Pause";
			break;
		case QUIT_CTL:
			text = "Abort Game";
			break;
		default:
			text = "";
			break;
	}
	m_controlLabels[index]->SetText(text);
}

void
ControlsDialogDelegate::ShowKeyLabel(int index)
{
	const char *text;

	if (m_keyinuseTimers[index]) {
		text = "That key is in use!";
	} else {
		text = SDL_GetKeyName(GetKeycode(index));
	}
	m_controlKeys[index]->SetText(text);
}

SDL_Keycode
ControlsDialogDelegate::GetKeycode(int index)
{
	switch (TranslateIndex(index)) {
		case FIRE_CTL:
			return m_controls.gFireControl;
		case THRUST_CTL:
			return m_controls.gThrustControl;
		case BRAKE_CTL:
			return m_controls.gBrakeControl;
		case SHIELD_CTL:
			return m_controls.gShieldControl;
		case TURNR_CTL:
			return m_controls.gTurnRControl;
		case TURNL_CTL:
			return m_controls.gTurnLControl;
		case PAUSE_CTL:
			return m_controls.gPauseControl;
		case QUIT_CTL:
			return m_controls.gQuitControl;
		default:
			return SDLK_UNKNOWN;
	}
}

void
ControlsDialogDelegate::SetKeycode(int index, SDL_Keycode keycode)
{
	switch (TranslateIndex(index)) {
		case FIRE_CTL:
			m_controls.gFireControl = keycode;
			break;
		case THRUST_CTL:
			m_controls.gThrustControl = keycode;
			break;
		case BRAKE_CTL:
			m_controls.gBrakeControl = keycode;
			break;
		case SHIELD_CTL:
			m_controls.gShieldControl = keycode;
			break;
		case TURNR_CTL:
			m_controls.gTurnRControl = keycode;
			break;
		case TURNL_CTL:
			m_controls.gTurnLControl = keycode;
			break;
		case PAUSE_CTL:
			m_controls.gPauseControl = keycode;
			break;
		case QUIT_CTL:
			m_controls.gQuitControl = keycode;
			break;
		default:
			break;
	}
}

static Player *GetKeyboardPlayer()
{
	return GetControlPlayer(CONTROL_KEYBOARD);
}

#define MAX_JOYSTICKS	MAX_PLAYERS

static Uint8 joystickMasks[MAX_JOYSTICKS] = {
	CONTROL_JOYSTICK1,
	CONTROL_JOYSTICK2,
	CONTROL_JOYSTICK3
};
struct Gamepad {
	SDL_JoystickID id;
	SDL_Gamepad *gamepad;
	RemotePlaySessionID_t sessionID;
};
static array<Gamepad> gamepads;

static void OpenGamepad(SDL_JoystickID id)
{
	Gamepad gamepad;

	gamepad.id = id;
	gamepad.gamepad = SDL_OpenGamepad(id);
	if (!gamepad.gamepad) {
		return;
	}
	gamepad.sessionID = GetRemoteSessionForGamepad(gamepad.gamepad);

	gamepads.add(gamepad);
}

static void UpdateGamepadHandle(SDL_JoystickID id)
{
	for (unsigned int i = 0; i < gamepads.length(); ++i) {
		Gamepad* gamepad = &gamepads[i];
		if (gamepad->id == id) {
			gamepad->sessionID = GetRemoteSessionForGamepad(gamepad->gamepad);
			break;
		}
	}
}

unsigned int GetNumGamepads()
{
	return gamepads.length();
}

static void CloseGamepad(SDL_JoystickID id)
{
	for (unsigned int i = 0; i < gamepads.length(); ++i) {
		Gamepad *gamepad = &gamepads[i];
		if (gamepad->id == id) {
			SDL_CloseGamepad(gamepad->gamepad);
			gamepads.removeAt(i);
			break;
		}
	}
}

static Player *GetJoystickPlayer(SDL_JoystickID id)
{
	int joystick_index = 0;
	for (unsigned int i = 0; i < gamepads.length(); ++i) {
		Gamepad *gamepad = &gamepads[i];
		if (gamepad->id == id) {
			if (gamepad->sessionID) {
				return GetControlPlayer(GetRemoteSessionControl(gamepad->sessionID));
			} else {
				return GetControlPlayer(joystickMasks[joystick_index]);
			}
		}

		if (!gamepad->sessionID) {
			++joystick_index;
		}
	}
	return NULL;
}

static void UpdateControl(Player *player)
{
	const Uint8 controlType = player->GetControlType();
	bool keys[FIRE_KEY+1];

	SDL_zeroa(keys);

	int joystick_index = 0;
	for (unsigned int i = 0; i < gamepads.length(); ++i) {
		Gamepad *gamepad = &gamepads[i];
		if ((gamepad->sessionID && (controlType & GetRemoteSessionControl(gamepad->sessionID))) ||
			(!gamepad->sessionID && (controlType & joystickMasks[joystick_index]))) {
			if (SDL_GetGamepadButton(gamepad->gamepad, SDL_GAMEPAD_BUTTON_SOUTH) ||
				SDL_GetGamepadAxis(gamepad->gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) >= 8000) {
				keys[FIRE_KEY] = true;
			}

			if (SDL_GetGamepadButton(gamepad->gamepad, SDL_GAMEPAD_BUTTON_EAST) ||
				SDL_GetGamepadAxis(gamepad->gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) >= 8000) {
				keys[SHIELD_KEY] = true;
			}

			if (SDL_GetGamepadButton(gamepad->gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT) ||
				SDL_GetGamepadAxis(gamepad->gamepad, SDL_GAMEPAD_AXIS_LEFTX) <= -16000 ||
				SDL_GetGamepadAxis(gamepad->gamepad, SDL_GAMEPAD_AXIS_RIGHTX) <= -16000) {
				keys[LEFT_KEY] = true;
			}

			if (SDL_GetGamepadButton(gamepad->gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT) ||
				SDL_GetGamepadAxis(gamepad->gamepad, SDL_GAMEPAD_AXIS_LEFTX) >= 16000 ||
				SDL_GetGamepadAxis(gamepad->gamepad, SDL_GAMEPAD_AXIS_RIGHTX) >= 16000) {
				keys[RIGHT_KEY] = true;
			}

			if (SDL_GetGamepadButton(gamepad->gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP) ||
				SDL_GetGamepadAxis(gamepad->gamepad, SDL_GAMEPAD_AXIS_LEFTY) <= -16000 ||
				SDL_GetGamepadAxis(gamepad->gamepad, SDL_GAMEPAD_AXIS_RIGHTY) <= -16000) {
				keys[THRUST_KEY] = true;
			}

			if (SDL_GetGamepadButton(gamepad->gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP) ||
				SDL_GetGamepadAxis(gamepad->gamepad, SDL_GAMEPAD_AXIS_LEFTY) <= -16000 ||
				SDL_GetGamepadAxis(gamepad->gamepad, SDL_GAMEPAD_AXIS_RIGHTY) <= -16000) {
				keys[BRAKE_KEY] = true;
			}
		}

		if (!gamepad->sessionID) {
			++joystick_index;
		}
	}

	const bool *keystate;
	if (controlType & CONTROL_KEYBOARD) {
		keystate = SDL_GetKeyboardState(nullptr);
	} else {
		keystate = GetRemotePlayerKeyboardState(controlType);
	}
	if (keystate) {
		if (keystate[SDL_GetScancodeFromKey(controls.gFireControl, SDL_KMOD_NONE)]) {
			keys[FIRE_KEY] = true;
		}
		if (keystate[SDL_GetScancodeFromKey(controls.gTurnRControl, SDL_KMOD_NONE)]) {
			keys[RIGHT_KEY] = true;
		}
		if (keystate[SDL_GetScancodeFromKey(controls.gTurnLControl, SDL_KMOD_NONE)]) {
			keys[LEFT_KEY] = true;
		}
		if (keystate[SDL_GetScancodeFromKey(controls.gShieldControl, SDL_KMOD_NONE)]) {
			keys[SHIELD_KEY] = true;
		}
		if (keystate[SDL_GetScancodeFromKey(controls.gThrustControl, SDL_KMOD_NONE)]) {
			keys[THRUST_KEY] = true;
		}
		if (keystate[SDL_GetScancodeFromKey(controls.gBrakeControl, SDL_KMOD_NONE)]) {
			keys[BRAKE_KEY] = true;
		}
	}

	player->SetControl(FIRE_KEY, keys[FIRE_KEY]);
	player->SetControl(SHIELD_KEY, keys[SHIELD_KEY]);
	player->SetControl(LEFT_KEY, keys[LEFT_KEY]);
	player->SetControl(RIGHT_KEY, keys[RIGHT_KEY]);
	player->SetControl(THRUST_KEY, keys[THRUST_KEY]);
	player->SetControl(BRAKE_KEY, keys[BRAKE_KEY]);
}

void HandleEvent(SDL_Event *event)
{
	Player *player;
	SDL_Keycode key;

	if (ui->HandleEvent(*event)) {
		return;
	}

	if (!gGameOn) {
		return;
	}

	switch (event->type) {
		/* -- Handle joystick axis motion */
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			player = GetJoystickPlayer(event->gaxis.which);
			if (!player) {
				break;
			}
			UpdateControl(player);
			break;

		/* -- Handle joystick button presses/releases */
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
			player = GetJoystickPlayer(event->gbutton.which);
			if (!player) {
				break;
			}
			switch (event->gbutton.button) {
			case SDL_GAMEPAD_BUTTON_START:
				if (!event->gbutton.down) {
					gGameInfo.TogglePauseRequest();
				}
				break;
			case SDL_GAMEPAD_BUTTON_BACK:
				if (!event->gbutton.down) {
					gGameInfo.SetLocalState(STATE_ABORT, true);
				}
				break;
			default:
				UpdateControl(player);
				break;
			}
			break;

		/* -- Handle Steam handle changing for a gamepad */
		case SDL_EVENT_GAMEPAD_STEAM_HANDLE_UPDATED:
			player = GetJoystickPlayer(event->gdevice.which);
			UpdateGamepadHandle(event->gdevice.which);
			if (player) {
				// Update the previous player's controls
				UpdateControl(player);
			}
			player = GetJoystickPlayer(event->gdevice.which);
			if (player) {
				// Update the new player's controls
				UpdateControl(player);
			}
			break;

		/* -- Handle key presses/releases */
		case SDL_EVENT_KEY_DOWN:
			key = event->key.key;

			player = GetKeyboardPlayer();
			if (!player) {
				break;
			}

			/* Update control key status */
			UpdateControl(player);
			break;

		case SDL_EVENT_KEY_UP:
			key = event->key.key;

			/* -- Handle special control keys */
			if ( key == SDLK_F1 ) {
				/* Special key --
					Switch displayed player
				 */
				RotatePlayerView();
				break;
			} else if ( key == SDLK_F3 ) {
				/* Special key --
					Do a screen dump here.
				 */
				screen->ScreenDump("ScreenShot",
							0, 0, 0, 0);
				break;
			} else if ( key == SDLK_RETURN &&
				    (event->key.mod & SDL_KMOD_ALT) ) {
				/* Special key --
					Toggle fullscreen mode
				 */
				screen->ToggleFullScreen();
				break;
			} else if ( key == controls.gPauseControl ) {
				gGameInfo.TogglePauseRequest();
				break;
			} else if ( key == controls.gQuitControl ) {
				gGameInfo.SetLocalState(STATE_ABORT, true);
				break;
			}

			player = GetKeyboardPlayer();
			if (!player) {
				break;
			}

			/* Update control key status */
			UpdateControl(player);
			break;

		case SDL_EVENT_REMOTE_INPUT:
			player = GetControlPlayer(event->user.code);
			if (!player) {
				break;
			}

			/* Update control key status */
			UpdateControl(player);
			break;

		case SDL_EVENT_OVERLAY_ACTIVATED:
			gGameInfo.SetPauseReason(PAUSE_OVERLAY, true);
			break;

		case SDL_EVENT_OVERLAY_DEACTIVATED:
			gGameInfo.SetPauseReason(PAUSE_OVERLAY, false);
			break;

		case SDL_EVENT_WINDOW_MINIMIZED:
			gGameInfo.SetPauseReason(PAUSE_MINIMIZED, true);
			break;

		case SDL_EVENT_WINDOW_RESTORED:
			gGameInfo.SetPauseReason(PAUSE_MINIMIZED, false);
			break;
	}
}

static bool SDLCALL GamepadEventWatch(void *userdata, SDL_Event *event)
{
	switch (event->type) {
		/* -- Handle gamepad added */
		case SDL_EVENT_GAMEPAD_ADDED:
			OpenGamepad(event->gdevice.which);
			gGameInfo.SetPauseReason(PAUSE_CONTROLLER, false);
			break;

		/* -- Handle gamepad removed */
		case SDL_EVENT_GAMEPAD_REMOVED:
			CloseGamepad(event->gdevice.which);
			gGameInfo.SetPauseReason(PAUSE_CONTROLLER, true);
			break;

		default:
			break;
	}
	return true;
}

void InitPlayerControls(void)
{
	SDL_JoystickID *ids = SDL_GetJoysticks(nullptr);
	if (ids) {
		for (int i = 0; ids[i]; ++i) {
			OpenGamepad(ids[i]);
		}
		SDL_free(ids);
	}
	SDL_AddEventWatch(GamepadEventWatch, nullptr);
}

void QuitPlayerControls(void)
{
	SDL_RemoveEventWatch(GamepadEventWatch, nullptr);
	for (unsigned int i = 0; i < gamepads.length(); ++i) {
		Gamepad *gamepad = &gamepads[i];
		SDL_CloseGamepad(gamepad->gamepad);
	}
	gamepads.clear();
}

