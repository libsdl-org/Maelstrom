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

#include "Maelstrom_Globals.h"
#include "continue.h"
#include "game.h"

#define CONTINUE_TIME 10

/* ----------------------------------------------------------------- */
/* -- Do the continue game display */

void ContinuePanelDelegate::OnShow()
{
	m_timeoutLabel = m_panel->GetElement<UIElement>("timeout");
	m_showTime = SDL_GetTicks();
	screen->ShowCursor();
	gGameInfo.SetLocalState(STATE_DIALOG, true);
}

void ContinuePanelDelegate::OnHide()
{
	screen->HideCursor();
	gGameInfo.SetLocalState(STATE_DIALOG, false);

	if (m_dialog->GetDialogStatus() == 1) {
		ContinueGame();
	}
}

void ContinuePanelDelegate::OnTick()
{
	if (m_showTime == 0) {
		// Timer is stopped, stay up until dismissed.
		return;
	}

	int remaining = CONTINUE_TIME - (int)((SDL_GetTicks() - m_showTime) / 1000);
	if (remaining <= 0) {
		ui->HidePanel(m_panel);
		return;
	}

	if (m_timeoutLabel) {
		char num[32];
		SDL_itoa(remaining, num, 10);
		m_timeoutLabel->SetText(num);
	}
}

bool ContinuePanelDelegate::HandleEvent(const SDL_Event &event)
{
	if (event.type == SDL_EVENT_QUIT) {
		ui->HidePanel(m_panel);
	}
	return false;
}
