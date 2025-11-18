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
#include "load.h"
#include "object.h"
#include "player.h"
#include "netplay.h"
#include "make.h"
#include "game.h"
#include "../screenlib/UIElement.h"

// Global variables set in this file...
GameInfo gGameInfo;
Replay  gReplay;
int	gGameOn;
int	gPaused;
int	gWave;
int	gBoomDelay;
int	gNextBoom;
int	gBoomPhase;
int	gNumRocks;
int	gLastStar;
int	gWhenDone;
int	gDisplayed;

int	gMultiplierShown;
int	gPrizeShown;
int	gBonusShown;
int	gWhenHoming;
int	gWhenGrav;
int	gWhenDamaged;
int	gWhenNova;
int	gShakeTime;
int	gFreezeTime;
Object *gEnemySprite;
int	gWhenEnemy;

int	gShownContinue;

/* ----------------------------------------------------------------- */
/* -- Setup the players for a new game */

static bool SetupPlayers(void)
{
	if ( CheckPlayers() < 0 )
		return false;

	/* Set up the controls for the game */
	if (gReplay.IsPlaying()) {
		gDisplayed = gReplay.GetDisplayPlayer();
	} else {
		gDisplayed = -1;
	}
	for (int i = 0; i < MAX_PLAYERS; ++i) {
		if (gGameInfo.IsValidPlayer(i)) {
			if (gGameInfo.IsLocalPlayer(i)) {
				if (gDisplayed < 0) {
					gDisplayed = i;
				}
			}
			gPlayers[i]->SetControlType(gGameInfo.GetPlayer(i)->controlMask);
		} else {
			gPlayers[i]->SetControlType(CONTROL_NONE);
		}
	}
	return true;
}

/* ----------------------------------------------------------------- */
/* -- Start a new game */

void NewGame(void)
{
	/* Start the replay */
	gReplay.HandleNewGame();

	/* Start up the random number generator */
	SeedRandom(gGameInfo.seed);

	/* Make sure we have a valid player list */
	if ( !SetupPlayers() ) {
		return;
	}
	InitPlayerControls();

	/* Send a "NEW_GAME" packet onto the network */
	if ( gGameInfo.IsMultiplayer() && gGameInfo.IsHosting() ) {
		if ( Send_NewGame() < 0) {
			return;
		}
	}
	gShownContinue = 0;

	ui->ShowPanel(PANEL_GAME);

}	/* -- NewGame */

void ContinueGame(void)
{
	int i;

	if (gReplay.IsPlaying()) {
		gReplay.ConsumeContinue();
	} else {
		gReplay.RecordContinue();
	}
	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}
		gPlayers[i]->Continue(gGameInfo.lives);
	}
	gShownContinue = 0;
}

bool
GamePanelDelegate::OnLoad()
{
	int i;
	char name[32];

	/* Initialize our panel variables */
	m_score = m_panel->GetElement<UIElement>("score");
	m_shield = m_panel->GetElement<UIElement>("shield");
	m_wave = m_panel->GetElement<UIElement>("wave");
	m_lives = m_panel->GetElement<UIElement>("lives");
	m_bonus = m_panel->GetElement<UIElement>("bonus");

	for (i = 0; (unsigned)i < SDL_arraysize(m_multiplier); ++i) {
		SDL_snprintf(name, sizeof(name), "multiplier%d", 2+i);
		m_multiplier[i] = m_panel->GetElement<UIElement>(name);
	}

	m_autofire = m_panel->GetElement<UIElement>("autofire");
	m_airbrakes = m_panel->GetElement<UIElement>("airbrakes");
	m_lucky = m_panel->GetElement<UIElement>("lucky");
	m_triplefire = m_panel->GetElement<UIElement>("triplefire");
	m_longfire = m_panel->GetElement<UIElement>("longfire");

	m_multiplayerCaption = m_panel->GetElement<UIElement>("multiplayer_caption");
	m_multiplayerColor = m_panel->GetElement<UIElement>("multiplayer_color");
	m_fragsLabel = m_panel->GetElement<UIElement>("frags_label");
	m_frags = m_panel->GetElement<UIElement>("frags");

	return true;
}

