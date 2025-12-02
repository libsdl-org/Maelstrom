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
#include "../screenlib/UIElement.h"
#include "../screenlib/UIElementRadio.h"
#include "lobby.h"
#include "protocol.h"
#include "netplay.h"
#include "game.h"

#ifdef SDL_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>
#include <iphlpapi.h>
#endif

#ifdef HAVE_GETIFADDRS
#include <ifaddrs.h>
#include <netinet/in.h>
#endif


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
		int num_gamepads = GetNumGamepads();
		SetControl(CONTROL_NONE, (m_index > 0) && m_game.IsHosting());
		SetControl(CONTROL_LOCAL, !m_game.OtherPlayerHasControl(m_index, CONTROL_LOCAL));
		SetControl(CONTROL_JOYSTICK1, num_gamepads > 0 && !m_game.OtherPlayerHasControl(m_index, CONTROL_JOYSTICK1));
		SetControl(CONTROL_JOYSTICK2, num_gamepads > 1 && !m_game.OtherPlayerHasControl(m_index, CONTROL_JOYSTICK2));
		SetControl(CONTROL_JOYSTICK3, num_gamepads > 2 && !m_game.OtherPlayerHasControl(m_index, CONTROL_JOYSTICK3));
		SetControl(CONTROL_REMOTE1, (m_index > 0) && m_game.IsHosting() && GetRemotePlayerName(CONTROL_REMOTE1) && !m_game.OtherPlayerHasControl(m_index, CONTROL_REMOTE1));
		SetControl(CONTROL_REMOTE2, (m_index > 0) && m_game.IsHosting() && GetRemotePlayerName(CONTROL_REMOTE2) && !m_game.OtherPlayerHasControl(m_index, CONTROL_REMOTE2));
		SetControl(CONTROL_NETWORK, (m_index > 0) && m_game.IsHosting());

		m_dialog->SetAnchor(LEFT, RIGHT, m_button, -4, 0);
		m_dialog->Show();
	}

private:
	int GetNumGamepads()
	{
		int num_gamepads = 0;
		SDL_JoystickID* gamepads = SDL_GetGamepads(nullptr);
		if (gamepads) {
			for (int i = 0; gamepads[i]; ++i) {
				SDL_Gamepad* gamepad = SDL_OpenGamepad(gamepads[i]);
				if (gamepad) {
					if (!GetRemoteSessionForGamepad(gamepad)) {
						++num_gamepads;
					}
					SDL_CloseGamepad(gamepad);
				}
			}
			SDL_free(gamepads);
		}
		return num_gamepads;
	}
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
	char name[32];

	m_hostOrJoin = m_dialog->GetElement<UIElementRadioGroup>("hostOrJoin");
	if (!m_hostOrJoin) {
		SDL_Log("Warning: Couldn't find radio group 'hostOrJoin'");
		return false;
	}
	m_hostOrJoin->SetValueCallback(this, &LobbyDialogDelegate::SetHostOrJoin);

	m_deathmatch = m_dialog->GetElement<UIElement>("deathmatch");
	if (!m_deathmatch) {
		SDL_Log("Warning: Couldn't find editbox 'deathmatch'");
		return false;
	}
	m_deathmatch->SetTextCallback(this, &LobbyDialogDelegate::DeathmatchChanged, nullptr);

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
		SDL_Log("Warning: Couldn't find element '%s'", name);
		return false;
	}
	return true;
}

void
LobbyDialogDelegate::OnShow()
{
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
		CloseSocket();
	}
}

