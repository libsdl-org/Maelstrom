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

#include <stdlib.h>
#include <time.h>

#include "SDL_net.h"
#include "Maelstrom_Globals.h"
#include "../screenlib/UIElement.h"
#include "../screenlib/UIElementCheckbox.h"
#include "../screenlib/UIElementRadio.h"
#include "lobby.h"
#include "protocol.h"
#include "netplay.h"
#include "game.h"

// Update the game list every 3 seconds
#define GLOBAL_SERVER_HOST	"obelix.dreamhost.com"
//#define GLOBAL_SERVER_HOST	"localhost"

// Define this if you want local broadcast in addition to the global server
#define LOBBY_BROADCAST


class SelectControlCallback : public UIClickCallback
{
public:
	SelectControlCallback(LobbyDialogDelegate *lobby, UIElement *dialog, GameInfo &game, int index, Uint8 controlType) :
		m_lobby(lobby), m_dialog(dialog), m_game(game), m_index(index), m_controlType(controlType) { }

	virtual void operator()() {
		// Kick any player that was connected
		if (m_controlType != CONTROL_NETWORK) {
			const GameInfoPlayer *player = m_game.GetPlayer(m_index);
			int nodeIndex = m_game.GetNodeIndex(player->nodeID);
			if (nodeIndex >= 0) {
				m_lobby->SendKick(nodeIndex);
			}
		}

		// Select the control and hide the dialog
		if (IS_LOCAL_CONTROL(m_controlType)) {
			m_game.SetPlayerSlot(m_index, prefs->GetString(PREFERENCES_HANDLE), m_controlType);
		} else {
			m_game.SetPlayerSlot(m_index, NULL, m_controlType);
		}
		m_dialog->Hide();
	}

private:
	LobbyDialogDelegate *m_lobby;
	UIElement *m_dialog;
	GameInfo &m_game;
	int m_index;
	Uint8 m_controlType;
};

class ControlClickCallback : public UIClickCallback
{
public:
	ControlClickCallback(LobbyDialogDelegate *lobby, UIElement *button, UIElement *dialog, GameInfo &game, int index) :
		m_lobby(lobby), m_button(button), m_dialog(dialog), m_game(game), m_index(index) { }

	virtual void operator()() {
		if (!m_game.IsLocalPlayer(m_index) && !m_game.IsHosting()) {
			return;
		}

		// Show the control dialog
		SetControl(CONTROL_NONE, (m_index > 0) && m_game.IsHosting());
#ifdef USE_TOUCHCONTROL
		SetControl(CONTROL_TOUCH, true);
		SetControl(CONTROL_KEYBOARD, false);
		SetControl(CONTROL_KEYBOARD|CONTROL_JOYSTICK1, false);
		SetControl(CONTROL_JOYSTICK1, false);
		SetControl(CONTROL_JOYSTICK2, false);
		SetControl(CONTROL_JOYSTICK3, false);
#else
		SetControl(CONTROL_TOUCH, false);
		SetControl(CONTROL_KEYBOARD, true);
		SetControl(CONTROL_KEYBOARD|CONTROL_JOYSTICK1, true);
		SetControl(CONTROL_JOYSTICK1, SDL_NumJoysticks() > 0);
		SetControl(CONTROL_JOYSTICK2, SDL_NumJoysticks() > 1);
		SetControl(CONTROL_JOYSTICK3, SDL_NumJoysticks() > 2);
#endif
		SetControl(CONTROL_NETWORK, (m_index > 0) && m_game.IsHosting());

		m_dialog->SetAnchor(LEFT, RIGHT, m_button, -4, 0);
		m_dialog->Show();
	}

private:
	void SetControl(Uint8 control, bool enabled) {
		char name[128];
		UIElement *element;

		SDL_snprintf(name, sizeof(name), "control%d", control);
		element = m_dialog->GetElement<UIElement>(name);
		if (!element) {
			return;
		}
		if (enabled) {
			element->SetClickCallback(new SelectControlCallback(m_lobby, m_dialog, m_game, m_index, control));
			element->Show();
		} else {
			element->Hide();
		}
	}

private:
	LobbyDialogDelegate *m_lobby;
	UIElement *m_button;
	UIElement *m_dialog;
	GameInfo &m_game;
	int m_index;
};


