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
#include "../screenlib/UIElementCheckbox.h"
#include "../screenlib/UIElementRadio.h"

#include "gameinfo.h"


GameInfo::GameInfo()
{
	localID = rand();
	Reset();
}

void
GameInfo::Reset()
{
	gameID = 0;
	seed = 0;
	wave = 0;
	lives = 0;
	turbo = 0;
	gameMode = 0;
	deathMatch = 0;
	numNodes = 0;
	SDL_zero(nodes);
	SDL_zero(players);
}

void
GameInfo::SetHost(Uint8 wave, Uint8 lives, Uint8 turbo, Uint8 deathMatch, bool kidMode)
{
	Reset();

	this->gameID = localID;
	this->seed = GetRandSeed();
	this->wave = wave;
	this->lives = deathMatch ? deathMatch : lives;
	this->turbo = turbo;
	this->gameMode = 0;
	if (kidMode) {
		this->gameMode |= GAME_MODE_KIDS;
	}
	if (deathMatch) {
		this->gameMode |= GAME_MODE_DEATHMATCH;
	}
	this->deathMatch = deathMatch;

	// We are the host node
	assert(HOST_NODE == 0);
	nodes[HOST_NODE].nodeID = localID;
	numNodes = 1;
}

void
GameInfo::SetPlayerSlot(int slot, const char *name, Uint8 controlMask)
{
	GameInfoPlayer *player = &players[slot];

	SDL_strlcpy(player->name, name ? name : "", sizeof(player->name));

	if (IS_LOCAL_CONTROL(controlMask)) {
		player->nodeID = localID;
	} else {
		player->nodeID = 0;
	}
	player->controlMask = controlMask;

	UpdateUI(player);
}

void
GameInfo::SetPlayerName(int slot, const char *name)
{
	GameInfoPlayer *player = &players[slot];

	SDL_strlcpy(player->name, name ? name : "", sizeof(player->name));

	UpdateUI(player);
}

bool
GameInfo::AddNetworkPlayer(Uint32 nodeID, const IPaddress &address, const char *name)
{
	int slot;

	if (IsFull()) {
		return false;
	}

	slot = numNodes;
	GameInfoNode *node = &nodes[slot];
	node->nodeID = nodeID;
	node->address = address;
	InitializePing(slot);
	++numNodes;

	for (slot = 0; slot < MAX_PLAYERS; ++slot) {
		if (!players[slot].nodeID &&
		    players[slot].controlMask == CONTROL_NETWORK) {
			break;
		}
	}
	assert(slot < MAX_PLAYERS);

	GameInfoPlayer *player = &players[slot];
	player->nodeID = nodeID;
	SDL_strlcpy(player->name, name, sizeof(player->name));

	UpdateUI(player);

	return true;
}

void
GameInfo::CopyFrom(const GameInfo &rhs)
{
	int i, j;

	gameID = rhs.gameID;
	seed = rhs.seed;
	wave = rhs.wave;
	lives = rhs.lives;
	turbo = rhs.turbo;
	gameMode = rhs.gameMode;
	deathMatch = rhs.deathMatch;

	for (i = 0; i < MAX_NODES; ++i) {
		const GameInfoNode *node = rhs.GetNode(i);
		if (nodes[i].nodeID != node->nodeID ||
		    nodes[i].address != node->address) {
			// See if this node was just slid down
			for (j = i+1; j < MAX_NODES; ++j) {
				if (nodes[j].nodeID == node->nodeID &&
				    nodes[j].address == node->address) {
					nodes[i].ping = nodes[j].ping;
					break;
				}
			}
			if (j == MAX_NODES) {
				// Reset the ping info
				InitializePing(i);
			}
		}
		nodes[i].nodeID = node->nodeID;
		nodes[i].address = node->address;
	}
	numNodes = rhs.numNodes;

	for (i = 0; i < MAX_PLAYERS; ++i) {
		const GameInfoPlayer *player = rhs.GetPlayer(i);
		players[i].nodeID = player->nodeID;
		SDL_memcpy(players[i].name, player->name,
			sizeof(players[i].name));
		if (players[i].nodeID == localID) {
			if (players[i].controlMask == CONTROL_NONE) {
				players[i].controlMask = CONTROL_LOCAL;
			}
		} else if (players[i].nodeID != 0) {
			players[i].controlMask = CONTROL_NETWORK;
		} else {
			players[i].controlMask = CONTROL_NONE;
		}
	}

	UpdateUI();
}

