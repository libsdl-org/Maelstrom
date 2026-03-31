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

/* ------------------------------------------------------------- */
/* 								 */
/* Maelstrom							 */
/* By Andrew Welch						 */
/* 								 */
/* Ported to Linux  (Spring 1995)				 */
/* Ported to Win95  (Fall   1996) -- not releasable		 */
/* Ported to SDL    (Fall   1997)                                */
/* By Sam Lantinga  (slouken@libsdl.org)			 */
/* 								 */
/* ------------------------------------------------------------- */

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>

#include "Maelstrom_Globals.h"
#include "load.h"
#include "init.h"
#include "fastrand.h"
#include "about.h"
#include "game.h"
#include "netplay.h"
#include "main.h"

#include "../utils/files.h"
#include "../screenlib/UIDialog.h"
#include "../screenlib/UIElement.h"
#include "../screenlib/UIElementCheckbox.h"
#include "../screenlib/UIElementEditbox.h"

#define MAELSTROM_ORGANIZATION	"Ambrosia Software"
#define MAELSTROM_NAME		"Maelstrom"

static const char *Version =
"Maelstrom v1.4.3 (GPL version 4.0.0) -- 10/08/2011 by Sam Lantinga\n";

// Global variables set in this file...
Bool	gInitializing = false;
Bool	gControlBrakes = false;
Bool	gNetworkAvailable = false;
Bool	gUpdateBuffer = false;
Bool	gDelaySound = false;
int		gDelayTicks = 0;
Bool	gRunning = true;


// Main Menu actions:
static void RunSinglePlayerGame()
{
	NewGame();
}

static void RunReplayGame(const char *file)
{
	if (!gReplay.Load(file)) {
		return;
	}

	gReplay.SetMode(REPLAY_PLAYBACK);
	RunSinglePlayerGame();
}

static void CheatDialogInit(void*, UIDialog *dialog)
{
	UIElementEditbox *editbox;

	editbox = dialog->GetElement<UIElementEditbox>("level");
	if (editbox) {
		editbox->SetFocus(true);
	}
}
static void CheatDialogDone(void*, UIDialog *dialog, int status)
{
	UIElementEditbox *editbox;
	UIElementCheckbox *checkbox;
	Uint8 wave = DEFAULT_START_WAVE;
	Uint8 lives = DEFAULT_START_LIVES;
	Uint8 turbo = DEFAULT_START_TURBO;

	if (status > 0) {
		editbox = dialog->GetElement<UIElementEditbox>("level");
		if (editbox) {
			wave = editbox->GetNumber();
			if (wave < 1 || wave > 40) {
				wave = DEFAULT_START_WAVE;
			}
		}

		editbox = dialog->GetElement<UIElementEditbox>("lives");
		if (editbox) {
			lives = editbox->GetNumber();
			if (lives < 1 || lives > 40) {
				lives = DEFAULT_START_LIVES;
			}
		}

		checkbox = dialog->GetElement<UIElementCheckbox>("turbofunk");
		if (checkbox) {
			turbo = checkbox->IsChecked();
		}

		Delay(SOUND_DELAY);
		sound->PlaySound(gNewLife, 5);
		Delay(SOUND_DELAY);
		gGameInfo.SetHost(wave, lives, turbo, 0, prefs->GetBool(PREFERENCES_KIDMODE));
		gGameInfo.SetPlayerSlot(0, prefs->GetString(PREFERENCES_HANDLE), CONTROL_LOCAL);
		RunSinglePlayerGame();
	}
}


/* ----------------------------------------------------------------- */
/* -- Print a Usage message and quit.
      In several places we depend on this function exiting.
 */
void PrintUsage(const char *progname)
{
	SDL_Log("Usage: %s <options>", progname);
	SDL_Log("Where <options> can be any of:\n"
"    --fullscreen      # Run Maelstrom in full-screen mode\n"
"    --windowed        # Run Maelstrom in windowed mode\n"
"    --geometry WxH    # Set the window size to WxH\n"
"    --control-brakes  # Allow manual brake control\n"
	);
}

