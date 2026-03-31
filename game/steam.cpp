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

#include <SDL3/SDL.h>

#include "steam.h"
#include "Localization.h"

#ifdef ENABLE_STEAM

#define strncpy	SDL_strlcpy

#include <steam/steam_api.h>

#include "../utils/array.h"

#include "gameinfo.h"


class SteamInterface
{
public:
	SteamInterface() { }
	~SteamInterface() { }

	bool Init();
	void Quit();

	bool StreamingToPhone();
	bool StreamingToTablet();

	void SetSteamTimelineMode(STEAM_TIMELINE_MODE mode);
	void SetSteamTimelineLevelStarted(int level);
	void AddSteamTimelineEvent(STEAM_TIMELINE_EVENT event);
	void UnlockAchievement(const char *achievement);

	void Update();

	Uint8 GetControlForSession(RemotePlaySessionID_t sessionID);

	RemotePlaySessionID_t GetRemoteSessionForGamepad(SDL_Gamepad *gamepad);

	const char *GetRemotePlayerName(Uint8 controlType);
	SDL_Surface *GetRemotePlayerAvatar(Uint8 controlType);
	const bool *GetRemotePlayerKeyboardState(Uint8 controlType);

	void EnableRemoteInput();
	void DisableRemoteInput();

private:
	struct RemoteSession_t
	{
		RemotePlaySessionID_t id;
		CSteamID steamID;
		char *name;
		bool keystate[SDL_SCANCODE_COUNT];
	};
	RemoteSession_t *GetSession(RemotePlaySessionID_t sessionID);
	RemoteSession_t *GetSessionForControl(Uint8 controlType);
	bool IsRemotePlayTogether(RemotePlaySessionID_t sessionID);

	void UpdatePlayers();
	void ProcessInput(const RemotePlayInput_t &input);

private:
	bool m_initialized = false;
	STEAM_TIMELINE_MODE m_gameMode = STEAM_TIMELINE_NONE;
	CSteamID m_steamID;

	array<RemoteSession_t *> m_sessions;
	RemoteSession_t *m_players[MAX_PLAYERS - 1] = { };

	STEAM_CALLBACK(SteamInterface, OnRemotePlaySessionConnected, SteamRemotePlaySessionConnected_t);
	STEAM_CALLBACK(SteamInterface, OnRemotePlaySessionAvatarLoaded, SteamRemotePlaySessionAvatarLoaded_t);
	STEAM_CALLBACK(SteamInterface, OnRemotePlaySessionDisconnected, SteamRemotePlaySessionDisconnected_t);
};
static SteamInterface steam;


bool SteamInterface::Init()
{
	SDL_setenv_unsafe("SteamAppId", "4239950", true);

	SteamErrMsg error;
	if (SteamAPI_InitEx(&error) != k_ESteamAPIInitResult_OK) {
		SDL_Log("Couldn't initialize Steam: %s", error);
		return false;
	}
	m_initialized = true;
	m_steamID = SteamUser()->GetSteamID();
	SteamController()->Init();

	SetSteamTimelineMode(STEAM_TIMELINE_LOADING);

	return true;
}

void SteamInterface::Quit()
{
	if (!m_initialized) {
		return;
	}

	for (unsigned int i = 0; i < m_sessions.length(); ++i) {
		delete m_sessions[i];
	}
	m_sessions.clear();

	SDL_zeroa(m_players);

	SteamController()->Shutdown();
	SteamAPI_Shutdown();
	m_initialized = false;
}

bool SteamInterface::StreamingToPhone()
{
	if (!m_initialized) {
		return false;
	}

	ISteamRemotePlay *pSteamRemotePlay = SteamRemotePlay();
	for (uint32 i = 0; i < pSteamRemotePlay->GetSessionCount(); ++i) {
		RemotePlaySessionID_t sessionID = pSteamRemotePlay->GetSessionID(i);

		// Skip Remote Play Together sessions
		if (pSteamRemotePlay->BSessionRemotePlayTogether(sessionID)) {
			continue;
		}

		ESteamDeviceFormFactor eFormFactor = pSteamRemotePlay->GetSessionClientFormFactor(sessionID);
		if (eFormFactor == k_ESteamDeviceFormFactorPhone) {
			return true;
		}
	}
	return false;
}

bool SteamInterface::StreamingToTablet()
{
	if (!m_initialized) {
		return false;
	}

	ISteamRemotePlay *pSteamRemotePlay = SteamRemotePlay();
	for (uint32 i = 0; i < pSteamRemotePlay->GetSessionCount(); ++i) {
		RemotePlaySessionID_t sessionID = pSteamRemotePlay->GetSessionID(i);

		// Skip Remote Play Together sessions
		if (pSteamRemotePlay->BSessionRemotePlayTogether(sessionID)) {
			continue;
		}

		ESteamDeviceFormFactor eFormFactor = pSteamRemotePlay->GetSessionClientFormFactor(sessionID);
		if (eFormFactor == k_ESteamDeviceFormFactorTablet) {
			return true;
		}
	}
	return false;
}

