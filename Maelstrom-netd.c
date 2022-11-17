
/* Here we go... */

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

/* We wait in a loop for players to connect and tell us how many people
   are playing.  Then, once all players have connected, then we broadcast
   the addresses of all players, and then start again.

   Note: We rely on our MAX_PLAYERS and the client's MAX_PLAYERS being
         the same.  Otherwise we crash.
*/

#include "protocol.h"

#define MAX_CONNECTIONS		64
#define UNCONNECTED		0
#define CONNECTED		1
#define ACTIVE			2
#define TIMEOUT			300		/* timeout in seconds */

typedef struct {
	int state;
	int sockfd;
	time_t timestamp;
	int player;
	int numplayers;
	unsigned char  *packet;
	int packetlen;
	struct sockaddr_in raddr;
	} connection;

connection players[MAX_CONNECTIONS];

/* This function disconnects a player */
void DisconnectPlayer(int which)
{
	if ( players[which].state == ACTIVE )
		(void) free(players[which].packet);
	close(players[which].sockfd);
	players[which].state = UNCONNECTED;
printf("Player on slot %d has been disconnected.\n", which);
}

void SendError(int which, char *message)
{
	unsigned char mesgbuf[BUFSIZ];
	int mesglen;

	mesglen = (2+strlen(message)+1);
	if ( mesglen > BUFSIZ ) {
		fprintf(stderr, "Fatal error: error message too long!\n");
		exit(3);
	}
	mesgbuf[0] = mesglen;
	mesgbuf[1] = NET_ABORT;
	strcpy((char *)&mesgbuf[2], message);
printf("Sending error '%s' to player in slot %d\n", message, which);
	(void) write(players[which].sockfd, mesgbuf, mesglen);

	DisconnectPlayer(which);
}

/* Uh oh, a fatal error.  Tell all currently connected players, and exit. */
void Fatal(char *message)
{
	int i;

	for ( i=0; i<MAX_CONNECTIONS; ++i ) {
		if ( players[i].state != UNCONNECTED )
			SendError(i, message);
	}
	exit(3);
}

/* This function checks for player timeouts */
void CheckPlayers(void)
{
	int i;
	time_t now;

	now = time(NULL);
	for ( i=0; i<MAX_CONNECTIONS; ++i ) {
		if ( players[i].state != CONNECTED )
			continue;
		if ( (now-players[i].timestamp) > TIMEOUT ) {
			SendError(i, "Server timed out waiting for packet");
		}
	}
}

/* This is the function that makes the server go 'round. :) */
/* We check to see if all advertised players have connected, and
   if so we blast everyone's address to each player.
*/
void CheckNewGame(void)
{
	unsigned char buffer[BUFSIZ];
	int first, i;
	int numplayers, players_on;
	int positions[MAX_PLAYERS];

	/* Find the first player */
	for ( i=0, first=(-1); i<MAX_CONNECTIONS; ++i ) {
		if ( (players[i].state == ACTIVE) && (players[i].player == 0) )
			break;
	}
	if ( i == MAX_CONNECTIONS ) /* No first player... */
		return;

	/* Now make sure everyone else agrees with the first player */
	first = i;
	numplayers = players[first].numplayers;
	for ( i=0; i<MAX_CONNECTIONS; ++i ) {
		if ( players[i].state != ACTIVE )
			continue;
		if ( players[i].numplayers != numplayers ) {
			sprintf(buffer,
				"There are %d, not %d players in this game",
					numplayers, players[i].numplayers);
			SendError(i, (char *)buffer);
		}
	}

	/* Now see if there are enough players! */
	for ( i=0, players_on=0; i<MAX_CONNECTIONS; ++i ) {
		if ( players[i].state == ACTIVE ) {
			positions[players[i].player] = i;
			++players_on;
		}
	}
	/* We've rejected all duplicate players, and extra players,
	   so players_on shouldn't be greater than numplayers.
	 */
	if ( players_on == numplayers ) {	/* Let's party!! */
		char *ptr;
		int   len;

printf("Let's party!!\n");
		len = (2+players[first].packetlen);
		buffer[1] = NEW_GAME;
		memcpy(&buffer[2], players[first].packet,
						players[first].packetlen);
		ptr = (char *)&buffer[len];
		for ( i=0; i<numplayers; ++i ) {
			connection *player = &players[positions[i]];

			strcpy(ptr, inet_ntoa(player->raddr.sin_addr));
printf("Setting up player %d at host %s and port ", i+1, ptr);
			len += strlen(ptr)+1;
			ptr += strlen(ptr)+1;
			sprintf(ptr, "%d", ntohs(player->raddr.sin_port));
printf("%s\n", ptr);
			len += strlen(ptr)+1;
			ptr += strlen(ptr)+1;
		}
		buffer[0] = len;

		for ( i=0; i<numplayers; ++i ) {
			(void) write(players[positions[i]].sockfd, buffer, len);
			DisconnectPlayer(positions[i]);
		}
	}
}

void I_Crashed(int sig)
{
	if ( sig == SIGSEGV )
		Fatal("Server crashed!");
	else
		Fatal("Server shut down!");
	exit(sig);
}

