/*
    MACLIB:  A companion library to SDL for working with Macintosh (tm) data
    Copyright (C) 1997-2021 Sam Lantinga <slouken@libsdl.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* A WAVE class that can load itself from WAVE files or Mac 'snd ' resources */

#include <stdio.h>
#include <stdarg.h>
#include "SDL_audio.h"
#include "Mac_Resource.h"

class Wave {

public:
	Wave() {
		Init();
	}
	Wave(const char *wavefile, Uint16 desired_rate = 0) {
		Init();
		Load(wavefile, desired_rate);
	}
	Wave(Mac_ResData *snd, Uint16 desired_rate = 0) {
		Init();
		Load(snd, desired_rate);
	}
	~Wave() {
		Free();
	}

	/* Load WAVE resources, converting to the desired sample rate */
	int Load(const char *wavefile, Uint16 desired_rate = 0);
	int Load(Mac_ResData *snd, Uint16 desired_rate = 0);
	int Save(const char *wavefile);

	void Rewind(void) {
		soundptr = sound_data;
		soundlen = sound_datalen;
	}
	void Forward(Uint32 distance) {
		soundlen -= distance;
		soundptr += distance;
	}
	Uint32 DataLeft(void) {
		return(soundlen > 0 ? soundlen : 0);
	}
	Uint8 *Data(void) {
		if ( soundlen > 0 )
			return(soundptr);
		return(NULL);
	}
	SDL_AudioSpec *Spec(void) {
		return(&spec);
	}
	Uint32 Frequency(Uint16 desired_rate = 0);
	Uint16 SampleSize(void) {
		return(((spec.format&0xFF)/8)*spec.channels);
	}
	int BitsPerSample(void) {
		return(spec.format&0xFF);
	}
	int Stereo(void) {
		return(spec.channels/2);
	}

	char *Error(void) {
		return(errstr);
	}

private:
	void Init(void);
	void Free(void);

	/* The SDL-ready audio specification */
	SDL_AudioSpec spec;
	Uint8 *sound_data;
	Uint32 sound_datalen;

	/* Current position of the WAVE file */
	Uint8 *soundptr;
	Sint32 soundlen;

	/* Utility functions */
	Uint32 ConvertRate(Uint16 rate_in, Uint16 rate_out, 
			Uint8 **samples, Uint32 n_samples, Uint8 s_size);
	
	/* Useful for getting error feedback */
	void error(const char *fmt, ...) {
		va_list ap;

		va_start(ap, fmt);
		SDL_vsnprintf(errbuf, sizeof(errbuf), fmt, ap);
		va_end(ap);
		errstr = errbuf;
	}
	char *errstr;
	char  errbuf[BUFSIZ];
};
