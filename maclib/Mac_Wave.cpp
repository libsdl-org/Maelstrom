/*
  maclib:  A companion library to SDL for working with Macintosh (tm) data
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* Microsoft WAVE file loading routines */

#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include "Mac_Wave.h"

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define wavesex32(x)	\
        ((x << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00) | (x >> 24))
#define wavesex16(x)	((x << 8) | (x >> 8))
#define snd_sex32(x)	(x)
#define snd_sex16(x)	(x)
#else
#define wavesex32(x)	(x)
#define wavesex16(x)	(x)
#define snd_sex32(x)	\
        ((x << 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00) | (x >> 24))
#define snd_sex16(x)	((x << 8) | (x >> 8))
#endif /* Endianness */

/*******************************************/
/* Define values for Macintosh SND format */
/*******************************************/

/* Different sound header formats */
#define FORMAT_1	0x0001
#define FORMAT_2	0x0002

/* The different types of sound data */
#define SAMPLED_SND	0x0005

/* Initialization commands */
#define MONO_SOUND	0x00000080
#define STEREO_SOUND	0x000000A0

/* The different sound commands; we only support BUFFER_CMD */
#define SOUND_CMD	0x8050		/* Different from BUFFER_CMD? */
#define BUFFER_CMD	0x8051

/* Different original sampling rates -- rate = (#define)>>16 */
#define rate44khz	0xAC440000		/* 44100.0 */
#define rate22khz	0x56EE8BA3		/* 22254.5 */
#define rate11khz	0x2B7745D0		/* 11127.3 */
#define rate11khz2	0x2B7745D1		/* 11127.3 (?) */

#define stdSH		0x00
#define extSH		0xFF
#define cmpSH		0xFE

/*******************************************/

#define snd_copy16(V, D)						\
{									\
	V = *((Uint16 *)D);						\
	D += 2;								\
	V = snd_sex16(V);						\
}
#define snd_copy32(V, D)						\
{									\
	memcpy(&V, D, sizeof(Uint32));					\
	D += 4;								\
	V = snd_sex32(V);						\
}

/*******************************************/

void
Wave:: Init(void) {
	sound_data = NULL;
	errstr = NULL;
}
void
Wave:: Free(void) {
	if ( sound_data ) {
		delete[] sound_data;
		sound_data = NULL;
	}
}

int
Wave:: Load(const char *wavefile, Uint16 desired_rate)
{
	Uint8 *samples;

	/* Free any existing WAVE data */
	Free();
	Init();

	/* Load the WAVE file */
	if ( !SDL_LoadWAV(wavefile, &spec, &samples, &sound_datalen) ) {
		error("%s", SDL_GetError());
		return(-1);
	}
	/* Copy malloc()'d data to new'd data */
	sound_data = new Uint8[sound_datalen];
	memcpy(sound_data, samples, sound_datalen);
	SDL_free(samples);

	/* Set the desired sample frequency */
	Frequency(desired_rate);

	/* Rewind and go! */
	Rewind();
	return(0);
}

Uint32
Wave:: Frequency(Uint16 desired_rate)
{
	if ( (desired_rate > 0) && (desired_rate != spec.freq) ) {
		Uint8 *samples;
		Uint32 datalen, samplesize;

		samples = sound_data;
		samplesize = SampleSize();
		datalen = ConvertRate(spec.freq, desired_rate,
				&samples, sound_datalen/samplesize, samplesize);
		if ( samples != sound_data ) {
			/* Create new sound data */
			delete[] sound_data; 
			sound_data = samples;
			sound_datalen = datalen*samplesize;

			/* Adjust the format */
			spec.freq = desired_rate;
		}
	}
	return(spec.freq);
}

