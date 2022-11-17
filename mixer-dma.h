
/* Maelstrom Sound Mixer class

	-Sam Lantinga			(5/5/95)
*/

#include "sound_cmds.h"
#include "sample.h"
#include "mydebug.h"


/* Include system specific audio header files */
#ifdef linux
#include <linux/soundcard.h>
#else
#ifdef __FreeBSD__
#include <machine/soundcard.h>
#else
#if defined(i386) && defined(__svr4__)
#include <sys/soundcard.h>
#else
#error Where is soundcard.h for this system?
#endif /* SVR4.2 */
#endif /* FreeBSD */
#endif /* Linux */

#define _PATH_DEV_DSP	"/dev/dsp"

#ifndef ASYNCHRONOUS_IO
#undef USE_POSIX_SIGNALS	/* POSIX signals are for ASYNC I/O */
#endif

#define UNSIGNED_AUDIO_DATA

#ifdef AUDIO_16BIT
#error 16 bit audio is not implemented in this module
#endif

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
#define MAX_AUDIO_VAL	((1<<(AUDIO_BITS-1))-1)
#define MIN_AUDIO_VAL	-(1<<(AUDIO_BITS-1))
#ifdef AUDIO_16BIT
#define AUDIO_DTYPE	signed short
#else
#define AUDIO_DTYPE	signed char
#endif
#endif /* UNSIGNED_AUDIO_DATA */

#ifdef USE_POSIX_SIGNALS
#include <signal.h>
#endif

/* FFFF = Max # fragments */
/* 0009 = 2^9 (512) bytes/fragment (lower this for faster reaction time) */
#define FRAG_SPEC	0xFFFF0009

#define WRITE_TIME	25000		/* Duration of a frag_size write */
#define TRAILER		128		/* Write a partial chunk of overlap */
#define SILENCE		(MAX_AUDIO_VAL/2)+1


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
	int   dsp_fd;			// The sound device file descriptor
	int   speed;			// The recording speed of the samples
	int   frag_size;		// The fragment size
	int   dma_size;			// The length of the DMA memory
	unsigned char *dma;		// The actual DMA memory

	static const int volumeTable[9];// Perceptually linear volume lookup
	float volume;			// Current volume of playing samples

	/* Some channels and the sound data to play */
	struct Channel {
		int   position;		// Offset into current sample
		int   in_use;		// Is this channel in use?
		Sample *sample;		// Current sample on this channel
	} channels[NUM_CHANNELS];

	AUDIO_DTYPE *clipped;		// 16 bit sound data to play

	int unclipped;			// Unclipped raw data 

	/* An informational function */
	int Device_Opened(void) {
		return(dsp_fd >= 0);
	}

#ifdef USE_POSIX_SIGNALS
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
		if ( select(io_fd+1,&fdset,NULL,NULL,&tv) == 1 ) {
			(*io_handler)(0);
			return(1);
		}
		return(0);
	}

};
