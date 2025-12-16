/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

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

/* This contains the network play functions and data */

#include "Maelstrom_Globals.h"
#include "netplay.h"
#include "protocol.h"

// Set this to 1 for normal debug info, and 2 for verbose packet logging
//#define DEBUG_NETWORK 1

// Define this to simulate packet loss
//#define DEBUG_PACKETLOSS 5

// How long we wait for an ack
#define NETWORK_TIMEOUT	2*FRAME_DELAY_MS


NET_DatagramSocket *gSocket = nullptr;

static Uint32 NextFrame;
static bool AdvancedFrame;

/* We keep one packet backlogged for retransmission */
static DynamicPacket OutBound[2];
static int           CurrOut;
#define CurrPacket	OutBound[CurrOut]
#define LastPacket	OutBound[!CurrOut]

/* The nodes we're currently waiting on for sync acks */
static Uint32 WaitingAcks[MAX_NODES];

/* We cache one packet if the other player is ahead of us */
static struct {
	Uint32 frame;
	DynamicPacket packet;
} CachedPacket[MAX_NODES];

/* When we're done we have our input for the frame */
static DynamicPacket QueuedInput;
static DynamicPacket FrameInput;


int CreateSocket(bool hosting)
{
	int port;

	/* Create the UDP socket */
	if (hosting) {
		port = NETPLAY_PORT;
	} else {
		port = 0;
	}
	gSocket = NET_CreateDatagramSocket(NULL, port);
	if ( gSocket == NULL ) {
		error("Couldn't create socket bound to port %d: %s\n", port, SDL_GetError());
		return(-1);
	}

#ifdef DEBUG_PACKETLOSS
	SDLNet_UDP_SetPacketLoss(gSocket, DEBUG_PACKETLOSS);
#endif

	return(0);
}

void CloseSocket(void)
{
	if (gSocket) {
		NET_DestroyDatagramSocket(gSocket);
		gSocket = NULL;
	}
}

void InitNetData(void)
{
	int i;

	/* Initialize network game variables */
	NextFrame = 1;
	AdvancedFrame = true;
	OutBound[0].Reset();
	OutBound[1].Reset();
	CurrOut = 0;
	SDL_zero(WaitingAcks);
	for (i = 0; i < MAX_NODES; ++i) {
		CachedPacket[i].frame = ~0u;
		CachedPacket[i].packet.Reset();
	}
	QueuedInput.Reset();
	FrameInput.Reset();
}

int CheckPlayers(void)
{
	int i;

	if (gGameInfo.GetNumPlayers() == 0) {
		error("No players specified!\r\n");
		return(-1);
	}

	bool foundLocalPlayer = false;
	for (i = 0; i < MAX_PLAYERS; ++i) {
		if (gGameInfo.IsLocalPlayer(i)) {
			foundLocalPlayer = true;
			break;
		}
	}
	if (!foundLocalPlayer) {
		error("Which player are you?\r\n");
		return(-1);
	}

	return(0);
}

void QueueInput(Uint8 value)
{
	QueuedInput.Write(value);
}

static Uint32 NodeTimeout(Uint32 now)
{
	Uint32 timeout;

	timeout = now + NETWORK_TIMEOUT;
	if (!timeout) {
		timeout = 1;
	}
	return timeout;
}

static bool WaitingForAck()
{
	for (int i = 0; i < gGameInfo.GetNumNodes(); ++i) {
		if (WaitingAcks[i]) {
			return true;
		}
	}
	return false;
}

static bool HasTimedOut(int index, Uint32 now)
{
	if (!WaitingAcks[index]) {
		return false;
	}
	return int(WaitingAcks[index]-now) < 0;
}

static Uint32 NextTimeout(Uint32 now)
{
	Uint32 timeout = NETWORK_TIMEOUT;
	for (int i = 0; i < gGameInfo.GetNumNodes(); ++i) {
		if (WaitingAcks[i]) {
			timeout = SDL_min((now - WaitingAcks[i]), timeout);
		}
	}
	return timeout;
}