bool
GameInfo::ReadFromPacket(DynamicPacket &packet)
{
	int i;

	if (!packet.Read(gameID)) {
		return false;
	}
	if (!packet.Read(seed)) {
		return false;
	}
	if (!packet.Read(wave)) {
		return false;
	}
	if (!packet.Read(lives)) {
		return false;
	}
	if (!packet.Read(turbo)) {
		return false;
	}
	if (!packet.Read(gameMode)) {
		return false;
	}
	if (!packet.Read(deathMatch)) {
		return false;
	}

	if (!packet.Read(numNodes)) {
		return false;
	}
	for (i = 0; i < MAX_NODES; ++i) {
		if (!packet.Read(nodes[i].nodeID)) {
			return false;
		}
		if (!packet.Read(nodes[i].address.host)) {
			return false;
		}
		if (!packet.Read(nodes[i].address.port)) {
			return false;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i) {
		if (!packet.Read(players[i].nodeID)) {
			return false;
		}
		if (!packet.Read(players[i].name, sizeof(players[i].name))) {
			return false;
		}
		if (players[i].nodeID) {
			players[i].controlMask = CONTROL_REPLAY;
		} else {
			players[i].controlMask = CONTROL_NONE;
		}
	}

	// We want to get the public address of the server
	// If we already have one, we assume that's the fastest interface
	if (!nodes[HOST_NODE].address.host) {
		nodes[HOST_NODE].address = packet.address;
	}

	return true;
}

void
GameInfo::WriteToPacket(DynamicPacket &packet)
{
	int i;

	packet.Write(gameID);
	packet.Write(seed);
	packet.Write(wave);
	packet.Write(lives);
	packet.Write(turbo);
	packet.Write(gameMode);
	packet.Write(deathMatch);

	packet.Write(numNodes);
	for (i = 0; i < MAX_NODES; ++i) {
		packet.Write(nodes[i].nodeID);
		packet.Write(nodes[i].address.host);
		packet.Write(nodes[i].address.port);
	}

	for (i = 0; i < MAX_PLAYERS; ++i) {
		packet.Write(players[i].nodeID);
		packet.Write(players[i].name);
	}
}

void
GameInfo::PrepareForReplay()
{
	// Clean up the game info for privacy and so replay works
	gameID = localID;

	SDL_zero(nodes);
	nodes[HOST_NODE].nodeID = localID;
	numNodes = 1;

	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (IsValidPlayer(i)) {
			players[i].nodeID = localID;
			players[i].controlMask = CONTROL_REPLAY;
		}
	}
}

bool
GameInfo::HasNode(Uint32 nodeID) const
{
	for (int i = 0; i < MAX_NODES; ++i) {
		if (nodes[i].nodeID == nodeID) {
			return true;
		}
	}
	return false;
}

bool
GameInfo::HasNode(const IPaddress &address) const
{
	for (int i = 0; i < MAX_NODES; ++i) {
		if (nodes[i].address == address) {
			return true;
		}
	}
	return false;
}

void
GameInfo::RemoveNode(Uint32 nodeID)
{
	int i;

	i = 0;
	while (i < GetNumNodes()) {
		if (nodeID == nodes[i].nodeID) {
			SDL_memcpy(&nodes[i], &nodes[i+1],
					(MAX_NODES-i-1)*sizeof(nodes[i]));
			SDL_zero(nodes[MAX_NODES-1]);
			--numNodes;
		} else {
			++i;
		}
	}

	for (i = 0; i < MAX_PLAYERS; ++i) {
		if (nodeID == players[i].nodeID) {
			RemovePlayer(i);
		}
	}
}

