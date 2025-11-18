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
#include "player.h"
#include "gameover.h"

/* ----------------------------------------------------------------- */
/* -- Do the game over display */

struct FinalScore {
	int Player;
	int Score;
	int Frags;
};

static int cmp_byscore(const void *pA, const void *pB)
{
	const FinalScore *A = static_cast<const FinalScore*>(pA);
	const FinalScore *B = static_cast<const FinalScore*>(pB);
	if (A->Score == B->Score) {
		// Sort lowest player first
		return A->Player - B->Player;
	}
	return B->Score - A->Score;
}

static int cmp_byfrags(const void *pA, const void *pB)
{
	const FinalScore *A = static_cast<const FinalScore*>(pA);
	const FinalScore *B = static_cast<const FinalScore*>(pB);
	if (A->Frags == B->Frags) {
		return cmp_byscore(A, B);
	}
	return B->Frags - A->Frags;
}

void GameOverPanelDelegate::OnShow()
{
	UIElement *image;
	UIElement *label;
	int i;

	gReplay.HandleGameOver();

	/* Get the final scoring */
	struct FinalScore *final = new struct FinalScore[MAX_PLAYERS];
	for ( i=0; i<MAX_PLAYERS; ++i ) {
		final[i].Player = i+1;
		final[i].Score = gPlayers[i]->GetScore();
		final[i].Frags = gPlayers[i]->GetFrags();
	}
	if ( gGameInfo.IsDeathmatch() )
		qsort(final,MAX_PLAYERS,sizeof(struct FinalScore),cmp_byfrags);
	else
		qsort(final,MAX_PLAYERS,sizeof(struct FinalScore),cmp_byscore);

	m_panel->HideAll();

	image = m_panel->GetElement<UIElement>("image");
	if (image) {
		image->Show();
	}

	/* Show the player ranking */
	if ( gGameInfo.IsMultiplayer() ) {
		int nextLabel = 1;
		for (i = 0; i < MAX_PLAYERS; ++i) {
			if (!gPlayers[final[i].Player-1]->IsValid()) {
				continue;
			}

			char name[32];
			char buffer[BUFSIZ], num1[12], num2[12];

			SDL_snprintf(name, sizeof(name), "rank%d", nextLabel);
			image = m_panel->GetElement<UIElement>(name);
			if (image) {
				SDL_snprintf(name, sizeof(name), "Images/player%d.bmp", final[i].Player);
				image->SetImage(name);
				image->Show();
			}

			SDL_snprintf(name, sizeof(name), "rank%d_label", nextLabel++);
			label = m_panel->GetElement<UIElement>(name);
			if (!label) {
				continue;
			}
			if (gGameInfo.IsDeathmatch()) {
				SDL_snprintf(num1, sizeof(num1), "%7d", final[i].Score);
				SDL_snprintf(num2, sizeof(num2), "%3d", final[i].Frags);
				SDL_snprintf(buffer, sizeof(buffer), "%s Points, %s Frags", num1, num2);
			} else {
				SDL_snprintf(num1, sizeof(num1), "%7d", final[i].Score);
				SDL_snprintf(buffer, sizeof(buffer), "%s Points", num1);
			}
			label->SetText(buffer);
			label->Show();
		}
	}
	delete[] final;

	/* -- See if they got a high score */
	m_handleLabel = NULL;
	if (gReplay.IsRecording() && !gReplay.HasContinues() &&
	    !gGameInfo.IsMultiplayer() && !gGameInfo.IsKidMode() &&
	    (gGameInfo.wave == 1) && (gGameInfo.lives == 3) &&
	    TheShip->GetScore() > 0) {
		for ( i = 0; i<NUM_SCORES; ++i ) {
			if ( TheShip->GetScore() >= (int)hScores[i].score ) {
				gLastHigh = i;
				BeginEnterName();
				break;
			}
		}
	} else {
		gLastHigh = -1;
	}

	m_showTime = SDL_GetTicks();
}

void GameOverPanelDelegate::OnHide()
{
	if (gReplay.IsRecording()) {
		// Save this as the last game
		gReplay.Save(LAST_REPLAY);
	}
	gReplay.SetMode(REPLAY_IDLE);

	/* Make sure we clear the game info so we don't crash trying to
	   update UI in a future replay
	*/
	gGameInfo.Reset();

	while ( sound->Playing() )
		Delay(SOUND_DELAY);
	HandleEvents(0);
}

void GameOverPanelDelegate::OnTick()
{
	if (m_handleLabel) {
		return;
	}

	/* -- Wait for the game over sound */
	if ( sound->Playing() )
		return;

	if (gGameInfo.IsMultiplayer()) { /* Let them watch their ranking */
		const Uint32 MULTIPLAYER_SHOW_TIME = 3000;
		if ((SDL_GetTicks() - m_showTime) < MULTIPLAYER_SHOW_TIME) {
			return;
		}
	}

	// We're done showing the scores
	ui->ShowPanel(PANEL_MAIN);
}

bool GameOverPanelDelegate::HandleEvent(const SDL_Event &event)
{
	char key;

	if (!m_handleLabel) {
		return false;
	}

	switch (event.type) {
	case SDL_KEYDOWN:
		return true;
	case SDL_KEYUP:
		switch (event.key.keysym.sym) {
			case SDLK_RETURN:
				FinishEnterName();
				return true;
			case SDLK_DELETE:
			case SDLK_BACKSPACE:
				if ( m_handleSize ) {
					sound->PlaySound(gExplosionSound, 5);
					--m_handleSize;
					m_handle[m_handleSize] = '\0';
				}
				break;
			default:
				break;
		}
		m_handleLabel->SetText(m_handle);
		return true;
	case SDL_TEXTINPUT:
		/* FIXME: No true UNICODE support in font */
		key = event.text.text[0];
		if (key >= ' ' && key <= '~') {
			if ( m_handleSize < MAX_NAMELEN ) {
				sound->PlaySound(gShotSound, 5);
				m_handle[m_handleSize++] = key;
				m_handle[m_handleSize] = '\0';
			} else
				sound->PlaySound(gBonk, 5);
		}
		m_handleLabel->SetText(m_handle);
		return true;
	}
	return false;
}

void GameOverPanelDelegate::BeginEnterName()
{
	UIElement *label;

	sound->PlaySound(gBonusShot, 5);

	/* -- Let them enter their name */
	label = m_panel->GetElement<UIElement>("name_label");
	if (label) {
		label->Show();
	}
	m_handleLabel = m_panel->GetElement<UIElement>("name");
	if (!m_handleLabel) {
		return;
	}
	m_handleLabel->Show();

	/* Get the previously used handle, if possible */
	const char *text = m_handleLabel->GetText();
	if (text) {
		SDL_strlcpy(m_handle, text, sizeof(m_handle));
	} else {
		*m_handle = '\0';
	}
	m_handleSize = SDL_strlen(m_handle);

	// Flush events before enabling text input
	HandleEvents(0);
	screen->EnableTextInput();
}

void GameOverPanelDelegate::FinishEnterName()
{
	screen->DisableTextInput();

	if (*m_handle) {
		GameInfo &info = gReplay.GetGameInfo();
		info.SetPlayerName(gDisplayed, m_handle);
		gReplay.Save();
		LoadScores();
	}

	sound->HaltSound();
	sound->PlaySound(gGotPrize, 6);

	ui->ShowPanel(PANEL_MAIN);
}
