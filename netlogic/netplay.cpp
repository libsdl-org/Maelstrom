
/* This contains the network play functions and data */

#include "Maelstrom_Globals.h"
#include "netplay.h"
#include "protocol.h"

#ifdef _SGI_SOURCE
#include <bstring.h>
#endif
#ifdef _AIX
#include <sys/select.h>
#endif
#include <errno.h>
#ifdef __WIN95__
#include <winsock.h>
#define write(fd, buf, len)	send(fd, buf, len, 0)
#define read(fd, buf, len)	recv(fd, buf, len, 0)
#define close(fd)		closesocket(fd)
#define perror(str)		neterror(str)
#else
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* Win95 */

int   gNumPlayers;
int   gOurPlayer;
int   gDeathMatch;
int   gNetFD;

static int            GotPlayer[MAX_PLAYERS];
static int            PlayPort[MAX_PLAYERS];
static struct sockaddr_in PlayAddr[MAX_PLAYERS];
static struct sockaddr_in ServAddr;
static int            FoundUs, UseServer;
static unsigned long  NextFrame;
static unsigned char  OutBufs[2][BUFSIZ];
static int            OutLens[2];
static int            CurrOut;
/* This is the data offset of a SYNC packet */
#define PDATA_OFFSET	(1+1+sizeof(unsigned long)+sizeof(unsigned long))

/* We keep one packet backlogged for retransmission */
#define OutBuf		OutBufs[CurrOut]
#define OutLen		OutLens[CurrOut]
#define LastBuf		OutBufs[CurrOut ? 0 : 1]
#define LastLen		OutLens[CurrOut ? 0 : 1]

static unsigned char *SyncPtrs[2][MAX_PLAYERS];
static unsigned char  SyncBufs[2][MAX_PLAYERS][BUFSIZ];
static int            SyncLens[2][MAX_PLAYERS];
static int            ThisSyncs[2];
static int            CurrIn;

/* We cache one packet if the other player is ahead of us */
#define SyncPtr		SyncPtrs[CurrIn]
#define SyncBuf		SyncBufs[CurrIn]
#define SyncLen		SyncLens[CurrIn]
#define ThisSync	ThisSyncs[CurrIn]
#define NextPtr		SyncPtrs[CurrIn ? 0 : 1]
#define NextBuf		SyncBufs[CurrIn ? 0 : 1]
#define NextLen		SyncLens[CurrIn ? 0 : 1]
#define NextSync	ThisSyncs[CurrIn ? 0 : 1]

#define TOGGLE(var)	var = (var ? 0 : 1)


#ifdef __WIN95__
static void neterror(char *str) {
	char *errmsg;

	switch (WSAGetLastError()) {
		case 0:
			errmsg="No error!";
			break;
		case WSANOTINITIALISED:
			errmsg="WSAStartup() has not been called";
			break;
		case WSAENETDOWN:
			errmsg="The network is down";
			break;
		case WSAEINVAL:
			errmsg="Invalid function parameter";
			break;
		case WSAEINTR:
			errmsg="Function cancelled by WSACancelBlockingCall()";
			break;
		case WSAEINPROGRESS:
			errmsg="A blocking network function is in progress";
			break;
		case WSAENOTSOCK:
			errmsg="Socket parameter not a socket";
			break;
		case WSAHOST_NOT_FOUND:
			errmsg="Host not found";
			break;
		case WSATRY_AGAIN:
			errmsg="Try again later";
			break;
		case WSANO_RECOVERY:
			errmsg="Unrecoverable error";
			break;
		default:
			errmsg="Unknown network error";
			break;
	}
	if ( *str )
		error("%s: %s\n", str, errmsg);
	else
		error("%s\n", errmsg);
}
#endif /* __WIN95__ */


