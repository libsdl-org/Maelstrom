

/* The sound server for Maelstrom!

   Much of this code has been adapted from 'sfxserver', written by
   Terry Evans, 1994.  Thanks! :)
*/

#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include "mixer.h"

/* From shared.cc */
extern void select_usleep(unsigned long usec);

extern "C" {
	extern char *getenv(char *);
};

#ifdef PLAY_DEV_AUDIO
static unsigned char snd2au(int sample);
#endif

/* This table gives a perceptually linear increase (logarithmic) in volume */
const int Mixer::volumeTable[9] = {0, 12, 19, 29, 45, 70, 107, 165, 255};

Mixer:: Mixer(char *device, unsigned short initvolume)
{
	int   i;

#ifdef _SGI_SOURCE
	audioport = NULL;
#else
	/* Set the sound card device */
	/* AUDIODEV is a Solaris-2.4 recommended environment variable */
	if ( (dsp_device=getenv("AUDIODEV")) == NULL ) {
#ifdef PLAY_DEV_AUDIO
		dsp_device = _PATH_DEV_AUDIO;
#else
		if ( device )
			dsp_device = device;
		else
			dsp_device = _PATH_DEV_DSP;
#endif /* PLAY_DEV_AUDIO */
	}
	dsp_fd = -1;
#endif /* Not SGI Source */

	/* Start up with initial volume */
	DSPopen(0);	/* Init device, but don't complain */
	(void) SetVolume(initvolume);

	/* Now set up the channels, and load some sounds */
	for ( i=0; i<NUM_CHANNELS; ++i ) {
		channels[i].position = 0;
		channels[i].in_use = 0;
		channels[i].sample = NULL;
	}
	io_handler = NULL;

	/* Get some sweet blessed silence */
	clipped = new AUDIO_DTYPE[frag_size];
	silence = new AUDIO_DTYPE[frag_size];

	/* Tell the silence to be quiet :) */
#ifdef UNSIGNED_AUDIO_DATA
	memset(silence, (MAX_AUDIO_VAL/2)+1, frag_size*(AUDIO_BITS/8));
#else
	memset(silence, 0, frag_size*(AUDIO_BITS/8));
#endif

#if defined(USE_POSIX_SIGNALS)
	sigemptyset(&blockio);
	sigaddset(&blockio, SIGIO);
#endif
}

Mixer:: ~Mixer()
{
	delete[] clipped;
	delete[] silence;
	DSPclose();
}

#ifdef _SGI_SOURCE
int
Mixer:: DSPopen(int complain)
{
	long pvbuf[2];
	ALconfig audioconfig;

	if ( audioport != NULL )	/* The device is already open. */
		return(0);

	speed = 11025;			/* Sampling speed of Maelstrom sound */
	frag_size = 512;


	pvbuf[0] = AL_OUTPUT_RATE;
	pvbuf[1] = speed;

	if( ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 2) < 0 ) {
		if ( complain ) {
			error("Mixer::DSPopen: ALsetparams failed!\n");
		}
		return(-1);
	}

	pvbuf[0] = AL_INPUT_RATE;
	pvbuf[1] = speed;

	if( ALsetparams(AL_DEFAULT_DEVICE, pvbuf, 2) < 0 ) {
		if ( complain ) {
			error("Mixer::DSPopen: ALsetparams failed!\n");
		}
		return(-1);
	}

	audioconfig = ALnewconfig();

