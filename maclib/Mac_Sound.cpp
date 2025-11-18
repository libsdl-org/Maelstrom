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

static int bogus_running = 0;

extern "C" {
static int BogusAudioThread(void *data)
{
	SDL_AudioSpec *spec;
	void (*fill)(void *userdata, Uint8 *stream, int len);
	Uint32 then;
	Uint32 playticks;
	Sint32 ticksleft;
	Uint8 *stream;

#ifdef NSIG
	/* Clear out any signal handlers */
	for ( int i = 0; i<NSIG; ++i )
		signal(i, SIG_DFL);
#else
	signal(SIGINT, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
#endif

	/* Get ready to roll.. */
	spec = (SDL_AudioSpec *)data;
	if ( spec->callback == NULL ) {
		for ( ; ; )
			Delay(60*60*60);	/* Delay 1 hour */
	}
	fill = spec->callback;
	playticks = ((Uint32)spec->samples*1000)/spec->freq;
	/* Fill in the spec */
	spec->size = (spec->format&0xFF)/8;
	spec->size *= spec->channels;
	spec->size *= spec->samples;
	stream = new Uint8[spec->size];

	while ( bogus_running ) {
		then = SDL_GetTicks();

		/* Fill buffer */
		if ( fill )
			(*fill)(spec->userdata, stream, spec->size);

		/* Calculate time left, and sleep */
		ticksleft = playticks-(SDL_GetTicks()-then);
		if ( ticksleft > 0 )
			SDL_Delay(ticksleft);
	}
	delete[] stream;

	return(0);
}

static void FillAudio(void *udata, Uint8 *stream, int len)
{
	Sound *sound = (Sound *)udata;
	
	Sound::FillAudioU8(sound, stream, len);
}
/* extern "C" */
};

Sound:: Sound(const char *soundfile, Uint8 vol) : ErrorBase()
{
	int           i, p;

	/* Initialize variables */
	volume  = 0;
	playing = 0;
	bogus_audio = NULL;
	InitHash();

	/* Allow ~ 1/30 second time-lag in audio buffer -- samples is x^2  */
	spec.freq = DSP_FREQUENCY;
	spec.format = AUDIO_U8;
	spec.channels = 1;
	spec.silence = 0x80;
	spec.samples = (spec.freq*spec.channels)/30;
	for ( p = 0; spec.samples > 1; ++p )
		spec.samples /= 2;
	++p;
	for ( i = 0; i < p; ++i )
		spec.samples *= 2;
	spec.callback = FillAudio;
	spec.userdata = (void *)this;

	/* Empty the channels and start the music :-) */
	HaltSound();
	if ( vol == 0 ) {
		bogus_running = 1;
		bogus_audio = SDL_CreateThread(BogusAudioThread, "Fake Audio", &spec);
	} else {
		Volume(vol);
	}
}

Sound:: ~Sound()
{
	if ( playing )
		SDL_CloseAudio();
	else
	if ( bogus_audio ) {
		bogus_running = 0;
		SDL_WaitThread(bogus_audio, NULL);
		bogus_audio = NULL;
	}
	FreeHash();
}

Uint8
Sound:: Volume(Uint8 vol)
{
	Uint8 active;

	active = playing;
	if ( (volume == 0) && (vol > 0) ) {
		/* Kill bogus sound thread */
		if ( bogus_audio ) {
			bogus_running = 0;
			SDL_WaitThread(bogus_audio, NULL);
			bogus_audio = NULL;
		}

		/* Try to open the audio */
		if ( SDL_OpenAudio(&spec, NULL) < 0 )
			vol = 0;		/* Fake sound */
		active = 1;
		SDL_PauseAudio(0);		/* Go! */
	}
	if ( vol > MAX_VOLUME )
		vol = MAX_VOLUME;
	volume = vol;

	if ( active && (volume == 0) ) {
		if ( playing )
			SDL_CloseAudio();
		active = 0;

		/* Run bogus sound thread */
		bogus_running = 1;
		bogus_audio = SDL_CreateThread(BogusAudioThread, "Fake Audio", &spec);
		if ( bogus_audio == NULL ) {
			/* Oh well... :-) */
		}
	}
	playing = active;
	return(volume);
}

Wave *
Sound:: LoadSound(Uint16 sndID)
{
	char file[128];
	SDL_RWops *fp;
	Wave *wave;
	SDL_AudioSpec spec;

	SDL_snprintf(file, sizeof(file), "Sounds/snd_%d.wav", sndID);
	fp = OpenRead(file);
	if (!fp) {
		fprintf(stderr, "Couldn't open %s\n", file);
		return NULL;
	}

	wave = new Wave;
	if (!SDL_LoadWAV_RW(fp, 1, &spec, &wave->data, &wave->size)) {
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
