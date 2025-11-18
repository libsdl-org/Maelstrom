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

#ifndef _gameinfo_h
#define _gameinfo_h

#include "protocol.h"
#include "packet.h"
#include "controls.h"

class UIElement;
class UIElementCheckbox;
class UIElementRadioGroup;

enum PLAYER_CONTROL {
	CONTROL_NONE,
	CONTROL_KEYBOARD  = 0x01,
	CONTROL_JOYSTICK1 = 0x02,
	CONTROL_JOYSTICK2 = 0x04,
	CONTROL_JOYSTICK3 = 0x08,
	CONTROL_TOUCH     = 0x10,
	CONTROL_NETWORK   = 0x20,
	CONTROL_REPLAY    = 0x40,
#ifdef USE_TOUCHCONTROL
	CONTROL_LOCAL     = CONTROL_TOUCH
#else
	CONTROL_LOCAL     = (CONTROL_KEYBOARD|CONTROL_JOYSTICK1)
#endif
};

enum GAME_MODE {
	GAME_MODE_DEATHMATCH = 0x01,
	GAME_MODE_KIDS       = 0x02,
};

#define IS_LOCAL_CONTROL(X)	(X != CONTROL_NONE && X != CONTROL_NETWORK && X != CONTROL_REPLAY)

enum NODE_STATE_FLAG {
	STATE_NONE	= 0x00,
	STATE_ABORT	= 0x01,
	STATE_PAUSE	= 0x02,
	STATE_BONUS	= 0x04,
	STATE_MINIMIZE	= 0x08,
	STATE_DIALOG	= 0x10,
};

enum PING_STATUS {
	PING_LOCAL,
	PING_GOOD,
	PING_OKAY,
	PING_BAD,
	PING_TIMEDOUT,
	NUM_PING_STATES
};

// This represents a physical machine (host/port combo) on the network
struct GameInfoNode
{
	Uint32 nodeID;
	IPaddress address;
	Uint8 state;

	struct {
		Uint32 lastPing;
		Uint32 roundTripTime;
		PING_STATUS status;
	} ping;

};

// This represents a player in the game, on a particular network node
//
// The hosting node may have any number of players
// The other nodes may each only have one player, to simplify things
// like the join/leave/kick process.
//
struct GameInfoPlayer
{
	Uint32 nodeID;
	char name[MAX_NAMELEN+1];
	Uint8 controlMask;

	struct {
		UIElement *element;
		UIElement *name;
		UIElement *host;
		UIElement *control;
		UIElement *ping_states[NUM_PING_STATES];
	} UI;
};

class GameInfo
{
public:
	GameInfo();

	// Equality operator for array operations
	bool operator ==(const GameInfo &rhs) {
		return gameID == rhs.gameID;
	}

	void Reset();

	void SetLocalID(Uint32 uniqueID) {
		localID = uniqueID;
	}

	void SetHost(Uint8 wave, Uint8 lives, Uint8 turbo, Uint8 deathMatch, bool kidMode);

	void SetPlayerSlot(int slot, const char *name, Uint8 controlMask);
	void SetPlayerName(int slot, const char *name);
	bool AddNetworkPlayer(Uint32 nodeID, const IPaddress &address, const char *name);

	void CopyFrom(const GameInfo &rhs);

	bool ReadFromPacket(DynamicPacket &packet);
	void WriteToPacket(DynamicPacket &packet);

	void PrepareForReplay();

	int GetNumNodes() const {
		return numNodes;
	}
	const GameInfoNode *GetHost() const {
		return GetNode(HOST_NODE);
	}
	const GameInfoNode *GetNode(int index) const {
		return &nodes[index];
	}
	const GameInfoNode *GetNodeByID(Uint32 nodeID) const {
		for (int i = 0; i < GetNumNodes(); ++i) {
			if (nodeID == nodes[i].nodeID) {
				return &nodes[i];
			}
		}
		return NULL;
	}
	int GetNodeIndex(Uint32 nodeID) const {
		for (int i = 0; i < GetNumNodes(); ++i) {
			if (nodeID == nodes[i].nodeID) {
				return i;
			}
		}
		return -1;
	}
	int GetLocalIndex() const {
		return GetNodeIndex(localID);
	}

	bool HasNode(Uint32 nodeID) const;
	bool HasNode(const IPaddress &address) const;
	void RemoveNode(Uint32 nodeID);

	const GameInfoPlayer *GetPlayer(int index) const {
		return &players[index];
	}
	const IPaddress &GetPlayerAddress(int index) const {
		const GameInfoPlayer *player = GetPlayer(index);
		const GameInfoNode *node = GetNodeByID(player->nodeID);
		return node->address;
	}
	void RemovePlayer(int index);

	bool IsHosting() const;
	bool IsMultiplayer() const;
	bool IsLocalNode(int index) const;
	bool IsNetworkNode(int index) const;

	bool IsValidPlayer(int index) const;
	bool IsLocalPlayer(int index) const;
	bool IsNetworkPlayer(int index) const;
	int GetNumPlayers() const;

	bool IsFull() const;

	bool IsDeathmatch() const {
		return (gameMode & GAME_MODE_DEATHMATCH) != 0;
	}
	bool IsKidMode() const {
		return (gameMode & GAME_MODE_KIDS) != 0;
	}

	void SetNodeState(int index, Uint8 state);
	Uint8 GetNodeState(int index) const;

	void SetLocalState(Uint8 state, bool enabled);
	void ToggleLocalState(Uint8 state);
	Uint8 GetLocalState() const {
		return GetNodeState(GetLocalIndex());
	}

	void BindPlayerToUI(int index, UIElement *element);
	void UpdateUI();
	void UpdateUI(GameInfoPlayer *player);

	void InitializePing();
	void InitializePing(int index);
	void UpdatePingTime(int index, Uint32 timestamp);
	void UpdatePingStatus();
	void UpdatePingStatus(int index);
	PING_STATUS GetPingStatus(int index);

public:
	Uint32 gameID;
	Uint32 seed;
	Uint8 wave;
	Uint8 lives;
	Uint8 turbo;
	Uint8 gameMode;
	Uint8 deathMatch;

	Uint32 localID;

protected:
	Uint8 numNodes;
	GameInfoNode nodes[MAX_NODES];
	GameInfoPlayer players[MAX_PLAYERS];
};

#endif // _gameinfo_h
