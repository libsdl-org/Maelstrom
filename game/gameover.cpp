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
		SDL_qsort(final,MAX_PLAYERS,sizeof(struct FinalScore),cmp_byfrags);
	else
		SDL_qsort(final,MAX_PLAYERS,sizeof(struct FinalScore),cmp_byscore);

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
				SDL_snprintf(name, sizeof(name), "player%d", final[i].Player);
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
}

void GameOverPanelDelegate::OnTick()
{
	if (m_handleLabel) {
		return;
	}

	/* -- Wait for the game over sound */
	if (sound->Playing()) {
		return;
	}

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
	SDL_Gamepad *gamepad;

	if (!m_handleLabel) {
		return false;
	}

	switch (event.type) {
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		return true;
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		gamepad = SDL_OpenGamepad(event.gbutton.which);
		if (gamepad) {
			switch (SDL_GetGamepadButtonLabel(gamepad, (SDL_GamepadButton)event.gbutton.button)) {
			case SDL_GAMEPAD_BUTTON_LABEL_A:
			case SDL_GAMEPAD_BUTTON_LABEL_CROSS:
				FinishEnterName();
				break;
			default:
				break;
			}
			SDL_CloseGamepad(gamepad);
		}
		return true;
	case SDL_EVENT_KEY_DOWN:
		return true;
	case SDL_EVENT_KEY_UP:
		switch (event.key.key) {
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
	case SDL_EVENT_TEXT_INPUT:
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
	m_handleSize = (int)SDL_strlen(m_handle);

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
	m_handleLabel = nullptr;

	sound->HaltSound();
	sound->PlaySound(gGotPrize, 6);
}