void
GamePanelDelegate::OnShow()
{
	int i;

	/* Initialize some game variables */
	gGameOn = 1;
	gPaused = 0;
	gWave = gGameInfo.wave - 1;
	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}
		gPlayers[i]->NewGame(gGameInfo.lives);
	}
	gLastStar = STAR_DELAY;
	gLastDrawn = 0L;
	gNumSprites = 0;

	if ( gGameInfo.IsMultiplayer() ) {
		if (m_multiplayerCaption) {
			m_multiplayerCaption->Show();
		}
		if (m_multiplayerColor) {
			m_multiplayerColor->Show();
		}
	} else {
		if (m_multiplayerCaption) {
			m_multiplayerCaption->Hide();
		}
		if (m_multiplayerColor) {
			m_multiplayerColor->Hide();
		}
	}
	if ( gGameInfo.IsDeathmatch() ) {
		if (m_fragsLabel) {
			m_fragsLabel->Show();
		}
		if (m_frags) {
			m_frags->Show();
		}
	} else {
		if (m_fragsLabel) {
			m_fragsLabel->Hide();
		}
		if (m_frags) {
			m_frags->Hide();
		}
	}

	NextWave();
}

void
GamePanelDelegate::OnHide()
{
	gGameOn = 0;

	/* -- Kill any existing sprites */
	while (gNumSprites > 0)
		delete gSprites[gNumSprites-1];

	sound->HaltSound();
}

void
GamePanelDelegate::OnTick()
{
	int i, j;
	SYNC_RESULT syncResult;

	if (!gGameOn) {
		// This generally shouldn't happen, but could if there were
		// a consistency error during a replay at the bonus screen.
		return;
	}

	if ( gGameInfo.GetLocalState() & STATE_BONUS ) {
		return;
	}

	/* -- Read in keyboard input for our ship */
	HandleEvents(0);

	/* -- Send Sync! signal to all players, and handle keyboard. */
	if (!gReplay.HandlePlayback()) {
		GameOver();
		return;
	}

	syncResult = SyncNetwork();

	// Update state and see if the local player aborted
	if ( !UpdateGameState() ) {
		GameOver();
		return;
	}
	switch (syncResult) {
		case SYNC_TIMEOUT:
			// The other players might be minimized or doing
			// the bonus screen and may not be able to respond.
			return;
		case SYNC_CORRUPT:
			// Uh oh...
			error("Network sync error, game aborted!\r\n");
			GameOver();
			return;
		case SYNC_NETERROR:
			// Uh oh...
			error("Network socket error, game aborted!\r\n");
			GameOver();
			return;
		case SYNC_COMPLETE:
			break;
	}
	gReplay.HandleRecording();

	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}
		gPlayers[i]->HandleKeys();
	}

	if ( gPaused ) {
		return;
	}

	/* -- Play the boom sounds */
	if ( --gNextBoom == 0 ) {
		if ( gBoomPhase ) {
			sound->PlaySound(gBoom1, 0);
			gBoomPhase = 0;
		} else {
			sound->PlaySound(gBoom2, 0);
			gBoomPhase = 1;
		}
		gNextBoom = gBoomDelay;
	}

	/* -- Do all hit detection */
	OBJ_LOOP(j, MAX_PLAYERS) {
		if (!gPlayers[j]->IsValid()) {
			continue;
		}
		if ( ! gPlayers[j]->Alive() )
			continue;

		if (gGameInfo.IsKidMode()) {
			bool enableShield = false;
			OBJ_LOOP(i, gNumSprites) {
				if (gSprites[i]->Collide(gPlayers[j], false)) {
					enableShield = true;
					break;
				}
			}
			gPlayers[j]->SetKidShield(enableShield);
		}

		/* This loop looks funny because gNumSprites can change 
		   dynamically during the loop as sprites are killed/created.
		   This same logic is used whenever looping where sprites
		   might be destroyed.
		*/
		OBJ_LOOP(i, gNumSprites) {
			if ( gSprites[i]->HitBy(gPlayers[j]) < 0 ) {
				delete gSprites[i];
				gSprites[i] = gSprites[gNumSprites];
			}
		}
		if ( gGameInfo.IsDeathmatch() ) {
			OBJ_LOOP(i, MAX_PLAYERS) {
				if (!gPlayers[i]->IsValid()) {
					continue;
				}
				if ( i == j )	// Don't shoot ourselves. :)
					continue;
				(void) gPlayers[i]->HitBy(gPlayers[j]);
			}
		}
	}
	if ( gEnemySprite ) {
		OBJ_LOOP(i, MAX_PLAYERS) {
			if (!gPlayers[i]->IsValid()) {
				continue;
			}
			if ( ! gPlayers[i]->Alive() )
				continue;
			(void) gPlayers[i]->HitBy(gEnemySprite);
		}
		OBJ_LOOP(i, gNumSprites) {
			if ( gSprites[i] == gEnemySprite )
				continue;
			if ( gSprites[i]->HitBy(gEnemySprite) < 0 ) {
				delete gSprites[i];
				gSprites[i] = gSprites[gNumSprites];
			}
		}
	}

	/* Handle all the shimmy and the shake. :-) */
	if ( gShakeTime && (gShakeTime-- > 0) ) {
		int shakeV;

		OBJ_LOOP(i, MAX_PLAYERS) {
			if (!gPlayers[i]->IsValid()) {
				continue;
			}
			shakeV = FastRandom(SHAKE_FACTOR);
			if ( ! gPlayers[i]->Alive() )
				continue;
			gPlayers[i]->Shake(FastRandom(SHAKE_FACTOR));
		}
		OBJ_LOOP(i, gNumSprites) {
			shakeV = FastRandom(SHAKE_FACTOR);
			gSprites[i]->Shake(FastRandom(SHAKE_FACTOR));
		}
	}

	/* -- Move all of the sprites */
	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}
		gPlayers[i]->Move(0);
	}
	OBJ_LOOP(i, gNumSprites) {
		if ( gSprites[i]->Move(gFreezeTime) < 0 ) {
			delete gSprites[i];
			gSprites[i] = gSprites[gNumSprites];
		}
	}
	if ( gFreezeTime )
		--gFreezeTime;

	DoHousekeeping();
}