void SteamInterface::SetSteamTimelineMode(STEAM_TIMELINE_MODE mode)
{
	if (!m_initialized) {
		return;
	}

	if (mode != m_gameMode) {
		switch (mode) {
		case STEAM_TIMELINE_LOADING:
			SteamTimeline()->SetTimelineGameMode(k_ETimelineGameMode_LoadingScreen);
			break;
		case STEAM_TIMELINE_MENUS:
			SteamTimeline()->ClearTimelineTooltip(0.0f);
			SteamTimeline()->SetTimelineGameMode(k_ETimelineGameMode_Menus);
			SteamFriends()->SetRichPresence( "steam_display", "" );
			break;
		case STEAM_TIMELINE_PLAYING:
			SteamTimeline()->SetTimelineGameMode(k_ETimelineGameMode_Playing);
			break;
		default:
			break;
		}
		m_gameMode = mode;
	}
}

void SteamInterface::SetSteamTimelineLevelStarted(int level)
{
	if (!m_initialized) {
		return;
	}

	char icon[32];
	SDL_snprintf(icon, sizeof(icon), "steam_%d", level);

	char wave[32];
	SDL_snprintf(wave, sizeof(wave), "Wave %d", level);

	SteamTimeline()->AddInstantaneousTimelineEvent("Next Wave", nullptr, icon, 0, 0.0f, k_ETimelineEventClipPriority_None);
	SteamTimeline()->SetTimelineTooltip(wave, 0.0f);

	SDL_snprintf(wave, sizeof(wave), "%d", level);
	SteamFriends()->SetRichPresence( "wave", wave );
	SteamFriends()->SetRichPresence( "steam_display", "#StatusPlaying" );
}

void SteamInterface::AddSteamTimelineEvent(STEAM_TIMELINE_EVENT event)
{
	if (!m_initialized) {
		return;
	}

	const char *title = nullptr;
	const char *icon = nullptr;
	switch (event) {
	case STEAM_TIMELINE_EVENT_DEATH:
		title = "Death";
		icon = "steam_death";
		break;
	case STEAM_TIMELINE_EVENT_ENEMY:
		title = "Aliens";
		icon = "steam_caution";
		break;
	case STEAM_TIMELINE_EVENT_MINE:
		title = "Homing Mine";
		icon = "steam_caution";
		break;
	case STEAM_TIMELINE_EVENT_GRAVITY:
		title = "Gravity Well";
		icon = "steam_caution";
		break;
	case STEAM_TIMELINE_EVENT_NOVA:
		title = "Nova";
		icon = "steam_explosion";
		break;
	default:
		break;
	}
	SteamTimeline()->AddInstantaneousTimelineEvent(title, nullptr, icon, 0, 0.0f, k_ETimelineEventClipPriority_Standard);
}

void SteamInterface::UnlockAchievement(const char *achievement)
{
	if (!m_initialized) {
		return;
	}

	SteamUserStats()->SetAchievement(achievement);
	SteamUserStats()->StoreStats();
}

void SteamInterface::Update()
{
	if (!m_initialized) {
		return;
	}

	SteamAPI_RunCallbacks();

	uint32 count;
	RemotePlayInput_t events[32];
	while ((count = SteamRemotePlay()->GetInput(events, SDL_arraysize(events))) > 0) {
		for (uint32 i = 0; i < count; ++i) {
			ProcessInput(events[i]);
		}
	}
}

SteamInterface::RemoteSession_t *SteamInterface::GetSession(RemotePlaySessionID_t sessionID)
{
	for (unsigned i = 0; i < m_sessions.length(); ++i) {
		RemoteSession_t *session = m_sessions[i];
		if (sessionID == session->id) {
			return session;
		}
	}
	return nullptr;
}

SteamInterface::RemoteSession_t *SteamInterface::GetSessionForControl(Uint8 controlType)
{
	switch (controlType) {
	case CONTROL_REMOTE1:
		return m_players[0];
	case CONTROL_REMOTE2:
		return m_players[1];
	default:
		return nullptr;
	}
}

Uint8 SteamInterface::GetControlForSession(RemotePlaySessionID_t sessionID)
{
	if (m_players[0] && sessionID == m_players[0]->id) {
		return CONTROL_REMOTE1;
	}
	if (m_players[1] && sessionID == m_players[1]->id) {
		return CONTROL_REMOTE2;
	}
	return 0;
}

