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

#include <stdio.h>
#include <stdarg.h>

#include "SDL_net.h"

#include "game/packet.h"
#include "game/protocol.h"
#include "utils/array.h"


// We'll let games stick around for 10 seconds before aging them out
#define GAME_LIFETIME	10000

static void log(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	fflush(stdout);
}

class AddressList : public array<IPaddress>
{
public:
	AddressList() : array<IPaddress>() { }

	bool ReadFromPacket(DynamicPacket &packet) {
		Uint8 count;
		IPaddress address;

		if (!packet.Read(count) || !count) {
			return false;
		}

		clear();
		SDL_zero(address);
		for (Uint8 i = 0; i < count; ++i) {
			if (!packet.Read(address.host)) {
				return false;
			}
			if (!packet.Read(address.port)) {
				return false;
			}
			add(address);
		}

		// Add the address that we saw this packet come from
		if (!find(packet.address)) {
			insert(packet.address, 0);
		}

		return true;
	}

	void WriteToPacket(DynamicPacket &packet) {
		packet.Write((Uint8)length());
		for (int i = 0; i < length(); ++i) {
			packet.Write(m_data[i].host);
			packet.Write(m_data[i].port);
		}
	}

	bool operator ==(const AddressList &rhs) const {
		if (length() != rhs.length()) {
			return false;
		}
		for (int i = 0; i < length(); ++i) {
			if (m_data[i] != rhs.m_data[i]) {
				return false;
			}
		}
		return true;
	}
};

class Game
{
public:
	Game() {
		m_timestamp = 0;
		m_list = 0;
		m_prev = 0;
		m_next = 0;
	}

	bool ReadFromPacket(DynamicPacket &packet) {
		return m_addresses.ReadFromPacket(packet);
	}

	void WriteToPacket(DynamicPacket &packet) {
		m_addresses.WriteToPacket(packet);
	}

	bool operator ==(const Game &rhs) {
		return m_addresses == rhs.m_addresses;
	}

	bool TimedOut(Uint32 now) {
		return (now - m_timestamp) > GAME_LIFETIME;
	}

	void Refresh() {
		m_timestamp = SDL_GetTicks();
	}

	const IPaddress *Address() const {
		return &m_addresses[0];
	}

	void Link(Game *&list) {
		Unlink();
		if (list) {
			list->m_prev = this;
		}
		m_next = list;
		list = this;
		m_list = &list;
	}

	void Unlink() {
		if (m_prev) {
			m_prev->m_next = m_next;
		} else if (m_list) {
			*m_list = m_next;
		}
		if (m_next) {
			m_next->m_prev = m_prev;
		}
		m_list = 0;
		m_prev = 0;
		m_next = 0;
	}

	Game *Prev() {
		return m_prev;
	}
	Game *Next() {
		return m_next;
	}

protected:
	Uint32 m_timestamp;
	AddressList m_addresses;
	Game **m_list;
	Game *m_prev, *m_next;
};

class GameList
{
public:
	GameList() {
		m_sock = NULL;
		m_list = 0;
		m_free = 0;
	}

	~GameList() {
		Game *game;

		while (m_list) {
			game = m_list;
			game->Unlink();
			delete game;
		}
		while (m_free) {
			game = m_free;
			game->Unlink();
			delete game;
		}
	}

	void SetSocket(UDPsocket sock) {
		m_sock = sock;
	}

	void ProcessList() {
		Game *game, *next;
		Uint32 now;

		now = SDL_GetTicks();
		game = m_list;
		while (game) {
			next = game->Next();
			if (game->TimedOut(now)) {
				log("Expiring game from %s:%d\n",
					SDLNet_ResolveIP(game->Address()),
					SDL_SwapBE16(game->Address()->port));
				game->Link(m_free);
			}
			game = next;
		}
	}