#ifdef AUDIO_16BIT
	if( ALsetwidth(audioconfig, AL_SAMPLE_16) < 0 ) {
#else
	if( ALsetwidth(audioconfig, AL_SAMPLE_8) < 0 ) {
#endif
		if ( complain ) {
			error("Mixer::DSPopen: ALsetwidth failed!\n");
		}
		return(-1);
	}

	if( ALsetqueuesize(audioconfig, frag_size*(AUDIO_BITS/8)) < 0 ) {
		if ( complain ) {
			error("Mixer::DSPopen: ALsetqueuesize failed!\n");
		}
		return(-1);
	}

	if( ALsetchannels(audioconfig, AL_MONO) < 0 ) {
		if ( complain ) {
			error("Mixer::DSPopen: ALsetchannels failed!\n");
		}
		return(-1);
	}

	if( (audioport = ALopenport("name", "w", audioconfig) ) == NULL ) {
		if ( complain ) {
			error("Mixer::DSPopen: ALopenport failed!\n");
		}
		return(-1);
	}

	return(0);
}
#else /* Not SGI code */
int
Mixer:: DSPopen(int complain)
{
	int  frag_spec = FRAG_SPEC;

	if ( dsp_fd >= 0 ) {	// The device is already open.
		return(0);
	}
	speed = 11025;			/* Sampling speed of Maelstrom sound */

#ifdef PLAY_DEV_AUDIO
	increment = speed / 8;
#endif /* PLAY_DEV_AUDIO */
	frag_size = (0x01<<(FRAG_SPEC&0x0F));

	/* Open the sound device (don't hang) */
	if ( (dsp_fd=open(dsp_device, (O_WRONLY|O_NONBLOCK), 0)) < 0 ) {
		if ( complain )
			perror("Mixer: Can't open sound card");
		return(-1);
	}

/* Do some system specific initialization */
#ifdef linux
#ifndef PLAY_DEV_AUDIO		/* VoxWare */
	if ( ioctl(dsp_fd, SNDCTL_DSP_SETFRAGMENT, &frag_spec) < 0 ) {
		if ( complain )
			perror("Mixer: Can't set frag spec");
		DSPclose();
		return(-1);
	}
	if ( ioctl(dsp_fd, SOUND_PCM_WRITE_RATE, &speed) < 0 ) {
		if ( complain )
			perror("Mixer: Can't set sampling rate");
		DSPclose();
		return(-1);
	}
	if ( ioctl(dsp_fd, SNDCTL_DSP_GETBLKSIZE, &frag_size) < 0 ) {
		if ( complain )
			perror("Mixer: Can't get fragment size");
		DSPclose();
		return(-1);
	}
#ifdef AUDIO_16BIT
#ifdef UNSIGNED_AUDIO_DATA
#error 16 bit audio data is signed!
#endif
	int audio_format;
	audio_format = AFMT_S16_LE;
	if ( ioctl(dsp_fd, SNDCTL_DSP_SETFMT, &audio_format) < 0 ) {
		if ( complain )
			perror("Mixer: Can't set audio format");
		DSPclose();
		return(-1);
	}
#endif /* AUDIO_16BIT */
#endif
#else /* Not Linux */
#ifdef _INCLUDE_HPUX_SOURCE
	struct audio_describe ainfo;
	int audio_ctl;
	int audio_format;
	struct audio_select_thresholds threshold;

	if ( (audio_ctl=open("/dev/audioCtl", (O_WRONLY|O_NDELAY), 0)) < 0 ) {
		if ( complain )
    			perror("Mixer: Can't open /dev/audioCtl");
		DSPclose();
		return(-1);
	}
	if ( ioctl(audio_ctl, AUDIO_DESCRIBE, &ainfo) < 0 ) {
		if ( complain )
			perror("Mixer: Can't get audio info");
		DSPclose();
		return(-1);
	}
#ifdef PLAY_DEV_AUDIO
	audio_format = AUDIO_FORMAT_ULAW;
	speed = 8000;
#else
#ifdef AUDIO_16BIT
	audio_format = AUDIO_FORMAT_LINEAR16BIT;
#else
	audio_format = AUDIO_FORMAT_LINEAR8BIT;
#endif
#endif
	if ( ioctl(audio_ctl, AUDIO_SET_DATA_FORMAT, audio_format) < 0 ) {
		if ( complain )
			perror("Mixer: Can't set audio format");
		DSPclose();
		return(-1);
	}
  	if ( ioctl(audio_ctl, AUDIO_SET_CHANNELS, 1) < 0 ) {
		if ( complain )
			perror("Mixer: Can't set audio to one channel");
		DSPclose();
		return(-1);
	}
	if ( ioctl(audio_ctl, AUDIO_SET_SAMPLE_RATE, speed) < 0 ) {
		if ( complain )
			perror("Mixer: Can't set sample rate");
		DSPclose();
		return(-1);
	}
	if ( ioctl(audio_ctl, AUDIO_GET_SEL_THRESHOLD, &threshold) < 0 ) {
		if ( complain )
			perror("Mixer: Couldn't get audio output threshold");
		DSPclose();
		return(-1);
	}
	threshold.write_threshold = frag_size;
	if ( ioctl(audio_ctl, AUDIO_SET_SEL_THRESHOLD, &threshold) < 0 ) {
#ifdef FATAL_NO_SET_FRAGSIZE
		if ( complain )
			perror("Mixer: Couldn't set audio output threshold");
		DSPclose();
		return(-1);
#endif
	}
	close(audio_ctl);
#endif /* HPUX */
#endif /* linux */

	/* This is necessary so that the sound server stays in sync */
	long flags;
	flags = fcntl(dsp_fd, F_GETFL, 0);
	flags |= O_SYNC;
	(void) fcntl(dsp_fd, F_SETFL, flags);

	return(0);
}
#endif /* Not SGI code */

