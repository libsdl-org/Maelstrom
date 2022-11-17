

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
#include <sys/mman.h>
#include "mixer.h"

/* From shared.cc */
extern void select_usleep(unsigned long usec);

extern "C" {
	extern char *getenv(char *);
};

/* This table gives a perceptually linear increase (logarithmic) in volume */
const int Mixer::volumeTable[9] = {0, 12, 19, 29, 45, 70, 107, 165, 255};

Mixer:: Mixer(char *device, unsigned short initvolume)
{
	int   i;

	/* Set the sound card device */
	/* AUDIODEV is a Solaris-2.4 recommended environment variable */
	if ( (dsp_device=getenv("AUDIODEV")) == NULL ) {
		if ( device )
			dsp_device = device;
		else
			dsp_device = _PATH_DEV_DSP;
	}
	dsp_fd = -1;

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

	/* Get our mixing buffer */
	clipped = new AUDIO_DTYPE[frag_size+TRAILER];
}

Mixer:: ~Mixer()
{
	delete[] clipped;
	DSPclose();
}

int
Mixer:: DSPopen(int complain)
{
	struct audio_buf_info ainfo;
	int  capabilities;
	int  trigger;
	int  frag_spec = FRAG_SPEC;

	if ( dsp_fd >= 0 ) {	// The device is already open.
		return(0);
	}
	speed = 11025;			/* Sampling speed of Maelstrom sound */

	/* Open the sound device (don't hang) */
	if ( (dsp_fd=open(dsp_device, (O_WRONLY|O_NONBLOCK), 0)) < 0 ) {
		if ( complain )
			perror("Mixer: Can't open sound card");
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
	if ( ioctl(dsp_fd, SNDCTL_DSP_SPEED, &speed) < 0 ) {
		if ( complain )
			perror("Mixer: Can't set sampling rate");
		DSPclose();
		return(-1);
	}
#endif /* AUDIO_16BIT */

	/* Make sure the driver can do DMA mapping */
	dma = NULL;
	if ( ioctl(dsp_fd, SNDCTL_DSP_GETCAPS, &capabilities) < 0 ) {
		if ( complain )
			perror("Mixer: Can't get audio capabilities");
		DSPclose();
		return(-1);
	}
	if ( !(capabilities & DSP_CAP_TRIGGER) ||
	     !(capabilities & DSP_CAP_MMAP) ) {
		if ( complain )
			error(
			"Mixer: Your audio card can't be memory mapped.\n");
		DSPclose();
		return(-1);
	}

	if ( ioctl(dsp_fd, SNDCTL_DSP_SETFRAGMENT, &frag_spec) < 0 ) {
		if ( complain )
			perror("Mixer: Can't set frag spec");
		DSPclose();
		return(-1);
	}
	if ( ioctl(dsp_fd, SNDCTL_DSP_GETOSPACE, &ainfo) < 0 ) {
		if ( complain )
			perror("Mixer: Can't get audio output params");
		DSPclose();
		return(-1);
	}
//printf("%d frags, each %d bytes\n", ainfo.fragstotal, ainfo.fragsize);
	dma_size = ainfo.fragstotal * ainfo.fragsize;
	frag_size = ainfo.fragsize;

//printf("Trying to map %d bytes of audio memory\n", dma_size);
	if ( (caddr_t)(dma=(unsigned char *)mmap(NULL, dma_size, PROT_WRITE,
			(MAP_FILE|MAP_SHARED), dsp_fd, 0)) == (caddr_t)-1) {
//printf("Pointer is 0x%x (failed)\n", dma);
		if ( complain )
			perror("Mixer: Can't map audio device to memory");
		DSPclose();
		return(-1);
	}
//printf("Pointer is 0x%x\n", dma);

	/* Toggle the device's enable bits */
	trigger = 0;
	(void) ioctl(dsp_fd, SNDCTL_DSP_SETTRIGGER, &trigger);
	trigger = PCM_ENABLE_OUTPUT;
	(void) ioctl(dsp_fd, SNDCTL_DSP_SETTRIGGER, &trigger);

	/* We're off and running! */
//printf("Open!\n");
	return(0);
}

void
Mixer:: DSPclose(void)
{
	if ( dsp_fd >= 0 ) {
		(void) close(dsp_fd);
		if ( dma ) {
			munmap((char *)dma, dma_size);
		}
		dsp_fd = -1;
	}
//printf("Closed!\n");
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
	unsigned short i;
	signed char data;
	fd_set dma_fdset;

//printf("<"); fflush(stdout);
	/* This is for mono output */
	for( frag_offset=0; frag_offset<frag_size; ++frag_offset ) {
		unclipped = 0;

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
		unclipped += SILENCE;
#endif /* UNSIGNED_AUDIO_DATA */

		if ( unclipped < MIN_AUDIO_VAL )
			clipped[frag_offset] = MIN_AUDIO_VAL;
		else if ( unclipped > MAX_AUDIO_VAL )
			clipped[frag_offset] = MAX_AUDIO_VAL;
		else
			clipped[frag_offset] = unclipped;
	}

	/* Add trailing sound to cover overlaps and prevent underruns */
	for( frag_offset=0; frag_offset<TRAILER; ++frag_offset ) {
		int choff;
		unclipped = 0;

#ifndef ASYNCHRONOUS_IO
		if ( (frag_offset%IO_CHECK) == 0 ) 
			Check_IO();
#endif /* ASYNCHRONOUS_IO */

		for( i=0; i<NUM_CHANNELS; ++i ) {
			/* See if the channel is in use */
			if ( channels[i].in_use ) {
				choff = channels[i].position+frag_offset;
				if( choff < channels[i].sample->len ) {
					/* Normalize the data */
					data = 
				(*(channels[i].sample->data + choff)^0x80);
					unclipped += data;
				}
			}
		}
		/* Apply volume */
		unclipped = (int)((float)unclipped * volume);

#ifdef UNSIGNED_AUDIO_DATA
		/* Re-normalize the values */
		unclipped += (MAX_AUDIO_VAL/2)+1;
#endif /* UNSIGNED_AUDIO_DATA */

		if ( unclipped < MIN_AUDIO_VAL )
			clipped[frag_size+frag_offset] = MIN_AUDIO_VAL;
		else if ( unclipped > MAX_AUDIO_VAL )
			clipped[frag_size+frag_offset] = MAX_AUDIO_VAL;
		else
			clipped[frag_size+frag_offset] = unclipped;
	}
#ifdef DEBUG
for( i=0; i<NUM_CHANNELS; ++i ) {
	if ( channels[i].in_use )
		error("Channel %hu: position = %d, len = %d\n", i, channels[i].position, channels[i].sample->len);
}
#endif
//printf(">"); fflush(stdout);

	/* Write out the data */
	if ( Device_Opened() ) {
		struct count_info count;

		/* Wait for the fragment to be written */
	select_it:
		FD_ZERO(&dma_fdset);
		FD_SET(dsp_fd, &dma_fdset);
		(void) select(dsp_fd+1, NULL, &dma_fdset, NULL, NULL);

		/* Find out where we are in the dma */
		if ( ioctl(dsp_fd, SNDCTL_DSP_GETOPTR, &count) < 0 ) {
			perror("Mixer: Can't determine DMA state");
			return;
		}
		/* Align the pointer to fragment boundary */
		count.ptr = (count.ptr/frag_size)*frag_size;
		if ( count.ptr == dma_size )
			count.ptr = 0;

		switch (count.blocks) {
			case 0:		/* Not ready yet? */
					goto select_it;
					break;
			case 1:		/* One block read, go for it */
					break;
			default:	/* Underrun!! */
#ifdef DEBUG
					error(
					"Audio underrun: missed %d fragments\n",
							count.blocks-1);
#endif
					break;
		}
		/* Chunk the data into the DMA buffer */
		if ( (count.ptr + frag_size + TRAILER) < dma_size ) {
			memcpy(dma+count.ptr, clipped, frag_size+TRAILER);
		} else {
			memcpy(dma+count.ptr, clipped, frag_size);
			memcpy(dma, clipped+frag_size, TRAILER);
		}
		/* Clean out the sound behind us -- sweet golden silence */
		if ( count.ptr == 0 ) {
			memset(dma+dma_size-frag_size, SILENCE, frag_size);
		} else {
			memset(dma+count.ptr-frag_size, SILENCE, frag_size);
		}
//printf("Count ptr = %d\n", count.ptr);
//printf("Audio blocks read:  %d\n", count.blocks);
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
