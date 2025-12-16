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

/* A WAVE class that can load itself from WAVE files or Mac 'snd ' resources */

#include <stdio.h>
#include <stdarg.h>
#include <SDL3/SDL.h>
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