void
Mixer:: DSPclose(void)
{
#ifdef _SGI_SOURCE
	if ( audioport != NULL ) {
		(void) ALcloseport(audioport);
		audioport = NULL;
	}
#else
	if ( dsp_fd >= 0 ) {
		(void) close(dsp_fd);
		dsp_fd = -1;
	}
#endif
}

int
Mixer:: SetVolume(unsigned short Volume)
{
	if ( Volume > 0x08 ) {
		error("Mixer: Warning: Volume is a range 0-8\n");
		return(-1);
	}
	if ( Volume ) {	// Don't set the volume if we can't open the device.
		if ( DSPopen(1) < 0 )
			return(-1);

#define SCALE_FACTOR	0.5
/*
	Note the scale factor;  it is an experimentally derived amount
	which prevents clipping to a large extent when multiple sounds
	are played at once, without sacrificing too much resolution.
*/
#ifdef AUDIO_16BIT
		/* 16 bits are enough to do sophisticated volume handling */
		volume = (float)(volumeTable[Volume]);
#else
		volume = (float)((Volume*32)-1)/255.0;
#endif
		volume *= SCALE_FACTOR;
	} else {
		volume = 0.0;
		DSPclose();
	}
	return(0);
}

int
Mixer:: SoundID(unsigned short channel)
{
	if ( channel > NUM_CHANNELS-1 )
		return(-1);
	if ( channels[channel].in_use )
		return(channels[channel].sample->ID);
	return(-1);
}

int
Mixer:: Play_Sample(unsigned short channel, Sample *sample)
{
	if ( channel > NUM_CHANNELS-1 )
		return(-1);

	channels[channel].position = 0;
	channels[channel].in_use = 1;
	channels[channel].sample = sample;
	return(0);
}

void
Mixer:: PlaySleep(void)
{
#ifdef UNDERFLOW_CLICK
#ifndef linux
#error Please modify this code for your architecture, if needed.
#endif
	/* We must write out silence to prevent the horrible clicking. :-} */
	int frag_offset;

	for ( frag_offset=0; frag_offset<frag_size; frag_offset += IO_CHECK ) {
		if ( Check_IO() )
			break;

		if ( Device_Opened() ) {
			/* Play the silence, however it is done */
			(void) write(dsp_fd, silence, IO_CHECK*(AUDIO_BITS/8));
//printf("."); fflush(stdout);
		} else {
			select_usleep((WRITE_TIME/frag_size)*IO_CHECK);
		}
	}
#else
#ifdef ASYNCHRONOUS_IO
	select_usleep(WRITE_TIME);
#else
	int frag_offset;

	for ( frag_offset=0; frag_offset<frag_size; frag_offset += IO_CHECK ) {
		if ( Check_IO() )
			break;
		select_usleep((WRITE_TIME/frag_size)*IO_CHECK);
	}
#endif /* ASYNCHRONOUS_IO */
#endif /* UNDERFLOW_CLICK */
}

