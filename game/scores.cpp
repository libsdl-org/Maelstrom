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

/* 
   This file handles the cheat dialogs and the high score file
*/

#include <stdlib.h>	// for qsort()

#include "../physfs/physfs.h"

#include "Maelstrom_Globals.h"
#include "scores.h"
#include "../utils/array.h"


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

void LoadScores(void)
{
	char path[1024];
	Replay replay;
	Scores score;
	array<Scores> scores;
	int i;

	FreeScores();

	// Load all the games
	char **rc = PHYSFS_enumerateFiles(REPLAY_DIRECTORY);
	char **f;
	for (f = rc; *f; ++f) {
		if (SDL_strcmp(*f, LAST_REPLAY) == 0) {
			continue;
		}
		if (!replay.Load(*f, true)) {
			continue;
		}

		SDL_strlcpy(score.name, replay.GetDisplayName(), sizeof(score.name));
		score.wave = replay.GetFinalWave();
		score.score = replay.GetFinalScore();
		score.file = *f;
		scores.add(score);
	}

	// Take the top 10
	if (scores.length() > 0) {
		qsort(&scores[0], scores.length(), sizeof(scores[0]), SortScores);
	}
	for (i = 0; i < scores.length() && i < NUM_SCORES; ++i) {
		hScores[i] = scores[i];
		hScores[i].file = SDL_strdup(scores[i].file);
	}

	// Trim the rest
	for ( ; i < scores.length(); ++i) {
		SDL_snprintf(path, sizeof(path), "%s/%s", REPLAY_DIRECTORY, scores[i].file);
		PHYSFS_delete(path);
	}

	PHYSFS_freeList(rc);
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

void ZapHighScores()
{
	char path[1024];

	// Delete all the games
	char **rc = PHYSFS_enumerateFiles(REPLAY_DIRECTORY);
	char **f;
	for (f = rc; *f; ++f) {
		SDL_snprintf(path, sizeof(path), "%s/%s", REPLAY_DIRECTORY, *f);
		PHYSFS_delete(path);
	}
	PHYSFS_freeList(rc);

	FreeScores();
	gLastHigh = -1;

	/* Fade the screen and redisplay scores */
	screen->FadeOut();
	Delay(SOUND_DELAY);
	sound->PlaySound(gExplosionSound, 5);
	gUpdateBuffer = true;
}