/* Most of this information came from the "Inside Macintosh" book series */
int
Wave:: Load(Mac_ResData *snd, Uint16 desired_rate)
{
	Uint8 *data;
	Uint8 *samples;
	Uint16 snd_version;
	int snd_channels;

	/* Free any existing WAVE data */
	Free();
	Init();

	/* Start loading the WAVE from the SND */
	data = snd->data;
	snd_copy16(snd_version, data);

	snd_channels = 1;			/* Is this always true? */
	if ( snd_version == FORMAT_1 ) {
		Uint16 n_types;		/* Number of sound data types */
		Uint16 f_type;		/* First sound data type */
		Uint32 init_op;		/* Initialization option (unused) */

		snd_copy16(n_types, data);
		if ( n_types != 1 ) {
			error("Multi-type sound not supported");
			return(-1);
		}
		snd_copy16(f_type, data);
		if ( f_type != SAMPLED_SND ) {
			error("Not a sampled sound resource");
			return(-1);
		}
		snd_copy32(init_op, data);
	} else if ( snd_version == FORMAT_2 ) {
		Uint16 ref_cnt;		/* (unused) */
	
		snd_copy16(ref_cnt, data);
	} else {
		error("Unknown sound format: 0x%X", snd_version);
		return(-1);
	}

	/* Next is the Sound commands section */
	{
		Uint16 num_cmds;	/* Number of sound commands */
		Uint16 command;		/* The first sound command */
		Uint16 param1;		/* BUFFER_CMD parameter 1 */
		Uint32 param2;		/* Offset to sampled data */

		snd_copy16(num_cmds, data);
		if ( num_cmds != 1 ) {
			error("Multi-command sound not supported");
			return(-1);
		}
		snd_copy16(command, data);
		if ( (command != BUFFER_CMD) && (command != SOUND_CMD) ) {
			error("Unknown sound command: 0x%X\n", command);
			return(-1);
		}
		snd_copy16(param1, data);
		/* Param1 is ignored (should be 0x0000) */

		snd_copy32(param2, data);
		/* Set 'data' to the offset of the sampled data */
		if ( param2 > snd->length ) {
			error("Offset too large -- corrupt sound?");
			return(-1);
		}
		data = snd->data+param2;
	}

	/* Next is the sampled sound header */
	{
		Uint32 sample_offset;
		Uint32 num_samples;
		Uint32 sample_rate;
		Uint32 loop_start, loop_end;
		Uint8  encoding;
		Uint8  freq_base;

		snd_copy32(sample_offset, data);
		/* FIXME: What's the interpretation of this offset? */
		if ( sample_offset != 0 ) {
			error("Sound samples don't immediately follow header");
			return(-1);
		}
		snd_copy32(num_samples, data);
		snd_copy32(sample_rate, data);
		/* Sound loops are ignored for now */
		snd_copy32(loop_start, data);
		snd_copy32(loop_end, data);
		encoding = *data++;
		if ( encoding != stdSH ) {
			error("Non-standard sound encoding: 0x%X", encoding);
			return(-1);
		}
		/* Frequency base might be used later */
		freq_base = *data++;
		(void)freq_base;
		
		/* Now allocate room for the sound */
		if ( num_samples > snd->length-(data-snd->data) ) {
			error("truncated sound resource");
			return(-1);
		}

		/* Convert the audio data to desired sample rates */
		samples = data;
		switch ( sample_rate ) {
			case rate11khz:
			case rate11khz2:
				/* Assuming 8-bit mono samples */
				if ( desired_rate == 0 )
					desired_rate = 11025;
				num_samples =
				  ConvertRate(sample_rate>>16, desired_rate,
						&samples, num_samples, 1);
				break;
			case rate22khz:
				/* Assuming 8-bit mono samples */
				if ( desired_rate == 0 )
					desired_rate = 22050;
				num_samples =
				  ConvertRate(sample_rate>>16, desired_rate,
						&samples, num_samples, 1);
				break;
			case rate44khz:
			default:
				if ( desired_rate == 0 ) {
					desired_rate = (sample_rate>>16);
					break;
				}
				num_samples =
				  ConvertRate(sample_rate>>16, desired_rate,
						&samples, num_samples, 1);
				break;
		}
		sample_rate = desired_rate;

		/* Fill in the audio spec */
		spec.freq = sample_rate;
		spec.format = SDL_AUDIO_U8;		/* The only format? */
		spec.channels = snd_channels;

		/* Save the audio data */
		sound_datalen = num_samples*snd_channels;
		if ( samples == data ) {
			sound_data = new Uint8[sound_datalen];
			memcpy(sound_data, samples, sound_datalen);
		} else {
			sound_data = samples;
		}
	}
	Rewind();
	return(0);
}