bool SteamInterface::IsRemotePlayTogether(RemotePlaySessionID_t sessionID)
{
	return SteamRemotePlay()->BSessionRemotePlayTogether(sessionID);
}

void SteamInterface::UpdatePlayers()
{
	for (unsigned int i = 0; i < SDL_arraysize(m_players); ++i) {
		if (m_players[i]) {
			continue;
		}

		for (unsigned int j = 0; j < m_sessions.length(); ++j) {
			RemoteSession_t *session = m_sessions[j];
			if (!GetControlForSession(session->id)) {
				m_players[i] = session;
				break;
			}
		}
	}

	SDL_Event event;
	event.type = SDL_EVENT_REMOTE_PLAYERS_CHANGED;
	SDL_PushEvent(&event);
}

void SteamInterface::ProcessInput(const RemotePlayInput_t &input)
{
	if (input.m_eType != k_ERemotePlayInputKeyDown && input.m_eType != k_ERemotePlayInputKeyUp) {
		return;
	}

	RemoteSession_t *session = GetSession(input.m_unSessionID);
	if (!session) {
		return;
	}

	if (input.m_Key.m_eScancode >= SDL_arraysize(session->keystate)) {
		return;
	}

	bool pressed = (input.m_eType == k_ERemotePlayInputKeyDown);
	session->keystate[input.m_Key.m_eScancode] = pressed;

	Uint8 controlType = GetControlForSession(input.m_unSessionID);
	if (controlType) {
		SDL_Event event;
		event.type = SDL_EVENT_REMOTE_INPUT;
		event.user.code = controlType;
		SDL_PushEvent(&event);
	}
}

RemotePlaySessionID_t SteamInterface::GetRemoteSessionForGamepad(SDL_Gamepad *gamepad)
{
	InputHandle_t handle = SDL_GetGamepadSteamHandle(gamepad);
	if (!handle) {
		return 0;
	}

	RemotePlaySessionID_t sessionID = SteamInput()->GetRemotePlaySessionID( handle );
	if (!IsRemotePlayTogether(sessionID)) {
		return 0;
	}
	return sessionID;
}

const char *SteamInterface::GetRemotePlayerName(Uint8 controlType)
{
	RemoteSession_t *session = GetSessionForControl(controlType);
	if (!session) {
		return nullptr;
	}
	return session->name;
}

SDL_Surface *SteamInterface::GetRemotePlayerAvatar(Uint8 controlType)
{
	RemoteSession_t *session = GetSessionForControl(controlType);
	if (!session) {
		return nullptr;
	}

	SDL_Surface *surface = nullptr;
	int image = SteamRemotePlay()->GetSmallSessionAvatar(session->id);
	if (image > 0) {
		uint32 width, height;
		if (!SteamUtils()->GetImageSize(image, &width, &height)) {
			SDL_Log("Couldn't get image size");
			return nullptr;
		}

		SDL_Surface *avatar = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
		if (!avatar) {
			return nullptr;
		}
		// Steam returns tightly packed images
		SDL_assert(avatar->pitch == width * 4);

		if (!SteamUtils()->GetImageRGBA(image, (uint8 *)avatar->pixels, avatar->h * avatar->pitch)) {
			SDL_Log("Couldn't get image pixels");
			SDL_DestroySurface(surface);
			return nullptr;
		}

		const int AVATAR_SIZE = 36;
		surface = SDL_CreateSurface(AVATAR_SIZE, AVATAR_SIZE, SDL_PIXELFORMAT_RGBA32);
		if (surface) {
			SDL_Rect rect;

			// Add a black border
			rect.w = avatar->w + 2;
			rect.h = avatar->h + 2;
			rect.x = (surface->w - rect.w) / 2;
			rect.y = (surface->h - rect.h) / 2;
			SDL_FillSurfaceRect(surface, &rect, SDL_MapSurfaceRGB(surface, 0, 0, 0));

			// Center the avatar
			rect.w = avatar->w;
			rect.h = avatar->h;
			rect.x = (surface->w - rect.w) / 2;
			rect.y = (surface->h - rect.h) / 2;
			SDL_BlitSurface(avatar, NULL, surface, &rect);
		}
		SDL_DestroySurface(avatar);
	}
	return surface;
}

const bool *SteamInterface::GetRemotePlayerKeyboardState(Uint8 controlType)
{
	RemoteSession_t *session = GetSessionForControl(controlType);
	if (!session) {
		return nullptr;
	}
	return session->keystate;
}

void SteamInterface::EnableRemoteInput()
{
	if (!m_initialized) {
		return;
	}

	SteamRemotePlay()->BEnableRemotePlayTogetherDirectInput();
}

void SteamInterface::DisableRemoteInput()
{
	if (!m_initialized) {
		return;
	}

	SteamRemotePlay()->DisableRemotePlayTogetherDirectInput();
}

