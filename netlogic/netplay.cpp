
/* This contains the network play functions and data */

#include <stdlib.h>

#include "SDL_net.h"
#include "SDL_endian.h"

#include "Maelstrom_Globals.h"
#include "netplay.h"
#include "protocol.h"


int   gNumPlayers;
int   gOurPlayer;
int   gDeathMatch;
UDPsocket gNetFD;

static int            GotPlayer[MAX_PLAYERS];
static IPaddress      PlayAddr[MAX_PLAYERS];
static IPaddress      ServAddr;
static int            FoundUs, UseServer;
static Uint32         NextFrame;
UDPpacket            *OutBound[2];
static int            CurrOut;
/* This is the data offset of a SYNC packet */
#define PDATA_OFFSET	(1+1+sizeof(Uint32)+sizeof(Uint32))

/* We keep one packet backlogged for retransmission */
#define OutBuf		OutBound[CurrOut]->data
#define OutLen		OutBound[CurrOut]->len
#define LastBuf		OutBound[!CurrOut]->data
#define LastLen		OutBound[!CurrOut]->len

static unsigned char *SyncPtrs[2][MAX_PLAYERS];
static unsigned char  SyncBufs[2][MAX_PLAYERS][BUFSIZ];
static int            SyncLens[2][MAX_PLAYERS];
static int            ThisSyncs[2];
static int            CurrIn;
static SDLNet_SocketSet SocketSet;

/* We cache one packet if the other player is ahead of us */
#define SyncPtr		SyncPtrs[CurrIn]
#define SyncBuf		SyncBufs[CurrIn]
#define SyncLen		SyncLens[CurrIn]
#define ThisSync	ThisSyncs[CurrIn]
#define NextPtr		SyncPtrs[!CurrIn]
#define NextBuf		SyncBufs[!CurrIn]
#define NextLen		SyncLens[!CurrIn]
#define NextSync	ThisSyncs[!CurrIn]

#define TOGGLE(var)	var = !var


int InitNetData(void)
{
	int i;

	/* Initialize the networking subsystem */
	if ( SDLNet_Init() < 0 ) {
		error("NetLogic: Couldn't initialize networking!\n");
		return(-1);
	}
	atexit(SDLNet_Quit);

	/* Create the outbound packets */
	for ( i=0; i<2; ++i ) {
		OutBound[i] = SDLNet_AllocPacket(BUFSIZ);
		if ( OutBound[i] == NULL ) {
			error("Out of memory (creating network buffers)\n");
			return(-1);
		}
	}

	/* Initialize network game variables */
	FoundUs   = 0;
	gOurPlayer  = -1;
	gDeathMatch = 0;
	UseServer = 0;
	for ( i=0; i<MAX_PLAYERS; ++i ) {
		GotPlayer[i] = 0;
		SyncPtrs[0][i] = NULL;
		SyncPtrs[1][i] = NULL;
	}
	OutBound[0]->data[0] = SYNC_MSG;
	OutBound[1]->data[0] = SYNC_MSG;
	/* Type field, frame sequence, current random seed */
	OutBound[0]->len = PDATA_OFFSET;
	OutBound[1]->len = PDATA_OFFSET;
	CurrOut = 0;

	ThisSyncs[0] = 0;
	ThisSyncs[1] = 0;
	CurrIn = 0;
	return(0);
}

void HaltNetData(void)
{
	SDLNet_Quit();
}