void
GameInfo::RemovePlayer(int index)
{
	GameInfoPlayer *player = &players[index];
	player->nodeID = 0;
	SDL_zero(player->name);
	player->controlMask = CONTROL_NETWORK;

	UpdateUI(player);
}

bool
GameInfo::IsHosting() const
{
	return localID == gameID;
}

bool
GameInfo::IsMultiplayer() const
{
	return (GetNumPlayers() > 1);
}

bool
GameInfo::IsLocalNode(int index) const
{
	if (index >= GetNumNodes()) {
		return false;
	}
	return (nodes[index].nodeID == localID);
}

bool
GameInfo::IsNetworkNode(int index) const
{
	if (index >= GetNumNodes()) {
		return false;
	}
	return (nodes[index].nodeID != localID);
}

bool
GameInfo::IsValidPlayer(int index) const
{
	if (!players[index].nodeID) {
		return false;
	}
	return true;
}

bool
GameInfo::IsLocalPlayer(int index) const
{
	if (!players[index].nodeID) {
		return false;
	}
	return (players[index].nodeID == localID);
}

bool
GameInfo::IsNetworkPlayer(int index) const
{
	if (!players[index].nodeID) {
		return false;
	}
	return (players[index].nodeID != localID);
}

int
GameInfo::GetNumPlayers() const
{
	int numPlayers = 0;
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (players[i].nodeID) {
			++numPlayers;
		}
	}
	return numPlayers;
}

bool
GameInfo::IsFull() const
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (!players[i].nodeID &&
		    players[i].controlMask == CONTROL_NETWORK) {
			return false;
		}
	}
	return true;
}

void
GameInfo::SetLocalState(Uint8 state, bool enabled)
{
	if (enabled) {
		nodes[GetLocalIndex()].state |= state;
	} else {
		nodes[GetLocalIndex()].state &= ~state;
	}
}

void
GameInfo::ToggleLocalState(Uint8 state)
{
	if (GetLocalState() & state) {
		SetLocalState(state, false);
	} else {
		SetLocalState(state, true);
	}
}

void
GameInfo::SetNodeState(int index, Uint8 state)
{
	nodes[index].state = state;
}

Uint8
GameInfo::GetNodeState(int index) const
{
	return nodes[index].state;
}

void
GameInfo::BindPlayerToUI(int index, UIElement *element)
{
	char name[32];
	GameInfoPlayer *player = &players[index];

	if (player->UI.element == element) {
		return;
	}

	player->UI.element = element;
	player->UI.name = element->GetElement<UIElement>("name");
	player->UI.host = element->GetElement<UIElement>("host");
	player->UI.control = element->GetElement<UIElement>("control");

	for (int i = 0; i < NUM_PING_STATES; ++i) {
		SDL_snprintf(name, sizeof(name), "ping%d", i);
		player->UI.ping_states[i] = element->GetElement<UIElement>(name);
	}

	UpdateUI(player);
}

void
GameInfo::UpdateUI()
{
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		UpdateUI(&players[i]);
	}
}

void
GameInfo::UpdateUI(GameInfoPlayer *player)
{
	if (!player->UI.element) {
		return;
	}

	if (player->UI.name) {
		if (player->name[0]) {
			player->UI.name->Show();
			player->UI.name->SetText(player->name);
		} else {
			player->UI.name->Hide();
		}
	}
	if (player->UI.host) {
		const GameInfoNode *node = GetNodeByID(player->nodeID);
		if (!node) {
			player->UI.host->Hide();
		} else if (node->nodeID == localID) {
			//player->UI.host->Show();
			//player->UI.host->SetText("localhost");
			player->UI.host->Hide();
		} else {
			player->UI.host->Show();
			player->UI.host->SetText(SDLNet_ResolveIP(&node->address));
		}
	}
	if (player->UI.control) {
		char name[128];
		SDL_snprintf(name, sizeof(name), "Images/control%d.bmp", player->controlMask);
		player->UI.control->SetImage(name);
	}
	for (int i = 0; i < NUM_PING_STATES; ++i) {
		UIElement *element = player->UI.ping_states[i];
		if (element) {
			const GameInfoNode *node = GetNodeByID(player->nodeID);
			if (node && node->ping.status == i) {
				element->Show();
			} else {
				element->Hide();
			}
		}
	}
}