LobbyDialogDelegate::LobbyDialogDelegate(UIPanel *panel) :
	UIDialogDelegate(panel),
	m_game(gGameInfo)
{
	m_state = STATE_NONE;
	m_lastPing = 0;
	m_lastRefresh = 0;
	m_requestSequence = 1;
}

bool
LobbyDialogDelegate::OnLoad()
{
	int i, count;
	IPaddress addresses[32];
	char name[32];

	// Get the address of the global server
	if (SDLNet_ResolveHost(&m_globalServer, GLOBAL_SERVER_HOST, LOBBY_PORT) < 0) {
		fprintf(stderr, "Warning: Couldn't resolve global server host %s\n", GLOBAL_SERVER_HOST);
	}

	// Get the addresses for this machine
	count = SDLNet_GetLocalAddresses(addresses, SDL_arraysize(addresses));
	m_addresses.clear();
	for (i = 0; i < count; ++i) {
		m_addresses.add(addresses[i]);
	}

	m_hostOrJoin = m_dialog->GetElement<UIElementRadioGroup>("hostOrJoin");
	if (!m_hostOrJoin) {
		fprintf(stderr, "Warning: Couldn't find radio group 'hostOrJoin'\n");
		return false;
	}
	m_hostOrJoin->SetValueCallback(this, &LobbyDialogDelegate::SetHostOrJoin);

	m_globalGame = m_dialog->GetElement<UIElementCheckbox>("globalGame");
	if (!m_globalGame) {
		fprintf(stderr, "Warning: Couldn't find checkbox 'globalGame'\n");
		return false;
	}
	if (m_globalServer.host == INADDR_NONE) {
		m_globalGame->SetChecked(false);
		m_globalGame->SetDisabled(true);
	}
	m_globalGame->SetClickCallback(this, &LobbyDialogDelegate::GlobalGameChanged);

	m_deathmatch = m_dialog->GetElement<UIElementRadioGroup>("deathmatch");
	if (!m_deathmatch) {
		fprintf(stderr, "Warning: Couldn't find radio group 'deathmatch'\n");
		return false;
	}
	m_deathmatch->SetValueCallback(this, &LobbyDialogDelegate::SetDeathmatch);
	if (!GetElement("gamelist", m_gameListArea)) {
		return false;
	}
	if (!GetElement("gameinfo", m_gameInfoArea)) {
		return false;
	}
	if (!GetElement("playButton", m_playButton)) {
		return false;
	}

	count = SDL_arraysize(m_gameListElements);
	for (i = 0; i < count; ++i) {
		SDL_snprintf(name, sizeof(name), "game%d", i+1);
		if (!GetElement(name, m_gameListElements[i])) {
			return false;
		}

		UIElement *button = m_gameListElements[i]->GetElement<UIElement>("join");
		if (button) {
			button->SetClickCallback(this, &LobbyDialogDelegate::JoinGameClicked, m_gameListElements[i]);
		}
	}

	m_controlDropdown = m_dialog->GetElement<UIElement>("control_dropdown");

	count = SDL_arraysize(m_gameInfoPlayers);
	for (i = 0; i < count; ++i) {
		SDL_snprintf(name, sizeof(name), "player%d", i+1);
		if (!GetElement(name, m_gameInfoPlayers[i])) {
			return false;
		}

		UIElement *controlButton = m_gameInfoPlayers[i]->GetElement<UIElement>("control");
		if (controlButton && m_controlDropdown) {
			controlButton->SetClickCallback(new ControlClickCallback(this, controlButton, m_controlDropdown, m_game, i));
		}
	}

	return true;
}

bool
LobbyDialogDelegate::GetElement(const char *name, UIElement *&element)
{
	element = m_dialog->GetElement<UIElement>(name);
	if (!element) {
		fprintf(stderr, "Warning: Couldn't find element '%s'\n", name);
		return false;
	}
	return true;
}

void
LobbyDialogDelegate::OnShow()
{
	// Seed the random number generator for our unique ID
	srand(time(NULL)+SDL_GetTicks());

	// Start up networking
	SetHostOrJoin(0, m_hostOrJoin->GetValue());
}

void
LobbyDialogDelegate::OnHide()
{
	// Start the game!
	if (m_dialog->GetDialogStatus() > 0) {
		SetState(STATE_PLAYING);
		NewGame();
	} else {
		SetState(STATE_NONE);
	}

	// Shut down networking
	HaltNetData();
}

