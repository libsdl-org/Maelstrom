
/* This module registers a high score with the official Maelstrom
   score server
*/
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>

#ifdef WIN32
extern "C" {
#define Win32_Winsock
#include <windows.h>
};
#else /* UNIX */
#include <unistd.h>
#ifndef __BEOS__
#include <arpa/inet.h>
#endif
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#endif /* WIN32 */

#include "Maelstrom_Globals.h"
#include "netscore.h"
#include "checksum.h"

#define NUM_SCORES	10		// Copied from scores.cc 

#ifdef WIN32
#define write(fd, buf, len)	send(fd, buf, len, 0)
#define read(fd, buf, len)	recv(fd, buf, len, 0)
#define close(fd)		closesocket(fd)

/* We need functions to start and end the networking */
int Win_StartNet(void)
{
	/* Start up the windows networking */
	WORD version_wanted = MAKEWORD(1,1);
	WSADATA wsaData;

	if ( WSAStartup(version_wanted, &wsaData) != 0 ) {
		error("Couldn't initialize Win32 networking!\n");
		return(-1);
	}
	return(0);
}
void Win_HaltNet(void)
{
	/* Clean up windows networking */
	if ( WSACleanup() == SOCKET_ERROR ) {
		if ( WSAGetLastError() == WSAEINPROGRESS ) {
			(void) WSACancelBlockingCall();
			(void) WSACleanup();
		}
	}
}
#endif

static int  Goto_ScoreServer(char *server, int port);
static void Leave_ScoreServer(int sockfd);

/* This function actually registers the high scores */
void RegisterHighScore(Scores high)
{
	int sockfd, i, n;
	unsigned char key[KEY_LEN];
	unsigned int  keynums[KEY_LEN];
	char netbuf[1024], *crc;

	if ( (sockfd=Goto_ScoreServer(SCORE_HOST, SCORE_PORT)) < 0 ) {
		error(
		"Warning: Couldn't connect to Maelstrom Score Server.\r\n");
		error("-- High Score not registered.\r\n");
		return;
	}

	/* Read the welcome banner */
	read(sockfd, netbuf, 1024-1);

	/* Get the key... */
	strcpy(netbuf, "SHOWKEY\n");
	write(sockfd, netbuf, strlen(netbuf));
	if ( read(sockfd, netbuf, 1024-1) <= 0 ) {
		error("Warning: Score Server protocol error.\r\n");
		error("-- High Score not registered.\r\n");
		return;
	}
	for ( i=0, n=0, crc=netbuf; i < KEY_LEN; ++i, ++n ) {
		key[i] = 0xFF;
		if ( ! (crc=strchr(++crc, ':')) ||
				(sscanf(crc, ": 0x%x", &keynums[i]) <= 0) )
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
	sprintf(netbuf, SCOREFMT, crc, high.name, high.score, high.wave);
	write(sockfd, netbuf, strlen(netbuf));
	if ( (n=read(sockfd, netbuf, 1024-1)) > 0 ) {
		netbuf[n] = '\0';
		if ( strncmp(netbuf, "Accepted!", 9) != 0 ) {
			error("New high score was rejected: %s",
								netbuf);
		}
	} else
		perror("Read error on socket");
	Leave_ScoreServer(sockfd);
}

/* This function is just a hack */
int GetLine(int sockfd, char *buffer, int maxlen)
{
	int packed = 0;
	static int lenleft, len;
	static char netbuf[1024], *ptr=NULL;

	if ( buffer == NULL ) {
		lenleft = 0;
		return(0);
	}
	if ( lenleft <= 0 ) {
		if ( (len=read(sockfd, netbuf, 1024)) <= 0 )
			return(-1);
		lenleft = len;
		ptr = netbuf;
	}
	while ( *ptr != '\n' ) {
		if ( lenleft <= 0 ) {
			if ( (len=read(sockfd, netbuf, 1024)) <= 0 ) {
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
	int  sockfd, i;
	char netbuf[1024], *ptr;

	if ( (sockfd=Goto_ScoreServer(SCORE_HOST, SCORE_PORT)) < 0 ) {
		error(
		"Warning: Couldn't connect to Maelstrom Score Server.\r\n");
		return(-1);
	}
	
	/* Read the welcome banner */
	read(sockfd, netbuf, 1024-1);

	/* Send our request */
	strcpy(netbuf, "SHOWSCORES\n");
	write(sockfd, netbuf, strlen(netbuf));

	/* Read the response */
	GetLine(sockfd, NULL, 0);
	GetLine(sockfd, netbuf, 1024-1);
	memset(&hScores, 0, NUM_SCORES*sizeof(Scores));
        for ( i=0; i<NUM_SCORES; ++i ) {
		if ( GetLine(sockfd, netbuf, 1024-1) < 0 ) {
			perror("Read error on socket stream");
			break;
		}
		strcpy(hScores[i].name, "Invalid Name");
		for ( ptr = netbuf; *ptr; ++ptr ) {
			if ( *ptr == '\t' ) {
				/* This is just to remove trailing whitespace
				   and make sure we don't overflow our buffer.
				*/
				char *tail = ptr;
				int   len;

				while ( (tail >= netbuf) && isspace(*tail) )
					*(tail--) = '\0';
				strncpy(hScores[i].name, netbuf,
						sizeof(hScores[i].name)-1);
				if ( (len=strlen(netbuf)) >
					(int)(sizeof(hScores[i].name)-1) )
					len = (sizeof(hScores[i].name)-1);
				hScores[i].name[len] = '\0';
				*ptr = '\t';
				break;
			}
		}
		if ( sscanf(ptr, "%u %u", &hScores[i].score,
						&hScores[i].wave) != 2 ) {
			error(
			"Warning: Couldn't read complete score list!\r\n");
			error("Line was: %s", netbuf);
			break;
		}
        }
	Leave_ScoreServer(sockfd);
	return(0);
}

static int timed_out;
static void timeout(int sig)
{
	timed_out = 1;
}
static int Goto_ScoreServer(char *server, int port)
{
	struct sockaddr_in serv_addr;
	struct hostent *hp;
	int sockfd;

#ifdef WIN32
	if ( Win_StartNet() < 0 )
		return(-1);
#endif
	/*
	 * Fill in the structure "serv_addr" with the address of the
	 * server that we want to connect with.
	 */
	memset(&serv_addr, 0, sizeof(serv_addr));
	if ( (serv_addr.sin_addr.s_addr=inet_addr(server)) == 0xFFFFFFFF ) {
		/* It's not a dotted-decimal address */
		if ( (hp=gethostbyname(server)) == NULL ) {
			/*error("%s: host name error.\n", server);*/
			return(-1);
		}
		else 
			memcpy(&serv_addr.sin_addr, hp->h_addr, hp->h_length);
	}
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_port        = htons(port);

	/*
	 * Open a TCP socket (an Internet stream socket).
	 */
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return(-1);

	/* Set up 30 second timeout just in case */
#ifdef SIGALRM
	signal(SIGALRM, timeout);
	alarm(30);
#endif

	timed_out=0;
	if ( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))
									< 0 ) {
#ifdef EAGAIN
		if ( errno == EINTR )
			errno = EAGAIN;
#endif
		return(-1);
	}
#ifdef SIGALRM
	alarm(0);		/* Reset alarm */
#endif
	if ( timed_out ) {
#ifdef EAGAIN
		errno = EAGAIN;
#endif
		return(-1);
	}
	return(sockfd);
}

static void Leave_ScoreServer(int sockfd)
{
	if ( sockfd >= 0 )
		close(sockfd);
#ifdef WIN32
	Win_HaltNet();
#endif
}