int AddPlayer(const char *player)
{
	int playernum;
	int portnum;
	char *host=NULL, *port=NULL;
	char *playerstr = SDL_strdup(player);

	/* Extract host and port information */
	if ( (port=strchr(playerstr, ':')) != NULL )
		*(port++) = '\0';
	if ( (host=strchr(playerstr, '@')) != NULL )
		*(host++) = '\0';

	/* Find out which player we are referring to */
	if (((playernum = atoi(playerstr)) <= 0) || (playernum > MAX_PLAYERS)) {
		error(
"Argument to '-player' must be in integer between 1 and %d inclusive.\r\n",
								MAX_PLAYERS);
		PrintUsage();
	}

	/* Do some error checking */
	if ( GotPlayer[--playernum] ) {
		error("Player %d specified multiple times!\r\n", playernum+1);
		SDL_free(playerstr);
		return(-1);
	}
	if ( port ) {
		portnum = atoi(port);
	} else {
		portnum = NETPLAY_PORT+playernum;
	}
	if ( host ) {
		/* Resolve the remote address */
		SDLNet_ResolveHost(&PlayAddr[playernum], host, portnum);
		if ( PlayAddr[playernum].host == INADDR_NONE ) {
			error("Couldn't resolve host name for %s\r\n", host);
			SDL_free(playerstr);
			return(-1);
		}
	} else { /* No host specified, local player */
		if ( FoundUs ) {
			error(
"More than one local player!  (players %d and %d specified as local players)\r\n",
						gOurPlayer+1, playernum+1);
			SDL_free(playerstr);
			return(-1);
		} else {
			gOurPlayer = playernum;
			FoundUs = 1;
			SDLNet_ResolveHost(&PlayAddr[playernum], NULL, portnum);
		}
	}

	/* We're done! */
	GotPlayer[playernum] = 1;
	SDL_free(playerstr);
	return(0);
}

int SetServer(const char *server)
{
	int portnum;
	char *host=NULL, *port=NULL;
	char *serverstr = SDL_strdup(server);

	/* Extract host and port information */
	if ( (host=strchr(serverstr, '@')) == NULL ) {
		error(
		"Server host must be specified in the -server option.\r\n");
		PrintUsage();
	} else
		*(host++) = '\0';
	if ( (port=strchr(serverstr, ':')) != NULL )
		*(port++) = '\0';

	/* We should know how many players we have now */
	if (((gNumPlayers = atoi(serverstr)) <= 0) ||
						(gNumPlayers > MAX_PLAYERS)) {
		error(
"The number of players must be an integer between 1 and %d inclusive.\r\n",
								MAX_PLAYERS);
		PrintUsage();
	}

	/* Resolve the remote address */
	if ( port ) {
		portnum = atoi(port);
	} else {
		portnum = NETPLAY_PORT-1;
	}
	SDLNet_ResolveHost(&ServAddr, host, portnum);
	if ( ServAddr.host == INADDR_NONE ) {
		error("Couldn't resolve host name for %s\r\n", host);
		SDL_free(serverstr);
		return(-1);
	}

	/* We're done! */
	UseServer = 1;
	SDL_free(serverstr);
	return(0);
}