void
LobbyDialogDelegate::OnPoll()
{
	if (m_state == STATE_NONE) {
		// Neither host nor join is checked
		return;
	}

	Uint32 now = SDL_GetTicks();
	if (!m_lastRefresh ||
	    (now - m_lastRefresh) > PING_INTERVAL) {
		if (m_state == STATE_HOSTING) {
			AdvertiseGame();
		} else if (m_state == STATE_LISTING) {
			GetGameList();
		} else if (m_state == STATE_JOINING) {
			SendJoinRequest();
		} else {
			GetGameInfo();
		}
		m_lastRefresh = now;
	}

	// See if there are any packets on the network
	m_packet.Reset();
	while (SDLNet_UDP_Recv(gNetFD, &m_packet)) {
		ProcessPacket(m_packet);
		m_packet.Reset();
	}

	// Do this after processing packets in case a pong was pending
	if (!m_lastPing || (now - m_lastPing) > PING_INTERVAL) {
		CheckPings();
		m_lastPing = now;
	}
}

void
LobbyDialogDelegate::SetHostOrJoin(void*, int value)
{
	// Remove the game before shutting down the network
	if (m_state == STATE_HOSTING && m_globalGame->IsChecked()) {
		RemoveGame();
	}

	// This is called when the lobby switches from hosting to joining
	HaltNetData();

	if (value > 0) {
		if (InitNetData(value == HOST_GAME) < 0) {
			m_hostOrJoin->SetValue(2);
			return;
		}

		Uint32 localID = rand();
		while (localID <= 1) {
			localID = rand();
		}
		m_game.SetLocalID(localID);

		if (value == HOST_GAME) {
			SetState(STATE_HOSTING);
		} else {
			SetState(STATE_LISTING);
		}
	} else {
		SetState(STATE_NONE);
	}
}

void
LobbyDialogDelegate::GlobalGameChanged(void*)
{
	m_lastRefresh = 0;

	if (!m_globalGame->IsChecked()) {
		if (m_state == STATE_HOSTING) {
			RemoveGame();
		} else {
			ClearGameList();
		}
	}
}

void
LobbyDialogDelegate::SetDeathmatch(void*, int value)
{
	if (value) {
		m_game.gameMode |= GAME_MODE_DEATHMATCH;
		m_game.deathMatch = (Uint8)value;
	} else {
		m_game.gameMode &= ~GAME_MODE_DEATHMATCH;
		m_game.deathMatch = 0;
	}
}

void
LobbyDialogDelegate::JoinGameClicked(void *_element)
{
	UIElement *element = (UIElement *)_element;
	for (int i = 0; (unsigned)i < SDL_arraysize(m_gameListElements); ++i) {
		if (element == m_gameListElements[i]) {
			// We found the one that was clicked!
			JoinGame(m_gameList[i]);
			break;
		}
	}
}

void
LobbyDialogDelegate::UpdateUI()
{
	if (m_state == STATE_NONE) {
		m_gameListArea->Hide();
		m_gameInfoArea->Hide();
	} else if (m_state == STATE_LISTING) {
		m_gameListArea->Show();
		m_gameInfoArea->Hide();
		for (int i = 0; (unsigned)i < SDL_arraysize(m_gameListElements); ++i) {
			if (i < m_gameList.length()) {
				m_gameListElements[i]->Show();
				m_gameList[i].BindPlayerToUI(0, m_gameListElements[i]);
			} else {
				m_gameListElements[i]->Hide();
			}
		}
	} else {
		m_gameInfoArea->Show();
		m_gameListArea->Hide();
		for (int i = 0; i < MAX_PLAYERS; ++i) {
			m_game.BindPlayerToUI(i, m_gameInfoPlayers[i]);
		}
		m_deathmatch->SetValue(m_game.deathMatch);
	}
	if (m_state == STATE_HOSTING) {
		m_playButton->SetDisabled(false);
	} else {
		m_playButton->SetDisabled(true);
	}
}

