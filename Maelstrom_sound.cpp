
/* The sound server for Maelstrom!

   Much of this code has been adapted from 'sfxserver', written by
   Terry Evans, 1994.  Thanks! :)
*/

#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#ifdef _SGI_SOURCE
#include <bstring.h>		/* For bzero() definition */
#endif

#include "soundres.h"
#include "mixer.h"
#include "sound_cmds.h"


extern void select_usleep(unsigned long usec);	// From shared.cc
extern char  *file2libpath(char *filename);	// From shared.cc
extern int Set_AsyncIO(int fd, void (*handler)(int sig), long handler_flags);

/* This is the mixing device */
static Mixer mixer(NULL, 0);
static SoundRes sounds(file2libpath("Maelstrom Sounds"));

static void Handle_IO(int sig);
static int lightning_fd = -1;			/* Asynchronous I/O Channel */
static int galumph_fd = -1;			/* Stop-wait I/O Channel */

void Handle_Signal(int sig)
{
	error("SoundServer: Killed with signal %d\r\n", sig);
	if ( sig == SIGSEGV )	// Dump core
		abort();
	exit(sig);
}

main(int argc, char *argv[])
{
	extern char *optarg;
	fd_set fdset;
	struct timeval tv;
	int c;
	
	/* Exit when parent's pipe is closed */
	signal(SIGPIPE, exit);

	/* Get socket fd argument. :) */
	while ( (c=getopt(argc, argv, "l:g:")) != EOF ) {
		switch (c) {
			case 'l': lightning_fd=atoi(optarg);
				  break;
			case 'g': galumph_fd=atoi(optarg);
				  break;
		}
	}
	if ( (lightning_fd < 0) || (galumph_fd < 0) ) {
		error("Must be called from Maelstrom!\n");
		exit(1);
	}

	/* Set up asynchronous I/O on the socket */
	/* The reason we need asynchronous I/O is that Maelstrom blocks
	   when playing a sound, waiting for the sound server (us) to
	   respond.  When we go a whole sound loop before checking for
	   I/O, the playing becomes jerky.  We need to service sound
	   requests immediately to prevent this.
	   Unfortunately, this causes race conditions on some systems.
	*/
#ifdef ASYNCHRONOUS_IO
#ifdef _SGI_SOURCE		/* The audio library needs restarts */
#define USE_RESTART
#endif /* _SGI_SOURCE */

#ifdef USE_RESTART		/* It's faster without SA_RESTART! ? */
#ifndef SA_RESTART
#define SA_RESTART	0
#endif
	Set_AsyncIO(lightning_fd, Handle_IO, SA_RESTART);
#else
	Set_AsyncIO(lightning_fd, Handle_IO, 0);
#endif
#else
	mixer.Setup_IO(lightning_fd, Handle_IO);
#endif /* ASYNCHRONOUS_SOUND */

	/* Set up signal handler */
	signal(SIGHUP, Handle_Signal);
	signal(SIGINT, Handle_Signal);
	signal(SIGQUIT, Handle_Signal);
	signal(SIGBUS, Handle_Signal);
	signal(SIGSEGV, Handle_Signal);
	signal(SIGTERM, Handle_Signal);

	/* See if there is pending I/O */
	FD_ZERO(&fdset);
	FD_SET(lightning_fd, &fdset);
	memset(&tv, 0, sizeof(tv));
#ifdef _INCLUDE_HPUX_SOURCE
	if ( select(lightning_fd+1,(int *)&fdset,NULL,NULL,&tv) == 1 )
#else
	if ( select(lightning_fd+1, &fdset, NULL, NULL, &tv) == 1 )
#endif
		Handle_IO(SIGIO);

	/* Now loop, waiting for events and playing sounds */
	for ( ; ; ) {
		mixer.Play();

#ifndef ASYNCHRONOUS_IO
		/* See if there is pending I/O */
		FD_ZERO(&fdset);
		FD_SET(lightning_fd, &fdset);
		memset(&tv, 0, sizeof(tv));
#ifdef _INCLUDE_HPUX_SOURCE
		if ( select(lightning_fd+1,(int *)&fdset,NULL,NULL,&tv) == 1 )
#else
		if ( select(lightning_fd+1,&fdset,NULL,NULL,&tv) == 1 )
#endif
			Handle_IO(SIGIO);
#endif /* ASYNCHRONOUS_IO */
	}
}

