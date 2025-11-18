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

#ifndef _main_h
#define _main_h

#include "../screenlib/UIPanel.h"

class UITexture;

class MainPanelDelegate : public UIPanelDelegate
{
public:
	MainPanelDelegate(UIPanel *panel) : UIPanelDelegate(panel) { }

	virtual bool OnLoad();
	virtual void OnShow();
	virtual void OnTick();
	virtual bool HandleEvent(const SDL_Event &event);
	virtual bool OnAction(UIBaseElement *sender, const char *action);

protected:
	void OnActionPlay();
	void OnActionMultiplayer();
	void OnActionMultiplayerActivated();
	void OnActionQuitGame();
	void OnActionVolumeDown();
	void OnActionVolumeUp();
	void OnActionSetVolume(int volume);
	void OnActionToggleFullscreen();
	void OnActionToggleKidMode(UIBaseElement *sender);
	void OnActionKidModeActivated();
	void OnActionScreenshot();
	void OnActionCheat();
	void OnActionRunLastReplay();
	void OnActionRunReplay(int index);
	void OnActionZapHighScores();

protected:
	UITexture *m_spinnerImage;
};

// The real main function in main.cpp
extern int MaelstromMain(int argc, char *argv[]);

#endif // _main_h
