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

#ifndef _protocol_h
#define _protocol_h

#include <SDL3_net/SDL_net.h>

/* Architecture
 *
 * The lobby server simply acts as an address broker, keeping a list
 * of IP addresses of game servers, and serving them to players
 * wanting to find games.
 *
 * The joining game and hosting games then communicate with each other
 * to create a list of games and join/leave them.
 */

/* Protocol messages */
enum LobbyProtocol {
	LOBBY_NONE,

	/**********/
	/* Messages between the joining game and the hosting game */

	LOBBY_OPEN_FIREWALL = 20,
	/* Sent by the hosting game in response to lobby server messages
	   to open the firewall for communication
	 */

	LOBBY_PING,
	/* Sent by players to verify the game and player state.

		Uint32 gameID
		Uint32 playerID
		Uint32 timestamp
	 */

	LOBBY_PONG,
	/* Echoed by the hosting game in response to LOBBY_PING

		Uint32 gameID
		Uint32 playerID
		Uint32 timestamp
	 */

	LOBBY_REQUEST_GAME_INFO,
	/* Sent by the joining game to get info for the game list

		Uint32 timestamp
	 */

	LOBBY_GAME_INFO,
	/* Sent by the hosting game, if there are slots open

		Uint32 timestamp
		Uint32 gameID
		Uint8 deathMatch;
		Uint32 player1_uniqueID;
		Uint32 player1_host;
		Uint16 player1_port;
		Uint8 player1_namelen
		char player1_name[]
		Uint32 player2_uniqueID;
		Uint32 player2_host;
		Uint16 player2_port;
		Uint8 player2_namelen
		char player2_name[]
		Uint32 player3_uniqueID;
		Uint32 player3_host;
		Uint16 player3_port;
		Uint8 player3_namelen
		char player3_name[]
	 */

	LOBBY_REQUEST_JOIN,
	/* Sent by the joining game

		Uint32 gameID
		Uint32 playerID
		Uint8 namelen
		char name[]
	*/

	LOBBY_REQUEST_LEAVE,
	/* Sent by the joining game

		Uint32 gameID
		Uint32 playerID
	*/

	LOBBY_KICK,
	/* Sent by the hosting game

		Uint32 gameID
		Uint32 playerID
	*/

	/* You can't add any more packets past here, look above for space! */
	LOBBY_PACKET_MAX = 256
};

/* Network protocol for synchronization and keystrokes */

#define LOBBY_MSG	0x00			/* Sent before game */
#define NEW_GAME	0x01			/* Sent by host at start */
#define NEW_GAME_ACK	0x02			/* Sent by players at start */
#define SYNC_MSG	0x04			/* Sent during game */

/* The default port for Maelstrom games */
#define LOBBY_PORT	0xAE00			/* port 44544 */
#define NETPLAY_PORT	0xAF00			/* port 44800 */

/* The minimum length of a new packet buffer */
#define NEW_PACKETLEN	(1+1+sizeof(Uint32))

/* Note: if you change MAX_PLAYERS, you need to modify the gPlayerColors
   array in player.cpp
*/
#define MAX_PLAYERS	3
#define MAX_NODES	MAX_PLAYERS

/* The index of the node hosting the game */
#define HOST_NODE	0

/* If the other side hasn't responded in 3 seconds, we'll drop them */
#define PING_INTERVAL	1000
#define PING_TIMEOUT	3000

/* The maximum characters in a player's handle */
#define MAX_NAMELEN	15

/* The maximum characters in a hostname */
#define MAX_HOSTNAME_LEN 128

#endif /* _protocol_h */