#define SLOW_CONVERT
#ifdef SLOW_CONVERT
/* This is relatively accurate, but not very fast */
Uint32
Wave:: ConvertRate(Uint16 rate_in, Uint16 rate_out, 
			Uint8 **samples, Uint32 n_samples, Uint8 s_size)
{
	double ipos, i_size;
	Uint32 opos;
	Uint32 n_in, n_out;
	Uint8 *input, *output;

	n_in = n_samples*s_size;
	input = *samples;
	n_out = (Uint32)(((double)rate_out/rate_in)*n_samples)+1;
	output = new Uint8[n_out*s_size];
	i_size = ((double)rate_in/rate_out)*s_size;
#ifdef CONVERTRATE_DEBUG
printf("%g seconds of input\n", (double)n_samples/rate_in);
printf("Input rate: %hu, Output rate: %hu, Input increment: %g\n", rate_in, rate_out, i_size/s_size);
printf("%g seconds of output\n", (double)n_out/rate_out);
#endif
	for ( ipos = 0, opos = 0; (Uint32)ipos < n_in; ) {
#ifdef CONVERTRATE_DEBUG
if ( opos >= n_out*s_size ) printf("Warning: buffer output overflow!\n");
#endif
		memcpy(&output[opos], &input[(Uint32)ipos], s_size);
		ipos += i_size;
		opos += s_size;
	}
	*samples = output;
	return(opos/s_size);
}
#else
#define CHECK_ERROR
/* This assumes that the rate in and the rate out are close to eachother,
   i.e. only one sample needs to be inserted or deleted for a certain
   number of skips.  It's fast, but if inaccurate it can lead to output
   distortion and buffer overrun.
*/
Uint32
Wave:: ConvertRate(Uint16 rate_in, Uint16 rate_out, 
			Uint8 **samples, Uint32 n_samples, Uint8 s_size)
{
	Uint8 *s_in, *s_out;
	Uint32 n_in,  n_out;		/* number of bytes in sample */
	Uint32 ideal_out;
	Uint32 p_in,  p_out;		/* byte position in sample */
	Uint16 run, runs, i;
	Uint8 free_sin;
	Uint8 srate_in, srate_out;
	Uint8 skip_in, stuff_out;
	int cumulative_error;

#ifdef CONVERTRATE_DEBUG
printf("%g seconds of input\n", (double)n_samples/rate_in);
#endif
	/* Initialize variables for input samples */
	srate_in = rate_in>>8;
	srate_out = rate_out>>8;
	free_sin = 0;
	s_in = *samples;
	n_in = n_samples*s_size;
	skip_in = srate_in/srate_out;
	stuff_out = srate_out/srate_in;

#ifdef CONVERTRATE_DEBUG
printf("Rate in: 0x%X, Rate out: 0x%X, Rate in/out = %d, Rate out/in = %d\n",
				rate_in>>8, rate_out>>8, skip_in, stuff_out);
#endif
	/* Use simple integer rate conversion to get sample rates close */
	if ( skip_in > 0 ) {
		/* rate_in is approximately some multiple of rate_out */
		n_out = n_in/skip_in;
		s_out = new Uint8[n_out];
		p_in  = 0;
		p_out = 0;
		while ( p_in < n_in ) {
			memcpy(&s_out[p_out], &s_in[p_in], s_size);
			p_in  += s_size*skip_in;
			p_out += s_size*1; 
		}
		free_sin = 1;
		s_in = s_out;
		n_in = n_out;
		rate_in /= skip_in;
	} else
	if ( stuff_out > 0 ) {
		/* rate_out is approximately some multiple of rate_in */
		n_out = n_in*stuff_out;
		s_out = new Uint8[n_out];
		p_in  = 0;
		p_out = 0;
		while ( p_in < n_in ) {
			for ( run = 0; run < stuff_out; ++run ) {
				memcpy(&s_out[p_out], &s_in[p_in], s_size);
				p_out += s_size;
			}
			p_in  += s_size;
		}
		free_sin = 1;
		s_in = s_out;
		n_in = n_out;
		rate_in *= stuff_out;
	}

	/* The algorithm: 
		Since rate_in is close to rate_out, we can go 'run' samples 
		before having to insert or delete a sample.  We always insert
		or delete a full sample, doing no floating point carry over
		for the next run.  If rate_in is relatively close to rate_out,
		the error is negligible.  The above rate change code works
		to shift standard Macintosh frequencies close to standard PC
		frequencies for no error with relatively short samples.
	*/
	if ( rate_in > rate_out ) {
		run = (rate_out/(rate_in-rate_out))*s_size;
		ideal_out = (Uint32)(((double)rate_out/rate_in)*n_in)*s_size;
		runs = ideal_out/run;
		n_out = (Uint32)run*runs+(n_in-(Uint32)(run+s_size)*runs);
		s_out = new Uint8[n_out];
		p_in = 0;
		p_out = 0;
#ifdef CHECK_ERROR
		printf("Ideal output: %lu bytes\n", ideal_out);
		cumulative_error = n_out - ideal_out;
		if ( cumulative_error != 0 ) {
			printf(
"WARNING: error in converting samples from rate %lu to %lu: %d bytes\n", 
					rate_in, rate_out, cumulative_error);
		}
#endif /* CHECK_ERROR */
		for ( i=0; i<runs; ++i ) {
			memcpy(&s_out[p_out], &s_in[p_in], run);
			p_in += run;
			p_out += run;
			p_in += s_size;
		}
		run = n_in-p_in;
		memcpy(&s_out[p_out], &s_in[p_in], run);
		p_out += run;
#ifdef CONVERTRATE_DEBUG
printf("rate_in = %hu, rate_out = %hu, n_in = %lu, n_out = %lu, p_out = %lu\n",
					rate_in, rate_out, n_in, n_out, p_out);
#endif
		if ( free_sin )
			delete[] s_in;
		*samples = s_out;
	} else
	if ( rate_out > rate_in ) {
		run = (rate_in/(rate_out-rate_in))*s_size;
		ideal_out = (Uint32)(((double)rate_in/rate_out)*n_in)*s_size;
		runs = ideal_out/run;
		n_out = (Uint32)runs*(run+s_size)+(n_in-(Uint32)runs*run);
		s_out = new Uint8[n_out];
		p_in = 0;
		p_out = 0;
#ifdef CHECK_ERROR
		printf("Ideal output: %lu bytes\n", ideal_out);
		cumulative_error = n_out - ideal_out;
		if ( cumulative_error != 0 ) {
			printf(
"WARNING: error in converting samples from rate %lu to %lu: %d bytes\n", 
					rate_in, rate_out, cumulative_error);
		}
#endif /* CHECK_ERROR */
		for ( i=0; i<runs; ++i ) {
			memcpy(&s_out[p_out], &s_in[p_in], run);
			p_in += run;
			p_out += run;
			memcpy(&s_out[p_out], &s_in[p_in], s_size);
			p_out += s_size;
		}
		run = n_in-p_in;
		memcpy(&s_out[p_out], &s_in[p_in], run);
		p_out += run;
#ifdef CONVERTRATE_DEBUG
printf("rate_in = %hu, rate_out = %hu, n_in = %lu, n_out = %lu, p_out = %lu\n",
					rate_in, rate_out, n_in, n_out, p_out);
#endif
		if ( free_sin )
			delete[] s_in;
		*samples = s_out;
	} else {
		p_out = n_in;
		*samples = s_in;
	}
#ifdef CONVERTRATE_DEBUG
printf("%g seconds of output\n", (double)(p_out/s_size)/rate_out);
#endif
	return(p_out/s_size);
}
#endif