#if !SDL_VERSION_ATLEAST(3, 5, 0)
static bool SDL_IsPhone(void)
{
#if defined(SDL_PLATFORM_ANDROID) || \
    (defined(SDL_PLATFORM_IOS) && !defined(SDL_PLATFORM_VISIONOS))
    if (!SDL_IsTablet() && !SDL_IsTV()) {
        return true;
    }
#endif
    return false;
}
#endif // SDL < 3.5.0

bool IsPhone(void)
{
	return (SDL_IsPhone() || SteamStreamingToPhone());
}

bool IsTablet(void)
{
	return (SDL_IsTablet() || SteamStreamingToTablet());
}

/* ----------------------------------------------------------------- */
/* -- Blitter main program */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	/* Command line flags */
	int window_width = 0;
	int window_height = 0;
	Uint32 window_flags = SDL_WINDOW_RESIZABLE;
#ifdef SDL_PLATFORM_EMSCRIPTEN
	window_flags |= SDL_WINDOW_FILL_DOCUMENT;
#else
	window_flags |= SDL_WINDOW_FULLSCREEN;
#endif
	window_flags |= SDL_WINDOW_HIGH_PIXEL_DENSITY;

	/* Initializing Steam can set up environment variables, so do this first */
	InitSteam();

	if ( !InitFilesystem(MAELSTROM_ORGANIZATION, MAELSTROM_NAME) ) {
		return SDL_APP_FAILURE;
	}

	/* Seed the random number generator */
	SeedRandom(0L);

	/* Parse command line arguments */
	for ( int i = 1; i < argc; ++i ) {
		if ( strcmp(argv[i], "--fullscreen") == 0 ) {
			window_flags |= SDL_WINDOW_FULLSCREEN;
		} else if ( strcmp(argv[i], "--windowed") == 0 ) {
			window_flags &= ~SDL_WINDOW_FULLSCREEN;
		} else if ( strcmp(argv[i], "--geometry") == 0 && argv[i+1]) {
			++i;
			if (SDL_sscanf(argv[i], "%dx%d", &window_width, &window_height) != 2) {
				PrintUsage(argv[0]);
				return SDL_APP_FAILURE;
			}
		} else if ( strcmp(argv[i], "--control-brakes") == 0 ) {
			gControlBrakes = true;
		} else if ( strcmp(argv[i], "-NSDocumentRevisionsDebugMode") == 0 && argv[i+1] ) {
			// Ignore Xcode debug option
			++i;
		} else if ( strcmp(argv[i], "--version") == 0 ) {
			error("%s", Version);
			return SDL_APP_SUCCESS;
		} else {
			PrintUsage(argv[0]);
			return SDL_APP_FAILURE;
		}
	}

	/* Initialize everything. :) */
	if (!StartInitialization(window_width, window_height, window_flags)) {
		/* An error message was already printed */
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_DROP_FILE) {
		SDL_free(gReplayFile);
		gReplayFile = SDL_strdup(event->drop.data);
	}

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	screen->ProcessEvent(event);

	HandleEvent(event);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	while (!screen->Fading()) {

		if (gDelaySound) {
			if (sound->Playing()) {
				ui->Draw(false);
				Delay(2);
				break;
			}
			gDelaySound = false;
		}

		if (gDelayTicks) {
			int ticks = SDL_min(gDelayTicks, 2);
			ui->Draw(false);
			Delay(ticks);
			gDelayTicks -= ticks;
			break;
		}

		if (gInitializing) {
			if (ContinueInitialization()) {
				ui->Draw();
				Delay(2);
				break;
			} else {
				return SDL_APP_FAILURE;
			}
		}

		ui->Draw();

		if (!gGameOn) {
			// If we got a replay event, start it up!
			if (gReplayFile) {
				RunReplayGame(gReplayFile);
				SDL_free(gReplayFile);
				gReplayFile = nullptr;
			}
		}

		UpdateSteam();

		DelayFrame();

		break;
	}

	if (screen->Fading()) {
		screen->FadeStep();
	}

	if (gRunning) {
		return SDL_APP_CONTINUE;
	} else {
		return SDL_APP_SUCCESS;
	}
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	SDL_free(gReplayFile);

	CleanUp();

	QuitSteam();
}


