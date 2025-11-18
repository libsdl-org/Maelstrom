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

#ifndef _replay_h
#define _replay_h

#include "gameinfo.h"
#include "packet.h"

// You should increment this every time game code changes in ways that would
// affect the random number sequence for a game.
//
// Examples of this would be changing the game play area, game info structure,
// game logic, etc.
//
#define REPLAY_VERSION	1
#define REPLAY_DIRECTORY "Games"
#define REPLAY_FILETYPE "mreplay"
#define LAST_REPLAY	"LastGame."REPLAY_FILETYPE

enum REPLAY_MODE {
	REPLAY_IDLE,
	REPLAY_RECORDING,
	REPLAY_PLAYBACK
};

class Replay
{
public:
	Replay();

	// You should never change this while the game is running
	void SetMode(REPLAY_MODE mode);

	bool IsPlaying() const {
		return m_mode == REPLAY_PLAYBACK;
	}
	bool IsRecording() const {
		return m_mode == REPLAY_RECORDING;
	}

	int GetDisplayPlayer() const {
		return m_finalPlayer;
	}
	const char *GetDisplayName() const {
		return m_game.GetPlayer(GetDisplayPlayer())->name;
	}
	int GetFinalWave() const {
		return m_finalWave;
	}
	bool HasContinues() const {
		return m_finalContinues > 0;
	}
	void RecordContinue() {
		++m_finalContinues;
	}
	void ConsumeContinue() {
		--m_finalContinues;
	}
	int GetFinalScore() const {
		return m_finalScore[GetDisplayPlayer()].Score;
	}
	int GetFinalFrags() const {
		return m_finalScore[GetDisplayPlayer()].Frags;
	}
	float GetReplayTime() const {
		// Return the approximage length of the replay, in seconds
		return (float)m_frameCount / 30.0f;
	}
	GameInfo &GetGameInfo() {
		return m_game;
	}

	bool Load(const char *file, bool headerOnly = false);
	bool Save(const char *file = NULL);

	void HandleNewGame();
	bool HandlePlayback();
	void HandleRecording();
	void HandleGameOver();

protected:
	REPLAY_MODE m_mode;
	GameInfo m_game;
	Uint32 m_seed;
	Uint32 m_frameCount;
	DynamicPacket m_data;
	DynamicPacket m_pausedInput;

	Uint8 m_finalPlayer;
	Uint8 m_finalWave;
	Uint8 m_finalContinues;
	struct FinalScore {
		Uint32 Score;
		Uint8 Frags;
	} m_finalScore[MAX_PLAYERS];
};

#endif // _replay_h
