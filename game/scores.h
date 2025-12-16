/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

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

#ifndef _scores_h
#define _scores_h

// Functions from scores.cpp
extern void LoadScores(void);
extern void FreeScores(void);
extern void ZapHighScores();

/* The high scores structure */
typedef	struct {
	char name[20];
	Uint32 wave;
	Uint32 score;	
	char *file;
} Scores;

#define NUM_SCORES	10

#endif // _scores_h
