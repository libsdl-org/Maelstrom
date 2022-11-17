
#include <sys/types.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "mydebug.h"

#ifdef _SGI_SOURCE
#include <bstring.h>		/* For bzero() declaration */
#endif
#ifdef _AIX
#include <sys/select.h>
#endif

#include "sound.h"

#ifndef SA_NOMASK	/* sparc */
#define SA_NOMASK	SA_NODEFER
#endif

#define SND_SERVER	"Maelstrom_sound"

/* From shared.cc -- for setting up asynchronous I/O on a file descriptor */
extern int Set_AsyncIO(int fd, void (*handler)(int sig), long handler_flags);

static SoundClient *soundclient=NULL;
void IO_Handler(int sig)
{
//error("Sound I/O!\n");
	soundclient->SoundEvent();
}

SoundClient:: SoundClient(void)
{
	int   lightning_fds[2], galumph_fds[2];
	char *argv[4];
	char  fd_buf1[24], fd_buf2[24];
	int   i;

	/* Make sure we only have one soundclient per program */
	if ( soundclient != NULL ) {
		error(
		"Only one asynchronous SoundClient per program, please.\n");
		exit(255);
	}
	soundclient = this;

	if ( socketpair(AF_UNIX, SOCK_STREAM, 0, lightning_fds) < 0 ) {
		perror("SoundClient: Can't create stream sockets");
		exit(255);
	}
	if ( socketpair(AF_UNIX, SOCK_STREAM, 0, galumph_fds) < 0 ) {
		perror("SoundClient: Can't create stream sockets");
		exit(255);
	}
	switch (sndserver_pid=fork()) {
		case -1:	perror("SoundClient: Fork failed");
				exit(255);
		case 0:		goto SndServer;
	}

	/* Initialize the channels */
	for ( i=0; i<NUM_CHANNELS; ++i ) {
		channels[i].in_use = 0;
		channels[i].Callback = NULL;
	}
	Callback_Q.next = NULL;
	Callback_Qtail = &Callback_Q;
	awaiting_reply = 0;

	/* Set ourselves up for asynchronous I/O */
	close(galumph_fds[0]); galumph_fd = galumph_fds[1];
	close(lightning_fds[1]); lightning_fd = lightning_fds[0];

	// Set up the I/O handler
#ifdef SA_RESTART
	Set_AsyncIO(lightning_fd, IO_Handler, SA_RESTART);
#else
	Set_AsyncIO(lightning_fd, IO_Handler, 0);
#endif

	/* Check and make sure the server is alive */
	sleep(1);		/* Wait for Sound Server to initialize */
	if ( QueryServer() < 0 ) {
		error("SoundClient: Couldn't contact Sound Server\n");
		exit(255);
	}
	return;

	/* This section of code starts up the sound server */
SndServer:
	close(galumph_fds[1]); lightning_fd = galumph_fds[0];
	close(lightning_fds[0]); galumph_fd = lightning_fds[1];
	argv[0] = SND_SERVER;
	sprintf(fd_buf1, "-g%d", galumph_fd);
	argv[1] = fd_buf1;
	sprintf(fd_buf2, "-l%d", lightning_fd);
	argv[2] = fd_buf2;
	argv[3] = NULL;
	if ( execvp(argv[0], argv) < 0 ) {
		perror("SoundClient: Can't start sound server");
		exit(255);
	}
}

SoundClient:: ~SoundClient()
{
#define POLITE_CLOSE
#ifdef POLITE_CLOSE
	int  retries;
	char cmdbuf[128];

	sprintf(cmdbuf, QUIT_FORMAT, QUIT_CMD);
	(void) write(galumph_fd, cmdbuf, strlen(cmdbuf)+1);
	close(galumph_fd);
	close(lightning_fd);
	for ( retries=2; retries > 0; --retries ) {
		if ( kill(sndserver_pid, 0) < 0 )
			break;
		sleep(1);
	}
	if ( retries ) {
		/* Kill it now even if it's not done */
		(void) kill(sndserver_pid, SIGTERM);
	}
#else
	close(galumph_fd);
	close(lightning_fd);
#endif
}