void SteamInterface::OnRemotePlaySessionAvatarLoaded(SteamRemotePlaySessionAvatarLoaded_t *pParam)
{
	UpdatePlayers();
}

void SteamInterface::OnRemotePlaySessionConnected(SteamRemotePlaySessionConnected_t *pParam)
{
	RemotePlaySessionID_t sessionID = pParam->m_unSessionID;

	if (IsRemotePlayTogether(sessionID)) {
		RemoteSession_t *session = new RemoteSession_t;
		session->id = sessionID;
		session->steamID = SteamRemotePlay()->GetSessionSteamID(sessionID);

		if (session->steamID.IsValid()) {
			session->name = SDL_strdup(SteamFriends()->GetFriendPersonaName(session->steamID));
		} else {
			uint32 unGuestID = SteamRemotePlay()->GetSessionGuestID(sessionID);
			if (unGuestID) {
				SDL_asprintf(&session->name, TEXT("Guest %u"), unGuestID);
			} else {
				session->name = NULL;
			}
		}
		SDL_zeroa(session->keystate);

		m_sessions.add(session);
	}

	UpdatePlayers();
}

void SteamInterface::OnRemotePlaySessionDisconnected(SteamRemotePlaySessionDisconnected_t *pParam)
{
	RemotePlaySessionID_t sessionID = pParam->m_unSessionID;
	RemoteSession_t *session = GetSession(sessionID);
	if (session) {
		m_sessions.remove(session);

		for (unsigned int i = 0; i < SDL_arraysize(m_players); ++i) {
			if (session == m_players[i]) {
				m_players[i] = nullptr;
				break;
			}
		}
		SDL_free(session->name);
		delete session;
	}
	UpdatePlayers();
}


bool InitSteam()
{
	return steam.Init();
}

bool SteamStreamingToPhone()
{
	return steam.StreamingToPhone();
}

bool SteamStreamingToTablet()
{
	return steam.StreamingToTablet();
}

Uint32 GetRemoteSessionForGamepad(SDL_Gamepad *gamepad)
{
	return steam.GetRemoteSessionForGamepad(gamepad);
}

Uint8 GetRemoteSessionControl(RemotePlaySessionID_t sessionID)
{
	return steam.GetControlForSession(sessionID);
}

const char *GetRemotePlayerName(Uint8 controlType)
{
	return steam.GetRemotePlayerName(controlType);
}

SDL_Surface *GetRemotePlayerAvatar(Uint8 controlType)
{
	return steam.GetRemotePlayerAvatar(controlType);
}

const bool *GetRemotePlayerKeyboardState(Uint8 controlType)
{
	return steam.GetRemotePlayerKeyboardState(controlType);
}

void EnableRemoteInput()
{
	steam.EnableRemoteInput();
}

void DisableRemoteInput()
{
	steam.DisableRemoteInput();
}

void SetSteamTimelineMode(STEAM_TIMELINE_MODE mode)
{
	steam.SetSteamTimelineMode(mode);
}

void SetSteamTimelineLevelStarted(int level)
{
	steam.SetSteamTimelineLevelStarted(level);
}

void AddSteamTimelineEvent(STEAM_TIMELINE_EVENT event)
{
	steam.AddSteamTimelineEvent(event);
}

void UnlockAchievement(const char *achievement)
{
	steam.UnlockAchievement(achievement);
}

void UpdateSteam()
{
	steam.Update();
}

void QuitSteam()
{
	steam.Quit();
}

#else

bool InitSteam()
{
	return false;
}

bool SteamStreamingToPhone()
{
	return false;
}

bool SteamStreamingToTablet()
{
	return false;
}

RemotePlaySessionID_t GetRemoteSessionForGamepad(SDL_Gamepad* gamepad)
{
	return 0;
}

Uint8 GetRemoteSessionControl(RemotePlaySessionID_t sessionID)
{
	return 0;
}

const char *GetRemotePlayerName(Uint8 controlType)
{
	return nullptr;
}

SDL_Surface *GetRemotePlayerAvatar(Uint8 controlType)
{
	return nullptr;
}

const bool *GetRemotePlayerKeyboardState(Uint8 controlType)
{
	return nullptr;
}

void EnableRemoteInput()
{
}

void DisableRemoteInput()
{
}

void SetSteamTimelineMode(STEAM_TIMELINE_MODE mode)
{
}

void SetSteamTimelineLevelStarted(int level)
{
}

void AddSteamTimelineEvent(STEAM_TIMELINE_EVENT event)
{
}

void UnlockAchievement(const char *achievement)
{
}

void UpdateSteam()
{
}

void QuitSteam()
{
}

#endif // ENABLE_STEAM