void InitNetData(void)
{
	int i;

#ifdef __WIN95__
	/* Start up the windows networking */
	WORD version_wanted = MAKEWORD(1,1);
	WSADATA wsaData;

	if ( WSAStartup(version_wanted, &wsaData) != 0 ) {
		error("NetLogic: Couldn't initialize networking!\n");
		return;
	}
#endif
	/* Initialize network game variables */
	FoundUs   = 0;
	gOurPlayer  = -1;
	gDeathMatch = 0;
	UseServer = 0;
	for ( i=0; i<MAX_PLAYERS; ++i ) {
		GotPlayer[i] = 0;
		PlayPort[i] = NETPLAY_PORT+i;
		SyncPtrs[0][i] = NULL;
		SyncPtrs[1][i] = NULL;
	}
	OutBufs[0][0] = SYNC_MSG;
	OutBufs[1][0] = SYNC_MSG;
	/* Type field, frame sequence, current random seed */
	OutLens[0] = PDATA_OFFSET;
	OutLens[1] = PDATA_OFFSET;
	CurrOut = 0;

	ThisSyncs[0] = 0;
	ThisSyncs[1] = 0;
	CurrIn = 0;
}

void HaltNetData(void)
{
#ifdef __WIN95__
	if ( WSACleanup() == SOCKET_ERROR ) {
		if ( WSAGetLastError() == WSAEINPROGRESS ) {
			(void) WSACancelBlockingCall();
			(void) WSACleanup();
		}
	}
#endif
}

int AddPlayer(char *playerstr)
{
	struct hostent *hp;
	int playernum;
	char *host=NULL, *port=NULL;

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
		PukeUsage();
	}

	/* Do some error checking */
	if ( GotPlayer[--playernum] ) {
		error("Player %d specified multiple times!\r\n", playernum+1);
		return(-1);
	}
	if ( host ) {
		/* Resolve the remote address */
		if ((PlayAddr[playernum].sin_addr.s_addr =
						inet_addr(host)) == 0xFFFFFFFF)
		{
			if ((hp=gethostbyname(host)) == NULL) {
				error(
			"Couldn't resolve host name for %s\r\n", host);
#ifdef __WIN95__
				perror("Problem was");
#endif
				return(-1);
			}
			memcpy(&PlayAddr[playernum].sin_addr,hp->h_addr_list[0],
					sizeof(PlayAddr[playernum].sin_addr));
		}
	} else { /* No host specified, local player */
		if ( FoundUs ) {
			error(
"More than one local player!  (players %d and %d specified as local players)\r\n",
						gOurPlayer+1, playernum+1);
			return(-1);
		} else {
			gOurPlayer = playernum;
			FoundUs = 1;
			PlayAddr[playernum].sin_addr.s_addr = htonl(INADDR_ANY);
		}
	}
	if ( port )
		PlayAddr[playernum].sin_port = htons(atoi(port));
	else
		PlayAddr[playernum].sin_port = htons(PlayPort[playernum]);
	PlayAddr[playernum].sin_family = AF_INET;

	/* We're done! */
	GotPlayer[playernum] = 1;
	return(0);
}

int SetServer(char *serverstr)
{
	struct hostent *hp;
	char *host=NULL, *port=NULL;

	/* Extract host and port information */
	if ( (host=strchr(serverstr, '@')) == NULL ) {
		error(
		"Server host must be specified in the -server option.\r\n");
		PukeUsage();
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
		PukeUsage();
	}

	/* Resolve the remote address */
	if ((ServAddr.sin_addr.s_addr = inet_addr(host)) == 0xFFFFFFFF) {
		if ((hp=gethostbyname(host)) == NULL) {
			error("Couldn't resolve host name for %s\r\n", host);
			return(-1);
		}
		memcpy(&ServAddr.sin_addr,hp->h_addr_list[0],
						sizeof(ServAddr.sin_addr));
	}
	if ( port )
		ServAddr.sin_port = htons(atoi(port));
	else
		ServAddr.sin_port = htons(NETPLAY_PORT-1);
	ServAddr.sin_family = AF_INET;

	/* We're done! */
	UseServer = 1;
	return(0);
}