void
SoundClient:: SoundEvent(void)
{
	char cmd, *command, cmdbuffer[1024];
	unsigned short channel;
	int len, cmdlen;

/* Check to make sure there is data on the sound connection */
	fd_set fds; struct timeval tv = { 0, 0 };

	FD_ZERO(&fds); FD_SET(lightning_fd, &fds);
#ifdef _INCLUDE_HPUX_SOURCE
	if ( select(lightning_fd+1, (int *)&fds, NULL, NULL, &tv) != 1 )
#else
	if ( select(lightning_fd+1, &fds, NULL, NULL, &tv) != 1 )
#endif
	{ /* This happens when a read for one interrupt reads data 
	     for a second interrupt, which then ends up here.
	   */
		return;
	}

	/* Read the event */
readagain:
	if ( (len=read(lightning_fd, cmdbuffer, 1024)) < 0 ) {
		if ( errno ) {
			/* We got a callback */
			if ( (errno == EINTR) || (errno == EAGAIN) )
				goto readagain;
			/* The child quit */
			if ( errno == EBADF )
				return;

			/* Huh?  An unknown error */
			perror(
			"SoundClient: Read error from SoundServer");
		} // Otherwise, the child process just quit.
		return;
	}

	for ( command=cmdbuffer; len>0; ) {
		switch (*command) {
			case CALLBACK_CMD:	sscanf(command, 
							CALLBACK_FORMAT, 
								&cmd, &channel);
						CallBack(channel);
						break;
			default:		error(
			"SoundClient: Unknown lightning command: '%c'\n",
								*command);
						break;
		}
		cmdlen = (strlen(command)+1); 
		command += cmdlen;
		len -= cmdlen;
	}
	return;
}

int
SoundClient:: SendNGetReply(char *cmdbuf, int timeout)
{
	struct timeval tv, *tp;
	fd_set fdset;
	int    nfds, len;
	char   reply[512];
	int    server_reply=0;

	/* Lock callback interrupts */
	if ( awaiting_reply > 0 ) {
		error("Warning: SendNGetReply() recursion!\n");
	}
	++awaiting_reply;

	if ( timeout ) {
		tv.tv_usec=0;
		tv.tv_sec=timeout;
		tp = &tv;
	} else
		tp = NULL;

	if ( write(galumph_fd, cmdbuf, strlen(cmdbuf)+1) < 0 ) {
		perror("SoundClient: Server write error");
		--awaiting_reply;
		return(-1);
	}
	FD_ZERO(&fdset);
	FD_SET(galumph_fd, &fdset);
#ifdef _INCLUDE_HPUX_SOURCE
	nfds=select(galumph_fd+1, (int *)&fdset, NULL, NULL, tp);
#else
	nfds=select(galumph_fd+1, &fdset, NULL, NULL, tp);
#endif

	if ( FD_ISSET(galumph_fd, &fdset) ) {
readagain:
		/* Don't read more than one reply */
		if ( (len=read(galumph_fd, reply, REPLY_LEN)) < 0 ) {
			if ( errno ) {
				/* We got a callback */
				if ( (errno == EINTR) || (errno == EAGAIN) )
					goto readagain;
				/* The child quit */
				if ( errno == EBADF )
					goto done;

				/* Huh?  An unknown error */
				perror(
				"SoundClient: Read error from SoundServer");
			}
			goto done;
		}
		reply[len]='\0';  /* Can we assume a null terminated reply? */
		if ( strlen(reply) != 1 ) {
			/* We have a protocol error:
				All replies are a single character.
			*/
			error(
			"SoundClient:  Warning: Sound protocol error!\n");
			error("\tbuffer = '%s', len = %d\n",
							reply, len);
		}
		switch (reply[0]) {
			case OKAY_CMD:		server_reply = 1;
						break;
			case NOKAY_CMD:		server_reply = -1;
						break;
			default:		error(
			"SoundClient: Unknown galumph command: '%c'\n",
								reply[0]);
						break;
		}
	} else {
		if ( nfds < 0 )
			perror("SoundClient: select() error");
		else
			error("SoundClient: Sound Server timed out.\n");
	}
done:
	/* Problem:
	   If the callback is calling us, and the sound server finishes
	   playing the sound before we return, we can end up going into
	   an infinite loop.  *sigh*  How do we prevent that?
	 */
	DequeueCallbacks();

	/* Unlock callback interrupts */
	--awaiting_reply;

	/* Note that there is a race condition on the callback queue
	   in the above code.  Since interrupts are not really locked,
	   merely queued, they can occur while the queue is being emptied.
	*/
	return(server_reply == 1 ? 0 : -1);
}

