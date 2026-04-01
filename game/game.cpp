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
int	gNumSmallRocksDestroyed;
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

			Uint8 controlMask = gGameInfo.GetPlayer(i)->controlMask;
			if (controlMask == CONTROL_LOCAL) {
				// Remove the controls that are used by other players
				for (int j = 0; j < MAX_PLAYERS; ++j) {
					if (!gGameInfo.IsValidPlayer(j)) {
						continue;
					}
					if (i == j) {
						continue;
					}
					Uint8 otherMask = gGameInfo.GetPlayer(j)->controlMask;
					if (otherMask != CONTROL_LOCAL) {
						controlMask &= ~otherMask;
					}
				}
			}
			gPlayers[i]->SetControlType(controlMask);
		} else {
			gPlayers[i]->SetControlType(CONTROL_NONE);
		}
	}

	EnableRemoteInput();

	return true;
}

/* ----------------------------------------------------------------- */
/* -- Start a new game */

void NewGame(void)
{
	InitNetData();

	/* Start the replay */
	gReplay.HandleNewGame();

	/* Start up the random number generator */
	SeedRandom(gGameInfo.seed);

	/* Make sure we have a valid player list */
	if ( !SetupPlayers() ) {
		return;
	}

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
	m_touchControls = m_panel->GetElement<UIElement>("touch_controls");
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

	m_paused = m_panel->GetElement<UIElement>("paused");

	m_zoom = false;

	return true;
}

void
GamePanelDelegate::OnShow()
{
	int i;

	UpdateZoom();

	SetSteamTimelineMode(STEAM_TIMELINE_PLAYING);

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
	if (m_paused) {
		m_paused->Hide();
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

	switch (m_state) {
	case STATE_SHOW_BONUS:
		ShowBonus();
		return;
	case STATE_BONUS_SHOW_VALUE:
		BonusShowValue();
		return;
	case STATE_BONUS_SHOW_MULTIPLIER:
		BonusShowMultiplier();
		return;
	case STATE_BONUS_DISPLAY_DELAY:
		BonusDisplayDelay();
		return;
	case STATE_BONUS_DISPLAY:
		BonusDisplay();
		return;
	case STATE_BONUS_CHECK_SOUND:
		BonusCheckSound();
		return;
	case STATE_BONUS_TAUNT:
		BonusTaunt();
		return;
	case STATE_BONUS_PRAISE:
		BonusPraise();
		return;
	case STATE_BONUS_COUNTDOWN:
		BonusCountdown();
		return;
	case STATE_BONUS_NEXT_WAVE:
		BonusNextWave();
		return;
	case STATE_BONUS_HIDE:
		BonusHide();
		return;
	case STATE_START_NEXT_WAVE:
		StartNextWave();
		return;
	default:
		break;
	}

	if (!gGameOn) {
		// This generally shouldn't happen, but could if there were
		// a consistency error during a replay at the bonus screen.
		return;
	}

	if ( gGameInfo.GetLocalState() & STATE_BONUS ) {
		return;
	}

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
				if (gSprites[i]->IsDangerous() && gSprites[i]->Collide(gPlayers[j], false)) {
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
		OBJ_LOOP(i, MAX_PLAYERS) {
			if (!gPlayers[i]->IsValid()) {
				continue;
			}
			if ( ! gPlayers[i]->Alive() )
				continue;
			gPlayers[i]->Shake();
		}
		OBJ_LOOP(i, gNumSprites) {
			gSprites[i]->Shake();
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

	if (m_state != STATE_PLAYING) {
		return;
	}

	/* Draw the status frame */
	DrawStatus(false);

	if ( gGameInfo.GetLocalState() & STATE_BONUS ) {
		return;
	}

	if (m_zoom) {
		StartZoomedDrawing();
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

	DrawBorder();

	if (m_zoom) {
		StopZoomedDrawing();
	}
}

bool
GamePanelDelegate::HandleEvent(const SDL_Event &event)
{
	if (event.type == SDL_EVENT_FINGER_DOWN) {
		if (m_touchControls) {
			m_touchControls->Show();
		}
	} else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED ||
	           event.type == SDL_EVENT_WINDOW_SAFE_AREA_CHANGED ||
	           event.type == SDL_EVENT_REMOTE_PLAYERS_CHANGED) {
		UpdateZoom();
	}
	return false;
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
		} else if (SDL_strcasecmp(action, "BRAKE") == 0) {
			control = BRAKE_KEY;
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
			Player *player = GetControlPlayer(CONTROL_KEYBOARD);
			if (player) {
				player->SetControl(control, down);
			}
		}
	} else {
		return false;
	}
	return true;
}

void
GamePanelDelegate::UpdateZoom()
{
	SDL_Renderer *renderer = screen->GetRenderer();
	SDL_Rect rect;
	int saved_w, saved_h;
	SDL_RendererLogicalPresentation saved_mode;
	SDL_GetRenderLogicalPresentation(renderer, &saved_w, &saved_h, &saved_mode);
	SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
	SDL_GetRenderSafeArea(renderer, &rect);
	SDL_SetRenderLogicalPresentation(renderer, saved_w, saved_h, saved_mode);

	// We can zoom if we're on a phone in landscape mode and not local multiplayer
	bool zoom = false;

	if (gAlwaysZoom || (IsPhone() && rect.w > rect.h)) {
		int i;

		int local_players = 0;
		OBJ_LOOP(i, MAX_PLAYERS) {
			if (!gPlayers[i]->IsValid()) {
				continue;
			}

			if (IS_LOCAL_CONTROL(gPlayers[i]->GetControlType())) {
				++local_players;
			}
		}
		if (local_players == 1) {
			zoom = true;
		}
	}

	if (zoom) {
		StartZoom(rect);
	} else {
		StopZoom();
	}
}

void
GamePanelDelegate::StartZoom(const SDL_Rect &rect)
{
	SDL_Renderer *renderer = screen->GetRenderer();
	float scale = (float)GAME_WIDTH / rect.w;
	int x = (int)SDL_round(rect.x * scale);
	int y = (int)SDL_round(rect.y * scale);
	int height = (int)SDL_round(rect.h * scale);
	ui->SetPosition(x, y);
	ui->SetSize(GAME_WIDTH, height);

	if (!m_texture) {
		m_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_UNKNOWN, SDL_TEXTUREACCESS_TARGET, GAME_WIDTH, GAME_HEIGHT);
	}

	m_zoom = true;
}