/* ----------------------------------------------------------------- */
/* -- Setup the main screen */

bool
MainPanelDelegate::OnLoad()
{
	UIElement *label;

	/* Set the version */
	label = m_panel->GetElement<UIElement>("version");
	if (label) {
		label->SetText(VERSION_STRING);
	}

	return true;
}

void
MainPanelDelegate::OnShow()
{
	SetSteamTimelineMode(STEAM_TIMELINE_MENUS);

	gUpdateBuffer = true;
}

void
MainPanelDelegate::OnTick()
{
	UIElement *label;
	char name[32];
	char text[128];

	if (m_bQuitting && !sound->Playing()) {
		gRunning = false;
	}

	if (!gUpdateBuffer) {
		return;
	}
	gUpdateBuffer = false;

	for (int index = 0; index < NUM_SCORES; index++) {
		Uint8 R, G, B;

		if ( gLastHigh == index ) {
			R = 0xFF;
			G = 0xFF;
			B = 0xFF;
		} else {
			R = 30000>>8;
			G = 30000>>8;
			B = 30000>>8;
		}

		SDL_snprintf(name, sizeof(name), "name_%d", index);
		label = m_panel->GetElement<UIElement>(name);
		if (label) {
			label->SetColor(R, G, B);
			label->SetText(hScores[index].name);
		}

		SDL_snprintf(name, sizeof(name), "score_%d", index);
		label = m_panel->GetElement<UIElement>(name);
		if (label) {
			label->SetColor(R, G, B);
			SDL_snprintf(text, sizeof(text), "%d", hScores[index].score);
			label->SetText(text);
		}

		SDL_snprintf(name, sizeof(name), "wave_%d", index);
		label = m_panel->GetElement<UIElement>(name);
		if (label) {
			label->SetColor(R, G, B);
			SDL_snprintf(text, sizeof(text), "%d", hScores[index].wave);
			label->SetText(text);
		}
	}

	label = m_panel->GetElement<UIElement>("last_score");
	if (label) {
		if (gReplay.Load(LAST_REPLAY, true)) {
			SDL_snprintf(text, sizeof(text), "%d", gReplay.GetFinalScore());
			label->SetText(text);
		} else {
			label->SetText("0");
		}
	}

	label = m_panel->GetElement<UIElement>("volume");
	if (label) {
		SDL_snprintf(text, sizeof(text), "%d", gSoundLevel.Value());
		label->SetText(text);
	}
}

bool
MainPanelDelegate::HandleEvent(const SDL_Event &event)
{
	/* -- Handle file drop requests */
	if ( event.type == SDL_EVENT_DROP_FILE ) {
		gReplayFile = SDL_strdup( event.drop.data );
		return true;
	}

	return false;
}

bool
MainPanelDelegate::OnAction(UIBaseElement *sender, const char *action)
{
	if (SDL_strcmp(action, "play") == 0) {
		OnActionPlay();
	} else if (SDL_strcmp(action, "multiplayer") == 0) {
		OnActionMultiplayer();
	} else if (SDL_strcmp(action, "quit") == 0) {
		OnActionQuitGame();
	} else if (SDL_strcmp(action, "volume_down") == 0) {
		OnActionVolumeDown();
	} else if (SDL_strcmp(action, "volume_up") == 0) {
		OnActionVolumeUp();
	} else if (SDL_strncmp(action, "setvolume", 9) == 0) {
		OnActionSetVolume(SDL_atoi(action+9));
	} else if (SDL_strcmp(action, "toggle_kidmode") == 0) {
		OnActionToggleKidMode(sender);
	} else if (SDL_strcmp(action, "toggle_fullscreen") == 0) {
		OnActionToggleFullscreen();
	} else if (SDL_strcmp(action, "screenshot") == 0) {
		OnActionScreenshot();
	} else if (SDL_strcmp(action, "cheat") == 0) {
		OnActionCheat();
	} else if (SDL_strcmp(action, "play_last") == 0) {
		OnActionRunLastReplay();
	} else if (SDL_strncmp(action, "play_", 5) == 0) {
		OnActionRunReplay(SDL_atoi(action+5));
	} else if (SDL_strcmp(action, "zap") == 0) {
		OnActionZapHighScores();
	} else {
		return false;
	}
	return true;
}