void
GamePanelDelegate::OnDraw(DRAWLEVEL drawLevel)
{
	int i;

	if (drawLevel != DRAWLEVEL_BACKGROUND) {
		return;
	}

	/* Draw the status frame */
	DrawStatus(false);

	if ( gGameInfo.GetLocalState() & STATE_BONUS ) {
		return;
	}

	/* -- Draw the star field */
	for ( i=0; i<MAX_STARS; ++i ) {
		int x = (gTheStars[i]->xCoord << SPRITE_PRECISION);
		int y = (gTheStars[i]->yCoord << SPRITE_PRECISION);
		GetRenderCoordinates(x, y);
		screen->DrawPoint(x, y, gTheStars[i]->color);
	}

	/* -- Blit all the sprites */
	OBJ_LOOP(i, gNumSprites)
		gSprites[i]->BlitSprite();
	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}
		gPlayers[i]->BlitSprite();
	}
}

bool
GamePanelDelegate::OnAction(UIBaseElement *sender, const char *action)
{
	if (SDL_strncmp(action, "CONTROL_", 8) == 0) {
		action += 8;

		bool down = false;
		if (SDL_strncmp(action, "DOWN_", 5) == 0) {
			down = true;
			action += 5;
		} else if (SDL_strncmp(action, "UP_", 3) == 0) {
			down = false;
			action += 3;
		}

		unsigned char control;
		if (SDL_strcasecmp(action, "THRUST") == 0) {
			control = THRUST_KEY;
		} else if (SDL_strcasecmp(action, "RIGHT") == 0) {
			control = RIGHT_KEY;
		} else if (SDL_strcasecmp(action, "LEFT") == 0) {
			control = LEFT_KEY;
		} else if (SDL_strcasecmp(action, "SHIELD") == 0) {
			control = SHIELD_KEY;
		} else if (SDL_strcasecmp(action, "FIRE") == 0) {
			control = FIRE_KEY;
		} else if (SDL_strcasecmp(action, "PAUSE") == 0) {
			control = PAUSE_KEY;
		} else if (SDL_strcasecmp(action, "ABORT") == 0) {
			control = ABORT_KEY;
		} else {
			error("Unknown control action '%s'", action);
			return false;
		}

		if (control == PAUSE_KEY || control == ABORT_KEY) {
			if (!down) {
				if (control == PAUSE_KEY) {
					gGameInfo.ToggleLocalState(STATE_PAUSE);
				}
				if (control == ABORT_KEY) {
					gGameInfo.SetLocalState(STATE_ABORT, true);
				}
			}
		} else {
			Player *player = GetControlPlayer(CONTROL_TOUCH);
			if (player) {
				player->SetControl(control, down);
			}
		}
	} else {
		return false;
	}
	return true;
}