void
LobbyDialogDelegate::OnPoll()
{
	if (m_state == STATE_NONE) {
		// Neither host nor join is checked
		return;
	}

	Uint64 now = SDL_GetTicks();
	if (!m_lastRefresh ||
	    (now - m_lastRefresh) > PING_INTERVAL) {
		if (m_state == STATE_HOSTING) {
			// Nothing to do
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
	NET_Datagram *datagram;
	DynamicPacket packet;
	while (NET_ReceiveDatagram(gSocket, &datagram) && datagram) {
		packet.data = datagram->buf;
		packet.len = datagram->buflen;
		packet.address.host = datagram->addr;
		packet.address.port = datagram->port;
		ProcessPacket(packet);
		packet.Reset();
	}
	packet.address.host = nullptr;

	// Do this after processing packets in case a pong was pending
	if (!m_lastPing || (now - m_lastPing) > PING_INTERVAL) {
		CheckPings();
		m_lastPing = now;
	}
}

void
LobbyDialogDelegate::SetHostOrJoin(void*, int value)
{
	// This is called when the lobby switches from hosting to joining
	CloseSocket();

	if (value > 0) {
		bool hosting = (value == HOST_GAME);
		if (CreateSocket(hosting) < 0) {
			m_hostOrJoin->SetValue(2);
			return;
		}

		Uint32 localID = SDL_rand_bits();
		while (localID <= 1) {
			localID = SDL_rand_bits();
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
LobbyDialogDelegate::DeathmatchChanged(void *, const char *text)
{
	m_game.deathMatch = SDL_atoi(text);
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
		for (unsigned int i = 0; i < SDL_arraysize(m_gameListElements); ++i) {
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

		char deathmatch[10];
		m_deathmatch->SetText(SDL_itoa(m_game.deathMatch, deathmatch, 10));
	}
	if (m_state == STATE_HOSTING) {
		m_playButton->SetDisabled(false);
		m_deathmatch->SetDisabled(false);
	} else {
		m_playButton->SetDisabled(true);
		m_deathmatch->SetDisabled(true);
	}
}

void
LobbyDialogDelegate::SetState(LOBBY_STATE state)
{
	int i;

	// Handle any state transitions here
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
		unsigned int i = 0;
		while (i < m_gameList.length()) {
			GameInfo &game = m_gameList[i];
			game.UpdatePingStatus(HOST_NODE);
			if (game.GetPingStatus(HOST_NODE) == PING_TIMEDOUT) {
//SDL_Log("Game timed out, removing from list");
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
//SDL_Log("Player timed out, removing from lobby");
				SendKick(i);
			}
		}
	} else if (m_state == STATE_JOINED) {
		m_game.UpdatePingStatus();
		if (m_game.GetPingStatus(HOST_NODE) == PING_TIMEDOUT) {
//SDL_Log("Game timed out, leaving lobby");
			SetState(STATE_LISTING);
		}
	}

	if (m_state == STATE_HOSTING || m_state == STATE_JOINED) {

		// Send pings to everyone who is still here
		m_packet.StartLobbyMessage(LOBBY_PING);
		m_packet.Write(m_game.gameID);
		m_packet.Write(m_game.localID);
		m_packet.Write((Uint32)SDL_GetTicks());

		for (int i = 0; i < m_game.GetNumNodes(); ++i) {
			if (m_game.IsNetworkNode(i)) {
				IPaddress address = m_game.GetNode(i)->address;
				NET_SendDatagram(gSocket, address.host, address.port, m_packet.data, m_packet.len);
			}
		}
	}
}

#ifdef HAVE_GETIFADDRS
static Uint32 SockAddrToUint32(struct sockaddr* a)
{
	return ((a) && (a->sa_family == AF_INET)) ? SDL_Swap32BE(((struct sockaddr_in*)a)->sin_addr.s_addr) : 0;
}
#endif

#if defined(SDL_PLATFORM_WIN32) || defined(HAVE_GETIFADDRS)
// convert a numeric IP address into its string representation
static void Inet_NtoA(Uint32 addr, char *ipbuf, size_t maxlen)
{
	SDL_snprintf(ipbuf, maxlen, "%u.%u.%u.%u", (addr >> 24) & 0xFF, (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, (addr >> 0) & 0xFF);
}

// convert a string representation of an IP address into its numeric equivalent
static Uint32 Inet_AtoN(const char* buf)
{
	// net_server inexplicably doesn't have this function; so I'll just fake it
	Uint32 ret = 0;
	int shift = 24;  // fill out the MSB first
	bool startQuad = true;
	while ((shift >= 0) && (*buf))
	{
		if (startQuad)
		{
			unsigned char quad = (unsigned char)atoi(buf);
			ret |= (((Uint32)quad) << shift);
			shift -= 8;
		}
		startQuad = (*buf == '.');
		buf++;
	}
	return ret;
}
#endif // SDL_PLATFORM_WIN32 || HAVE_GETIFADDRS

static bool NET_SendDatagramBroadcast(NET_DatagramSocket *sock, Uint16 port, const void* buf, int buflen)
{
#ifdef SDL_PLATFORM_WIN32
	// Windows XP style implementation
	HMODULE hiphlpapi = LoadLibraryA("Iphlpapi.dll");
	typedef DWORD (WINAPI *GetIpAddrTable_t)(PMIB_IPADDRTABLE pIpAddrTable, PULONG pdwSize, BOOL bOrder);
	GetIpAddrTable_t GetIpAddrTableFunc = (GetIpAddrTable_t)GetProcAddress(hiphlpapi, "GetIpAddrTable");
	typedef ULONG (WINAPI *GetAdaptersInfo_t)(PIP_ADAPTER_INFO AdapterInfo, PULONG SizePointer);
	GetAdaptersInfo_t GetAdaptersInfoFunc = (GetAdaptersInfo_t)GetProcAddress(hiphlpapi, "GetAdaptersInfo");

	// Adapted from example code at http://msdn2.microsoft.com/en-us/library/aa365917.aspx
	// Now get Windows' IPv4 addresses table.  Once again, we gotta call GetIpAddrTable()
	// multiple times in order to deal with potential race conditions properly.
	MIB_IPADDRTABLE* ipTable = NULL;
	{
		ULONG iptablelen = 0;
		for (int i = 0; i < 5; i++)
		{
			DWORD ipRet = GetIpAddrTableFunc(ipTable, &iptablelen, false);
			if (ipRet == ERROR_INSUFFICIENT_BUFFER)
			{
				free(ipTable);  // in case we had previously allocated it
				ipTable = (MIB_IPADDRTABLE*)malloc(iptablelen);
			}
			else if (ipRet == NO_ERROR) break;
			else
			{
				free(ipTable);
				ipTable = NULL;
				break;
			}
		}
	}

	if (ipTable)
	{
		// Try to get the Adapters-info table, so we can given useful names to the IP
		// addresses we are returning.  Gotta call GetAdaptersInfo() up to 5 times to handle
		// the potential race condition between the size-query call and the get-data call.
		// I love a well-designed API :^P
		IP_ADAPTER_INFO* pAdapterInfo = NULL;
		{
			ULONG bufLen = 0;
			for (int i = 0; i < 5; i++)
			{
				DWORD apRet = GetAdaptersInfoFunc(pAdapterInfo, &bufLen);
				if (apRet == ERROR_BUFFER_OVERFLOW)
				{
					free(pAdapterInfo);  // in case we had previously allocated it
					pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufLen);
				}
				else if (apRet == ERROR_SUCCESS) break;
				else
				{
					free(pAdapterInfo);
					pAdapterInfo = NULL;
					break;
				}
			}
		}

		for (DWORD i = 0; i < ipTable->dwNumEntries; i++)
		{
			const MIB_IPADDRROW& row = ipTable->table[i];

			// Now lookup the appropriate adaptor-name in the pAdaptorInfos, if we can find it
			const char* name = NULL;
			const char* desc = NULL;
			if (pAdapterInfo)
			{
				IP_ADAPTER_INFO* next = pAdapterInfo;
				while ((next) && (name == NULL))
				{
					IP_ADDR_STRING* ipAddr = &next->IpAddressList;
					while (ipAddr)
					{
						if (Inet_AtoN(ipAddr->IpAddress.String) == SDL_Swap32BE(row.dwAddr))
						{
							name = next->AdapterName;
							desc = next->Description;
							break;
						}
						ipAddr = ipAddr->Next;
					}
					next = next->Next;
				}
			}
			char namebuf[128];
			if (name == NULL)
			{
				SDL_snprintf(namebuf, sizeof(namebuf), "unnamed-%i", i);
				name = namebuf;
			}

			Uint32 ipAddr = SDL_Swap32BE(row.dwAddr);
			Uint32 netmask = SDL_Swap32BE(row.dwMask);
			Uint32 baddr = ipAddr & netmask;
			if (row.dwBCastAddr) baddr |= ~netmask;

			char ifaAddrStr[32];  Inet_NtoA(ipAddr, ifaAddrStr, sizeof(ifaAddrStr));
			char maskAddrStr[32]; Inet_NtoA(netmask, maskAddrStr, sizeof(maskAddrStr));
			char dstAddrStr[32];  Inet_NtoA(baddr, dstAddrStr, sizeof(dstAddrStr));
			//SDL_Log("  Found interface:  name=[%s] desc=[%s] address=[%s] netmask=[%s] broadcastAddr=[%s]\n", name, desc ? desc : "unavailable", ifaAddrStr, maskAddrStr, dstAddrStr);

			NET_Address *address = NET_ResolveHostname(dstAddrStr);
			NET_WaitUntilResolved(address, -1);
			NET_SendDatagram(sock, address, port, buf, buflen);
			NET_UnrefAddress(address);
		}

		free(pAdapterInfo);
		free(ipTable);
	}
	return true;
#elif defined(HAVE_GETIFADDRS)
	// BSD-style implementation
	struct ifaddrs* ifap;
	if (getifaddrs(&ifap) == 0)
	{
		struct ifaddrs* p = ifap;
		while (p)
		{
			Uint32 ifaAddr = SockAddrToUint32(p->ifa_addr);
			Uint32 maskAddr = SockAddrToUint32(p->ifa_netmask);
			Uint32 dstAddr = SockAddrToUint32(p->ifa_dstaddr);
			if (ifaAddr > 0)
			{
				char ifaAddrStr[32];  Inet_NtoA(ifaAddr, ifaAddrStr, sizeof(ifaAddrStr));
				char maskAddrStr[32]; Inet_NtoA(maskAddr, maskAddrStr, sizeof(maskAddrStr));
				char dstAddrStr[32];  Inet_NtoA(dstAddr, dstAddrStr, sizeof(dstAddrStr));
				//SDL_Log("  Found interface:  name=[%s] desc=[%s] address=[%s] netmask=[%s] broadcastAddr=[%s]\n", p->ifa_name, "unavailable", ifaAddrStr, maskAddrStr, dstAddrStr);

				NET_Address* address = NET_ResolveHostname(dstAddrStr);
				NET_WaitUntilResolved(address, -1);
				NET_SendDatagram(sock, address, port, buf, buflen);
				NET_UnrefAddress(address);
			}
			p = p->ifa_next;
		}
		freeifaddrs(ifap);
	}
	return true;
#else
	NET_Address *address = NET_ResolveHostname("255.255.255.255");
	NET_WaitUntilResolved(address, -1);
	bool result = NET_SendDatagram(sock, address, port, buf, buflen);
	NET_UnrefAddress(address);
	return result;
#endif
}

void
LobbyDialogDelegate::GetGameList()
{
	// Get game info for local games
	m_packet.StartLobbyMessage(LOBBY_REQUEST_GAME_INFO);
	m_packet.Write((Uint32)SDL_GetTicks());

	NET_SendDatagramBroadcast(gSocket, NETPLAY_PORT, m_packet.data, m_packet.len);
}

void
LobbyDialogDelegate::GetGameInfo()
{
	m_packet.StartLobbyMessage(LOBBY_REQUEST_GAME_INFO);
	m_packet.Write((Uint32)SDL_GetTicks());

	IPaddress address = m_game.GetHost()->address;
	NET_SendDatagram(gSocket, address.host, address.port, m_packet.data, m_packet.len);
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

	IPaddress address = m_game.GetHost()->address;
	NET_SendDatagram(gSocket, address.host, address.port, m_packet.data, m_packet.len);
}

void
LobbyDialogDelegate::SendLeaveRequest()
{
	m_packet.StartLobbyMessage(LOBBY_REQUEST_LEAVE);
	m_packet.Write(m_game.gameID);
	m_packet.Write(m_game.localID);

	IPaddress address = m_game.GetHost()->address;
	NET_SendDatagram(gSocket, address.host, address.port, m_packet.data, m_packet.len);
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

	NET_SendDatagram(gSocket, node->address.host, node->address.port, m_packet.data, m_packet.len);

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
LobbyDialogDelegate::ProcessPacket(DynamicPacket &packet)
{
	Uint8 cmd;

	if (!packet.Read(cmd)) {
		return;
	}
	if (cmd != LOBBY_MSG) {
		if (cmd == NEW_GAME) {
			ProcessNewGame(packet);
		}
		return;
	}
	if (!packet.Read(cmd)) {
		return;
	}

	if (m_state == STATE_HOSTING) {
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

	NET_SendDatagram(gSocket, packet.address.host, packet.address.port, m_reply.data, m_reply.len);
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

	NET_SendDatagram(gSocket, packet.address.host, packet.address.port, m_reply.data, m_reply.len);

	if (m_game.HasNode(packet.address)) {
		m_playButton->OnClick();
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

	NET_SendDatagram(gSocket, packet.address.host, packet.address.port, m_reply.data, m_reply.len);
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
			IPaddress address = m_game.GetNode(i)->address;
			NET_SendDatagram(gSocket, address.host, address.port, m_reply.data, m_reply.len);
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
		unsigned int i;
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

		UpdateUI();

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
