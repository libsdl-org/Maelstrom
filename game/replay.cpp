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

#include <zlib.h>
#include "../physfs/physfs.h"
#include "../utils/files.h"

#include "Maelstrom_Globals.h"
#include "netplay.h"
#include "player.h"
#include "replay.h"

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

static SDL_RWops *
CopyReplayIntoSandbox(const char *file)
{
	FILE *rfp;
	PHYSFS_File *wfp;
	const char *base;
	char path[1024];
	char data[1024];
	size_t size;

	base = SDL_max(SDL_strrchr(file, '/'), SDL_strrchr(file, '\\'));
	if (base) {
		base = base+1;
	} else {
		base = file;
	}

	PHYSFS_mkdir("tmp");
	SDL_snprintf(path, sizeof(path), "tmp/%s", base);

	rfp = fopen(file, "rb");
	if (!rfp) {
		fprintf(stderr, "Couldn't open %s\n", file);
		return NULL;
	}

	wfp = PHYSFS_openWrite(path);
	if (!wfp) {
		fprintf(stderr, "Couldn't write to %s: %s\n", path, PHYSFS_getLastError());
		fclose(rfp);
		return NULL;
	}

	while ((size = fread(data, 1, sizeof(data), rfp)) > 0) {
		if (!PHYSFS_writeBytes(wfp, data, size)) {
			goto write_error;
		}
	}
	if (!PHYSFS_close(wfp)) {
		goto write_error;
	}
	fclose(rfp);

	return OpenRead(path);

write_error:
	fprintf(stderr, "Error writing to %s: %s\n", path, PHYSFS_getLastError());
	fclose(rfp);
	PHYSFS_close(wfp);
	PHYSFS_delete(path);
	return NULL;
}

bool
Replay::Load(const char *file, bool headerOnly)
{
	char path[1024];
	SDL_RWops *fp;
	DynamicPacket data;
	uLongf destLen;
	Uint32 size;
	Uint32 compressedSize;

	// Open the file
	if (SDL_strchr(file, '/') == NULL) {
		SDL_snprintf(path, sizeof(path), "%s/%s", REPLAY_DIRECTORY, file);
		file = path;
	}
	fp = OpenRead(file);
	if (!fp) {
		// If the file is outside our sandbox, try to copy it in
		fp = CopyReplayIntoSandbox(file);
	}
	if (!fp) {
		fprintf(stderr, "Couldn't read %s: %s\n", file, SDL_GetError());
		return false;
	}

	Uint8 version;
	if (!SDL_RWread(fp, &version, 1, 1)) {
		goto read_error;
	}
	if (version != REPLAY_VERSION) {
		fprintf(stderr, "Unsupported version %d, expected %d\n", version, REPLAY_VERSION);
		goto error_return;
	}
	m_frameCount = SDL_ReadLE32(fp);
	m_finalPlayer = SDL_ReadU8(fp);
	m_finalWave = SDL_ReadU8(fp);
	m_finalContinues = SDL_ReadU8(fp);
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		m_finalScore[i].Score = SDL_ReadLE32(fp);
		m_finalScore[i].Frags = SDL_ReadU8(fp);
	}

	size = SDL_ReadLE32(fp);
	data.Reset();
	data.Grow(size);
	if (!SDL_RWread(fp, data.data, size, 1)) {
		goto read_error;
	}
	data.len = size;
	if (!m_game.ReadFromPacket(data)) {
		fprintf(stderr, "Couldn't read game information from %s\n", file);
		goto error_return;
	}

	if (!headerOnly) {
		size = SDL_ReadLE32(fp);
		m_data.Reset();
		m_data.Grow(size);

		compressedSize = SDL_ReadLE32(fp);
		data.Reset();
		data.Grow(compressedSize);

		if (!SDL_RWread(fp, data.data, compressedSize, 1)) {
			goto read_error;
		}
		destLen = size;
		if (uncompress(m_data.Data(), &destLen, data.Data(), compressedSize) != Z_OK) {
			fprintf(stderr, "Error uncompressing replay data\n");
			goto error_return;
		}
		m_data.len = size;
	}

	// We're done!
	SDL_RWclose(fp);
	return true;

read_error:
	fprintf(stderr, "Error reading from %s: %s\n", file, SDL_GetError());
error_return:
	SDL_RWclose(fp);
	return false;
}

bool
Replay::Save(const char *file)
{
	char path[1024];
	SDL_RWops *fp;
	DynamicPacket data;
	uLongf destLen;

	// Create the directory if needed
	PHYSFS_mkdir(REPLAY_DIRECTORY);

	// If we aren't given a file, construct one from the game data
	if (!file) {
		SDL_snprintf(path, sizeof(path), "%s/%s_%d_%d_%8.8x.%s", REPLAY_DIRECTORY, GetDisplayName(), GetFinalScore(), GetFinalWave(), m_seed, REPLAY_FILETYPE);
		file = path;
	} else if (SDL_strchr(file, '/') == NULL) {
		SDL_snprintf(path, sizeof(path), "%s/%s", REPLAY_DIRECTORY, file);
		file = path;
	}
	fp = OpenWrite(file);
	if (!fp) {
		fprintf(stderr, "Couldn't write to %s: %s\n", file, PHYSFS_getLastError());
		return false;
	}

	Uint8 version = REPLAY_VERSION;
	SDL_WriteU8(fp, version);
	SDL_WriteLE32(fp, m_frameCount);
	SDL_WriteU8(fp, m_finalPlayer);
	SDL_WriteU8(fp, m_finalWave);
	SDL_WriteU8(fp, m_finalContinues);
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		SDL_WriteLE32(fp, m_finalScore[i].Score);
		SDL_WriteU8(fp, m_finalScore[i].Frags);
	}

	data.Reset();
	m_game.WriteToPacket(data);
	SDL_WriteLE32(fp, data.Size());
	if (!SDL_RWwrite(fp, data.Data(), data.Size(), 1)) {
		goto write_error;
	}

	destLen = compressBound(m_data.Size());
	data.Reset();
	data.Grow(destLen);
	if (compress(data.Data(), &destLen, m_data.Data(), m_data.Size()) != Z_OK) {
		fprintf(stderr, "Error compressing replay data\n");
		goto error_return;
	}
	data.len = destLen;
	SDL_WriteLE32(fp, m_data.Size());
	SDL_WriteLE32(fp, data.Size());
	if (!SDL_RWwrite(fp, data.Data(), data.Size(), 1)) {
		goto write_error;
	}

	// We're done!
	if (SDL_RWclose(fp) < 0) {
		goto write_error;
	}
	return true;

write_error:
	fprintf(stderr, "Error writing to %s: %s\n", file, SDL_GetError());
error_return:
	SDL_RWclose(fp);
	PHYSFS_delete(path);
	return false;
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
		m_data.Read(value);
		QueueInput(value);
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
