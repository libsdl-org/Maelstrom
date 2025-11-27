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

#if __IPHONEOS__
#include "../Xcode-iOS/Maelstrom_GameKit.h"
#endif

#define MAELSTROM_ORGANIZATION	"Ambrosia Software"
#define MAELSTROM_NAME		"Maelstrom"

static const char *Version =
"Maelstrom v1.4.3 (GPL version 4.0.0) -- 10/08/2011 by Sam Lantinga\n";

// Global variables set in this file...
Bool	gUpdateBuffer;
Bool	gRunning;


// Main Menu actions:
static void RunSinglePlayerGame()
{
	if (InitNetData(false) < 0) {
		return;
	}
	NewGame();
	HaltNetData();
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
static char *progname;
void PrintUsage(void)
{
	SDL_Log("Usage: %s <options>", progname);
	SDL_Log("Where <options> can be any of:\n"
"	--fullscreen		# Run Maelstrom in full-screen mode\n"
"	--windowed		# Run Maelstrom in windowed mode\n"
	);
	exit(1);
}

/* ----------------------------------------------------------------- */
extern "C" void ShowFrame(void*);
extern "C"
void ShowFrame(void*)
{
	ui->Draw();

	if (!gGameOn) {
		// If we got a reply event, start it up!
		if (gReplayFile) {
			RunReplayGame(gReplayFile);
			SDL_free(gReplayFile);
			gReplayFile = NULL;
		}

		/* -- Get events */
		SDL_Event event;
		while ( screen->PollEvent(&event) ) {
			if ( ui->HandleEvent(event) )
				continue;
		}
	}
}

/* ----------------------------------------------------------------- */
/* -- Blitter main program */
int main(int argc, char *argv[])
{
	/* Command line flags */
	Uint32 window_flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_RESIZABLE;

	if ( !InitFilesystem(MAELSTROM_ORGANIZATION, MAELSTROM_NAME) ) {
		exit(1);
	}

	/* Seed the random number generator */
	SeedRandom(0L);

	/* Parse command line arguments */
#if /*!defined(FAST_ITERATION) ||*/ defined(__IPHONEOS__)
	window_flags |= SDL_WINDOW_FULLSCREEN;
#endif
	for ( progname=argv[0]; --argc; ++argv ) {
		if ( strcmp(argv[1], "--fullscreen") == 0 ) {
			window_flags |= SDL_WINDOW_FULLSCREEN;
		} else if ( strcmp(argv[1], "--windowed") == 0 ) {
			window_flags &= ~SDL_WINDOW_FULLSCREEN;
		} else if ( strcmp(argv[1], "--version") == 0 ) {
			error("%s", Version);
			exit(0);
		} else {
			PrintUsage();
		}
	}

	/* Initialize everything. :) */
	if ( DoInitializations(window_flags) < 0 ) {
		/* An error message was already printed */
		CleanUp();
		exit(1);
	}

	gRunning = true;

#ifdef WAIT_BOOM  // Don't wait for the boom, since we don't fade on iOS
	DropEvents();

#ifndef FAST_ITERATION
	while ( sound->Playing() )
		Delay(SOUND_DELAY);
#endif
#endif // WAIT_BOOM

	ui->ShowPanel(PANEL_MAIN);

#if __IPHONEOS__
	// Initialize the Game Center for scoring and matchmaking
	InitGameCenter();

	// Set up the game to run in the window animation callback on iOS
	// so that Game Center and so forth works correctly.
	SDL_SetiOSAnimationCallback(screen->GetWindow(), 2, ShowFrame, 0);
#else
	while ( gRunning ) {
		ShowFrame(0);

		if (!gGameOn || !gGameInfo.turbo) {
			DelayFrame();
		}
	}
	CleanUp();
#endif
	return 0;

}	/* -- main */


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

	m_spinnerImage = NULL;

	UIElement *m_spinner = m_panel->GetElement<UIElement>("spinner");
	if (m_spinner) {
		m_spinnerImage = m_spinner->GetImage();
	}

	return true;
}

void
MainPanelDelegate::OnShow()
{
	gUpdateBuffer = true;
}

void
MainPanelDelegate::OnTick()
{
	UIElement *label;
	char name[32];
	char text[128];

	// Rotate the spinner, if needed
	if (m_spinnerImage) {
		float angle = m_spinnerImage->Angle();
		const float FPS = (60.0f / FRAME_DELAY);
		const float SPINNER_RATE = 60.0f; // seconds per rotation
		const float increment = 360.0f / (SPINNER_RATE * FPS);
		m_spinnerImage->SetAngle(angle + increment);
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

	/* -- Handle window close requests */
	if ( event.type == SDL_EVENT_QUIT ) {
		OnActionQuitGame();
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
	while ( sound->Playing() )
		Delay(SOUND_DELAY);
	gRunning = false;
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
	Uint64 ticks;

	while ( ((ticks=SDL_GetTicks())-gLastDrawn) < FRAME_DELAY_MS ) {
		ui->Poll();
		SDL_Delay(1);
	}
	gLastDrawn = ticks;
}
