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

#ifndef _replay_h
#define _replay_h

#include "gameinfo.h"
#include "packet.h"

// You should increment this every time the replay header or game info changes
#define HEADER_VERSION	2

// You should increment this every time game code changes in ways that would
// affect the random number sequence for a game.
//
// Examples of this would be changing the game play area, game logic, etc.
//
#define REPLAY_VERSION	1

#define REPLAY_DIRECTORY "Games"
#define REPLAY_FILETYPE "mreplay"
#define LAST_REPLAY	"LastGame." REPLAY_FILETYPE

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
