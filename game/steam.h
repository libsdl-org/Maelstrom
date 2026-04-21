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

#ifndef _steam_h
#define _steam_h

#define SDL_EVENT_REMOTE_PLAYERS_CHANGED	(SDL_EVENT_USER + 1000)
#define SDL_EVENT_REMOTE_INPUT				(SDL_EVENT_USER + 1001)
#define SDL_EVENT_OVERLAY_ACTIVATED			(SDL_EVENT_USER + 1002)
#define SDL_EVENT_OVERLAY_DEACTIVATED		(SDL_EVENT_USER + 1003)

enum STEAM_TIMELINE_MODE
{
	STEAM_TIMELINE_NONE,
	STEAM_TIMELINE_LOADING,
	STEAM_TIMELINE_MENUS,
	STEAM_TIMELINE_PLAYING
};

enum STEAM_TIMELINE_EVENT
{
	STEAM_TIMELINE_EVENT_DEATH,
	STEAM_TIMELINE_EVENT_ENEMY,
	STEAM_TIMELINE_EVENT_MINE,
	STEAM_TIMELINE_EVENT_GRAVITY,
	STEAM_TIMELINE_EVENT_NOVA,
	STEAM_TIMELINE_EVENT_FREE_LIFE,
	STEAM_TIMELINE_EVENT_PRIZE_MACHINE_GUNS,
	STEAM_TIMELINE_EVENT_PRIZE_AIR_BRAKES,
	STEAM_TIMELINE_EVENT_PRIZE_LUCK,
	STEAM_TIMELINE_EVENT_PRIZE_TRIPLE_FIRE,
	STEAM_TIMELINE_EVENT_PRIZE_LONG_RANGE,
	STEAM_TIMELINE_EVENT_PRIZE_SHIELDS,
	STEAM_TIMELINE_EVENT_PRIZE_FREEZING,
	STEAM_TIMELINE_EVENT_PRIZE_EXPLOSION,
};

typedef Uint32 RemotePlaySessionID_t;

extern bool InitSteam();
extern bool SteamStreamingToPhone();
extern bool SteamStreamingToTablet();
extern bool GamepadInputFromMobileTouchController(SDL_Gamepad *gamepad);
extern RemotePlaySessionID_t GetRemoteSessionForGamepad(SDL_Gamepad *gamepad);
extern Uint8 GetRemoteSessionControl(RemotePlaySessionID_t sessionID);
extern const char *GetRemotePlayerName(Uint8 controlType);
extern SDL_Surface* GetRemotePlayerAvatar(Uint8 controlType);
extern const bool *GetRemotePlayerKeyboardState(Uint8 controlType);
extern void EnableRemoteInput();
extern void DisableRemoteInput();
extern void SetSteamTimelineMode(STEAM_TIMELINE_MODE mode);
extern void SetSteamTimelineLevelStarted(int level);
extern void AddSteamTimelineEvent(STEAM_TIMELINE_EVENT event);
extern void UnlockAchievement(const char *achievement);
extern void UpdateSteam();
extern void QuitSteam();

#endif // _steam_h