static bool ProcessSync(int index, DynamicPacket &packet)
{
	Uint32 seed;
	Uint8 state;

	if (!packet.Read(seed) || !packet.Read(state)) {
		error("Received short packet\r\n");
		return false;
	}

	if (seed != GetRandSeed()) {
		/* We're hosed, to correct this we would have to sync the complete game state */
		error("Error!! \a consistency problem expecting seed %8.8x, got seed %8.8x, aborting!!\r\n", GetRandSeed(), seed);
		return false;
	}

	gGameInfo.SetNodeState(index, state);

	// Should we validate that the input is for players from this node?
	//        ... nah... :)
	FrameInput.Write(packet);

	return true;
}

static SYNC_RESULT AwaitSync()
{
	int i;
	Uint32 frame;

	// Send the packet to anyone waiting
	Uint32 now = (Uint32)SDL_GetTicks();
	for (i = 0; i < gGameInfo.GetNumNodes(); ++i) {
		if (WaitingAcks[i]) {
#if DEBUG_NETWORK >= 2
error("Sending packet for current frame (%ld)\r\n", NextFrame);
#endif
			const GameInfoNode *node = gGameInfo.GetNode(i);
			NET_SendDatagram(gSocket, node->address.host, node->address.port, CurrPacket.data, CurrPacket.len);
			WaitingAcks[i] = NodeTimeout(now);
		}
	}

	// See if we have cached network packets
	// Note that we always send the packet for the current frame first
	for (i = 0; i < gGameInfo.GetNumNodes(); ++i) {
		if (CachedPacket[i].frame == NextFrame) {
			if (!ProcessSync(i, CachedPacket[i].packet)) {
				return SYNC_CORRUPT;
			}
			WaitingAcks[i] = 0;
			CachedPacket[i].frame = 0;
		}
	}

	/* Wait for Ack's */
	while (WaitingForAck()) {
		now = (Uint32)SDL_GetTicks();
		for (i = 0; i < gGameInfo.GetNumNodes(); ++i) {
			if (HasTimedOut(i, now)) {
#if DEBUG_NETWORK >= 1
error("Timed out waiting for frame %ld\r\n", NextFrame);
#endif
				return SYNC_TIMEOUT;
			}
		}

		int ready = NET_WaitUntilInputAvailable((void**)&gSocket, 1, NextTimeout(now));
		if (ready < 0) {
			error("Network error: SDLNet_CheckSockets()\r\n");
			return SYNC_NETERROR;
		}
		if (ready == 0) {
			continue;
		}

		/* We are guaranteed that there is data here */
		NET_Datagram *datagram;
		DynamicPacket packet;
		if (!NET_ReceiveDatagram(gSocket, &datagram) || !datagram) {
			error("Network error: NET_ReceiveDatagram()\r\n");
			return SYNC_NETERROR;
		}

		/* We have a packet! */
		packet.data = datagram->buf;
		packet.len = datagram->buflen;
		Uint8 cmd;
		if (!packet.Read(cmd)) {
			error("Received short packet\r\n");
			continue;
		}
		if (cmd == LOBBY_MSG) {
#if DEBUG_NETWORK >= 2
error("LOBBY_MSG packet\r\n");
#endif
			continue;
		}
		if (cmd == NEW_GAME) {
#if DEBUG_NETWORK >= 2
error("NEW_GAME packet\r\n");
#endif
			DynamicPacket reply;
			reply.Write((Uint8)NEW_GAME_ACK);
			reply.Write(gGameInfo.gameID);
			reply.Write(gGameInfo.localID);
			NET_SendDatagram(gSocket, datagram->addr, datagram->port, reply.data, reply.len);
			continue;
		}
		if (cmd == NEW_GAME_ACK) {
#if DEBUG_NETWORK >= 2
error("NEW_GAME_ACK packet\r\n");
#endif
			continue;
		}
		if (cmd != SYNC_MSG) {
			error("Unknown packet: 0x%x\r\n", cmd);
			continue;
		}

		Uint32 gameID;
		Uint32 nodeID;
		if (!packet.Read(gameID) || !packet.Read(nodeID)) {
			error("Received short packet\r\n");
			continue;
		}
		if (gameID != gGameInfo.gameID) {
			/* This must be for a different game */
			continue;
		}
		int index = gGameInfo.GetNodeIndex(nodeID);
		if (index < 0) {
			/* This must be from an old node */
			continue;
		}

		/* Check the frame number */
		if (!packet.Read(frame)) {
			error("Received short packet\r\n");
			continue;
		}
#if DEBUG_NETWORK >= 2
error("Received a packet of frame %lu from node %d\r\n", frame, index);
#endif
		if (frame == NextFrame) {
			/* Ignore it if it is a duplicate packet */
			if (!WaitingAcks[index]) {
#if DEBUG_NETWORK >= 1
error("Ignoring duplicate packet for frame %lu from node %d\r\n", frame, index);
#endif
				continue;
			}

			/* Do a consistency check!! */
			if (!ProcessSync(index, packet)) {
				return SYNC_CORRUPT;
			}
			WaitingAcks[index] = 0;
		} else if (frame == (NextFrame-1)) {
			/* We kept the last frame cached, so send it */
#if DEBUG_NETWORK >= 1
error("Transmitting packet for old frame (%lu)\r\n", frame);
#endif
			NET_SendDatagram(gSocket, datagram->addr, datagram->port, LastPacket.data, LastPacket.len);
		} else if (frame == (NextFrame+1)) {
#if DEBUG_NETWORK >= 1
error("Received packet for next frame! (%lu, current = %lu)\r\n",
					frame, NextFrame);
#endif
			/* Cache this frame for next round */
			CachedPacket[index].frame = frame;
			CachedPacket[index].packet.Reset();
			CachedPacket[index].packet.Write(packet);
			CachedPacket[index].packet.Seek(0);

			/* Let the node know we're still waiting */
			NET_SendDatagram(gSocket, datagram->addr, datagram->port, CurrPacket.data, CurrPacket.len);
		}
#if DEBUG_NETWORK >= 1
else
error("Received packet for really old frame! (%lu, current = %lu)\r\n",
							frame, NextFrame);
#endif
	}
	return SYNC_COMPLETE;
}