void
MainPanelDelegate::OnActionPlay()
{
	gGameInfo.SetHost(DEFAULT_START_WAVE, DEFAULT_START_LIVES, DEFAULT_START_TURBO, 0, prefs->GetBool(PREFERENCES_KIDMODE));
	gGameInfo.SetPlayerSlot(0, prefs->GetString(PREFERENCES_HANDLE), CONTROL_LOCAL);
	RunSinglePlayerGame();
}

void
MainPanelDelegate::OnActionMultiplayer()
{
	ui->ShowPanel(DIALOG_LOBBY);
}

void
MainPanelDelegate::OnActionQuitGame()
{
	m_bQuitting = true;
}

void
MainPanelDelegate::OnActionVolumeDown()
{
	if ( gSoundLevel > 0 ) {
		sound->Volume(--gSoundLevel);
		sound->PlaySound(gNewLife, 5);

		/* -- Draw the new sound level */
		gUpdateBuffer = true;
	}
}

void
MainPanelDelegate::OnActionVolumeUp()
{
	if ( gSoundLevel < 8 ) {
		sound->Volume(++gSoundLevel);
		sound->PlaySound(gNewLife, 5);

		/* -- Draw the new sound level */
		gUpdateBuffer = true;
	}
}

void
MainPanelDelegate::OnActionSetVolume(int volume)
{
	/* Make sure the device is working */
	sound->Volume(volume);

	/* Set the new sound level! */
	gSoundLevel = volume;
	sound->PlaySound(gNewLife, 5);

	/* -- Draw the new sound level */
	gUpdateBuffer = true;
}

void
MainPanelDelegate::OnActionToggleFullscreen()
{
	screen->ToggleFullScreen();
}

void
MainPanelDelegate::OnActionToggleKidMode(UIBaseElement *sender)
{
	UIElementCheckbox *checkbox = sender->Cast<UIElementCheckbox>();

	if (!checkbox) {
		return;
	}

	checkbox->SaveData(prefs);
}

void
MainPanelDelegate::OnActionScreenshot()
{
    // Dump just the score screen
    screen->ScreenDump("ScoreDump", 64, 48, 298, 384);
}

void
MainPanelDelegate::OnActionCheat()
{
	UIDialog *dialog;

	dialog = ui->GetPanel<UIDialog>(DIALOG_CHEAT);
	if (dialog) {
		dialog->SetDialogInitHandler(CheatDialogInit);
		dialog->SetDialogDoneHandler(CheatDialogDone);

		ui->ShowPanel(dialog);
	}
}

void
MainPanelDelegate::OnActionZapHighScores()
{
	ZapHighScores();

	// Fade the screen and redisplay scores
	if (ui->GetPanelTransition() == PANEL_TRANSITION_FADE) {
		screen->FadeOut();
		Delay(SOUND_DELAY);
	}
	sound->PlaySound(gExplosionSound, 5);
	gUpdateBuffer = true;
}

void
MainPanelDelegate::OnActionRunLastReplay()
{
	RunReplayGame(LAST_REPLAY);
}

void
MainPanelDelegate::OnActionRunReplay(int index)
{
	const char *file = hScores[index].file;
	if (file) {
		RunReplayGame(file);
	}
}


void DelayFrame(void)
{
	Uint64 ticks, delay = FRAME_DELAY_MS;

	if (gGameInfo.turbo) {
		// Turbo is 2x as fast
		delay /= 2;
	}

	while (((ticks=SDL_GetTicks())-gLastDrawn) < delay) {
		ui->Poll();
		SDL_Delay(1);
	}
	gLastDrawn = ticks;
}

void DelaySound()
{
	gDelaySound = true;
}

void DelayAndDraw(int ticks)
{
	gDelayTicks = ticks;
}