void
LobbyDialogDelegate::SetState(LOBBY_STATE state)
{
	int i;

	// Handle any state transitions here
	if (m_state == STATE_HOSTING && m_globalGame->IsChecked()) {
		RemoveGame();
	}
	if (m_state == STATE_HOSTING) {
		if (m_controlDropdown) {
			m_controlDropdown->Hide();
		}

		// Save the control preferences
		for (i = 0; i < MAX_PLAYERS; ++i) {
			char name[128];
			SDL_snprintf(name, sizeof(name), "Player%d.Controls", i+1);
			prefs->SetNumber(name, m_game.GetPlayer(i)->controlMask);
		}
		prefs->Save();
	}

	if (state == STATE_HOSTING || state == STATE_LISTING || state == STATE_NONE) {
		ClearGameInfo();
		ClearGameList();
	}

	if (state == STATE_NONE) {
		if (m_state == STATE_HOSTING) {
			// Notify the players that the game is gone
			for (i = 0; i < m_game.GetNumNodes(); ++i) {
				SendKick(i);
			}
		} else if (m_state == STATE_JOINING ||
			   m_state == STATE_JOINED) {
			// Notify the host that we're gone
			SendLeaveRequest();
		}
	} else if (state == STATE_HOSTING) {
		m_game.SetHost(DEFAULT_START_WAVE,
				DEFAULT_START_LIVES,
				DEFAULT_START_TURBO,
				prefs->GetNumber(PREFERENCES_DEATHMATCH),
				prefs->GetBool(PREFERENCES_KIDMODE));

		// Set up the controls for this game
		for (i = 0; i < MAX_PLAYERS; ++i) {
			Uint8 controlType;
			char name[128];
			SDL_snprintf(name, sizeof(name), "Player%d.Controls", i+1);
			controlType = prefs->GetNumber(name, (i == 0 ? CONTROL_LOCAL : CONTROL_NETWORK));
			if (IS_LOCAL_CONTROL(controlType)) {
				m_game.SetPlayerSlot(i, prefs->GetString(PREFERENCES_HANDLE), controlType);
			} else {
				m_game.SetPlayerSlot(i, NULL, controlType);
			}
		}
	} else if (state == STATE_LISTING) {
		ClearGameList();
	}

	// Set the state
	m_state = state;

	// Update the UI for the new state
	UpdateUI();

	m_lastPing = 0;

	// Send any packet requests immediately
	// Comment this out to simulate initial packet loss
	m_lastRefresh = 0;
}

void
LobbyDialogDelegate::CheckPings()
{
	// Check for ping timeouts
	if (m_state == STATE_LISTING) {
		bool removed = false;
		int i = 0;
		while (i < m_gameList.length()) {
			GameInfo &game = m_gameList[i];
			game.UpdatePingStatus(HOST_NODE);
			if (game.GetPingStatus(HOST_NODE) == PING_TIMEDOUT) {
//printf("Game timed out, removing from list\n");
				m_gameList.remove(game);
				removed = true;
			} else {
				++i;
			}
		}
		if (removed) {
			UpdateUI();
		}
	} else if (m_state == STATE_HOSTING) {
		m_game.UpdatePingStatus();
		for (int i = 0; i < m_game.GetNumNodes(); ++i) {
			if (m_game.GetPingStatus(i) == PING_TIMEDOUT) {
//printf("Player timed out, removing from lobby\n");
				SendKick(i);
			}
		}
	} else if (m_state == STATE_JOINED) {
		m_game.UpdatePingStatus();
		if (m_game.GetPingStatus(HOST_NODE) == PING_TIMEDOUT) {
//printf("Game timed out, leaving lobbyn");
			SetState(STATE_LISTING);
		}
	}

	if (m_state == STATE_HOSTING || m_state == STATE_JOINED) {

		// Send pings to everyone who is still here
		m_packet.StartLobbyMessage(LOBBY_PING);
		m_packet.Write(m_game.gameID);
		m_packet.Write(m_game.localID);
		m_packet.Write(SDL_GetTicks());

		for (int i = 0; i < m_game.GetNumNodes(); ++i) {
			if (m_game.IsNetworkNode(i)) {
				m_packet.address = m_game.GetNode(i)->address;
				
				SDLNet_UDP_Send(gNetFD, -1, &m_packet);
			}
		}
	}
}

void
LobbyDialogDelegate::AdvertiseGame()
{
	if (m_globalGame->IsChecked()) {
		m_packet.StartLobbyMessage(LOBBY_ANNOUNCE_GAME);
		PackAddresses(m_packet);
		m_packet.address = m_globalServer;

		SDLNet_UDP_Send(gNetFD, -1, &m_packet);
	}
}

