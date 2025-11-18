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
	if (!m_showTime) {
		m_showTime = 1;
	}
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

	int remaining = CONTINUE_TIME - (SDL_GetTicks() - m_showTime) / 1000;
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
    return false;
}