static inline void ReplyOkay(void)
{
	char replybuf[128];

	sprintf(replybuf, OKAY_FORMAT, OKAY_CMD);
	(void) write(lightning_fd, replybuf, strlen(replybuf)+1);
}

static inline void ReplyNOkay(void)
{
	char replybuf[128];

	sprintf(replybuf, NOKAY_FORMAT, NOKAY_CMD);
	(void) write(lightning_fd, replybuf, strlen(replybuf)+1);
}

static void CallBack(unsigned short channel)
{
	char cmdbuf[128];

#ifdef DEBUG
error("Calling back on channel %hu\n", channel);
#endif
	sprintf(cmdbuf, CALLBACK_FORMAT, CALLBACK_CMD, channel);
	(void) write(galumph_fd, cmdbuf, strlen(cmdbuf)+1);
}

/* This function is run asynchronously, when we are ready to receive commands */
static void Handle_IO(int sig)
{
	static Sample *samples[4];
	char           cmdbuf[128], cmd;
	short          sound_num;
	unsigned short channel;
	unsigned short volume;
	char          *command;
	int            cmdlen, len;
	fd_set fds; struct timeval tv = { 0, 0 };

	/* Make sure there's something to read! */
	FD_ZERO(&fds); FD_SET(lightning_fd, &fds);
#ifdef _INCLUDE_HPUX_SOURCE
	if ( select(lightning_fd+1, (int *)&fds, NULL, NULL, &tv) <= 0 ) {
#else
	if ( select(lightning_fd+1, &fds, NULL, NULL, &tv) <= 0 ) {
#endif
		return;
	}

	/* Read the command */
	if ( (len=read(lightning_fd, cmdbuf, 128)) <= 0 ) {
		/* The socket is probably closed */
		exit(0);
	}

	for ( command=cmdbuf; len>0; ) {
		switch (*command) {
			case LOAD_CMD:	sscanf(cmdbuf, LOAD_FORMAT, &cmd,
								&sound_num);
					if (sounds.LoadSound(sound_num) < 0) {
						error(
				"SoundServer: Invalid sound id: %hd\n",
								sound_num);
						ReplyNOkay();
					} else
						ReplyOkay();
					break;
			case PLAY_CMD:	sscanf(cmdbuf, PLAY_FORMAT, &cmd,
							&sound_num, &channel);
		  			samples[channel] =
						sounds.GetSound(sound_num);
		  			if ( ! samples[channel] ) {
						error(
				"SoundServer: Couldn't find sound id: %hd\n",
								sound_num);
						ReplyNOkay();
						break;
		  			}
					samples[channel]->callback = CallBack;
		  			mixer.Play_Sample(channel, 
							samples[channel]);
					ReplyOkay();
					break;
			case VOL_CMD:	sscanf(cmdbuf, VOL_FORMAT, &cmd,
								&volume);
					if ( mixer.SetVolume(volume) < 0 )
						ReplyNOkay();
					else
						ReplyOkay();
					break;
			case HALT_CMD:	sscanf(cmdbuf, HALT_FORMAT, &cmd,
								&channel);
					mixer.Halt(channel);
					ReplyOkay();
					break;
			case HALTALL_CMD:
					mixer.HaltAll();
					ReplyOkay();
					break;
			case QUERY_CMD:	
					ReplyOkay();
					break;
			case QUIT_CMD:	exit(0);
			default:	error(
			"SoundServer: Warning: undefined command: '%c'\n", 
								*command);
					break;
		}
		cmdlen = (strlen(command)+1); 
		command += cmdlen;
		len -= cmdlen;
	}
}
