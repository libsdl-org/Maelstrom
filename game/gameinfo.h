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
	CONTROL_REMOTE1   = 0x10,
	CONTROL_REMOTE2   = 0x20,
	CONTROL_NETWORK   = 0x40,
	CONTROL_REPLAY    = 0x80,
	CONTROL_LOCAL     = (CONTROL_KEYBOARD|CONTROL_JOYSTICK1|CONTROL_JOYSTICK2|CONTROL_JOYSTICK3)
};

enum GAME_MODE {
	GAME_MODE_KIDS           = 0x01,
	GAME_MODE_CONTROL_BRAKES = 0x02,
	GAME_MODE_DEATHMATCH     = 0x04,
	GAME_MODE_CHEAT          = 0x08,
};

#define IS_LOCAL_CONTROL(X)	(X & CONTROL_LOCAL)
#define IS_REMOTE_CONTROL(X) (X & (CONTROL_REMOTE1 | CONTROL_REMOTE2))

enum NODE_STATE_FLAG {
	STATE_NONE	= 0x00,
	STATE_ABORT	= 0x01,
	STATE_PAUSE	= 0x02,
	STATE_BONUS	= 0x04,
	STATE_DIALOG	= 0x10,
};

enum PAUSE_REASON {
	PAUSE_REQUEST		= 0x01,
	PAUSE_MINIMIZED		= 0x02,
	PAUSE_CONTROLLER	= 0x04,
	PAUSE_OVERLAY		= 0x08,
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

	void Reset() {
		nodeID = 0;
		address.Reset();
		state = 0;
		SDL_zero(ping);
	}
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
	Uint8 available;

	struct {
		UIElement *element;
		UIElement *join;
		UIElement *desc;
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

	void SetHost(Uint8 wave, Uint8 lives, Uint8 turbo, bool deathMatch, bool cheat);

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
	bool HasLocalControl() const;
	int GetNumPlayers() const;

	bool IsFull() const;

	bool IsDeathmatch() const {
		return (gameMode & GAME_MODE_DEATHMATCH) != 0;
	}
	bool IsCheat() const {
		return (gameMode & GAME_MODE_CHEAT) != 0;
	}
	bool IsKidMode() const {
		return (gameMode & GAME_MODE_KIDS) != 0;
	}
	bool ControlBrakes() const {
		return (gameMode & GAME_MODE_CONTROL_BRAKES) != 0;
	}

	void SetNodeState(int index, Uint8 state);
	Uint8 GetNodeState(int index) const;

	void TogglePauseRequest();
	void SetPauseReason(Uint8 reason, bool enabled);
	void SetLocalState(Uint8 state, bool enabled);
	void ToggleLocalState(Uint8 state);
	Uint8 GetLocalState() const {
		return GetNodeState(GetLocalIndex());
	}

	void BindPlayerToUI(int index, UIElement *element);
	void BindHostPlayerToUI(UIElement *element);
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

	Uint32 replayVersion;
	Uint32 spriteCRC;

	Uint32 localID;

	Uint8 paused;

protected:
	Uint8 numNodes;
	GameInfoNode nodes[MAX_NODES];
	GameInfoPlayer players[MAX_PLAYERS];
};

#endif // _gameinfo_h
