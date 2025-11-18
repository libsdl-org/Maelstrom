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

#ifndef _game_h
#define _game_h

/* ----------------------------------------------------------------- */
/* -- UI */

class UIElement;

class GamePanelDelegate : public UIPanelDelegate
{
public:
	GamePanelDelegate(UIPanel *panel) : UIPanelDelegate(panel) { }

	virtual bool OnLoad();
	virtual void OnShow();
	virtual void OnHide();
	virtual void OnTick();
	virtual void OnDraw(DRAWLEVEL drawLevel);
	virtual bool OnAction(UIBaseElement *sender, const char *action);

protected:
	void DrawStatus(Bool first);
	bool UpdateGameState();
	void DoHousekeeping();
	void DoBonus();
	void NextWave();
	void GameOver();

protected:
	UIElement *m_score;
	UIElement *m_shield;
	UIElement *m_wave;
	UIElement *m_lives;
	UIElement *m_bonus;

	UIElement *m_multiplier[4];
	UIElement *m_autofire;
	UIElement *m_airbrakes;
	UIElement *m_lucky;
	UIElement *m_triplefire;
	UIElement *m_longfire;

	UIElement *m_multiplayerCaption;
	UIElement *m_multiplayerColor;
	UIElement *m_fragsLabel;
	UIElement *m_frags;
};

/* ----------------------------------------------------------------- */
/* -- Game functions */

extern void NewGame(void);
extern void ContinueGame(void);
extern void GetRenderCoordinates(int &x, int &y);
extern void RenderSprite(UITexture *sprite, int x, int y, int w, int h);

#endif // _game_h