/* This MUST be called after command line options have been processed. */
int CheckPlayers(void)
{
	int i;
	int port;

	/* Check to make sure we have all the players */
	if ( ! UseServer ) {
		for ( i=0, gNumPlayers=0; i<MAX_PLAYERS; ++i ) {
			if ( GotPlayer[i] )
				++gNumPlayers;
		}
		/* Add ourselves if needed */
		if ( gNumPlayers == 0 ) {
			AddPlayer("1");
			gNumPlayers = 1;
			FoundUs = 1;
		}
		for ( i=0; i<gNumPlayers; ++i ) {
			if ( ! GotPlayer[i] ) {
				error(
"Player %d not specified!  Use the -player option for all players.\r\n", i+1);
				return(-1);
			}
		}
	}
	if ( ! FoundUs ) {
		error("Which player are you?  (Use the -player N option)\r\n");
		return(-1);
	}
	if ( (gOurPlayer+1) > gNumPlayers ) {
		error("You cannot be player %d in a %d player game.\r\n",
						gOurPlayer+1, gNumPlayers);
		return(-1);
	}
	if ( (gNumPlayers == 1) && gDeathMatch ) {
		error("Warning: No deathmatch in a single player game!\r\n");
		gDeathMatch = 0;
	}

	/* Oh heck, create the UDP socket here... */
	port = SDL_SwapBE16(PlayAddr[gOurPlayer].port);
	gNetFD = SDLNet_UDP_Open(port);
	if ( gNetFD == NULL ) {
		error("Couldn't create bound network socket");
		return(-1);
	}
	SocketSet = SDLNet_AllocSocketSet(1);
	if ( SocketSet == NULL ) {
		error("Couldn't create socket watch set");
		return(-1);
	}
	SDLNet_UDP_AddSocket(SocketSet, gNetFD);

	/* Now, so we can send to ourselves... */
	PlayAddr[gOurPlayer] = *SDLNet_UDP_GetPeerAddress(gNetFD, -1);
	if ( ! PlayAddr[gOurPlayer].host ) {
		SDLNet_ResolveHost(&PlayAddr[gOurPlayer], "127.0.0.1", port);
	}

	/* Bind all of our players to the channels */
	if ( ! UseServer ) {
		for ( i=0; i<gNumPlayers; ++i ) {
			SDLNet_UDP_Bind(gNetFD, 0, &PlayAddr[i]);
			SDLNet_UDP_Bind(gNetFD, i+1, &PlayAddr[i]);
		}
	}
	return(0);
}


void QueueKey(unsigned char Op, unsigned char Type)
{
	/* Drop keys on a full buffer (assumed never to really happen) */
	if ( OutLen >= (BUFSIZ-2) )
		return;

//error("Queued key 0x%.2x for frame %d\r\n", Type, NextFrame);
	OutBuf[OutLen++] = Op;
	OutBuf[OutLen++] = Type;
}

/* This function is called every frame, and is used to flush the network
   buffers, sending sync and keystroke packets.
   It is called AFTER the keyboard is polled, and BEFORE GetSyncBuf() is
   called by the player objects.

   Note:  We assume that FastRand() isn't called by an interrupt routine,
          otherwise we lose consistency.
*/
	