static void AdvanceFrame()
{
	CurrOut = !CurrOut;
	++NextFrame;
	AdvancedFrame = true;
}

/* This function is called every frame, and is used to flush the network
   buffers, sending sync and keystroke packets.
   It is called AFTER the keyboard is polled, and BEFORE GetSyncBuf() is
   called by the player objects.

   Note:  We assume that FastRand() isn't called by an interrupt routine,
          otherwise we lose consistency.
*/
	
SYNC_RESULT SyncNetwork(void)
{
	SYNC_RESULT result = SYNC_COMPLETE;
	int i;

	if (!AdvancedFrame) {
		// We still have some nodes we're waiting on...
		result = AwaitSync();
		if (result == SYNC_COMPLETE) {
			AdvanceFrame();
		}
		return result;
	}

	FrameInput.Reset();
	QueuedInput.Seek(0);
	FrameInput.Write(QueuedInput);

	// See if we need to do network synchronization
	Uint32 now = (Uint32)SDL_GetTicks();
	for (i = 0; i < gGameInfo.GetNumNodes(); ++i) {
		if (gGameInfo.IsNetworkNode(i)) {
			WaitingAcks[i] = NodeTimeout(now);
		} else {
			WaitingAcks[i] = 0;
		}
	}
	if (WaitingForAck()) {
		// Create the sync packet
		CurrPacket.Reset();
		CurrPacket.Write((Uint8)SYNC_MSG);
		CurrPacket.Write(gGameInfo.gameID);
		CurrPacket.Write(gGameInfo.localID);
		CurrPacket.Write(NextFrame);
		CurrPacket.Write(GetRandSeed());
		CurrPacket.Write(gGameInfo.GetLocalState());
		QueuedInput.Seek(0);
		CurrPacket.Write(QueuedInput);

		// Wait for sync packets from them
		result = AwaitSync();
	}
	QueuedInput.Reset();

	if (result == SYNC_COMPLETE) {
		AdvanceFrame();
	} else {
		AdvancedFrame = false;
	}
	return result;
}