void
GameInfo::InitializePing()
{
	for (int i = 0; i < MAX_NODES; ++i) {
		InitializePing(i);
	}
}

void
GameInfo::InitializePing(int index)
{
	GameInfoNode *node = &nodes[index];

	if (node->nodeID != localID) {
		node->ping.lastPing = SDL_GetTicks();
		node->ping.roundTripTime = 0;
		node->ping.status = PING_GOOD;
	}
}

void
GameInfo::UpdatePingTime(int index, Uint32 timestamp)
{
	Uint32 now;
	Uint32 elapsed;
	GameInfoNode *node;

	now = SDL_GetTicks();
	elapsed = (now - timestamp);

	node = &nodes[index];
	node->ping.lastPing = now;
	if (!node->ping.roundTripTime) {
		node->ping.roundTripTime = elapsed;
	} else {
		// Use a weighted average 2/3 previous value, 1/3 new value
		node->ping.roundTripTime = (2*node->ping.roundTripTime + 1*elapsed) / 3;
	}
}

void
GameInfo::UpdatePingStatus()
{
	for (int i = 0; i < GetNumNodes(); ++i) {
		UpdatePingStatus(i);
	}
}

void
GameInfo::UpdatePingStatus(int index)
{
	GameInfoNode *node = &nodes[index];

	if (!IsNetworkNode(index)) {
		node->ping.status = PING_LOCAL;
	} else {
		Uint32 sinceLastPing;

		sinceLastPing = int(SDL_GetTicks() - node->ping.lastPing);
		if (sinceLastPing < 2*PING_INTERVAL) {
			if (node->ping.roundTripTime <= 2*FRAME_DELAY_MS) {
#ifdef DEBUG_NETWORK
printf("Game 0x%8.8x: node 0x%8.8x round trip time %d (GOOD)\n",
	gameID, node->nodeID, node->ping.roundTripTime);
#endif
				node->ping.status = PING_GOOD;
			} else if (node->ping.roundTripTime <= 3*FRAME_DELAY_MS) {
#ifdef DEBUG_NETWORK
printf("Game 0x%8.8x: node 0x%8.8x round trip time %d (OKAY)\n",
	gameID, node->nodeID, node->ping.roundTripTime);
#endif
				node->ping.status = PING_OKAY;
			} else {
#ifdef DEBUG_NETWORK
printf("Game 0x%8.8x: node 0x%8.8x round trip time %d (BAD)\n",
	gameID, node->nodeID, node->ping.roundTripTime);
#endif
				node->ping.status = PING_BAD;
			}
		} else if (sinceLastPing < PING_TIMEOUT) {
#ifdef DEBUG_NETWORK
printf("Game 0x%8.8x: node 0x%8.8x since last ping %d (BAD)\n",
	gameID, node->nodeID, sinceLastPing);
#endif
			node->ping.status = PING_BAD;
		} else {
#ifdef DEBUG_NETWORK
printf("Game 0x%8.8x: node 0x%8.8x since last ping %d (TIMEDOUT)\n",
	gameID, node->nodeID, sinceLastPing);
#endif
			node->ping.status = PING_TIMEDOUT;
		}
	}

	// Update the UI for matching players
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (players[i].nodeID == node->nodeID) {
			UpdateUI(&players[i]);
		}
	}
}

PING_STATUS
GameInfo::GetPingStatus(int index)
{
	if (IsNetworkNode(index)) {
		return nodes[index].ping.status;
	} else {
		return PING_LOCAL;
	}
}