void
GamePanelDelegate::StopZoom()
{
	ui->SetPosition(0, 0);
	ui->SetSize(GAME_WIDTH, GAME_HEIGHT);

	if (m_texture) {
		SDL_DestroyTexture(m_texture);
		m_texture = nullptr;
	}

	m_zoom = false;
}

void
GamePanelDelegate::StartZoomedDrawing()
{
	SDL_Renderer *renderer = screen->GetRenderer();

	// Don't clip
	screen->GetClip(&m_savedClip);
	SDL_Rect clip = m_savedClip;
	clip.y = 0;
	clip.x = 0;
	clip.w = GAME_WIDTH;
	clip.h = GAME_HEIGHT;
	screen->ClipBlit(&clip);

	SDL_SetRenderTarget(renderer, m_texture);
	screen->Clear();
}

void
GamePanelDelegate::StopZoomedDrawing()
{
	SDL_Renderer *renderer = screen->GetRenderer();

	SDL_SetRenderTarget(renderer, nullptr);
	SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);

	int w = 0, h = 0;
	SDL_GetRenderOutputSize(renderer, &w, &h);

	int cameraX, cameraY;
	gPlayers[0]->GetCameraPos(&cameraX, &cameraY);
	GetRenderCoordinates(cameraX, cameraY);
	cameraX += (SPRITES_WIDTH / 2);
	cameraY += (SPRITES_WIDTH / 2);

	SDL_Rect src;
	if (w > h) {
		int visible_width = (GAME_WIDTH - (2 * SPRITES_WIDTH));
		float scale = (float)visible_width / w;
		src.w = visible_width;
		src.h = (int)SDL_roundf(h * scale);
		src.x = cameraX - src.w / 2;
		src.y = cameraY - src.h / 2;
	} else {
		int visible_height = (GAME_HEIGHT - (2 * SPRITES_WIDTH));
		float scale = (float)visible_height / h;
		src.w = (int)SDL_roundf(w * scale);
		src.h = visible_height;
		src.x = cameraX - src.w / 2;
		src.y = cameraY - src.h / 2;
	}
	float minu = (float)src.x / m_texture->w;
	float minv = (float)src.y / m_texture->h;
	float maxu = (float)(src.x + src.w) / m_texture->w;
	float maxv = (float)(src.y + src.h) / m_texture->h;

	SDL_FRect dst;
	dst.x = 0.0f;
	dst.y = 0.0f;
	dst.w = (float)w;
	dst.h = (float)h;

	SDL_FColor color = { 1.0f, 1.0f, 1.0f, 1.0f };
	SDL_Vertex verts[6];
	SDL_Vertex *vert = verts;
	/* 0 */
	vert->position.x = dst.x;
	vert->position.y = dst.y;
	vert->color = color;
	vert->tex_coord.x = minu;
	vert->tex_coord.y = minv;
	vert++;
	/* 1 */
	vert->position.x = dst.x + dst.w;
	vert->position.y = dst.y;
	vert->color = color;
	vert->tex_coord.x = maxu;
	vert->tex_coord.y = minv;
	vert++;
	/* 2 */
	vert->position.x = dst.x + dst.w;
	vert->position.y = dst.y + dst.h;
	vert->color = color;
	vert->tex_coord.x = maxu;
	vert->tex_coord.y = maxv;
	vert++;
	/* 0 */
	vert->position.x = dst.x;
	vert->position.y = dst.y;
	vert->color = color;
	vert->tex_coord.x = minu;
	vert->tex_coord.y = minv;
	vert++;
	/* 2 */
	vert->position.x = dst.x + dst.w;
	vert->position.y = dst.y + dst.h;
	vert->color = color;
	vert->tex_coord.x = maxu;
	vert->tex_coord.y = maxv;
	vert++;
	/* 3 */
	vert->position.x = dst.x;
	vert->position.y = dst.y + dst.h;
	vert->color = color;
	vert->tex_coord.x = minu;
	vert->tex_coord.y = maxv;
	vert++;

	SDL_SetRenderTextureAddressMode(renderer, SDL_TEXTURE_ADDRESS_WRAP, SDL_TEXTURE_ADDRESS_WRAP);
	SDL_RenderGeometry(renderer, m_texture, verts, 6, NULL, 0);
	SDL_SetRenderTextureAddressMode(renderer, SDL_TEXTURE_ADDRESS_AUTO, SDL_TEXTURE_ADDRESS_AUTO);

	SDL_SetRenderLogicalPresentation(renderer, ui->X() + ui->Width() + ui->X(), ui->Y() + ui->Height() + ui->Y(), SDL_LOGICAL_PRESENTATION_LETTERBOX);

	screen->ClipBlit(&m_savedClip);
}