int SyncNetwork(void)
{
	UDPpacket sent;
	Uint32 seed, frame;
	unsigned char buf[BUFSIZ];
	int index, nleft;

	/* Set the next inbound packet buffer */
	TOGGLE(CurrIn);

	/* Set the frame number */
	frame = NextFrame;
	SDLNet_Write32(frame, &OutBuf[1]);
	seed = GetRandSeed();
	SDLNet_Write32(seed, &OutBuf[1+sizeof(frame)]);

	/* Send the packet to all the players */
	SDLNet_UDP_Send(gNetFD, 0, OutBound[CurrOut]);
	for ( nleft=0, index=0; index<gNumPlayers; ++index ) {
		if ( SyncPtr[index] == NULL ) {
			++nleft;
		}
	}
	NextSync = 0;

	/* Get the inbound packet ready for data */
	sent.data = buf;
	sent.maxlen = sizeof(buf);

	/* Wait for Ack's */
	while ( nleft ) {
		int ready = SDLNet_CheckSockets(SocketSet, 1000+60*gOurPlayer);
		if ( ready == 0 ) {
error("Timed out waiting for frame %ld\r\n", NextFrame);
			/* Timeout, resend the sync packet */
			for ( index=0; index<gNumPlayers; ++index ) {
				if ( SyncPtr[index] == NULL ) {
					SDLNet_UDP_Send(gNetFD, index+1, OutBound[CurrOut]);
				}
			}
		}
		if ( ready <= 0 ) {
			continue;
		}

		/* We are guaranteed that there is data here */
		if ( SDLNet_UDP_Recv(gNetFD, &sent) <= 0 ) {
			error("Network error: SDLNet_UDP_Recv()");
			return(-1);
		}
//error("Received packet!\r\n");

		/* We have a packet! */
		if ( buf[0] == NEW_GAME ) {
			/* Send it back if we are not the server.. */
			if ( gOurPlayer != 0 ) {
				buf[1] = gOurPlayer;
				SDLNet_UDP_Send(gNetFD, -1, &sent);
			}
//error("NEW_GAME packet!\r\n");
			continue;
		}
		if ( buf[0] != SYNC_MSG ) {
			error("Unknown packet: 0x%x\n", buf[0]);
			continue;
		}
		if ( sent.channel <= 0 ) {
			error("Packet from unknown source\n");
			continue;
		}
		index = sent.channel - 1;

		/* Ignore it if it is a duplicate packet */
		if ( SyncPtr[index] != NULL ) {
			continue;
		}

		/* Check the frame number */
		frame = SDLNet_Read32(&buf[1]);
//error("Received a packet of frame %lu from player %d\r\n", frame, index+1);
		if ( frame != NextFrame ) {
			/* We kept the last frame cached, so send it */
			if ( frame == (NextFrame-1) ) {
error("Transmitting packet for old frame (%lu)\r\n", frame);
				SDLNet_UDP_Send(gNetFD, sent.channel, OutBound[!CurrOut]);
			} else if ( frame == (NextFrame+1) ) {
error("Received packet for next frame! (%lu, current = %lu)\r\n",
						frame, NextFrame);
				/* Send this player our current frame */
				SDLNet_UDP_Send(gNetFD, sent.channel, OutBound[CurrOut]);
				/* Cache this frame for next round,
				   skip consistency check, for now */
				memcpy(NextBuf[NextSync], &buf[PDATA_OFFSET], sent.len-PDATA_OFFSET);
				NextPtr[index] = NextBuf[NextSync];
				NextLen[index] = sent.len-PDATA_OFFSET;
				++NextSync;
			}
else
error("Warning! Received packet for really old frame! (%lu, current = %lu)\r\n",
							frame, NextFrame);
			/* Go to select, reset timeout */
			continue;
		}

		/* Do a consistency check!! */
		Uint32 newseed = SDLNet_Read32(&buf[1+sizeof(frame)]);
		if ( newseed != seed ) {
//error("New seed (from player %d) is: 0x%x\r\n", index+1, newseed);
			if ( gOurPlayer == 0 ) {
				error(
"Warning!! \a Frame consistency error with player %d!! (corrected)\r\n", index+1);
SDL_Delay(3000);
			} else	/* Player 1 sent us good seed */
				SeedRandom(newseed);
		}

		/* Okay, we finally have a valid timely packet */
		memcpy(SyncBuf[ThisSync], &buf[PDATA_OFFSET], sent.len-PDATA_OFFSET);
		SyncPtr[index] = SyncBuf[ThisSync];
		SyncLen[index] = sent.len-PDATA_OFFSET;
		++ThisSync;
		--nleft;
	}

	/* Set the next outbound packet buffer */
	++NextFrame;
	TOGGLE(CurrOut);
	OutLen = PDATA_OFFSET;

	return(0);
}

/* This function retrieves a particular player's network buffer */
int GetSyncBuf(int index, unsigned char **bufptr)
{
	int retlen;

	*bufptr = SyncPtr[index];
	SyncPtr[index] = NULL;
	retlen = SyncLen[index];
	SyncLen[index] = 0;
#ifdef SERIOUS_DEBUG
if ( retlen > 0 ) {
	for ( int i=1; i<retlen; i+=2 ) {
		error(
"Keystroke (key = 0x%.2x) for player %d on frame %d!\r\n",
					(*bufptr)[i], index+1, NextFrame);
	}
}
#endif
	return(retlen);
}


inline void SuckPackets(void)
{
	UDPpacket sent;
	unsigned char buf[BUFSIZ];

	sent.data = buf;
	sent.maxlen = sizeof(buf);
	while ( SDLNet_UDP_Recv(gNetFD, &sent) ) {
		/* Keep sucking */ ;
	}
}
	