/* This MUST be called after command line options have been processed. */
int CheckPlayers(void)
{
	int i;

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
	if ( (gNetFD=socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("Couldn't create network socket");
		return(-1);
	}
	/* Bind our address so we can be sent to */
	if ( bind(gNetFD, (struct sockaddr *)&PlayAddr[gOurPlayer],
					sizeof(PlayAddr[gOurPlayer])) < 0 ) {
		perror("Couldn't bind socket to local port");
		close(gNetFD);
		return(-1);
	}

	/* Now, so we can send to ourselves... */
	PlayAddr[gOurPlayer].sin_addr.s_addr = inet_addr("127.0.0.1");

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
	int  nleft;
	int  i, clen, len;
	struct sockaddr_in from;
	unsigned long frame, seed, newseed;
	struct timeval timeout;
	fd_set fdset;
	unsigned char buf[BUFSIZ];

	/* Set the next inbound packet buffer */
	TOGGLE(CurrIn);

	/* Set the frame number */
	frame = NextFrame;
//error("Sending out %d packets of frame %lu\n", gNumPlayers, frame);
	frame = htonl(frame);
	memcpy(&OutBuf[1], &frame, sizeof(frame));
	seed = GetRandSeed();
//error("My seed is: 0x%x\r\n", seed);
	seed = htonl(seed);
	memcpy(&OutBuf[1+sizeof(frame)], &seed, sizeof(seed));
	seed = ntohl(seed);
//error("My seed is: 0x%x\r\n", seed);

	/* Send all the packets */
	for ( nleft=gNumPlayers, i=gNumPlayers; i--; ) {
		if (sendto(gNetFD,(char *)OutBuf,OutLen,0,(struct sockaddr *)
				&PlayAddr[i], sizeof(PlayAddr[i])) != OutLen) {
			/* Clear errno here, so select() doesn't get it */
			errno = 0;
		}
		if ( SyncPtr[i] != NULL )
			--nleft;
	}
	NextSync = 0;

	/* Wait for Ack's */
	while ( nleft ) {
		/* Set the timeout */
		timeout.tv_sec = 1;
		timeout.tv_usec = (60*gOurPlayer);

		/* Set the UDP file descriptor, and wait! */
//error("Waiting for packet on frame %d...\r\n", NextFrame);
getit:
//error("select()...\r\n");
		FD_ZERO(&fdset);
		FD_SET(gNetFD, &fdset);
#ifdef _INCLUDE_HPUX_SOURCE
		if ( select(gNetFD+1, (int *)&fdset, NULL, NULL, &timeout)
								<= 0 ) {
#else
		if ( select(gNetFD+1, &fdset, NULL, NULL, &timeout) <= 0 ) {
#endif
#ifdef __WIN95__
			if ( ! WSAGetLastError() ) {
#else
			if ( ! errno ) {
#endif
error("Timed out waiting for frame %ld\r\n", NextFrame);
				/* Timeout, resend the sync packet */
				for ( i=gNumPlayers; i--; ) {
					if ( SyncPtr[i] == NULL ) {
		(void) sendto(gNetFD, (char *)OutBuf, OutLen, 0,
			(struct sockaddr *)&PlayAddr[i], sizeof(PlayAddr[i]));
					}
				}
				continue;
			} else if ( (errno != EINTR) && (errno != EAGAIN) ) {
				perror("select() error");
				return(-1);
			}
			/* Don't reset the timer -- we had sound I/O */
//perror("Select error, EINTR");
			errno = 0;
			goto getit;
		}

		/* We are guaranteed that there is data here */
readit:
		clen = sizeof(from);
		len = recvfrom(gNetFD, (char *)buf, BUFSIZ, 0, 
					(struct sockaddr *)&from, &clen);
		if ( len <= 0 ) {
			if ( errno == EINTR ) {
				errno = 0;
				goto readit;
			}

			perror("Network error: recvfrom()");
			return(-1);
		}
//error("Received packet!\r\n");

		/* We have a packet! */
		if ( buf[0] == NEW_GAME ) {
			/* Send it back if we are not the server.. */
			if ( gOurPlayer != 0 ) {
				buf[1] = gOurPlayer;
				(void) sendto(gNetFD, (char *)buf, len, 0,
					(struct sockaddr *)&from, clen);
			}
//error("NEW_GAME packet!\r\n");
			continue;
		}
		if ( buf[0] != SYNC_MSG ) {
			error("Unknown packet: 0x%x\r\n", buf[0]);
			continue;
		}

		/* Loop, check the address */
		for ( i=gNumPlayers; i--; ) {
			if ( SyncPtr[i] != NULL )
				continue;

			/* Check both the host AND port!! :-) */
			if ( (memcmp(&from.sin_addr.s_addr,
					&PlayAddr[i].sin_addr.s_addr,
					sizeof(from.sin_addr.s_addr)) != 0) ||
			     (from.sin_port != PlayAddr[i].sin_port) )
				continue;

			/* Check the frame number */
			memcpy(&frame, &buf[1], sizeof(frame));
			frame = ntohl(frame);
//error("Received a packet of frame %lu from player %d\r\n", frame, i+1);
			if ( frame != NextFrame ) {
				/* We kept the last frame cached, so send it */
				if ( frame == (NextFrame-1) ) {
error("Transmitting packet for old frame (%lu)\r\n", frame);
					(void) sendto(gNetFD, (char *)LastBuf,
						LastLen, 0,
						(struct sockaddr *)&PlayAddr[i],
							sizeof(PlayAddr[i]));
				} else if ( frame == (NextFrame+1) ) {
error("Received packet for next frame! (%lu, current = %lu)\r\n",
							frame, NextFrame);
					/* Send this player our current frame */
					(void) sendto(gNetFD, (char *)OutBuf,
							OutLen, 0,
			(struct sockaddr *)&PlayAddr[i], sizeof(PlayAddr[i]));
					/* Cache this frame for next round,
					   skip consistency check, for now */
					memcpy(NextBuf[NextSync], &buf[PDATA_OFFSET], len-PDATA_OFFSET);
					NextPtr[i] = NextBuf[NextSync];
					NextLen[i] = len-PDATA_OFFSET;
					++NextSync;
				}
else
error("Warning! Received packet for really old frame! (%lu, current = %lu)\r\n",
							frame, NextFrame);
				/* Go to select, reset timeout */
				break;
			}

			/* Do a consistency check!! */
			memcpy(&newseed, &buf[1+sizeof(frame)],
							sizeof(newseed));
			newseed = ntohl(newseed);
			if ( newseed != seed ) {
//error("New seed (from player %d) is: 0x%x\r\n", i+1, newseed);
				if ( gOurPlayer == 0 ) {
					error(
"Warning!! \a Frame consistency error with player %d!! (corrected)\r\n", i+1);
sleep(3);
				} else	/* Player 1 sent us good seed */
					SeedRandom(newseed);
			}

			/* Okay, we finally have a valid timely packet */
			memcpy(SyncBuf[ThisSync], &buf[PDATA_OFFSET], len-PDATA_OFFSET);
			SyncPtr[i] = SyncBuf[ThisSync];
			SyncLen[i] = len-PDATA_OFFSET;
			++ThisSync;
			--nleft;

			/* Get out of the address check loop */
			break;
		}
		/* We assume NO CODE here!! */
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
	struct timeval timeout;
	fd_set fdset;
	char   netbuf[BUFSIZ];
	int    clen;
	struct sockaddr_in from;
	
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	for ( ; ; ) {
		FD_ZERO(&fdset);
		FD_SET(gNetFD, &fdset);
#ifdef _INCLUDE_HPUX_SOURCE
		if ( select(gNetFD+1, (int *)&fdset, NULL, NULL, &timeout)
									!= 1 )
#else
		if ( select(gNetFD+1, &fdset, NULL, NULL, &timeout) != 1 )
#endif
			break;

		/* Suck up the packet */
		clen = sizeof(from);
		(void) recvfrom(gNetFD, netbuf, BUFSIZ, 0, 
					(struct sockaddr *)&from, &clen);
	}
}
	

static inline void MakeNewPacket(int Wave, int Lives, int Turbo,
							unsigned char *packet)
{
	unsigned long seed, wave, lives;

	packet[0] = NEW_GAME;
	packet[1] = gOurPlayer;
	packet[2] = (unsigned char)Turbo;
	wave = Wave;
	wave = htonl(wave);
	memcpy(&packet[3], &wave, sizeof(wave));
	if ( gDeathMatch )
		lives = (gDeathMatch|0x8000);
	else
		lives = Lives;
	lives = htonl(lives);
	memcpy(&packet[3+sizeof(wave)], &lives, sizeof(lives));
	seed = GetRandSeed();
	seed = htonl(seed);
	memcpy(&packet[3+sizeof(wave)+sizeof(lives)], &seed, sizeof(seed));
}

/* Flash an error up on the screen and pause for 3 seconds */
static void ErrorMessage(char *message, int errnum)
{
	char   mesgbuf[BUFSIZ];
	time_t then, now;

	/* Display the error message */
	if ( errnum )
		sprintf(mesgbuf, "%s: %s", message, strerror(errnum));
	else
		strcpy(mesgbuf, message);
	Message(mesgbuf);

	/* Wait exactly (almost) 3 seconds */
	then = time(NULL);
	do {
		sleep(1);
		now = time(NULL);
	} while ( (now-then) < 3 );
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
	unsigned char  netbuf[BUFSIZ], sendbuf[NEW_PACKETLEN+4+1];
	char          *ptr;
	unsigned long  wave, lives, seed, myport;
	struct timeval timeout;
	fd_set         fdset;
	int            i, len, clen;
	int            sockfd, done = 0;

	/* Our address server connection is through TCP */
	if ( (sockfd=socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		ErrorMessage("Can't create stream socket", errno);
		return(-1);
	}

	Message("Connecting to Address Server");
	if ( connect(sockfd, (struct sockaddr *)&ServAddr, sizeof(ServAddr))
									< 0 ) {
		ErrorMessage("Connection failed", errno);
		close(sockfd);
		return(-1);
	}

	MakeNewPacket(*Wave, *Lives, *Turbo, sendbuf);
	myport = htonl((unsigned long)ntohs(PlayAddr[gOurPlayer].sin_port));
//printf("My port = %lu  (size = %d)\r\n", ntohl(myport), sizeof(myport));
	memcpy(&sendbuf[NEW_PACKETLEN], &myport, sizeof(myport));
	sendbuf[NEW_PACKETLEN+4] = (unsigned char)gNumPlayers;
	if ( write(sockfd, sendbuf, NEW_PACKETLEN+4+1) != NEW_PACKETLEN+4+1 ) {
		ErrorMessage("Socket write error", errno);
		close(sockfd);
		return(-1);
	}

	Message("Waiting for other players");
	len = 0;
	while ( ! done ) {
		/* Set the timeout */
		timeout.tv_sec = 1;	/* Poll for I/O every 1 second */
		timeout.tv_usec = 0;

		/* Set the UDP file descriptor, and wait! */
		FD_ZERO(&fdset);
		FD_SET(sockfd, &fdset);
#ifdef _INCLUDE_HPUX_SOURCE
		if ( select(sockfd+1, (int *)&fdset, NULL, NULL, &timeout)
								<= 0 ) {
#else
		if ( select(sockfd+1, &fdset, NULL, NULL, &timeout) <= 0 ) {
#endif
#ifdef __WIN95__
			if ( ! WSAGetLastError() ) {
#else
			if ( ! errno ) {	// Timeout, handle keys.
#endif
				HandleEvents(0);
				/* Peek at key buffer for Quit key */
				for ( i=(PDATA_OFFSET+1); i<OutLen; i += 2 ) {
					if ( OutBuf[i] == ABORT_KEY ) {
						OutLen = PDATA_OFFSET;
						netbuf[0] = NET_ABORT;
						(void)write(sockfd, netbuf, 1);
						close(sockfd);
						return(-1);
					}
				}
				OutLen = PDATA_OFFSET;
			} else {
				/* We ignore other errors.. who cares. :) */
				errno = 0;
			}
			continue;
		}

		/* We are guaranteed that there is data here */
		if ( (clen = read(sockfd, &netbuf[len], BUFSIZ-len-1)) <= 0 ) {
			ErrorMessage("Error reading player addresses", errno);
			close(sockfd);
			return(-1);
		}
		len += clen;

		/* The very first byte is a packet length */
		if ( len < netbuf[0] )
			continue;

		if ( netbuf[0] <= 1 ) {
			ErrorMessage("Error: Short server packet!", 0);
			close(sockfd);
			return(-1);
		}
		switch ( netbuf[1] ) {
			case NEW_GAME:	/* Extract parameters, addresses */
				*Turbo = (int)netbuf[2];
				memcpy(&wave, &netbuf[3], sizeof(wave));
				*Wave = ntohl(wave);
				memcpy(&lives,
					&netbuf[3+sizeof(wave)],sizeof(lives));
				lives = ntohl(lives);
				if ( lives & 0x8000 )
					gDeathMatch = (lives&(~0x8000));
				else
					*Lives = lives;
				memcpy(&seed,
					&netbuf[3+sizeof(wave)+sizeof(lives)],
								sizeof(seed));
				seed = ntohl(seed);
				SeedRandom(seed);
//error("Seed is 0x%x\r\n", seed);

				ptr = (char *)&netbuf[3+sizeof(wave)+
						sizeof(lives)+sizeof(seed)];
				for ( i=0; i<gNumPlayers; ++i ) {
					if ( i == gOurPlayer ) {
						/* Skip address */
						ptr += (strlen(ptr)+1);
						ptr += (strlen(ptr)+1);
						continue;
					}

					/* Resolve the remote address */
					PlayAddr[i].sin_addr.s_addr =
							inet_addr(ptr);
					ptr += (strlen(ptr)+1);
					PlayAddr[i].sin_port = htons(atoi(ptr));
//printf("Port = %s\r\n", ptr);
					ptr += (strlen(ptr)+1);
					PlayAddr[i].sin_family = AF_INET;
				}
				close(sockfd);
				done = 1;
				break;

			case NET_ABORT:	/* Some error? */
				netbuf[len] = '\0';
				ErrorMessage((char *)&netbuf[2], 0);
				close(sockfd);
				return(-1);

			default:	/* Huh? */
					break;
		}
	}
	NextFrame = 0L;
	return(0);
}

/* This function sends a NEWGAME packet, and waits for all other players
   to respond in kind.
   This function is not very robust in handling errors such as multiple
   machines thinking they are the same player.  The address server is
   supposed to handle such things gracefully.
*/
int Send_NewGame(int *Wave, int *Lives, int *Turbo)
{
	unsigned char netbuf[BUFSIZ], sendbuf[NEW_PACKETLEN];
	char message[BUFSIZ];
	int  nleft, n;
	int  acked[MAX_PLAYERS];
	int  i, clen, len;
	struct sockaddr_in from;
	struct timeval timeout;
	fd_set fdset;

	/* Don't do the usual rigamarole if we have a game server */
	if ( UseServer )
		return(AlertServer(Wave, Lives, Turbo));

	/* Send all the packets */
	MakeNewPacket(*Wave, *Lives, *Turbo, sendbuf);
	for ( i=gNumPlayers; i--; ) {
		if ( sendto(gNetFD, (char *)sendbuf, NEW_PACKETLEN, 0,
				(struct sockaddr *) &PlayAddr[i],
					sizeof(PlayAddr[i])) != 
						NEW_PACKETLEN ) {
			perror("Warning: sending of NEW_GAME packet failed");
		}
	}
	for ( i=gNumPlayers; i--; )
		acked[i] = 0;

	/* Wait for Ack's */
	for ( nleft=gNumPlayers, n=0; nleft; ) {
		/* Show a status */
		strcpy(message, "Waiting for players:");
		for ( i=0; i<gNumPlayers; ++i ) {
			if ( ! acked[i] )
				sprintf(&message[strlen(message)], " %d", i+1);
		}
		Message(message);

		/* Set the timeout */
		timeout.tv_sec = 1;	/* Poll for I/O every 1 second */
		timeout.tv_usec = 0;

getit:
		/* Set the UDP file descriptor, and wait! */
		FD_ZERO(&fdset);
		FD_SET(gNetFD, &fdset);
#ifdef _INCLUDE_HPUX_SOURCE
		if ( select(gNetFD+1, (int *)&fdset, NULL, NULL, &timeout)
								<= 0 ) {
#else
		if ( select(gNetFD+1, &fdset, NULL, NULL, &timeout) <= 0 ) {
#endif
#ifdef __WIN95__
			if ( ! WSAGetLastError() ) {
#else
			if ( ! errno ) {	// Timeout, handle keys.
#endif
				HandleEvents(0);
				/* Peek at key buffer for Quit key */
				for ( i=(PDATA_OFFSET+1); i<OutLen; i += 2 ) {
					if ( OutBuf[i] == ABORT_KEY ) {
						OutLen = PDATA_OFFSET;
						return(-1);
					}
				}
				OutLen = PDATA_OFFSET;
			} else {
				/* We ignore other errors.. who cares. :) */
				errno = 0;
			}

			/* Every three seconds...resend the new game packet */
			if ( (n++)%3 != 0 )
				continue;

			for ( i=gNumPlayers; i--; ) {
				if ( ! acked[i] ) {
		(void) sendto(gNetFD, (char *)sendbuf, NEW_PACKETLEN, 0,
			(struct sockaddr *)&PlayAddr[i], sizeof(PlayAddr[i]));
				}
			}
			continue;
		}

		/* We are guaranteed that there is data here */
		clen = sizeof(from);
		len = recvfrom(gNetFD, (char *)netbuf, BUFSIZ, 0, 
					(struct sockaddr *)&from, &clen);
		if ( len <= 0 ) {
			perror("Network error in Send_NewGame(): recvfrom()");
			return(-1);
		}

		/* We have a packet! */
		if ( netbuf[0] != NEW_GAME ) {
			/* Continue on the select() */
#ifdef VERBOSE
			error("Unknown packet: 0x%x\r\n", netbuf[0]);
#endif
			goto getit;
		}

		/* Loop, check the address */
		for ( i=gNumPlayers; i--; ) {
			if ( acked[i] )
				continue;

			/* Check both the host AND port!! :-) */
			if ( (memcmp(&from.sin_addr.s_addr,
					&PlayAddr[i].sin_addr.s_addr,
					sizeof(from.sin_addr.s_addr)) != 0) ||
			     (from.sin_port != PlayAddr[i].sin_port) )
				continue;

			/* Check the player... */
			if ( (i != gOurPlayer) && (netbuf[1] == gOurPlayer) ) {
				/* Print message, sleep 3 seconds absolutely */
				sprintf(message, 
	"Error: Another player (%d) thinks they are player 1!\r\n", i+1);
				ErrorMessage(message, 0);
				/* Suck up retransmission packets */
				SuckPackets();
				return(-1);
			}

			/* Check them off our list.. */
			acked[i] = 1;
			--nleft;
			break;
		}
	}
	NextFrame = 0L;
	return(0);
}

int Await_NewGame(int *Wave, int *Lives, int *Turbo)
{
	unsigned char netbuf[BUFSIZ];
	int   i, clen, len, gameon;
	struct sockaddr_in from;
	fd_set fdset;
	struct timeval timeout;
	unsigned long seed, wave, lives;

	/* Don't do the usual rigamarole if we have a game server */
	if ( UseServer )
		return(AlertServer(Wave, Lives, Turbo));


	/* Set the UDP file descriptor, and wait! */
	gameon = 0;
	Message("Awaiting Player 1 (server)");

	while ( ! gameon ) {
		timeout.tv_sec = 1;	/* Poll for I/O every 1 second */
		timeout.tv_usec = 0;
		FD_ZERO(&fdset);
		FD_SET(gNetFD, &fdset);
#ifdef _INCLUDE_HPUX_SOURCE
		if ( select(gNetFD+1, (int *)&fdset, NULL, NULL, &timeout)
								<= 0 ) {
#else
		if ( select(gNetFD+1, &fdset, NULL, NULL, &timeout) <= 0 ) {
#endif
#ifdef __WIN95__
			if ( ! WSAGetLastError() ) {
#else
			if ( ! errno ) {	// Timeout, handle keys.
#endif
				HandleEvents(0);
				/* Peek at key buffer for Quit key */
				for ( i=(PDATA_OFFSET+1); i<OutLen; i += 2 ) {
					if ( OutBuf[i] == ABORT_KEY ) {
						OutLen = PDATA_OFFSET;
						return(-1);
					}
				}
				OutLen = PDATA_OFFSET;
				continue;
			}
			if ( (errno == EINTR) && (errno != EAGAIN) ) {
				errno = 0;
				continue;
			}
			perror("Select() error in Await_NewGame()");
			return(-1);
		}

		/* We are guaranteed that there is data here */
		clen = sizeof(from);
		len = recvfrom(gNetFD, (char *)netbuf, BUFSIZ, 0, 
					(struct sockaddr *)&from, &clen);
		if ( len < 0 ) {
			perror("Network error in Await_NewGame(): recvfrom()");
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
		memcpy(&wave, &netbuf[3], sizeof(wave));
		*Wave = ntohl(wave);
		memcpy(&lives, &netbuf[3+sizeof(wave)], sizeof(lives));
		lives = ntohl(lives);
		if ( lives & 0x8000 )
			gDeathMatch = (lives&(~0x8000));
		else
			*Lives = lives;
		memcpy(&seed, &netbuf[3+sizeof(wave)+sizeof(lives)],
								sizeof(seed));
		seed = ntohl(seed);
		SeedRandom(seed);
//error("Seed is 0x%x\r\n", seed);

		netbuf[1] = gOurPlayer;
		(void) sendto(gNetFD, (char *)netbuf, len, 0,
					(struct sockaddr *)&from,sizeof(from));

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