/* ----------------------------------------------------------------- */
/* -- Draw the status display */

void
GamePanelDelegate::DrawStatus(Bool first)
{
	static int lastScores[MAX_PLAYERS], lastLife[MAX_PLAYERS];
	int Score;
	int MultFactor;
	int i;
	char numbuf[128];

/* -- Draw the status display */

	if (first && gWave == 1) {
		OBJ_LOOP(i, MAX_PLAYERS) {
			lastLife[i] = lastScores[i] = 0;
		}
	}

	if ( gGameInfo.IsMultiplayer() ) {
#ifndef USE_TOUCHCONTROL
		if (gReplay.IsPlaying()) {
			char caption[BUFSIZ];

			SDL_snprintf(caption, sizeof(caption), "Displaying player %d - press F1 to change", gDisplayed+1);
			if (m_multiplayerCaption) {
				m_multiplayerCaption->SetText(caption);
			}
		}
#endif // USE_TOUCHCONTROL

		/* Fill in the color by the frag count */
		if (m_multiplayerColor) {
			m_multiplayerColor->SetColor(TheShip->Color());
		}

		SDL_snprintf(numbuf, sizeof(numbuf), "%-3.1d", TheShip->GetFrags());
		if (m_frags) {
			m_frags->SetText(numbuf);
		}
	}

	int fact = ((SHIELD_WIDTH - 2) * TheShip->GetShieldLevel()) / MAX_SHIELD;
	if (m_shield) {
		m_shield->SetWidth(fact);
	}
	
	MultFactor = TheShip->GetBonusMult();
	for (i = 0; (unsigned)i < SDL_arraysize(m_multiplier); ++i) {
		if (!m_multiplier[i]) {
			continue;
		}
		if (MultFactor == 2+i) {
			m_multiplier[i]->Show();
		} else {
			m_multiplier[i]->Hide();
		}
	}

	if (m_autofire) {
		if ( TheShip->GetSpecial(MACHINE_GUNS) ) {
			m_autofire->Show();
		} else {
			m_autofire->Hide();
		}
	}
	if (m_airbrakes) {
		if ( TheShip->GetSpecial(AIR_BRAKES) ) {
			m_airbrakes->Show();
		} else {
			m_airbrakes->Hide();
		}
	}
	if (m_lucky) {
		if ( TheShip->GetSpecial(LUCKY_IRISH) ) {
			m_lucky->Show();
		} else {
			m_lucky->Hide();
		}
	}
	if (m_triplefire) {
		if ( TheShip->GetSpecial(TRIPLE_FIRE) ) {
			m_triplefire->Show();
		} else {
			m_triplefire->Hide();
		}
	}
	if (m_longfire) {
		if ( TheShip->GetSpecial(LONG_RANGE) ) {
			m_longfire->Show();
		} else {
			m_longfire->Hide();
		}
	}

	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}
		Score = gPlayers[i]->GetScore();

		if ( i == gDisplayed && m_score ) {
			SDL_snprintf(numbuf, sizeof(numbuf), "%d", Score);
			m_score->SetText(numbuf);
		}

		if (!gGameInfo.IsDeathmatch()) {
			if (lastScores[i] == Score)
				continue;

			/* -- See if they got a new life */
			lastScores[i] = Score;
			if ((Score - lastLife[i]) >= NEW_LIFE) {
				if (!gPlayers[i]->IsGhost()) {
					gPlayers[i]->IncrLives(1);
					if ( gGameInfo.IsLocalPlayer(i) )
						sound->PlaySound(gNewLife, 5);
				}
				lastLife[i] = (Score / NEW_LIFE) * NEW_LIFE;
			}
		}
	}

	if (m_wave) {
		SDL_snprintf(numbuf, sizeof(numbuf), "%d", gWave);
		m_wave->SetText(numbuf);
	}

	if (m_lives) {
		SDL_snprintf(numbuf, sizeof(numbuf), "%-3.1d", TheShip->GetLives());
		m_lives->SetText(numbuf);
	}

	if (m_bonus) {
		SDL_snprintf(numbuf, sizeof(numbuf), "%-7.1d", TheShip->GetBonus());
		m_bonus->SetText(numbuf);
	}

}	/* -- DrawStatus */

