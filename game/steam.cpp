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

#include "steam.h"

#ifdef ENABLE_STEAM

#include <SDL3/SDL.h>
#define strncpy	SDL_strlcpy

#include <steam/steam_api.h>

class SteamCallbacks
{
public:
	SteamCallbacks() { }
	~SteamCallbacks() { }

private:
	STEAM_CALLBACK(SteamCallbacks, OnRemotePlaySessionConnected, SteamRemotePlaySessionConnected_t);
	STEAM_CALLBACK(SteamCallbacks, OnRemotePlaySessionDisconnected, SteamRemotePlaySessionDisconnected_t);
};
static SteamCallbacks steam;

void SteamCallbacks::OnRemotePlaySessionConnected(SteamRemotePlaySessionConnected_t *pParam)
{
}

void SteamCallbacks::OnRemotePlaySessionDisconnected(SteamRemotePlaySessionDisconnected_t *pParam)
{
}


bool InitSteam()
{
	SDL_setenv_unsafe("SteamAppId", "480", true);

	SteamErrMsg error;
	if (SteamAPI_InitEx(&error) != k_ESteamAPIInitResult_OK) {
		SDL_Log("Couldn't initialize Steam: %s", error);
		return false;
	}

    // Enable passing remote mouse and keyboard into our application
    SteamRemotePlay()->BEnableRemotePlayTogetherDirectInput();

	return true;
}

void UpdateSteam()
{
	SteamAPI_RunCallbacks();

    uint32 count;
    RemotePlayInput_t events[32];
    while ((count = SteamRemotePlay()->GetInput(events, SDL_arraysize(events))) > 0) {
        // Process remote input
    }
}

void QuitSteam()
{
	SteamAPI_Shutdown();
}

#else

bool InitSteam()
{
	return false;
}

void UpdateSteam()
{
}

void QuitSteam()
{
}

#endif // ENABLE_STEAM