	void ProcessPacket(DynamicPacket &packet) {
		Uint8 cmd;

		if (!packet.Read(cmd)) {
			return;
		}
		if (cmd != LOBBY_MSG) {
			return;
		}
		if (!packet.Read(cmd)) {
			return;
		}

		switch (cmd) {
			case LOBBY_ANNOUNCE_GAME:
				ProcessAnnounceGame(packet);
				break;
			case LOBBY_REMOVE_GAME:
				ProcessRemoveGame(packet);
				break;
			case LOBBY_REQUEST_GAME_SERVERS:
				ProcessRequestGames(packet);
				break;
		}
	}

	void ProcessAnnounceGame(DynamicPacket &packet) {
		Game *newGame;

		if (m_free) {
			newGame = m_free;
		} else {
			newGame = new Game;
		}
		if (!newGame->ReadFromPacket(packet)) {
			log("Invalid game from %s:%d\n",
				SDLNet_ResolveIP(&packet.address),
				SDL_SwapBE16(packet.address.port));

			newGame->Link(m_free);
			return;
		}

		for (Game *game = m_list; game; game = game->Next()) {
			if (*game == *newGame) {
				//log("Refreshing game from %s:%d\n",
				//	SDLNet_ResolveIP(&packet.address),
				//	SDL_SwapBE16(packet.address.port));

				game->Refresh();
				newGame->Link(m_free);
				return;
			}
		}

		log("Adding game from %s:%d\n",
			SDLNet_ResolveIP(&packet.address),
			SDL_SwapBE16(packet.address.port));

		newGame->Refresh();
		newGame->Link(m_list);
	}

	void ProcessRemoveGame(DynamicPacket &packet) {
		Game *newGame;

		if (m_free) {
			newGame = m_free;
		} else {
			newGame = new Game;
		}
		if (!newGame->ReadFromPacket(packet)) {
			newGame->Link(m_free);
			return;
		}

		for (Game *game = m_list; game; game = game->Next()) {
			if (*game == *newGame) {
				log("Removing game from %s:%d\n",
					SDLNet_ResolveIP(&packet.address),
					SDL_SwapBE16(packet.address.port));
				game->Link(m_free);
				break;
			}
		}
		newGame->Link(m_free);
	}

	void ProcessRequestGames(DynamicPacket &packet) {
		AddressList addresses;
		Game *game;

		if (!addresses.ReadFromPacket(packet)) {
			return;
		}

		// Send back a list of all games
		int mark;
		m_reply.StartLobbyMessage(LOBBY_GAME_SERVERS);
		mark = m_reply.Tell(); 
		m_reply.Write((Uint8)0);
		int count;
		for (game = m_list; game; game = game->Next()) {
			game->WriteToPacket(m_reply);
			++count;
			if (count == 255) {
				// That's it, I'm cutting you off...
				break;
			}
		}
		m_reply.Seek(mark);
		m_reply.Write((Uint8)count);
		m_reply.address = packet.address;
		SDLNet_UDP_Send(m_sock, -1, &m_reply);

		// Send the requesting player to all game servers
		m_reply.StartLobbyMessage(LOBBY_ANNOUNCE_PLAYER);
		addresses.WriteToPacket(m_reply);
		for (game = m_list; game; game = game->Next()) {
			m_reply.address = *game->Address();
			SDLNet_UDP_Send(m_sock, -1, &m_reply);
		}
	}

protected:
	UDPsocket m_sock;
	Game *m_list;
	Game *m_free;
	DynamicPacket m_reply;
};

int main(int argc, char *argv[])
{
	UDPsocket sock;
	DynamicPacket packet;
	GameList games;

	sock = SDLNet_UDP_Open(LOBBY_PORT);
	if (!sock) {
		fprintf(stderr, "Couldn't create socket on port %d: %s\n",
			LOBBY_PORT, SDL_GetError());
		exit(1);
	}
	games.SetSocket(sock);

	for ( ; ; ) {
		games.ProcessList();

		while (SDLNet_UDP_Recv(sock, &packet)) {
			games.ProcessPacket(packet);
			packet.Reset();
		}

		SDL_Delay(100);
	}
	SDLNet_UDP_Close(sock);
}