/* ----------------------------------------------------------------- */
/* -- Update game state based on node state */

bool
GamePanelDelegate::UpdateGameState()
{
	int i;
	int paused;

	// Check for game over
	for (i = 0; i < gGameInfo.GetNumNodes(); ++i) {
		if (gGameInfo.GetNodeState(i) & STATE_ABORT) {
			return false;
		}
	}

	// Check for pause status
	paused = 0;
	for (i = 0; i < gGameInfo.GetNumNodes(); ++i) {
		paused |= gGameInfo.GetNodeState(i);
	}
	if ((paused & (STATE_PAUSE|STATE_MINIMIZE)) &&
	    !(gPaused & (STATE_PAUSE|STATE_MINIMIZE))) {
		sound->PlaySound(gPauseSound, 5);
	}
	gPaused = paused;

	return true;
}

/* ----------------------------------------------------------------- */
/* -- Do some housekeeping! */

void
GamePanelDelegate::DoHousekeeping()
{
	int i;

	/* -- Maybe throw a multiplier up on the screen */
	if (gMultiplierShown && (--gMultiplierShown == 0) )
		MakeMultiplier();
	
	/* -- Maybe throw a prize(!) up on the screen */
	if (gPrizeShown && (--gPrizeShown == 0) )
		MakePrize();
	
	/* -- Maybe throw a bonus up on the screen */
	if (gBonusShown && (--gBonusShown == 0) )
		MakeBonus();

	/* -- Maybe make a nasty enemy fighter? */
	if (gWhenEnemy && (--gWhenEnemy == 0) )
		MakeEnemy();

	/* -- Maybe create a transcenfugal vortex */
	if (gWhenGrav && (--gWhenGrav == 0) )
		MakeGravity();
	
	/* -- Maybe create a recified space vehicle */
	if (gWhenDamaged && (--gWhenDamaged == 0) )
		MakeDamagedShip();
	
	/* -- Maybe create a autonominous tracking device */
	if (gWhenHoming && (--gWhenHoming == 0) )
		MakeHoming();
	
	/* -- Maybe make a supercranial destruction thang */
	if (gWhenNova && (--gWhenNova == 0) )
		MakeNova();

	/* -- Maybe create a new star ? */
	if ( --gLastStar == 0 ) {
		gLastStar = STAR_DELAY;
		SetStar(FastRandom(MAX_STARS));
	}
	
	/* -- Time for the next wave? */
	if (gNumRocks == 0) {
		if ( gWhenDone == 0 )
			gWhenDone = DEAD_DELAY;
		else if ( --gWhenDone == 0 )
			NextWave();
	}

	/* -- Make sure someone is still playing... */
	bool PlayersLeft = false;
	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}
		if ( gPlayers[i]->Kicking() ) {
			PlayersLeft = true;
			break;
		}
	}
	if ( !PlayersLeft ) {
		if (gReplay.IsPlaying()) {
			if (gReplay.HasContinues()) {
				ContinueGame();
			} else {
				GameOver();
			}
		} else if (!gGameInfo.IsMultiplayer() && !gShownContinue) {
			gShownContinue = 1;
			ui->ShowPanel(PANEL_CONTINUE);
		} else {
			GameOver();
		}
	}

}	/* -- DoHousekeeping */

/* ----------------------------------------------------------------- */
/* -- Do the bonus display */