/* This function retrieves the input for the frame */
int GetSyncBuf(Uint8 **bufptr)
{
	*bufptr = FrameInput.data;
	return FrameInput.len;
}

/* This function sends a NEW_GAME packet, and waits for all other players
   to respond with NEW_GAME_ACK
*/
int Send_NewGame()
{
	char message[BUFSIZ];
	int  i, j;
	DynamicPacket newgame;

	/* Send all the packets */
	newgame.Write((Uint8)NEW_GAME);
	gGameInfo.WriteToPacket(newgame);
	for (i = 0; i < gGameInfo.GetNumNodes(); ++i) {
		if (gGameInfo.IsNetworkNode(i)) {
			const GameInfoNode *node = gGameInfo.GetNode(i);
			NET_SendDatagram(gSocket, node->address.host, node->address.port, newgame.data, newgame.len);
		}
	}

	/* Get ready for responses */
	Uint32 now = (Uint32)SDL_GetTicks();
	for (i = 0; i < gGameInfo.GetNumNodes(); ++i) {
		if (gGameInfo.IsNetworkNode(i)) {
			WaitingAcks[i] = NodeTimeout(now);
		} else {
			WaitingAcks[i] = 0;
		}
	}

	/* Wait for Ack's */
	while (WaitingForAck()) {
		/* Show a status */
		SDL_strlcpy(message, "Waiting for players:", sizeof(message));
		for (i = 0; i < MAX_PLAYERS; ++i) {
			const GameInfoPlayer *player = gGameInfo.GetPlayer(i);
			for (j = 0; j < gGameInfo.GetNumNodes(); ++j) {
				if (player->nodeID == gGameInfo.GetNode(j)->nodeID) {
					SDL_snprintf(&message[SDL_strlen(message)], sizeof(message)-SDL_strlen(message), " %d", i+1);
					break;
				}
			}
		}
		//Message(message);

		now = (Uint32)SDL_GetTicks();
		for (i = 0; i < gGameInfo.GetNumNodes(); ++i) {
			if (HasTimedOut(i, now)) {
				const GameInfoNode *node = gGameInfo.GetNode(i);
				NET_SendDatagram(gSocket, node->address.host, node->address.port, newgame.data, newgame.len);
				WaitingAcks[i] = NodeTimeout(now);
			}
		}
		int ready = NET_WaitUntilInputAvailable((void**)&gSocket, 1, NextTimeout(now));
		if (ready < 0) {
			error("Network error: NET_WaitUntilInputAvailable()\r\n");
			return(-1);
		}
		if (ready == 0) {
			continue;
		}

		/* We are guaranteed that there is data here */
		NET_Datagram* datagram;
		DynamicPacket packet;
		if (!NET_ReceiveDatagram(gSocket, &datagram)) {
			error("Network error: SDLNet_UDP_Recv()\r\n");
			return(-1);
		}

		/* We have a packet! */
		packet.data = datagram->buf;
		packet.len = datagram->buflen;
		Uint8 cmd;
		Uint32 gameID;
		Uint32 nodeID;
		if (!packet.Read(cmd) || cmd != NEW_GAME_ACK) {
			/* Continue waiting */
			continue;
		}
		if (!packet.Read(gameID) || !packet.Read(nodeID)) {
			continue;
		}
		if (gameID != gGameInfo.gameID) {
			/* This must be for a different game */
			continue;
		}
		if (!nodeID) {
			continue;
		}
		int index = gGameInfo.GetNodeIndex(nodeID);
		if (index < 0) {
			/* This must be from an old node */
			continue;
		}
		WaitingAcks[index] = 0;
	}
	return(0);
}
