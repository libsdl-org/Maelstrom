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

#ifndef _gameover_h
#define _gameover_h

#include "protocol.h"

class GameOverPanelDelegate : public UIPanelDelegate
{
public:
	GameOverPanelDelegate(UIPanel *panel) : UIPanelDelegate(panel) { }

	virtual void OnShow();
	virtual void OnHide();
	virtual void OnTick();
	virtual bool HandleEvent(const SDL_Event &event);

protected:
	void BeginEnterName();
	void FinishEnterName();

protected:
	UIElement *m_handleLabel;
	int m_handleSize;
	char m_handle[MAX_NAMELEN+1];
	Uint32 m_showTime;
};

#endif // _gameover_h
