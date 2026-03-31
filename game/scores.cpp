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

/* 
   This file handles the cheat dialogs and the high score file
*/

#include "Maelstrom_Globals.h"
#include "scores.h"
#include "../utils/array.h"
#include "../utils/files.h"


Scores hScores[NUM_SCORES];

static int SortScores(const void *_a, const void *_b)
{
	const Scores *a = (const Scores *)_a;
	const Scores *b = (const Scores *)_b;

	if (a->score == b->score) {
		if (a->wave == b->wave) {
			return SDL_strcmp(a->name, b->name);
		}
		return b->wave - a->wave;
	}
	return b->score - a->score;
}

static SDL_EnumerationResult SDLCALL LoadScoresCallback(void *userdata, const char *dirname, const char *fname)
{
	array<Scores> *scores = reinterpret_cast<array<Scores> *>(userdata);
	Replay replay;
	Scores score;

	if (SDL_strcmp(fname, LAST_REPLAY) == 0) {
		return SDL_ENUM_CONTINUE;
	}
	if (replay.Load(fname, true)) {
		SDL_strlcpy(score.name, replay.GetDisplayName(), sizeof(score.name));
		score.wave = replay.GetFinalWave();
		score.score = replay.GetFinalScore();
	} else {
		SDL_zero(score);
	}
	score.file = SDL_strdup(fname);
	scores->add(score);

	return SDL_ENUM_CONTINUE;
}

void LoadScores(void)
{
	char path[1024];
	array<Scores> scores;
	unsigned int i;

	FreeScores();

	// Load all the games
	SDL_Storage *storage = OpenUserStorage();
	if (!storage) {
		return;
	}
	SDL_EnumerateStorageDirectory(storage, REPLAY_DIRECTORY, LoadScoresCallback, &scores);

	// Take the top 10
	if (scores.length() > 0) {
		SDL_qsort(&scores[0], scores.length(), sizeof(scores[0]), SortScores);
	}
	for (i = 0; i < scores.length() && i < NUM_SCORES; ++i) {
		if (!scores[i].score) {
			break;
		}
		hScores[i] = scores[i];
	}

	// Trim the rest
	for ( ; i < scores.length(); ++i) {
		SDL_snprintf(path, sizeof(path), "%s/%s", REPLAY_DIRECTORY, scores[i].file);
		SDL_RemoveStoragePath(storage, path);
		SDL_free(scores[i].file);
	}

	SDL_CloseStorage(storage);
}

void FreeScores(void)
{
	// Free the existing scores
	int i;
	for (i = 0; i < NUM_SCORES; ++i) {
		if (hScores[i].file) {
			SDL_free(hScores[i].file);
		}
	}
	SDL_zero(hScores);
}

static SDL_EnumerationResult SDLCALL DeleteScoresCallback(void *userdata, const char *dirname, const char *fname)
{
	SDL_Storage *storage = (SDL_Storage *)userdata;

	char path[256];
	SDL_snprintf(path, sizeof(path), "%s%s", dirname, fname);
	SDL_RemoveStoragePath(storage, path);
	return SDL_ENUM_CONTINUE;
}

void ZapHighScores()
{
	// Delete all the games
	SDL_Storage *storage = OpenUserStorage();
	if (!storage) {
		return;
	}
	SDL_EnumerateStorageDirectory(storage, REPLAY_DIRECTORY, DeleteScoresCallback, storage);
	SDL_CloseStorage(storage);

	FreeScores();
	gLastHigh = -1;
}