static inline void MakeNewPacket(int Wave, int Lives, int Turbo,
					unsigned char *packet)
{
	*packet++ = NEW_GAME;
	*packet++ = gOurPlayer;
	*packet++ = (unsigned char)Turbo;
	SDLNet_Write32(Wave, packet);
	packet += 4;
	if ( gDeathMatch ) {
		Lives = (gDeathMatch|0x8000);
	}
	SDLNet_Write32(Lives, packet);
	packet += 4;
	SDLNet_Write32(GetRandSeed(), packet);
}

/* Flash an error up on the screen and pause for 3 seconds */
static void ErrorMessage(const char *message)
{
	/* Display the error message */
	Message(message);

	/* Wait exactly (almost) 3 seconds */
	SDL_Delay(3000);
}

/* If we use an address server, we go here, instead of using Send_NewGame()
   and Await_NewGame()

   The server simply sucks up packets until it gets all player packets.
   It then does error checking, making sure all players agree about who
   they are and how many players will be in the game.  Then it spits a
   packet containing all the player addresses to each player, and then
   waits for a new game...

   We will send a "Hi there" packet to the server and keep resending until
   either the server sends back an error packet, we get an abort signal from
   the user, or we get an addresses packet from the server.
*/
static int AlertServer(int *Wave, int *Lives, int *Turbo)
{
	TCPsocket sock;
	SDLNet_SocketSet socketset;
	Uint8 netbuf[BUFSIZ], sendbuf[NEW_PACKETLEN+4+1];
	char *ptr;
	int i, len, lenread;
	Uint32 lives, seed;
	int waiting;
	int status;
	const char *message = NULL;

	/* Our address server connection is through TCP */
	Message("Connecting to Address Server");
	sock = SDLNet_TCP_Open(&ServAddr);
	if ( sock == NULL ) {
		ErrorMessage("Connection failed");
		return(-1);
	}
	socketset = SDLNet_AllocSocketSet(1);
	if ( socketset == NULL ) {
		status = -1;
		message = "Couldn't create socket set";
		goto done;
	}
	SDLNet_TCP_AddSocket(socketset, sock);

	MakeNewPacket(*Wave, *Lives, *Turbo, sendbuf);
	len = NEW_PACKETLEN;
	SDLNet_Write32(SDL_SwapBE16(PlayAddr[gOurPlayer].port), sendbuf+len);
	len += 4;
	sendbuf[len] = (Uint8)gNumPlayers;
	len += 1;
	if ( SDLNet_TCP_Send(sock, sendbuf, len) != len ) {
		status = -1;
		message = "Socket write error";
		goto done;
	}

	Message("Waiting for other players");
	status = 0;
	len = 0;
	lenread = 0;
	waiting = 1;
	while ( waiting ) {
		if ( SDLNet_CheckSockets(socketset, 1000) <= 0 ) {
			HandleEvents(0);
			/* Peek at key buffer for Quit key */
			for ( i=(PDATA_OFFSET+1); i<OutLen; i += 2 ) {
				if ( OutBuf[i] == ABORT_KEY ) {
					netbuf[0] = NET_ABORT;
					SDLNet_TCP_Send(sock, netbuf, 1);
					waiting = 0;
					status = -1;
				}
			}
			OutLen = PDATA_OFFSET;
			continue;
		}

		/* We are guaranteed that there is data here */
		len = SDLNet_TCP_Recv(sock, &netbuf[len], BUFSIZ-len-1);
		if ( len <= 0 ) {
			waiting = 0;
			status = -1;
			message = "Error reading player addresses";
			continue;
		}
		lenread += len;

		/* The very first byte is a packet length */
		if ( len < netbuf[0] )
			continue;

		if ( netbuf[0] <= 1 ) {
			waiting = 0;
			status = -1;
			message = "Error: Short server packet!";
			continue;
		}
		switch ( netbuf[1] ) {
			case NEW_GAME:	/* Extract parameters, addresses */
				*Turbo = (int)netbuf[2];
				len = 3;
				*Wave = SDLNet_Read32(&netbuf[len]);
				len += 4;
				lives = SDLNet_Read32(&netbuf[len]);
				len += 4;
				if ( lives & 0x8000 )
					gDeathMatch = (lives&(~0x8000));
				else
					*Lives = lives;
				seed = SDLNet_Read32(&netbuf[len]);
				len += 4;
				SeedRandom(seed);
//error("Seed is 0x%x\r\n", seed);

				ptr = (char *)&netbuf[len];
				for ( i=0; i<gNumPlayers; ++i ) {
					if ( i == gOurPlayer ) {
						/* Skip address */
						ptr += (strlen(ptr)+1);
						ptr += (strlen(ptr)+1);
						continue;
					}

					/* Resolve the remote address */
					char *host, *port;
					host = ptr;
					ptr += strlen(host)+1;
					port = ptr;
					ptr += strlen(port)+1;
					SDLNet_ResolveHost(&PlayAddr[i], host, atoi(port));
//printf("Port = %s\r\n", ptr);
				}
				waiting = 0;
				break;

			case NET_ABORT:	/* Some error? */
				netbuf[len] = '\0';
				message = (char *)&netbuf[2];
				waiting = 0;
				status = -1;
				break;

			default:	/* Huh? */
				break;
		}
	}
	for ( i=0; i<gNumPlayers; ++i ) {
		SDLNet_UDP_Bind(gNetFD, 0, &PlayAddr[i]);
		SDLNet_UDP_Bind(gNetFD, i+1, &PlayAddr[i]);
	}
	NextFrame = 0L;
done:
	if ( (status < 0) && message ) {
		ErrorMessage(message);
	}
	return(status);
}

