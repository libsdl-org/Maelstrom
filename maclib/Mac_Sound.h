/*
    MACLIB:  A companion library to SDL for working with Macintosh (tm) data
    Copyright (C) 1997  Sam Lantinga

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

    Sam Lantinga
    5635-34 Springhouse Dr.
    Pleasanton, CA 94588 (USA)
    slouken@devolution.com
*/

/* This is a general mixer that takes Macintosh sound resource files
   and mixes various sounds on command.
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "SDL_types.h"
#include "SDL_mutex.h"
#include "SDL_thread.h"
#include "SDL_audio.h"
#include "Mac_Wave.h"

#define MAX_VOLUME	8		/* Software volume ranges from 0 - 8 */
#define NUM_CHANNELS	4		/* 4 sound mixing channels, limit 128 */
#define DSP_FREQUENCY	11025		/* Convert the SNDs to this frequency */

class Sound {

public:
	Sound(const char *soundfile, Uint8 vol = 4);
	~Sound();

	/* Set volume in the range 0-8 */
	Uint8 Volume(Uint8 vol);
	Uint8 Volume(void) {
		return(volume);
	}

	/* Play the requested sound */
	int  PlaySound(Uint16 sndID, Uint8 priority = 0, 
				void (*callback)(Uint8 channel) = NULL) {
		Uint8 i;

		for ( i=0; i<NUM_CHANNELS; ++i ) {
			if ( priority > Priority(i) )
				return(PlaySound(sndID, priority, i, callback));
		}
		return(-1);
	}
	int  PlaySound(Uint16 sndID, Uint8 priority, Uint8 channel,
				void (*callback)(Uint8 channel) = NULL);

	/* Stop mixing on the requested channels */
	void HaltSound(void) {
		Uint8 i;

		for ( i=0; i<NUM_CHANNELS; ++i )
			HaltSound(i);
	}
	void HaltSound(Uint8 channel) {
		channels[channel].len = 0;
	}

	/* Find out if a sound is playing on a channel */
	int Playing(Sint16 sndID = 0, Sint8 *channel = NULL) {
		Uint8 i;

		for ( i=0; i<NUM_CHANNELS; ++i ) {
			if ( channels[i].len <= 0 )
				continue;
			if ( (sndID == 0) || (sndID == channels[i].ID) ) {
				if ( channel )
					*channel = i;
				return(1);
			}
		}
		return(0);
	}
	Sint16 Priority(Uint8 channel) {
		if ( channels[channel].len > 0 )
			return(channels[channel].priority);
		return(-1);
	}
	int  ChannelFree(Sint8 channel = -1) {
		Uint8 i;

		/* Checking a particular channel? */
		if ( channel >= 0 )
			return((channels[channel].len > 0) ? 1 : 0);

		/* Checking any channel */
		for ( i=0; i<NUM_CHANNELS; ++i ) {
			if ( channels[i].len > 0 )
				return(1);
		}
		return(0);
	}

	char *Error(void) {
		return(errstr);
	}

	/* These functions really do all the work */
	static void FillAudioU8(Sound *sound, Uint8 *stream, int len);

private:
	Uint8 playing;

	struct channel {
		Uint16 ID;
		Sint16 priority;
		int len;	/* Signed, so race conditions can make it < 0 */
		Uint8 *src;
		void (*callback)(Uint8 channel);
	} channels[NUM_CHANNELS];

	SDL_AudioSpec *spec;
	Uint8      volume;

	/* Fake audio handler, in case we can't open the real thing */
	SDL_Thread *bogus_audio;

	/* Functions for getting and setting a hash indexed by Uint16 */
	/* We use a sparse tiered pointer page scheme :-)
	   It's probably not unique, but I made this version up. :-)
	*/
	Wave ***hashpage;
	void  InitHash(void) {
		hashpage = new Wave **[256];
		memset(hashpage, 0, 256*sizeof(Wave **));
	}
	void  Hash(Uint16 index, Wave *value) {
		Uint8 upper, lower;
		upper = index>>8;
		lower = index&0xFF;
		if ( hashpage[upper] == NULL ) {
			hashpage[upper] = new Wave *[256];
			memset(hashpage[upper], 0, 256*sizeof(Wave *));
		}
		if ( hashpage[upper][lower] ) {
#ifdef DEBUG_HASH
printf("Warning: Hash page %d/%d already used!\n", upper, lower);
#endif
			delete hashpage[upper][lower];
		}
#ifdef DEBUG_HASH
printf("Saving Wave id %hu to hash page %d/%d\n", index, upper, lower);
#endif
		hashpage[upper][lower] = value;
	}
	Wave *Hash(Uint16 index) {
		Uint8 upper, lower;
		upper = index>>8;
		lower = index&0xFF;
		if ( hashpage[upper] == NULL )
			return(NULL);
		return(hashpage[upper][lower]);
	}
	void  FreeHash(void) {
		Uint16 upper, lower;

		for ( upper = 0; upper < 256; ++upper ) {
			if ( hashpage[upper] ) {
				for ( lower = 0; lower < 256; ++lower ) {
					if ( hashpage[upper][lower] ) {
#ifdef DEBUG_HASH
printf("Freeing Wave id %hu at hash page %d/%d\n",(upper<<8)|lower,upper,lower);
#endif
						delete hashpage[upper][lower];
					}
				}
				delete[] hashpage[upper];
			}
		}
		delete[] hashpage;
	}

	/* Useful for getting error feedback */
	void error(char *fmt, ...) {
		va_list ap;

		va_start(ap, fmt);
		vsprintf(errbuf, fmt, ap);
		va_end(ap);
		errstr = errbuf;
	}
	char *errstr;
	char  errbuf[BUFSIZ];
};