void
LobbyDialogDelegate::RemoveGame()
{
	m_packet.StartLobbyMessage(LOBBY_REMOVE_GAME);
	PackAddresses(m_packet);
	m_packet.address = m_globalServer;

	SDLNet_UDP_Send(gNetFD, -1, &m_packet);
}

void
LobbyDialogDelegate::GetGameList()
{
	if (m_globalGame->IsChecked()) {
		m_packet.StartLobbyMessage(LOBBY_REQUEST_GAME_SERVERS);
		PackAddresses(m_packet);
		m_packet.address = m_globalServer;

		SDLNet_UDP_Send(gNetFD, -1, &m_packet);
	}

#ifdef LOBBY_BROADCAST
	// Get game info for local games
	m_packet.StartLobbyMessage(LOBBY_REQUEST_GAME_INFO);
	m_packet.Write(SDL_GetTicks());
	m_packet.address.host = INADDR_BROADCAST;
	m_packet.address.port = SDL_SwapBE16(NETPLAY_PORT);
	SDLNet_UDP_Send(gNetFD, -1, &m_packet);
#endif
}

void
LobbyDialogDelegate::GetGameInfo()
{
	m_packet.StartLobbyMessage(LOBBY_REQUEST_GAME_INFO);
	m_packet.Write(SDL_GetTicks());
	m_packet.address = m_game.GetHost()->address;
	SDLNet_UDP_Send(gNetFD, -1, &m_packet);
}

void
LobbyDialogDelegate::JoinGame(GameInfo &game)
{
	m_game.CopyFrom(game);
	m_game.InitializePing();
	SetState(STATE_JOINING);
}

void
LobbyDialogDelegate::SendJoinRequest()
{
	m_packet.StartLobbyMessage(LOBBY_REQUEST_JOIN);
	m_packet.Write(m_game.gameID);
	m_packet.Write(m_game.localID);
	m_packet.Write(prefs->GetString(PREFERENCES_HANDLE));
	m_packet.address = m_game.GetHost()->address;

	SDLNet_UDP_Send(gNetFD, -1, &m_packet);
}

void
LobbyDialogDelegate::SendLeaveRequest()
{
	m_packet.StartLobbyMessage(LOBBY_REQUEST_LEAVE);
	m_packet.Write(m_game.gameID);
	m_packet.Write(m_game.localID);
	m_packet.address = m_game.GetHost()->address;

	SDLNet_UDP_Send(gNetFD, -1, &m_packet);
}

void
LobbyDialogDelegate::SendKick(int index)
{
	const GameInfoNode *node;

	if (!m_game.IsNetworkNode(index)) {
		return;
	}

	node = m_game.GetNode(index);
	m_packet.StartLobbyMessage(LOBBY_KICK);
	m_packet.Write(m_game.gameID);
	m_packet.Write(node->nodeID);
	m_packet.address = node->address;

	SDLNet_UDP_Send(gNetFD, -1, &m_packet);

	// Now remove them from the game list
	m_game.RemoveNode(node->nodeID);
}

void
LobbyDialogDelegate::ClearGameInfo()
{
	m_game.Reset();
}

void
LobbyDialogDelegate::ClearGameList()
{
	m_gameList.clear();
}

void
LobbyDialogDelegate::PackAddresses(DynamicPacket &packet)
{
	Uint16 port;

	port = SDLNet_UDP_GetPeerAddress(gNetFD, -1)->port;

	m_packet.Write((Uint8)m_addresses.length());
	for (int i = 0; i < m_addresses.length(); ++i) {
		m_packet.Write(m_addresses[i].host);
		m_packet.Write(port);
	}
}

