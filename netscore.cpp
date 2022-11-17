
/* This module registers a high score with the official Maelstrom
   score server
*/
#include <ctype.h>

#include "SDL_net.h"

#include "Maelstrom_Globals.h"
#include "netscore.h"
#include "checksum.h"

#define NUM_SCORES	10		// Copied from scores.cc 

static TCPsocket Goto_ScoreServer(const char *server, int port);
static void Leave_ScoreServer(TCPsocket remote);

/* This function actually registers the high scores */
void RegisterHighScore(Scores high)
{
	TCPsocket remote;
	int i, n;
	unsigned char key[KEY_LEN];
	unsigned int  keynums[KEY_LEN];
	char netbuf[1024], *crc;

	remote = Goto_ScoreServer(SCORE_HOST, SCORE_PORT);
	if ( remote == NULL ) {
		error(
		"Warning: Couldn't connect to Maelstrom Score Server.\r\n");
		error("-- High Score not registered.\r\n");
		return;
	}

	/* Read the welcome banner */
	SDLNet_TCP_Recv(remote, netbuf, 1024);

	/* Get the key... */
	SDL_strlcpy(netbuf, "SHOWKEY\n", sizeof(netbuf));
	SDLNet_TCP_Send(remote, netbuf, strlen(netbuf));
	if ( SDLNet_TCP_Recv(remote, netbuf, 1024) <= 0 ) {
		error("Warning: Score Server protocol error.\r\n");
		error("-- High Score not registered.\r\n");
		return;
	}
	for ( i=0, n=0, crc=netbuf; i < KEY_LEN; ++i, ++n ) {
		key[i] = 0xFF;
		if ( ! (crc=strchr(++crc, ':')) ||
				(SDL_sscanf(crc, ": 0x%x", &keynums[i]) <= 0) )
			break;
	}
/*error("%d items read:\n", n);*/
	if ( n != KEY_LEN )
		error("Warning: short authentication key.\n");
	for ( i=0; i<n; ++i ) {
		key[i] = (keynums[i]&0xFF);
/*error("\t0x%.2x\n", key[i]);*/
	}

	/* Send the scores */
	crc = get_checksum(key, KEY_LEN);
	SDL_snprintf(netbuf, sizeof(netbuf), SCOREFMT, crc, high.name, high.score, high.wave);
	SDLNet_TCP_Send(remote, netbuf, strlen(netbuf));
	n = SDLNet_TCP_Recv(remote, netbuf, 1024);
	if ( n > 0 ) {
		netbuf[n] = '\0';
		if ( strncmp(netbuf, "Accepted!", 9) != 0 ) {
			error("New high score was rejected: %s", netbuf);
		}
	} else
		perror("Read error on socket");
	Leave_ScoreServer(remote);
}

/* This function is just a hack */
int GetLine(TCPsocket remote, char *buffer, int maxlen)
{
	int packed = 0;
	static int lenleft, len;
	static char netbuf[1024], *ptr=NULL;

	if ( buffer == NULL ) {
		lenleft = 0;
		return(0);
	}
	if ( lenleft <= 0 ) {
		len = SDLNet_TCP_Recv(remote, netbuf, 1024);
		if ( len <= 0 )
			return(-1);
		lenleft = len;
		ptr = netbuf;
	}
	while ( (*ptr != '\n') && (*ptr != '\r') ) {
		if ( lenleft <= 0 ) {
			len = SDLNet_TCP_Recv(remote, netbuf, 1024);
			if ( len <= 0 ) {
				*buffer = '\0';
				return(packed);
			}
			lenleft = len;
			ptr = netbuf;
		}
		if ( maxlen == 0 ) {
			*buffer = '\0';
			return(packed);
		}
		*(buffer++) = *(ptr++);
		++packed;
		--maxlen;
		--lenleft;
	}
	++ptr; --lenleft;
	*buffer = '\0';
	return(packed);
}

/* Load the scores from the network score server */
int NetLoadScores(void)
{
	TCPsocket remote;
	int  i;
	char netbuf[1024], *ptr;

	remote = Goto_ScoreServer(SCORE_HOST, SCORE_PORT);
	if ( remote == NULL ) {
		error(
		"Warning: Couldn't connect to Maelstrom Score Server.\r\n");
		return(-1);
	}
	
	/* Read the welcome banner */
	SDLNet_TCP_Recv(remote, netbuf, 1024);

	/* Send our request */
	SDL_strlcpy(netbuf, "SHOWSCORES\n", sizeof(netbuf));
	SDLNet_TCP_Send(remote, netbuf, strlen(netbuf));

	/* Read the response */
	GetLine(remote, NULL, 0);
	GetLine(remote, netbuf, 1024-1);
	memset(&hScores, 0, NUM_SCORES*sizeof(Scores));
        for ( i=0; i<NUM_SCORES; ++i ) {
		if ( GetLine(remote, netbuf, 1024-1) < 0 ) {
			perror("Read error on socket stream");
			break;
		}
		SDL_strlcpy(hScores[i].name, "Invalid Name", sizeof(hScores[i].name));
		for ( ptr = netbuf; *ptr; ++ptr ) {
			if ( *ptr == '\t' ) {
				/* This is just to remove trailing whitespace
				   and make sure we don't overflow our buffer.
				*/
				char *tail = ptr;
				int   len;

				while ( (tail >= netbuf) && isspace(*tail) )
					*(tail--) = '\0';
				SDL_strlcpy(hScores[i].name, netbuf,
						sizeof(hScores[i].name));
				if ( (len=strlen(netbuf)) >
					(int)(sizeof(hScores[i].name)-1) )
					len = (sizeof(hScores[i].name)-1);
				hScores[i].name[len] = '\0';
				*ptr = '\t';
				break;
			}
		}
		if ( SDL_sscanf(ptr, "%u %u", &hScores[i].score,
						&hScores[i].wave) != 2 ) {
			error(
			"Warning: Couldn't read complete score list!\r\n");
			error("Line was: %s", netbuf);
			break;
		}
        }
	Leave_ScoreServer(remote);
	return(0);
}

static TCPsocket Goto_ScoreServer(const char *server, int port)
{
	TCPsocket remote;
	IPaddress remote_address;

	if ( SDLNet_Init() < 0 ) {
		return(NULL);
	}

	/*
	 * Fill in the structure "serv_addr" with the address of the
	 * server that we want to connect with.
	 */
	SDLNet_ResolveHost(&remote_address, server, port);
	if ( remote_address.host == INADDR_NONE ) {
		/*error("%s: host name error.\n", server);*/
		return(NULL);
	}

	/*
	 * Open a TCP socket (an Internet stream socket).
	 */
	remote = SDLNet_TCP_Open(&remote_address);
	return(remote);
}

static void Leave_ScoreServer(TCPsocket remote)
{
	SDLNet_TCP_Close(remote);
	SDLNet_Quit();
}