/* Set up the I/O handler */
void
Mixer:: Setup_IO(int fd, void (*handler)(int))
{
	io_fd = fd;
	io_handler = handler;
}

void
Mixer:: Play(void)
{
	int frag_offset;
	int num_playing;
	unsigned short i;
	signed char data;

	/* The idea of blocking if there's nothing to play is a good one
	   on machines that handle underflow as silence.
	   It releases the CPU, and results in smoother game play.
	   On systems that don't support underflow silence very well,
	   we need to explicitly write out silence.

	   If we wait for I/O, we should make the check, to see if there
	   are any sounds playing, an atomic one.  USE_POSIX_SIGNALS does
	   the right thing.
	*/
#if defined(USE_POSIX_SIGNALS)
	sigset_t omask;
     	if ( sigprocmask(SIG_BLOCK, &blockio, &omask) < 0 )
		perror("Warning: Can't block I/O signal");
#endif

	/* Check to see if there are sounds playing */
	for ( num_playing=0, i=0; i<NUM_CHANNELS; ++i ) {
		if ( channels[i].in_use )
			++num_playing;
	}
#if !defined(USE_POSIX_SIGNALS)
	/* If there is nothing to play, just write out silence */
	if ( num_playing == 0 ) {
		PlaySleep();
		return;
	}
#else
	/* We wait for an I/O signal here */
	if ( num_playing == 0 ) {
		sigsuspend(&omask);

		/* Unblock the I/O signals and return */
     		if ( sigprocmask(SIG_SETMASK, &omask, NULL) < 0 )
			perror("Warning: Can't unblock I/O signal");
		return;
	}
     	if ( sigprocmask(SIG_SETMASK, &omask, NULL) < 0 )
		perror("Warning: Can't unblock I/O signal");
#endif


#ifdef PLAY_DEV_AUDIO
	int sum=0;
#endif
	/* This is for mono output */
	for( frag_offset=0; frag_offset<frag_size; ++frag_offset ) {
		unclipped = 0;
		num_playing = 0;

#ifndef ASYNCHRONOUS_IO
		if ( (frag_offset%IO_CHECK) == 0 ) 
			Check_IO();
#endif /* ASYNCHRONOUS_IO */

		for( i=0; i<NUM_CHANNELS; ++i ) {
			/* See if the channel is in use */
			if ( channels[i].in_use ) {
				/* Normalize the data */
				data = 
		(*(channels[i].sample->data + channels[i].position)^0x80);
				unclipped += data;

				/* See if this sample is done being played */
				if( ++channels[i].position 
						>= channels[i].sample->len ) {
					channels[i].in_use = 0;
					if ( channels[i].sample->callback )
						(*channels[i].sample->callback)(i);
				}
			}
		}
		/* Apply volume */
		unclipped = (int)((float)unclipped * volume);

#ifdef UNSIGNED_AUDIO_DATA
		/* Re-normalize the values */
		unclipped += (MAX_AUDIO_VAL/2)+1;

#ifdef PLAY_DEV_AUDIO
		sum += increment;
		while ( sum > 0 ) {
			sum -= 1000;
			for( i=0; i<NUM_CHANNELS; ++i ) {
				/* See if the channel is in use */
				if ( channels[i].in_use ) {
					++channels[i].position;
				}
			}
		}
		for( i=0; i<NUM_CHANNELS; ++i ) {
			/* See if the channel is in use */
			if ( channels[i].in_use ) {
				--channels[i].position;
			}
		}
		unclipped = snd2au((0x80-unclipped)*16);
#endif /* PLAY_DEV_AUDIO */
#endif /* UNSIGNED_AUDIO_DATA */

		if ( unclipped < MIN_AUDIO_VAL )
			clipped[frag_offset] = MIN_AUDIO_VAL;
		else if ( unclipped > MAX_AUDIO_VAL )
			clipped[frag_offset] = MAX_AUDIO_VAL;
		else
			clipped[frag_offset] = unclipped;
	}
#ifdef DEBUG
for( i=0; i<NUM_CHANNELS; ++i ) {
	if ( channels[i].in_use )
		error("Channel %hu: position = %d, len = %d\n", i, channels[i].position, channels[i].sample->len);
}
#endif

	/* Write out the data */
	if ( Device_Opened() ) {
#ifdef _SGI_SOURCE
		while ( ALgetfillable(audioport) < frag_size ) {
			/* Wait til there's enough room to write whole sample */
			select_usleep(1);
		}
		if ( ALwritesamps(audioport, clipped, frag_size) < 0 ) {
			error("Mixer::Play: ALwritesamps (Play) failed!\n");
		}
#else  /* Normal device write */
#ifdef sparc
	drain_it:
		if ( ioctl(dsp_fd, AUDIO_DRAIN, 0) < 0 ) {
			if ( errno == EINTR )
				goto drain_it;
		}
#endif
	write_frag:
		if ( write(dsp_fd, clipped, frag_size*(AUDIO_BITS/8))
						!= frag_size*(AUDIO_BITS/8) ) {
			if ( errno == EINTR )  // Interrupted system call...
				// This should happen (SA_RESTART)
				goto write_frag;
			else {
				perror("Mixer: Can't write to audio device");
				return;
			}
		}
#endif /* Not SGI Source */
	} else
		PlaySleep();
}