void
LobbyDialogDelegate::ProcessPacket(DynamicPacket &packet)
{
	Uint8 cmd;

	if (!m_packet.Read(cmd)) {
		return;
	}
	if (cmd != LOBBY_MSG) {
		if (cmd == NEW_GAME) {
			ProcessNewGame(packet);
		}
		return;
	}
	if (!m_packet.Read(cmd)) {
		return;
	}

	if (m_state == STATE_HOSTING) {
		if (cmd == LOBBY_ANNOUNCE_PLAYER) {
			if (m_globalGame->IsChecked()) {
				ProcessAnnouncePlayer(packet);
			}
			return;
		}

		if (m_game.IsFull() && !m_game.HasNode(packet.address)) {
			return;
		}

		if (cmd == LOBBY_PING) {
			ProcessPing(packet);
		} else if (cmd == LOBBY_PONG) {
			ProcessPong(packet);
		} else if (cmd == LOBBY_REQUEST_GAME_INFO) {
			ProcessRequestGameInfo(packet);
		} else if (cmd == LOBBY_REQUEST_JOIN) {
			ProcessRequestJoin(packet);
		} else if (cmd == LOBBY_REQUEST_LEAVE) {
			ProcessRequestLeave(packet);
		}
		return;

	}

	if (m_state == STATE_LISTING) {
		if (cmd == LOBBY_GAME_SERVERS) {
			if (m_globalGame->IsChecked()) {
				ProcessGameServerList(packet);
			}
			return;
		}
	}

	// These packets we handle in all the join states
	if (cmd == LOBBY_PING) {
		ProcessPing(packet);
	} else if (cmd == LOBBY_PONG) {
		ProcessPong(packet);
	} else if (cmd == LOBBY_GAME_INFO) {
		ProcessGameInfo(packet);
	} else if (cmd == LOBBY_KICK) {
		ProcessKick(packet);
	}
}

void
LobbyDialogDelegate::ProcessPing(DynamicPacket &packet)
{
	Uint32 gameID;
	Uint32 nodeID;
	Uint32 timestamp;

	if (m_state != STATE_HOSTING && m_state != STATE_JOINED) {
		return;
	}
	if (!packet.Read(gameID) || gameID != m_game.gameID) {
		return;
	}
	if (!packet.Read(nodeID) || !m_game.HasNode(nodeID)) {
		return;
	}
	if (!packet.Read(timestamp)) {
		return;
	}

	m_reply.StartLobbyMessage(LOBBY_PONG);
	m_reply.Write(gameID);
	m_reply.Write(nodeID);
	m_reply.Write(timestamp);
	m_reply.address = packet.address;

	SDLNet_UDP_Send(gNetFD, -1, &m_reply);
}

void
LobbyDialogDelegate::ProcessPong(DynamicPacket &packet)
{
	Uint32 gameID;
	Uint32 nodeID;
	Uint32 timestamp;

	if (m_state != STATE_HOSTING && m_state != STATE_JOINED) {
		return;
	}
	if (!packet.Read(gameID) || gameID != m_game.gameID) {
		return;
	}
	if (!packet.Read(nodeID) || nodeID != m_game.localID) {
		return;
	}
	if (!packet.Read(timestamp)) {
		return;
	}

	for (int i = 0; i < m_game.GetNumNodes(); ++i) {
		if (packet.address == m_game.GetNode(i)->address) {
			m_game.UpdatePingTime(i, timestamp);
		}
	}
}

void
LobbyDialogDelegate::ProcessNewGame(DynamicPacket &packet)
{
	GameInfo game;

	if (!game.ReadFromPacket(packet)) {
		return;
	}
	if (game.gameID != m_game.gameID) {
		// Probably an old packet...
		return;
	}
	if (m_game.IsHosting()) {
		// They can't tell us to start!
		return;
	}

	// Ooh, ooh, they're starting!
	m_game.CopyFrom(game);

	// Send a response
	m_reply.Reset();
	m_reply.Write((Uint8)NEW_GAME_ACK);
	m_reply.Write(m_game.gameID);
	m_reply.Write(m_game.localID);
	m_reply.address = packet.address;

	SDLNet_UDP_Send(gNetFD, -1, &m_reply);

	if (m_game.HasNode(packet.address)) {
		m_playButton->OnClick();
	}
}

void
LobbyDialogDelegate::ProcessAnnouncePlayer(DynamicPacket &packet)
{
	Uint8 count;
	IPaddress address;

	// Open the firewall so this player can contact us.
	m_reply.StartLobbyMessage(LOBBY_OPEN_FIREWALL);

	if (!packet.Read(count)) {
		return;
	}
	for (Uint8 i = 0; i < count; ++i) {
		if (!packet.Read(address.host) ||
		    !packet.Read(address.port)) {
			return;
		}
		m_reply.address = address;
		
		SDLNet_UDP_Send(gNetFD, -1, &m_reply);
	}
}

void
LobbyDialogDelegate::ProcessRequestGameInfo(DynamicPacket &packet)
{
	Uint32 timestamp;

	if (!packet.Read(timestamp)) {
		return;
	}

	m_reply.StartLobbyMessage(LOBBY_GAME_INFO);
	m_reply.Write(timestamp);
	m_game.WriteToPacket(m_reply);
	m_reply.address = packet.address;

	SDLNet_UDP_Send(gNetFD, -1, &m_reply);
}

