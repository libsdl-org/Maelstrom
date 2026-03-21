/*
  screenlib:  A simple window and UI library based on the SDL library
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

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

#include "UIManager.h"
#include "UIPanel.h"
#include "UIElementButton.h"

UIElementType UIElementButton::s_elementType;


UIElementButton::UIElementButton(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIElement(parent, name, drawEngine)
{
	// Turn on mouse events at the UIElement level
	m_mouseEnabled = true;

	m_pressSound = NULL;
	m_releaseSound = NULL;
	m_clickSound = NULL;
	m_clickPanel = NULL;
	m_buttonState = NUM_BUTTON_STATES;
	SDL_zero(m_stateImages);
}

UIElementButton::~UIElementButton()
{
	if (m_pressSound) {
		SDL_free(m_pressSound);
	}
	if (m_releaseSound) {
		SDL_free(m_releaseSound);
	}
	if (m_clickSound) {
		SDL_free(m_clickSound);
	}
	if (m_clickPanel) {
		SDL_free(m_clickPanel);
	}

	SetImage((UITexture*)0);
	for (unsigned int i = 0; i < SDL_arraysize(m_stateImages); ++i) {
		UITexture *image = m_stateImages[i];
		if (image) {
			image->SetLocked(false);
			m_ui->FreeImage(image);
		}
	}
}

bool
UIElementButton::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	if (!UIElement::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("hotkey", 0, false);
	if (attr) {
		if (!ParseHotkey(attr, &m_hotkey)) {
			return false;
		}
	}

	attr = node->first_attribute("hotkey2", 0, false);
	if (attr) {
		if (!ParseHotkey(attr, &m_hotkey2)) {
			return false;
		}
	}

	// Load the button state images, if any
	static const char *stateImageNames[] = {
		"normal_image",
		"pressed_image",
		"disabled_image",
	};
	SDL_COMPILE_TIME_ASSERT(stateImageNames, SDL_arraysize(stateImageNames) == NUM_BUTTON_STATES);

	for (int i = 0; i < NUM_BUTTON_STATES; ++i) {
		attr = node->first_attribute(stateImageNames[i], 0, false);
		if (!attr) {
			continue;
		}

		UITexture *image = m_ui->CreateImage(attr->value());
		if (image) {
			image->SetLocked(true);
			m_stateImages[i] = image;
		} else {
			SDL_Log("Warning: Couldn't load image '%s'", attr->value());
			return false;
		}
	}
	SetButtonState(BUTTON_STATE_NORMAL);

	LoadString(node, "pressSound", m_pressSound);
	LoadString(node, "releaseSound", m_releaseSound);
	LoadString(node, "clickSound", m_clickSound);
	LoadString(node, "clickPanel", m_clickPanel);

	return true;
}

bool
UIElementButton::ParseHotkey(rapidxml::xml_attribute<> *attr, Hotkey *hotkey)
{
	const char *value = attr->value();
	const char *hyphen = SDL_strchr(value, '-');

	if (hyphen) {
		size_t len = size_t(hyphen-value);
		if (SDL_strncasecmp(value, "ALT", len) == 0) {
			hotkey->mod = SDL_KMOD_ALT;
		} else if (SDL_strncasecmp(value, "CTRL", len) == 0 ||
				   SDL_strncasecmp(value, "Control", len) == 0) {
			hotkey->mod = SDL_KMOD_CTRL;
		} else if (SDL_strncasecmp(value, "SHIFT", len) == 0) {
			hotkey->mod = SDL_KMOD_SHIFT;
		} else {
			SetError("Couldn't interpret hotkey value '%s'", value);
			return false;
		}
		value = hyphen+1;
	} else {
		hotkey->mod = 0;
	}

	if (strcmp(value, "any") == 0) {
		/* This will be a catch-all button */
		hotkey->key = ~0;
	} else {
		hotkey->key = SDL_GetKeyFromName(value);
		if (hotkey->key == SDLK_UNKNOWN) {
			SetError("Couldn't interpret hotkey value '%s'", value);
			return false;
		}
	}
	return true;
}

bool
UIElementButton::ShouldHandleKey(const SDL_KeyboardEvent *key, Hotkey *hotkey)
{
	if (key->key == hotkey->key &&
	    (!hotkey->mod || (key->mod & hotkey->mod))) {
		return true;
	}
	if (hotkey->key == ~0u) {
		switch (key->key) {
			// Ignore modifier keys
			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
			case SDLK_LCTRL:
			case SDLK_RCTRL:
			case SDLK_LALT:
			case SDLK_RALT:
			case SDLK_LGUI:
			case SDLK_RGUI:
				return false;
			default:
				return true;
		}
	}
	return false;
}

bool
UIElementButton::HandleEvent(const SDL_Event &event)
{
	if (event.type == SDL_EVENT_KEY_DOWN &&
	    (ShouldHandleKey(&event.key, &m_hotkey) ||
	     ShouldHandleKey(&event.key, &m_hotkey2))) {
		if (!m_mousePressed) {
			m_mousePressed = true;
			OnMouseDown();
		}
		return true;
	}

	if (event.type == SDL_EVENT_KEY_UP &&
	    (ShouldHandleKey(&event.key, &m_hotkey) ||
	     ShouldHandleKey(&event.key, &m_hotkey2))) {
		if (m_mousePressed) {
			m_mousePressed = false;
			OnMouseUp();
		}
		OnClick();
		return true;
	}

	return UIElement::HandleEvent(event);
}

void
UIElementButton::OnMouseDown()
{
	UIElement::OnMouseDown();

	SetButtonState(BUTTON_STATE_PRESSED);
}

void
UIElementButton::OnMouseUp()
{
	UIElement::OnMouseUp();

	SetButtonState(BUTTON_STATE_NORMAL);
}

void
UIElementButton::UpdateDisabledState()
{
	UIElement::UpdateDisabledState();

	if (IsDisabled()) {
		SetButtonState(BUTTON_STATE_DISABLED);
	} else {
		SetButtonState(BUTTON_STATE_NORMAL);
	}
}

void
UIElementButton::SetButtonState(BUTTON_STATE state)
{
	if (state == m_buttonState) {
		return;
	}

	if (state == BUTTON_STATE_PRESSED) {
		m_textOffset.x = 1;
		m_textOffset.y = 1;
	} else {
		m_textOffset.x = 0;
		m_textOffset.y = 0;
	}

	if (m_stateImages[state]) {
		SetImage(m_stateImages[state]);
	}

	if (m_buttonState == BUTTON_STATE_NORMAL && state == BUTTON_STATE_PRESSED) {
		if (m_pressSound) {
			GetUI()->PlaySound(m_pressSound);
		}
	}
	if (m_buttonState == BUTTON_STATE_PRESSED && state == BUTTON_STATE_NORMAL) {
		if (m_releaseSound) {
			GetUI()->PlaySound(m_releaseSound);
		}
	}

	m_buttonState = state;
}

void
UIElementButton::OnClick()
{
	if (m_clickSound) {
		GetUI()->PlaySound(m_clickSound);
	}
	if (m_clickPanel) {
		GetUI()->ShowPanel(m_clickPanel);
	}
	UIElement::OnClick();
}