void
GamePanelDelegate::DoBonus()
{
	UIPanel *panel;
	UIElement *image;
	UIElement *label;
	UIElement *bonus;
	UIElement *score;
	int i;
	char numbuf[128];

	/* -- Now do the bonus */
	sound->HaltSound();

	panel = ui->GetPanel(PANEL_BONUS);
	if (!panel) {
		return;
	}
	panel->HideAll();

	/* -- Set the wave completed message */
	label = panel->GetElement<UIElement>("wave");
	if (label) {
		SDL_snprintf(numbuf, sizeof(numbuf), "Wave %d completed.", gWave);
		label->SetText(numbuf);
		label->Show();
	}
	label = panel->GetElement<UIElement>("bonus_label");
	if (label) {
		label->Show();
	}
	label = panel->GetElement<UIElement>("score_label");
	if (label) {
		label->Show();
	}
		
	gGameInfo.SetLocalState(STATE_BONUS, true);

	/* Fade out */
	screen->FadeOut();

	ui->ShowPanel(PANEL_BONUS);
	ui->Draw();

	/* Fade in */
	screen->FadeIn();
	while ( sound->Playing() )
		Delay(SOUND_DELAY);

	/* -- Count the score down */

	bonus = panel->GetElement<UIElement>("bonus");
	score = panel->GetElement<UIElement>("score");
	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}
		if (i != gDisplayed) {
			gPlayers[i]->MultBonus();
			continue;
		}

		if (TheShip->GetBonusMult() != 1) {
			if (bonus) {
				SDL_snprintf(numbuf, sizeof(numbuf), "%-5.1d", TheShip->GetBonus());
				bonus->SetText(numbuf);
				bonus->Show();
			}
			bonus = panel->GetElement<UIElement>("multiplied_bonus");

			TheShip->MultBonus();
			Delay(SOUND_DELAY);
			sound->PlaySound(gMultiplier, 5);

			SDL_snprintf(numbuf, sizeof(numbuf), "multiplier%d", TheShip->GetBonusMult());
			image = panel->GetElement<UIElement>(numbuf);
			if (image) {
				image->Show();
			}

			ui->Draw();
			Delay(60);
		}
	}
	Delay(SOUND_DELAY);
	sound->PlaySound(gFunk, 5);

	if (bonus) {
		SDL_snprintf(numbuf, sizeof(numbuf), "%-5.1d", TheShip->GetBonus());
		bonus->SetText(numbuf);
		bonus->Show();
	}
	if (score) {
		SDL_snprintf(numbuf, sizeof(numbuf), "%-5.1d", TheShip->GetScore());
		score->SetText(numbuf);
		score->Show();
	}
	ui->Draw();
	Delay(60);

	/* -- Praise them or taunt them as the case may be */
	if (TheShip->GetBonus() == 0) {
		Delay(SOUND_DELAY);
		sound->PlaySound(gNoBonus, 5);
	}
	if (TheShip->GetBonus() > 10000) {
		Delay(SOUND_DELAY);
		sound->PlaySound(gPrettyGood, 5);
	}
	while ( sound->Playing() )
		Delay(SOUND_DELAY);

	/* -- Count the score down */
	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}
		if (i != gDisplayed) {
			while ( gPlayers[i]->GetBonus() > 500 ) {
				gPlayers[i]->IncrScore(500);
				gPlayers[i]->IncrBonus(-500);
			}
			continue;
		}

		while (TheShip->GetBonus() > 0) {
			while ( sound->Playing() )
				Delay(SOUND_DELAY);

			sound->PlaySound(gBonk, 5);
			if ( TheShip->GetBonus() >= 500 ) {
				TheShip->IncrScore(500);
				TheShip->IncrBonus(-500);
			} else {
				TheShip->IncrScore(TheShip->GetBonus());
				TheShip->IncrBonus(-TheShip->GetBonus());
			}
	
			if (bonus) {
				SDL_snprintf(numbuf, sizeof(numbuf), "%-5.1d", TheShip->GetBonus());
				bonus->SetText(numbuf);
			}
			if (score) {
				SDL_snprintf(numbuf, sizeof(numbuf), "%-5.1d", TheShip->GetScore());
				score->SetText(numbuf);
			}

			ui->Draw();
		}
	}
	while ( sound->Playing() )
		Delay(SOUND_DELAY);
	HandleEvents(10);

	/* -- Draw the "next wave" message */
	label = panel->GetElement<UIElement>("next");
	if (label) {
		SDL_snprintf(numbuf, sizeof(numbuf), "Prepare for Wave %d...", gWave+1);
		label->SetText(numbuf);
		label->Show();
	}
	ui->Draw();
	HandleEvents(100);

	ui->HidePanel(PANEL_BONUS);

	gGameInfo.SetLocalState(STATE_BONUS, false);

	/* Fade out and prepare for drawing the next wave */
	screen->FadeOut();
	screen->Clear();

}	/* -- DoBonus */