void
LobbyDialogDelegate::ProcessRequestJoin(DynamicPacket &packet)
{
	Uint32 gameID;
	Uint32 nodeID;
	char name[MAX_NAMELEN+1];

	if (!packet.Read(gameID) || gameID != m_game.gameID) {
		return;
	}
	if (!packet.Read(nodeID)) {
		return;
	}
	if (!packet.Read(name, sizeof(name))) {
		return;
	}
	if (m_game.HasNode(nodeID)) {
		// We already have this node, ignore it
		return;
	}

	m_game.AddNetworkPlayer(nodeID, packet.address, name);

	// Let everybody know!
	m_reply.StartLobbyMessage(LOBBY_GAME_INFO);
	m_reply.Write((Uint32)0);
	m_game.WriteToPacket(m_reply);
	for (int i = 0; i < m_game.GetNumNodes(); ++i) {
		if (m_game.IsNetworkNode(i)) {
			m_reply.address = m_game.GetNode(i)->address;
			SDLNet_UDP_Send(gNetFD, -1, &m_reply);
		}
	}
}

void
LobbyDialogDelegate::ProcessRequestLeave(DynamicPacket &packet)
{
	Uint32 gameID;
	Uint32 nodeID;

	if (!packet.Read(gameID) || gameID != m_game.gameID) {
		return;
	}
	if (!packet.Read(nodeID) || !m_game.HasNode(nodeID)) {
		return;
	}
	if (nodeID == m_game.localID) {
		return;
	}

	// Okay, clear them from the list!
	m_game.RemoveNode(nodeID);
}

void
LobbyDialogDelegate::ProcessGameInfo(DynamicPacket &packet)
{
	Uint32 timestamp;
	GameInfo game;

	if (!packet.Read(timestamp)) {
		return;
	}

	if (!game.ReadFromPacket(packet)) {
		return;
	}

	if (m_state == STATE_LISTING) {
		// Add or update the game list
		int i;
		for (i = 0; i < m_gameList.length(); ++i) {
			if (game.gameID == m_gameList[i].gameID) {
				m_gameList[i].CopyFrom(game);
				break;
			}
		}
		if (i == m_gameList.length()) {
			game.InitializePing();
			m_gameList.add(game);
		}
		if (timestamp) {
			m_gameList[i].UpdatePingTime(HOST_NODE, timestamp);
			m_gameList[i].UpdatePingStatus(HOST_NODE);
		}
		UpdateUI();
	} else {
		if (game.gameID != m_game.gameID) {
			// Probably an old packet...
			return;
		}

		m_game.CopyFrom(game);

		if (m_state == STATE_JOINING) {
			if (m_game.HasNode(m_game.localID)) {
				// We successfully joined the game
				SetState(STATE_JOINED);
			}
		} else {
			if (!m_game.HasNode(m_game.localID)) {
				// We were kicked from the game
				SetState(STATE_LISTING);
			}
		}
	}
}

void
LobbyDialogDelegate::ProcessKick(DynamicPacket &packet)
{
	Uint32 gameID;
	Uint32 nodeID;

	if (m_state != STATE_JOINING && m_state != STATE_JOINED) {
		return;
	}
	if (!packet.Read(gameID) || gameID != m_game.gameID) {
		return;
	}
	if (!packet.Read(nodeID) || nodeID != m_game.localID) {
		return;
	}

	SetState(STATE_LISTING);
}

void
LobbyDialogDelegate::ProcessGameServerList(DynamicPacket &packet)
{
	Uint8 serverCount, count;
	IPaddress address;

	// Request game information from the servers
	m_reply.StartLobbyMessage(LOBBY_REQUEST_GAME_INFO);
	m_reply.Write(SDL_GetTicks());

	if (!packet.Read(serverCount)) {
		return;
	}
	for (Uint8 i = 0; i < serverCount; ++i) {
		if (!packet.Read(count)) {
			return;
		}
		for (Uint8 j = 0; j < count; ++j) {
			if (!packet.Read(address.host) ||
			    !packet.Read(address.port)) {
				return;
			}
			m_reply.address = address;
			
			SDLNet_UDP_Send(gNetFD, -1, &m_reply);
		}
	}
}