main(int argc, char *argv[])
{
	int netfd, i, slot;
	struct sockaddr_in serv_addr;

	/******************************************************
	 *
	 * Phase 1: Initialization
	 *
	 ******************************************************/

	/* Create a socket */
	if ( (netfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		perror("Can't open stream socket");
		exit(3);
	}
	/* Bind it to our address */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port        = htons(NETPLAY_PORT-1);
	if (bind(netfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("Can't bind local address");
		exit(3);
	}
	if ( listen(netfd, MAX_PLAYERS) < 0 ) {
		perror("listen() failed");
		exit(3);
	}

	/* Initialize player structures */
	for ( i=0; i<MAX_CONNECTIONS; ++i )
		players[i].state = UNCONNECTED;

	signal(SIGINT, I_Crashed);
	signal(SIGSEGV, I_Crashed);

	
	/******************************************************
	 *
	 * Phase 2: Wait for all players
	 *
	 ******************************************************/
printf("Waiting for players...\n");
	for ( ; ; ) {
		fd_set fdset;
		struct timeval tv;
		int maxfd = 0;

		FD_ZERO(&fdset);
		for ( i=0; i<MAX_CONNECTIONS; ++i ) {
			if ( players[i].state != UNCONNECTED ) {
				if ( maxfd < players[i].sockfd )
					maxfd = players[i].sockfd;
				FD_SET(players[i].sockfd, &fdset);
			}
		}
		if ( maxfd < netfd )
			maxfd = netfd;
		FD_SET(netfd, &fdset);

		tv.tv_sec = 60;
		tv.tv_usec = 0;
		if ( select(maxfd+1, &fdset, NULL, NULL, &tv) <= 0 ) {
			if ( errno != 0 ) {
				perror("select() error");
				exit(3);
			}
			CheckPlayers();
			continue;
		}

		/* Check for new players first */
		if ( FD_ISSET(netfd, &fdset) ) {
			int sockfd, clilen;

			for ( i=0; i<MAX_CONNECTIONS; ++i ) {
				if ( players[i].state == UNCONNECTED )
					break;
			}
			if ( i == MAX_CONNECTIONS ) {
				fprintf(stderr, "Out of connections!!\n");
				exit(3);
			}
			slot = i;

			clilen = sizeof(players[slot].raddr);
			if ( (sockfd=accept(netfd, (struct sockaddr *)
					&players[slot].raddr, &clilen)) < 0 ) {
				perror("accept() error");
				exit(3);
			}
			players[slot].timestamp = time(NULL);
			players[slot].sockfd = sockfd;
			players[slot].state = CONNECTED;
printf("Connection received on port %d\n", i);

			/* We continue so that we can get all players quickly */
			continue;
		}

		for ( i=0; i<MAX_CONNECTIONS; ++i ) {
			unsigned char data[BUFSIZ];
			unsigned long cliport;
			int len, player;
			int numplayers;

			if ( (players[i].state == UNCONNECTED) ||
					! FD_ISSET(players[i].sockfd, &fdset) )
				continue;

			/* A player with active data! */
			slot = i;
			if ( (len=read(players[slot].sockfd, data, BUFSIZ))
								<= 0 ) {
				/* Wierd.  Close connection */
				DisconnectPlayer(slot);
				continue;
			}

			/* Are they cancelling a connection? */
			if ( data[0] == NET_ABORT ) {
				DisconnectPlayer(slot);
				continue;
			}

			if ( data[0] != NEW_GAME ) {
				fprintf(stderr,
					"Unknown client packet: 0x%.2x\n", 
								data[0]);
				DisconnectPlayer(slot);
				continue;
			}

			if ( len != NEW_PACKETLEN+4+1 ) {
				fprintf(stderr,
			"Short client packet! (len was %d, expected %d)\n", 
						len, NEW_PACKETLEN+4+1);
				SendError(slot, "Server received short packet");
				continue;
			}
				

			/* Yay!  Active connection! */
			player = data[1];
			/* Be wary, client sizeof(unsigned long) and our
			   sizeof(unsigned long) may differ.  We assume
			   sizeof(unsigned long) is 4.
			*/
			memcpy(&cliport, &data[NEW_PACKETLEN], sizeof(cliport));
			numplayers = data[NEW_PACKETLEN+4];
			for ( i=0; i<MAX_CONNECTIONS; ++i ) {
				if ( players[i].state == ACTIVE ) {
					if ( player == players[i].player )
						break;
				}
			}
			/* Is there already player N? */
			if ( i != MAX_CONNECTIONS ) {
				char message[BUFSIZ];

				sprintf(message, "Player %d is already on!",
								player+1);
				SendError(slot, message);
				continue;
			}

			/* Set the player up */
			players[slot].state = ACTIVE;
			players[slot].player = player;
			players[slot].numplayers = numplayers;
			players[slot].packetlen = len-(2+4+1);
			if ( (players[slot].packet = (unsigned char *)
				malloc(players[slot].packetlen)) == NULL) {
				perror("Out of memory");
				Fatal("Server ran out of memory");
			}
			memcpy(players[slot].packet, &data[2],
						players[slot].packetlen);
			/* This is important! */
			players[slot].raddr.sin_port = 
						htons((short)ntohl(cliport));
printf("Player %d arrived on port %d\n", player+1, slot);
printf("  the remote address is %s:%lu\n",
	inet_ntoa(players[slot].raddr.sin_addr),
	ntohs(players[slot].raddr.sin_port));
		}
		CheckNewGame();
		CheckPlayers();
	}
	/* Never reached */
}

