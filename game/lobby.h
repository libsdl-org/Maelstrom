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

#ifndef _lobby_h
#define _lobby_h

#include "protocol.h"
#include "packet.h"
#include "gameinfo.h"
#include "../utils/array.h"
#include "../screenlib/UIDialog.h"

// Forward declarations of UI elements
class UIElementCheckbox;
class UIElementRadioGroup;

enum {
	HOST_GAME = 1,
	JOIN_GAME = 2,
};

class LobbyDialogDelegate : public UIDialogDelegate
{
public:
	LobbyDialogDelegate(UIPanel *panel);

	virtual bool OnLoad() override;
	virtual void OnShow() override;
	virtual void OnHide() override;
	virtual void OnPoll() override;
	virtual bool HandleEvent(const SDL_Event &event) override;

	void SendKick(int index);

protected:
	bool GetElement(const char *name, UIElement *&element);
	void SetHostOrJoin(void *, int value);
	void JoinGameClicked(void *element);
	void DeathmatchChanged(void *, int value);
	void LivesChanged(void *, const char *text);

	void UpdateUI();

	void CheckPings();
	void GetGameList();
	void GetGameInfo();
	void JoinGame(GameInfo &game);
	void SendJoinRequest();
	void SendLeaveRequest();
	void ClearGameInfo();
	void ClearGameList();

	void ProcessPacket(DynamicPacket &packet);
	void ProcessPing(DynamicPacket &packet);
	void ProcessPong(DynamicPacket &packet);
	void ProcessNewGame(DynamicPacket &packet);
	void ProcessRequestGameInfo(DynamicPacket &packet);
	void ProcessRequestJoin(DynamicPacket &packet);
	void ProcessRequestLeave(DynamicPacket &packet);
	void ProcessGameInfo(DynamicPacket &packet);
	void ProcessKick(DynamicPacket &packet);

protected:
	enum LOBBY_STATE {
		STATE_NONE,
		STATE_HOSTING,
		STATE_LISTING,
		STATE_JOINING,
		STATE_JOINED,
		STATE_PLAYING
	} m_state;

	Uint64 m_lastPing;
	Uint64 m_lastRefresh;
	Uint32 m_requestSequence;

	GameInfo &m_game;
	array<GameInfo> m_gameList;

	DynamicPacket m_packet, m_reply;

	UIElementRadioGroup *m_hostOrJoin;
	UIElementRadioGroup *m_deathmatch;
	UIElement *m_lives;
	UIElement *m_livesLabel;
	UIElement *m_livesValue;
	UIElement *m_frags;
	UIElement *m_fragsLabel;
	UIElement *m_fragsValue;
	UIElement *m_gameListArea;
	UIElement *m_gameListElements[5];
	UIElement *m_gameInfoArea;
	UIElement *m_gameInfoPlayers[MAX_PLAYERS];
	UIElement *m_playButton;
	UIElement *m_controlDropdown;

protected:
	void SetState(LOBBY_STATE state);
};

#endif // _lobby_h