void
SoundClient:: CallBack(unsigned short channel)
{
	channels[channel].in_use = 0;

	if ( awaiting_reply > 0 )
		QueueCallback(channel);
	else {
		if ( channels[channel].Callback )
			channels[channel].Callback(channel);
	}
#ifdef DEBUG
error("Channel %hu finished playing!\n", channel);
#endif
}

int
SoundClient:: QueryServer(void)
{
	char cmdbuf[128];

	sprintf(cmdbuf, QUERY_FORMAT, QUERY_CMD);
	return(SendNGetReply(cmdbuf, 10));
}

int
SoundClient:: SetVolume(unsigned short vol)
{
	char cmdbuf[128];

	sprintf(cmdbuf, VOL_FORMAT, VOL_CMD, vol);
	return(SendNGetReply(cmdbuf, 0));
}

int
SoundClient:: LoadSound(short sndID)
{
	char cmdbuf[128];

	sprintf(cmdbuf, LOAD_FORMAT, LOAD_CMD, sndID);
	return(SendNGetReply(cmdbuf, 0));
}

int
SoundClient:: PlaySound(short sndID, short priority, 
					void (*STCallBack)(unsigned short))
{
	unsigned short channel;

	for ( channel=0; channel<NUM_CHANNELS; ++channel) {
		if ( priority > Status(channel) )
			break;
	}
	if ( channel == NUM_CHANNELS )
		return(-1);
	return(PlayChannel(sndID, priority, channel, STCallBack));
}

int
SoundClient:: PlayChannel(short sndID, short priority, 
		unsigned short theChannel, void (*STCallBack)(unsigned short))
{
	char cmdbuf[128];

	if ( priority <= Status(theChannel) )
		return(-1);

	channels[theChannel].in_use = 1;
	channels[theChannel].ID = sndID;
	channels[theChannel].priority = priority;
	channels[theChannel].Callback = STCallBack;
#ifdef DEBUG
error("Playing sound %hd on channel %hu\n", sndID, theChannel);
#endif
	sprintf(cmdbuf, PLAY_FORMAT, PLAY_CMD, sndID, theChannel);
	return((SendNGetReply(cmdbuf, 2) < 0) ? -1 : theChannel);
}

void
SoundClient:: HaltSounds(void)
{
	unsigned short channel;
	char cmdbuf[128];

	sprintf(cmdbuf, HALTALL_FORMAT, HALTALL_CMD);
	for ( channel=0; channel<NUM_CHANNELS; ++channel )
		channels[channel].in_use = 0;
	SendNGetReply(cmdbuf, 0);
}

void
SoundClient:: HaltChannel(unsigned short channel)
{
	char cmdbuf[128];

	sprintf(cmdbuf, HALT_FORMAT, HALT_CMD, channel);
	channels[channel].in_use = 0;
	SendNGetReply(cmdbuf, 0);
}


int
SoundClient:: SoundID(unsigned short channel)
{
	if ( channels[channel].in_use )
		return(channels[channel].ID);
	return(-1);
}

int
SoundClient:: IsSoundPlaying(short sndID)
{
	unsigned short channel;

	for ( channel=0; channel<NUM_CHANNELS; ++channel) {
		if ( sndID ) {
			if ( sndID == SoundID(channel) ) {
#ifdef DEBUG
error("Sound %d is still playing!\n", SoundID(channel));
#endif
				return(1);
			}
		} else {
			if ( SoundID(channel) != -1 ) {
#ifdef DEBUG
error("Sound %d is still playing!\n", SoundID(channel));
#endif
				return(1);
			}
		}
	}
	return(0);
}


int
SoundClient:: Status(unsigned short channel)
{
	if ( channels[channel].in_use )
		return(channels[channel].priority);
	return(-1);
}

void
SoundClient:: ChannelStatus(unsigned short *chanOnePriority,
			    unsigned short *chanTwoPriority,
			    unsigned short *chanThreePriority,
			    unsigned short *chanFourPriority)
{
	*chanOnePriority = Status(CHANNEL_ONE);
	*chanTwoPriority = Status(CHANNEL_TWO);
	*chanThreePriority = Status(CHANNEL_THREE);
	*chanFourPriority = Status(CHANNEL_FOUR);
}