/* This function sends a NEWGAME packet, and waits for all other players
   to respond in kind.
   This function is not very robust in handling errors such as multiple
   machines thinking they are the same player.  The address server is
   supposed to handle such things gracefully.
*/
int Send_NewGame(int *Wave, int *Lives, int *Turbo)
{
	Uint8 netbuf[BUFSIZ], sendbuf[NEW_PACKETLEN];
	char message[BUFSIZ];
	int  nleft, n;
	int  acked[MAX_PLAYERS];
	int  i;
	UDPpacket newgame, sent;

	/* Don't do the usual rigamarole if we have a game server */
	if ( UseServer )
		return(AlertServer(Wave, Lives, Turbo));

	/* Send all the packets */
	MakeNewPacket(*Wave, *Lives, *Turbo, sendbuf);
	newgame.data = sendbuf;
	newgame.len = sizeof(sendbuf);
	SDLNet_UDP_Send(gNetFD, 0, &newgame);

	/* Get ready for responses */
	memset(acked, 0, (sizeof acked));
	sent.data = netbuf;
	sent.maxlen = sizeof(netbuf);

	/* Wait for Ack's */
	for ( nleft=gNumPlayers, n=0; nleft; ) {
		/* Show a status */
		SDL_strlcpy(message, "Waiting for players:", sizeof(message));
		for ( i=0; i<gNumPlayers; ++i ) {
			if ( ! acked[i] )
				SDL_snprintf(&message[strlen(message)], sizeof(message)-strlen(message), " %d", i+1);
		}
		Message(message);

		if ( SDLNet_CheckSockets(SocketSet, 1000) <= 0 ) {
			HandleEvents(0);
			/* Peek at key buffer for Quit key */
			for ( i=(PDATA_OFFSET+1); i<OutLen; i += 2 ) {
				if ( OutBuf[i] == ABORT_KEY ) {
					OutLen = PDATA_OFFSET;
					return(-1);
				}
			}
			OutLen = PDATA_OFFSET;

			/* Every three seconds...resend the new game packet */
			if ( (n++)%3 != 0 )
				continue;

			for ( i=gNumPlayers; i--; ) {
				if ( ! acked[i] ) {
					SDLNet_UDP_Send(gNetFD, i+1, &newgame);
				}
			}
			continue;
		}

		/* We are guaranteed that there is data here */
		if ( SDLNet_UDP_Recv(gNetFD, &sent) <= 0 ) {
			ErrorMessage("Network error receiving packets");
			return(-1);
		}

		/* We have a packet! */
		if ( netbuf[0] != NEW_GAME ) {
			/* Continue waiting */
#ifdef VERBOSE
			error("Unknown packet: 0x%x\r\n", netbuf[0]);
#endif
			continue;
		}

		/* Loop, check the address */
		for ( i=gNumPlayers; i--; ) {
			if ( acked[i] )
				continue;

			/* Check both the host AND port!! :-) */
			if ( (sent.address.host != PlayAddr[i].host) ||
			     (sent.address.port != PlayAddr[i].port) )
				continue;

			/* Check the player... */
			if ( (i != gOurPlayer) && (netbuf[1] == gOurPlayer) ) {
				/* Print message, sleep 3 seconds absolutely */
				SDL_snprintf(message, sizeof(message),
	"Error: Another player (%d) thinks they are player 1!\r\n", i+1);
				ErrorMessage(message);
				/* Suck up retransmission packets */
				SuckPackets();
				return(-1);
			}

			/* Check them off our list.. */
			--nleft;
			acked[i] = 1;
			break;
		}
	}
	NextFrame = 0L;
	return(0);
}