void
Mixer:: Halt(unsigned short channel)
{
	channels[channel].in_use = 0;
}

void
Mixer:: HaltAll(void)
{
	for ( unsigned short i=0; i<NUM_CHANNELS; ++i )
		Halt(i);
}

#ifdef PLAY_DEV_AUDIO
/* This function (snd2au()) copyrighted: */
/************************************************************************/
/*      Copyright 1989 by Rich Gopstein and Harris Corporation          */
/*                                                                      */
/*      Permission to use, copy, modify, and distribute this software   */
/*      and its documentation for any purpose and without fee is        */
/*      hereby granted, provided that the above copyright notice        */
/*      appears in all copies and that both that copyright notice and   */
/*      this permission notice appear in supporting documentation, and  */
/*      that the name of Rich Gopstein and Harris Corporation not be    */
/*      used in advertising or publicity pertaining to distribution     */
/*      of the software without specific, written prior permission.     */
/*      Rich Gopstein and Harris Corporation make no representations    */
/*      about the suitability of this software for any purpose.  It     */
/*      provided "as is" without express or implied warranty.           */
/************************************************************************/

static unsigned char snd2au(int sample)
{

	int mask;

	if (sample < 0) {
		sample = -sample;
		mask = 0x7f;
	} else {
		mask = 0xff;
	}

	if (sample < 32) {
		sample = 0xF0 | 15 - (sample / 2);
	} else if (sample < 96) {
		sample = 0xE0 | 15 - (sample - 32) / 4;
	} else if (sample < 224) {
		sample = 0xD0 | 15 - (sample - 96) / 8;
	} else if (sample < 480) {
		sample = 0xC0 | 15 - (sample - 224) / 16;
	} else if (sample < 992) {
		sample = 0xB0 | 15 - (sample - 480) / 32;
	} else if (sample < 2016) {
		sample = 0xA0 | 15 - (sample - 992) / 64;
	} else if (sample < 4064) {
		sample = 0x90 | 15 - (sample - 2016) / 128;
	} else if (sample < 8160) {
		sample = 0x80 | 15 - (sample - 4064) /  256;
	} else {
		sample = 0x80;
	}
	return (mask & sample);
}
#endif /* PLAY_DEV_AUDIO */
