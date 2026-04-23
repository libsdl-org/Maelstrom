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

	m_panel->HideAll();

	image = m_panel->GetElement<UIElement>("image");
	if (image) {
		image->Show();
	}

	m_handleLabel = nullptr;

	m_state = STATE_SHOWING;

	DelaySound();
}

void GameOverPanelDelegate::OnHide()
{
	if (gReplay.IsRecording()) {
		// Save this as the last game
		gReplay.Save(LAST_REPLAY);
		gLastGameID = gReplay.GetGameInfo().gameID;
	}
	gReplay.SetMode(REPLAY_IDLE);

	/* Make sure we clear the game info so we don't crash trying to
	   update UI in a future replay
	*/
	gGameInfo.Reset();
}

void GameOverPanelDelegate::HandleShown()
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
	if (gReplay.IsRecording() && !gReplay.HasContinues() &&
	    !gGameInfo.IsMultiplayer() && !gGameInfo.IsKidMode() &&
	    (gGameInfo.wave == 1) && (gGameInfo.lives <= DEFAULT_START_LIVES) &&
	    TheShip->GetScore() > 0) {
		for ( i = 0; i<NUM_SCORES; ++i ) {
			if ( TheShip->GetScore() >= (int)hScores[i].score ) {
				BeginEnterName();
				break;
			}
		}
	}

	if (m_state == STATE_SHOWING) {
		m_state = STATE_FINISHED_NAME;
		m_readyTime = SDL_GetTicks();
	}
}

void GameOverPanelDelegate::HandleEnableIME()
{
	screen->EnableTextInput(m_handleLabel->X(), m_handleLabel->Y(), m_handleLabel->Width(), m_handleLabel->Height());
	m_state = STATE_ENTERING_NAME;
}

void GameOverPanelDelegate::OnTick()
{
	switch (m_state) {
	case STATE_SHOWING:
		HandleShown();
		break;
	case STATE_ENABLE_IME:
		HandleEnableIME();
		break;
	case STATE_ENTERING_NAME:
		// Wait until we're done
		break;
	case STATE_FINISHED_NAME:
		HandleFinishedName();
		break;
	case STATE_DONE:
		// We're done showing the scores
		ui->ShowPanel(PANEL_MAIN);
		break;
	}
}

bool GameOverPanelDelegate::HandleEvent(const SDL_Event &event)
{
	char key;

	if (!m_handleLabel) {
		return false;
	}

	switch (event.type) {
	case SDL_EVENT_KEY_DOWN:
		return true;
	case SDL_EVENT_KEY_UP:
		switch (event.key.key) {
			case SDLK_ESCAPE:
				m_handle[0] = '\0';
				FinishEnterName();
				return true;
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
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		return true;
	case SDL_EVENT_MOUSE_BUTTON_UP:
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		FinishEnterName();
		return true;
	case SDL_EVENT_SCREEN_KEYBOARD_HIDDEN:
		FinishEnterName();
		return true;
	default:
		break;
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

	m_state = STATE_ENABLE_IME;
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
	DelaySound();

	m_state = STATE_FINISHED_NAME;
	m_readyTime = SDL_GetTicks();
}

void GameOverPanelDelegate::HandleFinishedName()
{
	if (gGameInfo.IsMultiplayer()) { /* Let them watch their ranking */
		const Uint32 MULTIPLAYER_SHOW_TIME = 3000;
		if ((SDL_GetTicks() - m_readyTime) < MULTIPLAYER_SHOW_TIME) {
			return;
		}
	}

	m_state = STATE_DONE;
}
