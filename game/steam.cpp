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

#include <SDL3/SDL.h>

#include "steam.h"

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

	void Update();

	Uint8 GetControlForSession(RemotePlaySessionID_t sessionID);

	RemotePlaySessionID_t GetRemoteSessionForGamepad(SDL_Gamepad *gamepad);

	const char *GetRemotePlayerName(Uint8 controlType);
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
	CSteamID m_steamID;

	array<RemoteSession_t *> m_sessions;
	RemoteSession_t *m_players[MAX_PLAYERS - 1] = { };

	STEAM_CALLBACK(SteamInterface, OnRemotePlaySessionConnected, SteamRemotePlaySessionConnected_t);
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

// FIXME: We need a Steamworks API for this
bool SteamInterface::IsRemotePlayTogether(RemotePlaySessionID_t sessionID)
{
	RemoteSession_t *session = GetSession(sessionID);
	if (!session) {
		return false;
	}

	if (session->steamID == m_steamID) {
		return false;
	}
	return true;
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

void SteamInterface::OnRemotePlaySessionConnected(SteamRemotePlaySessionConnected_t *pParam)
{
	RemotePlaySessionID_t sessionID = pParam->m_unSessionID;
	RemoteSession_t *session = new RemoteSession_t;
	session->id = sessionID;
	session->steamID = SteamRemotePlay()->GetSessionSteamID(sessionID);
	session->name = SDL_strdup("");
	SDL_zeroa(session->keystate);
	m_sessions.add(session);

	UpdatePlayers();
}

void SteamInterface::OnRemotePlaySessionDisconnected(SteamRemotePlaySessionDisconnected_t *pParam)
{
	RemotePlaySessionID_t sessionID = pParam->m_unSessionID;
	RemoteSession_t *session = GetSession(sessionID);
	if (!session) {
		return;
	}

	m_sessions.remove(session);

	for (unsigned int i = 0; i < SDL_arraysize(m_players); ++i) {
		if (session == m_players[i]) {
			m_players[i] = nullptr;
			break;
		}
	}
	SDL_free(session->name);
	delete session;

	UpdatePlayers();
}


bool InitSteam()
{
	return steam.Init();
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

void UpdateSteam()
{
}

void QuitSteam()
{
}

#endif // ENABLE_STEAM
