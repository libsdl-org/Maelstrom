/*
  screenlib:  A simple window and UI library based on the SDL library
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

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

#ifndef _UIElementEditbox_h
#define _UIElementEditbox_h

// This is a simple editbox class
// It currently doesn't support multiline or editing within the line,
// though the latter could be supported fairly easily.

#include "UIElementButton.h"


class UIElementEditbox : public UIElementButton
{
DECLARE_TYPESAFE_CLASS(UIElement)
public:
	UIElementEditbox(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine);
	virtual ~UIElementEditbox();

	override bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);

	override bool HandleEvent(const SDL_Event &event);

	override void OnClick() {
		SetFocus(!HasFocus());
	}

	bool IsHighlighted() const {
		return m_highlight;
	}

	void SetFocus(bool focus);
	void SetFocusNext();
	bool HasFocus() const {
		return m_focus;
	}

	void SetTextMax(int maxLen);

	override void SetText(const char *text);
	const char *GetText() const {
		return m_text;
	}

	void SetNumber(int value) {
		char buffer[32];
		SDL_snprintf(buffer, sizeof(buffer), "%d", value);
		SetText(buffer);
	}
	int GetNumber() const {
		return SDL_atoi(m_text);
	}

protected:
	// These can be overridden by inheriting classes
	virtual void OnHighlightChanged() { }
	virtual void OnTextChanged();

	void SetHighlight(bool highlight);

protected:
	bool m_focus;
	bool m_highlight;
	bool m_numeric;
	int m_textMax;
	int m_textLen;
	char *m_text;
};

#endif // _UIElementEditbox_h
