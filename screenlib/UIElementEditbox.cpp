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

#include "SDL_FrameBuf.h"
#include "UIElementEditbox.h"

UIElementType UIElementEditbox::s_elementType;


UIElementEditbox::UIElementEditbox(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIElementButton(parent, name, drawEngine)
{
	m_focus = false;
	m_highlight = false;
	m_numeric = false;

	// This is a reasonable default for a non-wrapping editbox
	m_textMax = 128;
	m_textLen = 0;
	m_text = new char[m_textMax];
	m_text[0] = '\0';
}

UIElementEditbox::~UIElementEditbox()
{
	delete[] m_text;

	if (m_focus) {
		m_screen->DisableTextInput();
	}
}

bool
UIElementEditbox::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	if (!UIElementButton::Load(node, templates)) {
		return false;
	}

	LoadBool(node, "numeric", m_numeric);

	int maxLen;
	if (LoadNumber(node, "maxLen", maxLen)) {
		SetTextMax(maxLen);
	}

	return true;
}

bool
UIElementEditbox::HandleEvent(const SDL_Event &event)
{
	if (!m_focus) {
		return UIElementButton::HandleEvent(event);
	}

	if (event.type == SDL_KEYUP) {
		switch (event.key.keysym.sym) {
			// This is confusing in dialogs which are canceled by Escape.
			//case SDLK_ESCAPE:
			//	SetFocus(false);
			//	return true;
			case SDLK_TAB:
				SetFocusNext();
				return true;
			case SDLK_DELETE:
			case SDLK_BACKSPACE:
				if (m_textLen > 0) {
					if (m_highlight) {
						m_textLen = 0;
						m_text[0] = '\0';
					} else {
						--m_textLen;
						m_text[m_textLen] = '\0';
					}
				}
				SetHighlight(false);
				OnTextChanged();
				return true;
			default:
				break;
		}
	}

	if (event.type == SDL_TEXTINPUT) {
		// Note, this doesn't support non-ASCII characters at the moment
		// To do that we would have to separate m_textMax and the size
		// of the text buffer and it gets complicated for in-line editing.
		char ch = event.text.text[0];
		bool valid;
		if (m_numeric) {
			valid = (ch >= '0' && ch <= '9');
		} else {
			valid = (ch >= ' ' && ch <= '~');
		}
		if (valid && (m_highlight || (m_textLen < m_textMax))) {
			if (m_highlight) {
				m_textLen = 0;
			}
			m_text[m_textLen++] = ch;
			m_text[m_textLen] = '\0';
			SetHighlight(false);
			OnTextChanged();
		}
		return true;
	}

	return UIElementButton::HandleEvent(event);
}

void
UIElementEditbox::SetFocus(bool focus)
{
	if (focus == m_focus) {
		return;
	}

	m_focus = focus;

	if (m_focus) {
		array<UIElementEditbox*> editboxes;

		SetHighlight(true);

		// Take focus away from other editboxes
		m_parent->FindElements<UIElementEditbox>(editboxes);
		for (unsigned int i = 0; i < editboxes.length(); ++i) {
			if (editboxes[i] != this) {
				editboxes[i]->SetFocus(false);
			}
		}
	} else {
		SetHighlight(false);
	}

	if (m_focus) {
		m_screen->EnableTextInput();
	} else {
		m_screen->DisableTextInput();
	}
}

void
UIElementEditbox::SetHighlight(bool highlight)
{
	if (highlight != m_highlight) {
		m_highlight = highlight;
		OnHighlightChanged();
	}
}

void
UIElementEditbox::SetFocusNext()
{
	UIElementEditbox *editbox;

	// We always lose focus even if we don't find another editbox
	SetFocus(false);

	editbox = m_parent->FindElement<UIElementEditbox>(this);
	if (editbox) {
		editbox->SetFocus(true);
	}
}

void
UIElementEditbox::SetTextMax(int maxLen)
{
	char *oldText = m_text;

	m_textMax = maxLen;
	m_text = new char[m_textMax+1];
	if (m_textLen <= m_textMax) {
		SDL_strlcpy(m_text, oldText, m_textMax+1);
	} else {
		SetText(oldText);
	}
	delete[] oldText;
}

void
UIElementEditbox::SetText(const char *text)
{
	SDL_strlcpy(m_text, text, m_textMax+1);
	m_textLen = SDL_strlen(m_text);
	OnTextChanged();
}

void
UIElementEditbox::OnTextChanged()
{
	UIElement::SetText(m_text);
}
