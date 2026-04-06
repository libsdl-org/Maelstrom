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

#include "Maelstrom_Globals.h"
#include "netplay.h"
#include "player.h"
#include "replay.h"

#include "../miniz/miniz.h"
#include "../utils/files.h"

// Define this to get extremely verbose debug printing
//#define DEBUG_REPLAY

#define DELTA_SIZEMASK	0x7F
#define DELTA_SEED	0x80


Replay::Replay()
{
	m_mode = REPLAY_IDLE;
}

void
Replay::SetMode(REPLAY_MODE mode)
{
	m_mode = mode;
}

/* The file format is as follows:

	Uint8 version
	Uint32 frameCount
	Uint8 finalPlayer
	Uint8 finalWave
	{
		Uint32 score;
		Uint8  frags;
	} finalScore[MAX_PLAYERS]

	Uint32 gameSize
	GameInfo game

	Uint32 dataSize
	Uint32 compressed_size
	Uint8 compressed_data[]
*/

bool
Replay::Load(const char *file, bool headerOnly)
{
	char path[1024];
	SDL_IOStream *fp = nullptr;
	DynamicPacket data;
	uLongf destLen;
	Uint32 size;
	Uint32 compressedSize;
	bool result = false;

	// Open the file
	if (SDL_strchr(file, '/') == NULL) {
		SDL_snprintf(path, sizeof(path), "%s/%s", REPLAY_DIRECTORY, file);
		fp = OpenUserFile(path);
	} else {
		fp = SDL_IOFromFile(file, "rb");
	}
	if (!fp) {
		if (SDL_strcmp(file, LAST_REPLAY) != 0) {
			SDL_Log("Couldn't open %s: %s", file, SDL_GetError());
		}
		goto done;
	}

	Uint8 version;
	if (!SDL_ReadIO(fp, &version, 1)) {
		SDL_Log("Couldn't read data: %s", SDL_GetError());
		goto done;
	}
	if (version != HEADER_VERSION) {
		SDL_Log("Unsupported version %d, expected %d", version, HEADER_VERSION);
		goto done;
	}
	SDL_ReadU32LE(fp, &m_frameCount);
	SDL_ReadU8(fp, &m_finalPlayer);
	SDL_ReadU8(fp, &m_finalWave);
	SDL_ReadU8(fp, &m_finalContinues);
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		SDL_ReadU32LE(fp, &m_finalScore[i].Score);
		SDL_ReadU8(fp, &m_finalScore[i].Frags);
	}

	SDL_ReadU32LE(fp, &size);
	data.Reset();
	data.Grow(size);
	if (!SDL_ReadIO(fp, data.data, size)) {
		SDL_Log("Couldn't read data: %s", SDL_GetError());
		goto done;
	}
	data.len = size;
	if (!m_game.ReadFromPacket(data)) {
		SDL_Log("Couldn't read game information: %s", SDL_GetError());
		goto done;
	}

	if (!headerOnly) {
		if (m_game.replayVersion != REPLAY_VERSION) {
			SDL_Log("Unsupported data version %d, expected %d", version, REPLAY_VERSION);
			goto done;
		}

		if (m_game.spriteCRC != gSpriteCRC) {
			SDL_Log("Game uses a different sprite pack, ignoring");
			goto done;
		}

		SDL_ReadU32LE(fp, &size);
		m_data.Reset();
		m_data.Grow(size);

		SDL_ReadU32LE(fp, &compressedSize);
		data.Reset();
		data.Grow(compressedSize);

		if (!SDL_ReadIO(fp, data.data, compressedSize)) {
			SDL_Log("Couldn't read data: %s", SDL_GetError());
			goto done;
		}
		destLen = size;
		if (uncompress(m_data.Data(), &destLen, data.Data(), compressedSize) != Z_OK) {
			SDL_Log("Error uncompressing replay data");
			goto done;
		}
		m_data.len = size;
	}

	result = true;

done:
	SDL_CloseIO(fp);
	return result;
}