/* ----------------------------------------------------------------- */
/* -- Start the next wave! */

void
GamePanelDelegate::NextWave()
{
	int	i, x, y;
	int	NewRoids;
	short	temp;

	gEnemySprite = NULL;

	/* -- Initialize some variables */
	gNumRocks = 0;
	gShakeTime = 0;
	gFreezeTime = 0;

	if (gWave != (gGameInfo.wave - 1))
		DoBonus();

	gWave++;

	/* See about the Multiplier */
	if ( FastRandom(2) )
		gMultiplierShown = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gMultiplierShown = 0;

	/* See about the Prize */
	if ( FastRandom(2) )
		gPrizeShown = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gPrizeShown = 0;

	/* See about the Bonus */
	if ( FastRandom(2) )
		gBonusShown = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gBonusShown = 0;

	/* See about the Gravity */
	if (FastRandom(10 + gWave) > 11)
		gWhenGrav = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gWhenGrav = 0;

	/* See about the Nova */
	if (FastRandom(10 + gWave) > 13)
		gWhenNova = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gWhenNova = 0;

	/* See about the Enemy */
	if (FastRandom(3) == 0)
		gWhenEnemy = ((FastRandom(30) * 60)/FRAME_DELAY);
	else
		gWhenEnemy = 0;

	/* See about the Damaged Ship */
	if (FastRandom(10) == 0)
		gWhenDamaged = ((FastRandom(60) * 60L)/FRAME_DELAY);
	else
		gWhenDamaged = 0;

	/* See about the Homing Mine */
	if (FastRandom(10 + gWave) > 12)
		gWhenHoming = ((FastRandom(60) * 60L)/FRAME_DELAY);
	else
		gWhenHoming = 0;

	temp = gWave / 4;
	if (temp < 1)
		temp = 1;

	NewRoids = FastRandom(temp) + (gWave / 5) + 3;

	/* -- Kill any existing sprites */
	while (gNumSprites > 0)
		delete gSprites[gNumSprites-1];

	/* -- Initialize some variables */
	gLastDrawn = 0L;
	gBoomDelay = (60/FRAME_DELAY);
	gNextBoom = gBoomDelay;
	gBoomPhase = 0;
	gWhenDone = 0;

	/* -- Create the ship's sprite */
	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}
		gPlayers[i]->NewWave();
	}
	DrawStatus(true);

	/* -- Create some asteroids */
	for (i = 0; i < NewRoids; i++) {
		int	randval;
	
		x = FastRandom(GAME_WIDTH) * SCALE_FACTOR;
		y = 0;
	
		randval = FastRandom(10);

		/* -- See what kind of asteroid to make */
		if (randval == 0)
			MakeSteelRoid(x, y);
		else
			MakeLargeRock(x, y);
	}

}	/* -- NextWave */

/* ----------------------------------------------------------------- */
/* -- End the game */

void
GamePanelDelegate::GameOver()
{
	ui->ShowPanel(PANEL_GAMEOVER);

	QuitPlayerControls();
}

/* ----------------------------------------------------------------- */
/* -- Convert from sprite coordinates to render coordinates */

void GetRenderCoordinates(int &x, int &y)
{
	x = gScrnRect.x + (int)(((float)x * gScrnRect.w) / (GAME_WIDTH << SPRITE_PRECISION));
	y = gScrnRect.y + (int)(((float)y * gScrnRect.h) / (GAME_HEIGHT << SPRITE_PRECISION));
}

/* ----------------------------------------------------------------- */
/* -- Render a sprite on the screen */

void RenderSprite(UITexture *sprite, int x, int y, int w, int h)
{
	GetRenderCoordinates(x, y);
	w = (int)(((float)w * gScrnRect.w) / GAME_WIDTH);
	h = (int)(((float)h * gScrnRect.h) / GAME_HEIGHT);
	screen->QueueBlit(sprite->Texture(), x, y, w, h, DOCLIP);
}
