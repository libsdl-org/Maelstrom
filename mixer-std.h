
/* Maelstrom Sound Mixer class

	-Sam Lantinga			(5/5/95)
*/

#ifdef _SGI_SOURCE
#include <bstring.h>
#endif
#ifdef _AIX
#include <sys/select.h>
#endif

#include "sound_cmds.h"
#include "sample.h"
#include "mydebug.h"


/* Include system specific audio header files */
extern "C" {
#ifdef linux
#include <linux/soundcard.h>
#undef sparc				/* For sparc Linux */
#else
#ifdef sparc
#ifdef __SVR4   /* Solaris */
#include <sys/audioio.h>
#else           /* SunOS */
#include <sun/audioio.h>
#endif
#else /* not sparc */
#ifdef _INCLUDE_HPUX_SOURCE
#include <sys/audio.h>
#else
#ifdef _SGI_SOURCE
#include <audio.h>
#endif /* SGI */
#endif /* HPUX */
#endif /* sparc */
#endif /* linux */
};

#define _PATH_DEV_AUDIO	"/dev/audio"
#ifdef linux
#define _PATH_DEV_DSP	"/dev/dsp"
#else
#define _PATH_DEV_DSP	_PATH_DEV_AUDIO
#endif /* linux */

#ifndef ASYNCHRONOUS_IO
#undef USE_POSIX_SIGNALS	/* POSIX signals are for ASYNC I/O */
#endif

#ifndef _SGI_SOURCE	/* SGI only uses signed 8-bit audio data */
#define UNSIGNED_AUDIO_DATA
#endif /* _SGI_SOURCE */

/* Handle all the signed/unsigned audio data oddness. :) */
#ifdef AUDIO_16BIT
#define AUDIO_BITS	16
#undef UNSIGNED_AUDIO_DATA		/* 16 bit data is signed :) */
#else
#define AUDIO_BITS	8
#endif /* AUDIO_16BIT */

#ifdef UNSIGNED_AUDIO_DATA
#define MAX_AUDIO_VAL	((1<<AUDIO_BITS)-1)
#define MIN_AUDIO_VAL	0
#ifdef AUDIO_16BIT
#define AUDIO_DTYPE	unsigned short
#else
#define AUDIO_DTYPE	unsigned char
#endif
#else /* Signed Audio Data */
#ifdef PLAY_DEV_AUDIO
#error Cannot use PLAY_DEV_AUDIO with signed audio data
#endif
#define MAX_AUDIO_VAL	((1<<(AUDIO_BITS-1))-1)
#define MIN_AUDIO_VAL	-(1<<(AUDIO_BITS-1))
#ifdef AUDIO_16BIT
#define AUDIO_DTYPE	signed short
#else
#define AUDIO_DTYPE	signed char
#endif
#endif /* UNSIGNED_AUDIO_DATA */

/* Linux version 2.1.0 and earlier has a nasty click on audio underflow */
#if defined(linux) && !defined(AUDIO_16BIT)
#define UNDERFLOW_CLICK
#endif

#ifdef UNDERFLOW_CLICK
/* If we need to write out silence, then blocking on POSIX signals is 
   not something we want to do.
*/
#undef USE_POSIX_SIGNALS
#endif

#ifdef USE_POSIX_SIGNALS
#include <signal.h>
#endif

/* 0002 = 2 fragments */
/* 0009 = 2^9 (512) bytes/fragment (lower this for faster reaction time) */
#define FRAG_SPEC	0x00020009

#define IO_CHECK	128	/* Check every 128 bytes for I/O */
#define WRITE_TIME	25000	/* Duration of a frag_size write */


/* Woah!  The CLASS! :) */

class Mixer {

public:
	Mixer(char *device, unsigned short initvolume);
	~Mixer();

	int  SetVolume(unsigned short Volume);
	int  SoundID(unsigned short channel);
	int  Play_Sample(unsigned short channel, Sample *sample);
	void Play(void);
	void Halt(unsigned short channel);
	void HaltAll(void);
	void Setup_IO(int fd, void (*handler)(int));

private:
	void PlaySleep(void);		// Sleep instead of play sound
	int  DSPopen(int complain);	// Open the sound device
	void DSPclose(void);		// Close the sound device

	char *dsp_device;		// The pathname of the sound device
#ifdef _SGI_SOURCE
	ALport audioport;		// Our interface to SGI audio device
#else
	int   dsp_fd;			// The sound device file descriptor
#endif
	int   speed;			// The recording speed of the samples
	int   frag_size;		// The fragment size

	static const int volumeTable[9];// Perceptually linear volume lookup
	float volume;			// Current volume of playing samples

	/* Some channels and the sound data to play */
	struct Channel {
		int   position;		// Offset into current sample
		int   in_use;		// Is this channel in use?
		Sample *sample;		// Current sample on this channel
	} channels[NUM_CHANNELS];

	AUDIO_DTYPE *clipped;		// 16 bit sound data to play
	AUDIO_DTYPE *silence;		// Just plain silence

	int unclipped;			// Unclipped raw data 
#ifdef PLAY_DEV_AUDIO
	int increment;			// Frequency relation constant
#endif

	/* An informational function */
	int Device_Opened(void) {
#ifdef _SGI_SOURCE
		return(audioport != NULL);
#else
		return(dsp_fd >= 0);
#endif
	}

#if defined(USE_POSIX_SIGNALS)
	sigset_t    blockio;
#endif
	
	/* See if there is pending I/O */
	void (*io_handler)(int);	// I/O handler
	int   io_fd;			// I/O fd
	int Check_IO(void) {
		struct timeval tv;
		fd_set fdset;

		if ( ! io_handler )
			return(0);

		FD_ZERO(&fdset);
		FD_SET(io_fd, &fdset);
		memset(&tv, 0, sizeof(tv));
#ifdef _INCLUDE_HPUX_SOURCE
		if ( select(io_fd+1,(int *)&fdset,NULL,NULL,&tv) == 1 ) {
#else
		if ( select(io_fd+1,&fdset,NULL,NULL,&tv) == 1 ) {
#endif
			(*io_handler)(0);
			return(1);
		}
		return(0);
	}

};
