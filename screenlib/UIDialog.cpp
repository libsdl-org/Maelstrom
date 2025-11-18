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
#include "UIManager.h"
#include "UIDialog.h"

UIElementType UIDialog::s_elementType;


UIDialog::UIDialog(UIManager *ui, const char *name) :
	UIPanel(ui, name)
{
	m_fullscreen = false;
	m_status = 0;
	m_handleInit = NULL;
	m_handleInitData = NULL;
	m_handleDone = NULL;
	m_handleDoneData = NULL;
}

void
UIDialog::Show()
{
	m_status = 0;

	if (m_handleInit) {
		m_handleInit(m_handleInitData, this);
	}

	UIPanel::Show();
}

void
UIDialog::Hide()
{
	UIPanel::Hide();

	if (m_handleDone) {
		m_handleDone(m_handleDoneData, this, m_status);
	}
}

bool
UIDialog::HandleEvent(const SDL_Event &event)
{
	if (UIPanel::HandleEvent(event)) {
		return true;
	}

	// Dialogs grab keyboard and mouse events
	if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP ||
	    event.type == SDL_TEXTEDITING || event.type == SDL_TEXTINPUT ||
	    event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP ||
	    event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEWHEEL) {
		// Press escape to cancel out of dialogs
		if (event.type == SDL_KEYUP &&
		    event.key.keysym.sym == SDLK_ESCAPE) {
			GetUI()->HidePanel(this);
		}
		return true;
	}

	return false;
}

UIDialogDelegate::UIDialogDelegate(UIPanel *panel) :
	UIPanelDelegate(panel)
{
	if (panel->IsA(UIDialog::GetType())) {
		m_dialog = static_cast<UIDialog*>(panel);
	} else {
		m_dialog = NULL;
	}
	assert(m_dialog);
}
