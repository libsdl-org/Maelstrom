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

#include "Maelstrom_Globals.h"
#include "load.h"
#include "init.h"
#include "fastrand.h"
#include "about.h"
#include "game.h"
#include "netplay.h"
#include "main.h"

#include "../physfs/physfs.h"

#include "../screenlib/UIDialog.h"
#include "../screenlib/UIElement.h"
#include "../screenlib/UIElementCheckbox.h"
#include "../screenlib/UIElementEditbox.h"

#if __IPHONEOS__
#include "../Xcode-iOS/Maelstrom_GameKit.h"
#endif

#define MAELSTROM_ORGANIZATION	"AmbrosiaSW"
#define MAELSTROM_NAME		"Maelstrom"
#define MAELSTROM_DATA	"Maelstrom_Data.zip"

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
	error("Usage: %s <options>\n\n", progname);
	error("Where <options> can be any of:\n\n"
"	-fullscreen		# Run Maelstrom in full-screen mode\n"
"	-windowed		# Run Maelstrom in windowed mode\n"
"	-classic		# Run Maelstrom in 640x480 classic mode\n"
	);
	error("\n");
	exit(1);
}

/* ----------------------------------------------------------------- */
/* -- Initialize PHYSFS and mount the data archive */
static bool
InitFilesystem(const char *argv0)
{
	const char *prefspath;

	if (!PHYSFS_init(argv0)) {
		error("Couldn't initialize PHYSFS: %s\n", PHYSFS_getLastError());
		return false;
	}

	// Set up the write directory for this platform
#ifdef __ANDROID__
	prefspath = SDL_AndroidGetInternalStoragePath();
#else
	prefspath = PHYSFS_getPrefDir(MAELSTROM_ORGANIZATION, MAELSTROM_NAME);
#endif
	if (!prefspath) {
		error("Couldn't get preferences path for this platform\n");
		return false;
	}
	if (!PHYSFS_setWriteDir(prefspath)) {
		error("Couldn't set write directory to %s: %s\n", prefspath, PHYSFS_getLastError());
		return false;
	}

	/* Put the write directory first in the search path */
	PHYSFS_mount(prefspath, NULL, 0);

#ifdef __ANDROID__
	// We'll use SDL's asset manager code path for Android
	return true;

#else
	/* Then add the base directory to the search path */
	PHYSFS_mount(PHYSFS_getBaseDir(), NULL, 0);

	/* Then add the data file, which could be appended to the executable */
	if (PHYSFS_mount(argv0, "/", 1)) {
		return true;
	}

	/* ... or not */
	char path[4096];
	SDL_snprintf(path, SDL_arraysize(path), "%s%s", PHYSFS_getBaseDir(), MAELSTROM_DATA);
	if (PHYSFS_mount(path, "/", 1)) {
		return true;
	}

	SDL_snprintf(path, SDL_arraysize(path), "%sContents/Resources/%s", PHYSFS_getBaseDir(), MAELSTROM_DATA);
	if (PHYSFS_mount(path, "/", 1)) {
		return true;
	}

	error("Couldn't find %s", MAELSTROM_DATA);
	return false;
#endif // __ANDROID__
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
int MaelstromMain(int argc, char *argv[])
{
	/* Command line flags */
	Uint32 window_flags = 0;
	Uint32 render_flags = SDL_RENDERER_PRESENTVSYNC;

	if ( !InitFilesystem(argv[0]) ) {
		exit(1);
	}

	/* Seed the random number generator */
	SeedRandom(0L);

	/* Parse command line arguments */
#if /*!defined(FAST_ITERATION) ||*/ defined(__IPHONEOS__)
	window_flags |= SDL_WINDOW_FULLSCREEN;
#endif
	for ( progname=argv[0]; --argc; ++argv ) {
		if ( strcmp(argv[1], "-fullscreen") == 0 ) {
			window_flags |= SDL_WINDOW_FULLSCREEN;
		} else if ( strcmp(argv[1], "-windowed") == 0 ) {
			window_flags &= ~SDL_WINDOW_FULLSCREEN;
		} else if ( strcmp(argv[1], "-classic") == 0 ) {
			gClassic = true;
		} else if ( strcmp(argv[1], "-version") == 0 ) {
			error("%s", Version);
			exit(0);
		} else if ( strcmp(argv[1], "-help") == 0 ) {
			PrintUsage();
		}
	}

	/* Initialize everything. :) */
	if ( DoInitializations(window_flags, render_flags) < 0 ) {
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
	SDL_iPhoneSetAnimationCallback(screen->GetWindow(), 2, ShowFrame, 0);
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
	/* -- Handle store events */
	if (store->HandleEvent(event)) {
		return true;
	}

	/* -- Handle file drop requests */
	if ( event.type == SDL_DROPFILE ) {
		gReplayFile = event.drop.file;
		return true;
	}

	/* -- Handle window close requests */
	if ( event.type == SDL_QUIT ) {
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
	} else if (SDL_strcmp(action, "multiplayer_activated") == 0) {
		OnActionMultiplayerActivated();
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
	} else if (SDL_strcmp(action, "kidmode_activated") == 0) {
		OnActionKidModeActivated();
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
	store->ActivateFeature(FEATURE_NETWORK, "multiplayer_activated");
}

void
MainPanelDelegate::OnActionMultiplayerActivated()
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

	if (checkbox->IsChecked()) {
		if (!store->HasFeature(FEATURE_KIDMODE)) {
			checkbox->SetChecked(false);
			store->ActivateFeature(FEATURE_KIDMODE, "kidmode_activated");
		}
	}
	checkbox->SaveData(prefs);
}

void
MainPanelDelegate::OnActionKidModeActivated()
{
	UIElementCheckbox *checkbox = m_panel->GetElement<UIElementCheckbox>("kidmode");

	if (!checkbox) {
		return;
	}
	checkbox->SetChecked(true);
}

void
MainPanelDelegate::OnActionScreenshot()
{
	if (gClassic) {
		// Dump just the score screen
		screen->ScreenDump("ScoreDump", 64, 48, 298, 384);
	} else {
		// Get a screenshot of our lovely new main screen
		screen->ScreenDump("ScreenShot", 0, 0, 0, 0);
	}
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
	Uint32 ticks;

	while ( ((ticks=SDL_GetTicks())-gLastDrawn) < FRAME_DELAY_MS ) {
		ui->Poll();
		SDL_Delay(1);
	}
	gLastDrawn = ticks;
}
