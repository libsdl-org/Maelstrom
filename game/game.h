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

#ifndef _game_h
#define _game_h

/* ----------------------------------------------------------------- */
/* -- UI */

class UIElement;

class GamePanelDelegate : public UIPanelDelegate
{
public:
	GamePanelDelegate(UIPanel *panel) : UIPanelDelegate(panel) { }
	virtual ~GamePanelDelegate();

	virtual bool OnLoad();
	virtual void OnShow();
	virtual void OnHide();
	virtual void OnTick();
	virtual void OnDraw(DRAWLEVEL drawLevel);
	virtual bool OnAction(UIBaseElement *sender, const char *action);

	void ObserveEvent(const SDL_Event *event);

protected:
	static bool SDLCALL EventWatch(void *userdata, SDL_Event *event);

	void ShowTouchControls();
	void HideTouchControls();
	void HandleTouchFading();
	void UpdateZoom();
	void StartZoomUI(const SDL_Rect &rect);
	void StopZoomUI();
	void ToggleZoomGame();
	void StartZoomedDrawing();
	void StopZoomedDrawing();
	void DrawStatus(Bool first);
	bool UpdateGameState();
	void DoHousekeeping();
	void DoBonus();
	void ShowBonus();
	void BonusShowValue();
	void BonusShowMultiplier();
	void BonusDisplayDelay();
	void BonusDisplay();
	void BonusCheckSound();
	void BonusTaunt();
	void BonusPraise();
	void BonusCountdown();
	void BonusNextWave();
	void BonusHide();
	void NextWave();
	void StartNextWave();
	void GameOver();

protected:
	UIElement *m_touchControls;

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

	UIElement *m_paused;
	UIElement *m_zoomIn;
	UIElement *m_zoomOut;

	UITexture *m_background;

	enum {
		STATE_PLAYING,
		STATE_SHOW_BONUS,
		STATE_BONUS_SHOW_VALUE,
		STATE_BONUS_SHOW_MULTIPLIER,
		STATE_BONUS_DISPLAY_DELAY,
		STATE_BONUS_DISPLAY,
		STATE_BONUS_CHECK_SOUND,
		STATE_BONUS_TAUNT,
		STATE_BONUS_PRAISE,
		STATE_BONUS_COUNTDOWN,
		STATE_BONUS_NEXT_WAVE,
		STATE_BONUS_HIDE,
		STATE_START_NEXT_WAVE,
	} m_state;

	SDL_Texture *m_texture = nullptr;
	SDL_Rect m_savedClip;

	Uint64 m_lastTouch;
	bool m_touchFading;
	int m_fadeStep;
};

/* ----------------------------------------------------------------- */
/* -- Game functions */

extern void NewGame(void);
extern void ContinueGame(void);
extern void GetRenderCoordinates(int &x, int &y);
extern void RenderSprite(UITexture *sprite, int x, int y, int w, int h);

#endif // _game_h