void
GamePanelDelegate::DrawBorder()
{
	if (m_zoom) {
		return;
	}

	SDL_Rect rect;
	screen->GetClip(&rect);
	rect.x -= 1;
	rect.y -= 1;
	rect.w += 2;
	rect.h += 2;
	screen->DrawRect(rect.x, rect.y, rect.w, rect.h, screen->MapRGB(0x75, 0x75, 0xFF));
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
		if (gReplay.IsPlaying() && SDL_HasKeyboard()) {
			char caption[BUFSIZ];

			SDL_snprintf(caption, sizeof(caption), "Displaying player %d - press F1 to change", gDisplayed+1);
			if (m_multiplayerCaption) {
				m_multiplayerCaption->SetText(caption);
			}
		}

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
	bool locally_paused = false;
	int index_paused = -1;
	paused = 0;
	for (i = 0; i < gGameInfo.GetNumNodes(); ++i) {
		Uint8 state = gGameInfo.GetNodeState(i);
		paused |= state;
		if (state & (STATE_PAUSE|STATE_MINIMIZE)) {
			if (i == gGameInfo.GetLocalIndex()) {
				locally_paused = true;
			} else {
				index_paused = i;
			}
		}
	}
	if ((paused & (STATE_PAUSE|STATE_MINIMIZE)) &&
	    !(gPaused & (STATE_PAUSE|STATE_MINIMIZE))) {
		sound->PlaySound(gPauseSound, 5);
	}
	if (m_paused) {
		// Update the pause label
		if (paused & (STATE_PAUSE|STATE_MINIMIZE)) {
			char label[128] = { 0 };
			if (!locally_paused) {
				const GameInfoPlayer *player = gGameInfo.GetPlayer(index_paused);
				if (*player->name) {
					SDL_snprintf(label, sizeof(label), "Paused by %s", player->name);
				}
			}
			if (!*label) {
				SDL_strlcpy(label, "Paused", sizeof(label));
			}
			m_paused->SetText(label);
			m_paused->Show();
		} else if (gPaused & (STATE_PAUSE|STATE_MINIMIZE)) {
			m_paused->Hide();
		}
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
	UIElement *label;
	char numbuf[128];
	int i;

	/* -- Now do the bonus */
	sound->HaltSound();

	panel = ui->GetPanel(PANEL_BONUS);
	if (!panel) {
		m_state = STATE_START_NEXT_WAVE;
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

	// Handle the bonus for other players
	OBJ_LOOP(i, MAX_PLAYERS) {
		if (!gPlayers[i]->IsValid()) {
			continue;
		}

		if (gPlayers[i]->CanGetSinglePlayerAchievement()) {
			int bonus = gPlayers[i]->GetBonus() * gPlayers[i]->GetBonusMult();
			if (bonus >= 30000) {
				UnlockAchievement("ACHIEVEMENT_BONUS_30");
			} else if (bonus >= 25000) {
				UnlockAchievement("ACHIEVEMENT_BONUS_25");
			} else if (bonus >= 20000) {
				UnlockAchievement("ACHIEVEMENT_BONUS_20");
			} else if (bonus >= 15000) {
				UnlockAchievement("ACHIEVEMENT_BONUS_15");
			} else if (bonus >= 10000) {
				UnlockAchievement("ACHIEVEMENT_BONUS_10");
			} else if (bonus >= 5000) {
				UnlockAchievement("ACHIEVEMENT_BONUS_5");
			}
		}

		if (i != gDisplayed) {
			gPlayers[i]->MultBonus();
			gPlayers[i]->IncrScore(gPlayers[i]->GetBonus());
			gPlayers[i]->IncrBonus(-gPlayers[i]->GetBonus());
		}
	}

	/* Fade out */
	screen->FadeOut();

	m_state = STATE_SHOW_BONUS;
}


void
GamePanelDelegate::ShowBonus()
{
	ui->ShowPanel(PANEL_BONUS);

	/* Fade in */
	screen->FadeIn();
	DelaySound();
	m_state = STATE_BONUS_SHOW_VALUE;
}

void
GamePanelDelegate::BonusShowValue()
{
	UIPanel *panel;
	UIElement *bonus;
	char numbuf[128];

	if (TheShip->GetBonusMult() != 1) {
		panel = ui->GetPanel(PANEL_BONUS);
		bonus = panel->GetElement<UIElement>("bonus");
		if (bonus) {
			SDL_snprintf(numbuf, sizeof(numbuf), "%-5.1d", TheShip->GetBonus());
			bonus->SetText(numbuf);
			bonus->Show();
		}

		TheShip->MultBonus();
		DelayAndDraw(SOUND_DELAY);
		m_state = STATE_BONUS_SHOW_MULTIPLIER;
		return;
	}
	m_state = STATE_BONUS_DISPLAY_DELAY;
}

void
GamePanelDelegate::BonusShowMultiplier()
{
	UIPanel *panel;
	UIElement *image;
	char numbuf[128];

	sound->PlaySound(gMultiplier, 5);

	panel = ui->GetPanel(PANEL_BONUS);
	SDL_snprintf(numbuf, sizeof(numbuf), "multiplier%d", TheShip->GetBonusMult());
	image = panel->GetElement<UIElement>(numbuf);
	if (image) {
		image->Show();
	}

	DelayAndDraw(60);

	m_state = STATE_BONUS_DISPLAY_DELAY;
}

void
GamePanelDelegate::BonusDisplayDelay()
{
	DelayAndDraw(SOUND_DELAY);
	m_state = STATE_BONUS_DISPLAY;
}

void
GamePanelDelegate::BonusDisplay()
{
	UIPanel *panel;
	UIElement *bonus;
	UIElement *score;
	char numbuf[128];

	sound->PlaySound(gFunk, 5);

	panel = ui->GetPanel(PANEL_BONUS);
	if (TheShip->GetBonusMult() != 1) {
		bonus = panel->GetElement<UIElement>("multiplied_bonus");
	} else {
		bonus = panel->GetElement<UIElement>("bonus");
	}
	score = panel->GetElement<UIElement>("score");

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
	DelayAndDraw(60);

	m_state = STATE_BONUS_CHECK_SOUND;
}

void
GamePanelDelegate::BonusCheckSound()
{
	/* -- Praise them or taunt them as the case may be */
	if (TheShip->GetBonus() == 0) {
		DelayAndDraw(SOUND_DELAY);
		m_state = STATE_BONUS_TAUNT;
		UnlockSinglePlayerAchievement("ACHIEVEMENT_BONUS_0");
		return;
	}
	if (TheShip->GetBonus() > 10000) {
		DelayAndDraw(SOUND_DELAY);
		m_state = STATE_BONUS_PRAISE;
		return;
	}
	DelaySound();
	m_state = STATE_BONUS_COUNTDOWN; 
}

void
GamePanelDelegate::BonusTaunt()
{
	sound->PlaySound(gNoBonus, 5);
	DelaySound();
	m_state = STATE_BONUS_COUNTDOWN; 
}

void
GamePanelDelegate::BonusPraise()
{
	sound->PlaySound(gPrettyGood, 5);
	DelaySound();
	m_state = STATE_BONUS_COUNTDOWN; 
}

void
GamePanelDelegate::BonusCountdown()
{
	UIPanel *panel;
	UIElement *bonus;
	UIElement *score;
	char numbuf[128];

	panel = ui->GetPanel(PANEL_BONUS);
	if (TheShip->GetBonusMult() != 1) {
		bonus = panel->GetElement<UIElement>("multiplied_bonus");
	}
	else {
		bonus = panel->GetElement<UIElement>("bonus");
	}
	score = panel->GetElement<UIElement>("score");

	if (TheShip->GetBonus() > 0) {
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
		DelaySound();
		return;
	}
	m_state = STATE_BONUS_NEXT_WAVE;
}

void
GamePanelDelegate::BonusNextWave()
{
	UIPanel *panel;
	UIElement *label;
	char numbuf[128];

	/* -- Draw the "next wave" message */
	panel = ui->GetPanel(PANEL_BONUS);
	label = panel->GetElement<UIElement>("next");
	if (label) {
		SDL_snprintf(numbuf, sizeof(numbuf), "Prepare for Wave %d...", gWave+1);
		label->SetText(numbuf);
		label->Show();
	}
	DelayAndDraw(6);
	m_state = STATE_BONUS_HIDE;
}

void
GamePanelDelegate::BonusHide()
{
	ui->HidePanel(PANEL_BONUS);

	gGameInfo.SetLocalState(STATE_BONUS, false);

	/* Fade out and prepare for drawing the next wave */
	screen->FadeOut();
	screen->Clear();

	m_state = STATE_START_NEXT_WAVE;
}

/* ----------------------------------------------------------------- */
/* -- Start the next wave! */

void
GamePanelDelegate::NextWave()
{
	gEnemySprite = NULL;

	/* -- Initialize some variables */
	gNumRocks = 0;
	gNumSmallRocksDestroyed = 0;
	gShakeTime = 0;
	gFreezeTime = 0;

	if (gWave > 0 && (gWave == 1 || (gWave % 5) == 0)) {
		char achievement[32];

		SDL_snprintf(achievement, sizeof(achievement), "ACHIEVEMENT_WAVE_%d", gWave);
		UnlockSinglePlayerAchievement(achievement);
	}

	if (gWave != (gGameInfo.wave - 1)) {
		DoBonus();
	} else {
		m_state = STATE_START_NEXT_WAVE;
	}
}

void
GamePanelDelegate::StartNextWave()
{
	int	i, x, y;
	int	NewRoids;
	short	temp;

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

	SetSteamTimelineLevelStarted(gWave);

	m_state = STATE_PLAYING;

}	/* -- NextWave */

/* ----------------------------------------------------------------- */
/* -- End the game */

void
GamePanelDelegate::GameOver()
{
	CloseSocket();

	DisableRemoteInput();

	ui->ShowPanel(PANEL_GAMEOVER);

	StopZoom();
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

	// Render the other sides of the sprite
	if (x < 0) {
		x += GAME_WIDTH;
		screen->QueueBlit(sprite->Texture(), x, y, w, h, DOCLIP);
	} else if ((x + w) > GAME_WIDTH) {
		x -= GAME_WIDTH;
		screen->QueueBlit(sprite->Texture(), x, y, w, h, DOCLIP);
	}
	if (y < 0) {
		y += GAME_HEIGHT;
		screen->QueueBlit(sprite->Texture(), x, y, w, h, DOCLIP);

		if (x < 0) {
			x += GAME_WIDTH;
			screen->QueueBlit(sprite->Texture(), x, y, w, h, DOCLIP);
		} else if ((x + w) > GAME_WIDTH) {
			x -= GAME_WIDTH;
			screen->QueueBlit(sprite->Texture(), x, y, w, h, DOCLIP);
		}
	} else if ((y + h) > GAME_HEIGHT) {
		y -= GAME_HEIGHT;
		screen->QueueBlit(sprite->Texture(), x, y, w, h, DOCLIP);

		if (x < 0) {
			x += GAME_WIDTH;
			screen->QueueBlit(sprite->Texture(), x, y, w, h, DOCLIP);
		} else if ((x + w) > GAME_WIDTH) {
			x -= GAME_WIDTH;
			screen->QueueBlit(sprite->Texture(), x, y, w, h, DOCLIP);
		}
	}
}
