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

#ifndef _lobby_h
#define _lobby_h

#include "SDL_net.h"
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

	override bool OnLoad();
	override void OnShow();
	override void OnHide();
	override void OnPoll();

	void SendKick(int index);

protected:
	bool GetElement(const char *name, UIElement *&element);
	void SetHostOrJoin(void*, int value);
	void GlobalGameChanged(void*);
	void SetDeathmatch(void*, int value);
	void JoinGameClicked(void *element);

	void UpdateUI();

	void CheckPings();
	void AdvertiseGame();
	void RemoveGame();
	void GetGameList();
	void GetGameInfo();
	void JoinGame(GameInfo &game);
	void SendJoinRequest();
	void SendLeaveRequest();
	void ClearGameInfo();
	void ClearGameList();

	void PackAddresses(DynamicPacket &packet);

	void ProcessPacket(DynamicPacket &packet);
	void ProcessPing(DynamicPacket &packet);
	void ProcessPong(DynamicPacket &packet);
	void ProcessNewGame(DynamicPacket &packet);
	void ProcessAnnouncePlayer(DynamicPacket &packet);
	void ProcessRequestGameInfo(DynamicPacket &packet);
	void ProcessRequestJoin(DynamicPacket &packet);
	void ProcessRequestLeave(DynamicPacket &packet);
	void ProcessGameServerList(DynamicPacket &packet);
	void ProcessGameInfo(DynamicPacket &packet);
	void ProcessKick(DynamicPacket &packet);

protected:
	IPaddress m_globalServer;
	array<IPaddress> m_addresses;

	enum LOBBY_STATE {
		STATE_NONE,
		STATE_HOSTING,
		STATE_LISTING,
		STATE_JOINING,
		STATE_JOINED,
		STATE_PLAYING
	} m_state;

	Uint32 m_lastPing;
	Uint32 m_lastRefresh;
	Uint32 m_requestSequence;

	GameInfo &m_game;
	array<GameInfo> m_gameList;

	DynamicPacket m_packet, m_reply;

	UIElementRadioGroup *m_hostOrJoin;
	UIElementCheckbox *m_globalGame;
	UIElementRadioGroup *m_deathmatch;
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