int
Wave:: Save(const char *wavefile)
{
	/* Normally, these chunks come consecutively in a WAVE file */
	SDL_IOStream *dst;
	Uint32 wavelen;
	struct WaveFMT {
		Uint32	FMTchunk;
		Uint32	fmtlen;
		Uint16	encoding;	
		Uint16	channels;	/* 1 = mono, 2 = stereo */
		Uint32	frequency;	/* One of 11025, 22050, or 44100 Hz */
		Uint32	byterate;	/* Average bytes per second */
		Uint16	samplesize;	/* Bytes per sample block */
		Uint16	bitspersample;	/* One of 8, 12, 16 */
	} format;

	/* Open the WAV file for writing */
	dst = SDL_IOFromFile(wavefile, "wb");
	if ( dst == NULL ) {
		error("Couldn't open %s for writing", wavefile);
		return(-1);
	}

	/* Convert all the information about the sound chunk to WAVE format */
	memcpy(&format.FMTchunk, "fmt ", 4);
	format.fmtlen = sizeof(format)-2*sizeof(Uint32);
	format.encoding = 1;
	format.channels = spec.channels;
	format.frequency = spec.freq;
	format.byterate = format.frequency*format.channels;
	format.samplesize = ((spec.format&0xFF)/8)*spec.channels;
	format.bitspersample = (spec.format&0xFF);

	/* Swap the WAVE format information to little-endian format */
	format.fmtlen		= SDL_Swap32LE(format.fmtlen);
	format.encoding		= SDL_Swap16LE(format.encoding);
	format.channels		= SDL_Swap16LE(format.channels);
	format.frequency	= SDL_Swap32LE(format.frequency);
	format.byterate		= SDL_Swap32LE(format.byterate);
	format.samplesize	= SDL_Swap16LE(format.samplesize);
	format.bitspersample	= SDL_Swap16LE(format.bitspersample);

	/* Figure out how big the RIFF chunk will be */
	wavelen = sizeof(Uint32)+sizeof(format)+2*sizeof(Uint32)+sound_datalen;

	/* Save the WAVE */
	if ( !SDL_WriteIO(dst, "RIFF", sizeof(Uint32)) ||
	     !SDL_WriteU32LE(dst, wavelen) ||
	     !SDL_WriteIO(dst, "WAVE", sizeof(Uint32)) ||
	     !SDL_WriteIO(dst, &format, sizeof(format)) ||
	     !SDL_WriteIO(dst, "data", sizeof(Uint32)) ||
	     !SDL_WriteU32LE(dst, sound_datalen) ||
	     !SDL_WriteIO(dst, sound_data, sound_datalen) ) {
		error("Couldn't write to %s", wavefile);
		SDL_CloseIO(dst);
		return(-1);
	}
	if ( !SDL_CloseIO(dst) ) {
		error("Couldn't write to %s", wavefile);
		return(-1);
	}
	return(0);
}
