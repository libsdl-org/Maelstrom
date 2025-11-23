/*
  maclib:  A companion library to SDL for working with Macintosh (tm) data
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

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

#include <stdio.h>
#include <signal.h>

#include "Mac_Sound.h"
#include "Mac_Compat.h"
#include "../utils/files.h"

static void SDLCALL FillAudio(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
	Sound *sound = (Sound *)userdata;

	if (additional_amount > 0) {
		Uint8* data = SDL_stack_alloc(Uint8, additional_amount);
		if (data) {
			Sound::FillAudioU8(sound, data, additional_amount);
			SDL_PutAudioStreamData(stream, data, additional_amount);
			SDL_stack_free(data);
		}
	}
}

Sound:: Sound(const char *soundfile, Uint8 vol) : ErrorBase()
{
	/* Initialize variables */
	volume  = 0;
	InitHash();

	SDL_AudioSpec spec = { SDL_AUDIO_U8, 1, DSP_FREQUENCY };
	stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, FillAudio, this);

	/* Empty the channels and start the music :-) */
	HaltSound();
	Volume(vol);
	SDL_ResumeAudioStreamDevice(stream);
}

Sound:: ~Sound()
{
	SDL_DestroyAudioStream(stream);
	FreeHash();
}

Uint8
Sound:: Volume(Uint8 vol)
{
	if ( vol > MAX_VOLUME )
		vol = MAX_VOLUME;
	volume = vol;

	return(volume);
}

Wave *
Sound:: LoadSound(Uint16 sndID)
{
	char file[128];
	SDL_IOStream *fp;
	Wave *wave;
	SDL_AudioSpec spec;

	SDL_snprintf(file, sizeof(file), "Sounds/snd#%d.wav", sndID);
	fp = OpenRead(file);
	if (!fp) {
		fprintf(stderr, "Couldn't open %s\n", file);
		return NULL;
	}

	wave = new Wave;
	if (!SDL_LoadWAV_IO(fp, 1, &spec, &wave->data, &wave->size)) {
		fprintf(stderr, "Couldn't decode audio from %s\n", file);
		delete wave;
		return NULL;
	}
	Hash(sndID, wave);

	return wave;
}

int
Sound:: PlaySound(Uint16 sndID, Uint8 priority, Uint8 channel,
					void (*callback)(Uint8 channel))
{
	Wave *wave;

	if ( priority <= Priority(channel) )
		return(-1);

	wave = Hash(sndID);
	if ( wave == NULL ) {
		wave = LoadSound(sndID);
	}
	if ( wave == NULL )
		return(-1);

	channels[channel].ID = sndID;
	channels[channel].priority = priority;
	channels[channel].len = wave->size;
	channels[channel].src = wave->data;
	channels[channel].callback = callback;
#ifdef DEBUG_SOUND
printf("Playing sound %hu on channel %d\n", sndID, channel);
#endif
	return(0);
}

/* This has to be a very fast routine, otherwise sound will lag and crackle */
void
Sound:: FillAudioU8(Sound *sound, Uint8 *stream, int len)
{
	int i, s;

	/* Mix in each of the channels, assuming 8-bit unsigned audio data */
	while ( len-- ) {
		s = 0;
		for ( i=0; i<NUM_CHANNELS; ++i ) {
			if ( sound->channels[i].len > 0 ) {
				/*
				  Possible race condition:
				  If the channel is halted here,
					len = 0 then we do '--len'
				  len = -1, but that's okay.
				*/
				--sound->channels[i].len;
				s += *(sound->channels[i].src)-0x80;
				++sound->channels[i].src;
				/*
				  Possible race condition:
				  If a sound is played here,
					len > 0, then we do 'if len <= 0'
				  We never call back on channel.. bad.
				*/
				if ( sound->channels[i].len <= 0 ) {
#ifdef DEBUG_SOUND
printf("Channel %d finished\n", i);
#endif
					/* This is critical */
					void (*callback)(Uint8);
					callback = sound->channels[i].callback;
					if ( callback )
						(*callback)(i);
				}
			}
		}
		/* handle volume */
		s = (s*sound->volume)/MAX_VOLUME;

		/* convert to 8-bit unsigned */
		s += 0x80;

		/* clip */
		if ( s > 0xFE )	/* 0xFF causes static on some audio systems */
			*stream++ = 0xFE;
		else
		if ( s < 0x00 )
			*stream++ = 0x00;
		else
			*stream++ = (Uint8)s;
	}
}
