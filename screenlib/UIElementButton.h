/*
  screenlib:  A simple window and UI library based on the SDL library
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

#ifndef _UIElementButton_h
#define _UIElementButton_h

#include "UIElement.h"


class UIElementButton : public UIElement
{
DECLARE_TYPESAFE_CLASS(UIElement)
public:
	enum BUTTON_STATE {
		BUTTON_STATE_NORMAL,
		BUTTON_STATE_PRESSED,
		BUTTON_STATE_DISABLED,
		NUM_BUTTON_STATES
	};

public:
	UIElementButton(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine);
	virtual ~UIElementButton();

	virtual bool Load(rapidxml::xml_node<> *node, const UITemplates *templates) override;

	virtual bool HandleEvent(const SDL_Event &event) override;
	virtual void OnMouseDown() override;
	virtual void OnMouseUp() override;

	UITexture *GetButtonStateImage(BUTTON_STATE state) {
		return m_stateImages[state];
	}

protected:
	struct Hotkey {
		SDL_Keycode key = SDLK_UNKNOWN;
		int mod = 0;
	};

	void SetButtonState(BUTTON_STATE state);

	virtual void UpdateDisabledState() override;
	virtual void OnClick() override;

	bool ParseHotkey(rapidxml::xml_attribute<> *attr, Hotkey *hotkey);
	bool ShouldHandleKey(const SDL_KeyboardEvent* key, Hotkey* hotkey);

protected:
	Hotkey m_hotkey;
	Hotkey m_hotkey2;
	char *m_pressSound;
	char *m_releaseSound;
	char *m_clickSound;
	char *m_clickPanel;
	BUTTON_STATE m_buttonState;
	UITexture *m_stateImages[NUM_BUTTON_STATES];
};

#endif // _UIElementButton_h