bool
Replay::Save(const char *file)
{
	char path[1024];
	SDL_IOStream *fp = nullptr;
	DynamicPacket data;
	uLongf destLen;
	bool result = false;

	fp = SDL_IOFromDynamicMem();
	if (!fp) {
		SDL_Log("Couldn't create dynamic memory: %s", SDL_GetError());
		goto done;
	}

	SDL_WriteU8(fp, HEADER_VERSION);
	SDL_WriteU32LE(fp, m_frameCount);
	SDL_WriteU8(fp, m_finalPlayer);
	SDL_WriteU8(fp, m_finalWave);
	SDL_WriteU8(fp, m_finalContinues);
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		SDL_WriteU32LE(fp, m_finalScore[i].Score);
		SDL_WriteU8(fp, m_finalScore[i].Frags);
	}

	data.Reset();
	m_game.WriteToPacket(data);
	SDL_WriteU32LE(fp, data.Size());
	if (!SDL_WriteIO(fp, data.Data(), data.Size())) {
		SDL_Log("Error writing to %s: %s", file, SDL_GetError());
		goto done;
	}

	destLen = compressBound(m_data.Size());
	data.Reset();
	data.Grow(destLen);
	if (compress(data.Data(), &destLen, m_data.Data(), m_data.Size()) != Z_OK) {
		SDL_Log("Error compressing replay data");
		goto done;
	}
	data.len = (int)destLen;
	SDL_WriteU32LE(fp, m_data.Size());
	SDL_WriteU32LE(fp, data.Size());
	if (!SDL_WriteIO(fp, data.Data(), data.Size())) {
		SDL_Log("Error writing to %s: %s", file, SDL_GetError());
		goto done;
	}

	// Create the file
	if (!file) {
		// If we aren't given a file, construct one from the game data
		SDL_snprintf(path, sizeof(path), "%s/%s_%d_%d_%8.8x.%s", REPLAY_DIRECTORY, GetDisplayName(), GetFinalScore(), GetFinalWave(), m_seed, REPLAY_FILETYPE);
		result = SaveUserFile(path, fp);
	} else if (SDL_strchr(file, '/') == NULL) {
		SDL_snprintf(path, sizeof(path), "%s/%s", REPLAY_DIRECTORY, file);
		result = SaveUserFile(path, fp);
	} else {
		void *file_data = SDL_GetPointerProperty(SDL_GetIOProperties(fp), SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER, NULL);
		Uint64 file_length = SDL_GetIOSize(fp);

		result = SDL_SaveFile(file, file_data, (size_t)file_length);
		if (!result) {
			SDL_Log("Couldn't save data: %s", SDL_GetError());
		}
		SDL_CloseIO(fp);
	}
	// The stream was closed above
	fp = NULL;

done:
	SDL_CloseIO(fp);
	return result;
}

void
Replay::HandleNewGame()
{
	if (m_mode == REPLAY_IDLE) {
		m_mode = REPLAY_RECORDING;
	}

	if (m_mode == REPLAY_RECORDING) {
		m_game.CopyFrom(gGameInfo);
		m_game.PrepareForReplay();
		m_data.Reset();
		m_pausedInput.Reset();

		// We'll be recording new info here.
		m_frameCount = 0;
		m_finalPlayer = 0;
		m_finalWave = 0;
		m_finalContinues = 0;
		SDL_zero(m_finalScore);

	} else if (m_mode == REPLAY_PLAYBACK) {
		gGameInfo.CopyFrom(m_game);
		gGameInfo.PrepareForReplay();
		m_data.Seek(0);
	}
	m_seed = m_game.seed;
}

bool
Replay::HandlePlayback()
{
	if (m_mode != REPLAY_PLAYBACK) {
		return true;
	}

	if (gPaused) {
		return true;
	}

	Uint8 delta;
	if (!m_data.Read(delta)) {
		// That's it, end of recording
#ifdef DEBUG_REPLAY
printf("Replay complete!\n");
#endif
		return false;
	}

	// Check to make sure we haven't gotten a consistency error
	if (delta & DELTA_SEED) {
		if (!m_data.Read(m_seed)) {
			error("Error in replay, missing data\r\n");
			return false;
		}
	}
	if (m_seed != GetRandSeed()) {
		error("Error!! \a consistency problem expecting seed %8.8x, got seed %8.8x, aborting!!\r\n", m_seed, GetRandSeed());
		return false;
	}

	// Add the input for this frame
	int size = (delta & DELTA_SIZEMASK);
	Uint8 value;
	if (m_data.Size() < size) {
		error("Error in replay, missing data\r\n");
		return false;
	}
	for (int i = 0; i < size; ++i) {
		if (m_data.Read(value)) {
			QueueInput(value);
		}
	}

	++m_frameCount;

#ifdef DEBUG_REPLAY
printf("Read %d bytes for frame %d, pos = %d, seed = %8.8x\n", size, m_frameCount, m_data.Tell(), m_seed);
#endif
	return true;
}

void
Replay::HandleRecording()
{
	if (m_mode != REPLAY_RECORDING) {
		return;
	}

	// Get the input for this frame
	Uint8 *data;
	int size = GetSyncBuf(&data);

	// If we're paused, save this data for the next unpaused frame
	if (gPaused) {
		m_pausedInput.Write(data, size);
		return;
	}
	assert(size+m_pausedInput.Size() < DELTA_SIZEMASK);

	Uint8 delta;
	delta = (Uint8)size+m_pausedInput.Size();

	// Add it to our data buffer
	Uint32 seed = GetRandSeed();
	if (seed != m_seed) {
		delta |= DELTA_SEED;
		m_data.Write(delta);
		m_data.Write(seed);
		m_seed = seed;
	} else {
		m_data.Write(delta);
	}

	if (m_pausedInput.Size() > 0) {
		m_pausedInput.Seek(0);
		m_data.Write(m_pausedInput);
		m_pausedInput.Reset();
	}
	m_data.Write(data, size);

	++m_frameCount;

#ifdef DEBUG_REPLAY
printf("Wrote %d bytes for frame %d, size = %d, seed = %8.8x\n", size, m_frameCount, m_data.Size(), m_seed);
#endif
}

void
Replay::HandleGameOver()
{
	if (m_mode != REPLAY_RECORDING) {
		return;
	}

	m_finalPlayer = gDisplayed;
	m_finalWave = gWave;

	for (int i = 0; i < MAX_PLAYERS; ++i) {
		m_finalScore[i].Score = gPlayers[i]->GetScore();
		m_finalScore[i].Frags = gPlayers[i]->GetFrags();
	}
}