int Await_NewGame(int *Wave, int *Lives, int *Turbo)
{
	unsigned char netbuf[BUFSIZ];
	int len, gameon;
	UDPpacket sent;
	Uint32 lives, seed;

	/* Don't do the usual rigamarole if we have a game server */
	if ( UseServer )
		return(AlertServer(Wave, Lives, Turbo));

	/* Get ready to wait for server */
	Message("Awaiting Player 1 (server)");
	sent.data = netbuf;
	sent.maxlen = sizeof(netbuf);

	gameon = 0;
	while ( ! gameon ) {
		if ( SDLNet_CheckSockets(SocketSet, 1000) <= 0 ) {
			HandleEvents(0);
			/* Peek at key buffer for Quit key */
			for ( int i=(PDATA_OFFSET+1); i<OutLen; i += 2 ) {
				if ( OutBuf[i] == ABORT_KEY ) {
					OutLen = PDATA_OFFSET;
					return(-1);
				}
			}
			OutLen = PDATA_OFFSET;
			continue;
		}

		/* We are guaranteed that there is data here */
		if ( SDLNet_UDP_Recv(gNetFD, &sent) <= 0 ) {
			ErrorMessage("Network error receiving packets");
			return(-1);
		}

		/* We have a packet! */
		if ( netbuf[0] != NEW_GAME ) {
#ifdef VERBOSE
			error(
			"Await_NewGame(): Unknown packet: 0x%x\r\n", netbuf[0]);
#endif
			continue;
		}

		/* Extract the RandomSeed and return the packet */
		*Turbo = (int)netbuf[2];
		len = 3;
		*Wave = SDLNet_Read32(&netbuf[len]);
		len += 4;
		lives = SDLNet_Read32(&netbuf[len]);
		len += 4;
		if ( lives & 0x8000 )
			gDeathMatch = (lives&(~0x8000));
		else
			*Lives = lives;
		seed = SDLNet_Read32(&netbuf[len]);
		len += 4;
		SeedRandom(seed);
//error("Seed is 0x%x\r\n", seed);

		netbuf[1] = gOurPlayer;
		SDLNet_UDP_Send(gNetFD, 1, &sent);

		/* Note that we don't guarantee delivery of the NEW_GAME ack.
		   That's okay, we have the checksum.  We will hang on the very
		   first frame, and we echo back all NEW_GAME packets at that
		   point as well.
		*/
		NextFrame = 0L;
		gameon = 1;
	}
	return(0);
}
