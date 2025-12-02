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

#ifndef _steam_h
#define _steam_h

#define SDL_EVENT_REMOTE_PLAYERS_CHANGED	(SDL_EVENT_USER + 0)
#define SDL_EVENT_REMOTE_INPUT				(SDL_EVENT_USER + 1)

typedef Uint32 RemotePlaySessionID_t;

extern bool InitSteam();
extern RemotePlaySessionID_t GetRemoteSessionForGamepad(SDL_Gamepad *gamepad);
extern Uint8 GetRemoteSessionControl(RemotePlaySessionID_t sessionID);
extern const char *GetRemotePlayerName(Uint8 controlType);
extern const bool *GetRemotePlayerKeyboardState(Uint8 controlType);
extern void EnableRemoteInput();
extern void DisableRemoteInput();
extern void UpdateSteam();
extern void QuitSteam();

#endif // _steam_h
