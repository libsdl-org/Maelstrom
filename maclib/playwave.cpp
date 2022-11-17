/*
    PLAYWAVE:  A WAVE file player using the maclib and SDL libraries
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

/* Very simple WAVE player */

#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_audio.h"
#include "SDL_mutex.h"
#include "Mac_Wave.h"

static SDL_mutex *done;
static Wave *wave;
static Uint8 silence;

void fillerup(void *unused, Uint8 *stream, int len)
{
	Sint32 waveleft;

	/* Are we done? */
	if ( wave->DataLeft() == 0 ) {
		memset(stream, silence, len);
		SDL_mutexV(done);
		return;
	}

	/* Fill streaming buffer */
	waveleft = wave->DataLeft();
	if ( waveleft > len )
		waveleft = len;
	memcpy(stream, wave->Data(), waveleft);
	wave->Forward(waveleft);
}

void CleanUp(int status)
{
	SDL_CloseAudio();
	SDL_DestroyMutex(done);
	SDL_Quit();
	delete wave;
	exit(status);
}

int main(int argc, char *argv[])
{
	Mac_Resource *macx;
	Mac_ResData  *snd;
	SDL_AudioSpec *spec;
	Uint16        rate;

	if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}
	rate = 0;
	if ( (argc >= 3) && (strcmp(argv[1], "-rate") == 0) ) {
		int i;
		rate = (Uint16)atoi(argv[2]);
		for ( i=3; argv[i]; ++i ) {
			argv[i-2] = argv[i];
		}
		argv[i-2] = NULL;
		argc -= 2;
	}
	switch (argc) {
		case 2:
			/* Load the wave file into memory */
			wave = new Wave(argv[1], rate);
			if ( wave->Error() ) {
				fprintf(stderr, "%s\n", wave->Error());
				delete wave;
				exit(255);
			}
			break;
		case 3:
			macx = new Mac_Resource(argv[1]);
			if ( macx->Error() ) {
				fprintf(stderr, "%s\n", macx->Error());
				delete macx;
				exit(255);
			}
			if ( (argv[2][0] >= '0') && (argv[2][0] <= '9') )
				snd = macx->Resource("snd ", atoi(argv[2]));
			else
				snd = macx->Resource("snd ", argv[2]);
			if ( snd == NULL ) {
				fprintf(stderr, "%s\n", macx->Error());
				delete macx;
				exit(255);
			}
			wave = new Wave(snd, rate);
			delete macx;
			if ( wave->Error() ) {
				fprintf(stderr, "%s\n", wave->Error());
				delete wave;
				exit(255);
			}
			break;
		default:
	fprintf(stderr, "Usage: %s [-rate <rate>] <wavefile>\n", argv[0]);
	fprintf(stderr, "or..\n");
	fprintf(stderr, "       %s [-rate <rate>] <snd_fork> [soundnum]\n",
								argv[0]);
			exit(1);
	}
	spec = wave->Spec();
	silence = ((spec->format&AUDIO_U8) ? 0x80 : 0x00);
	spec->callback = fillerup;

#ifdef SAVE_THE_WAVES
	if ( wave->Save("save.wav") < 0 )
		fprintf(stderr, "Warning: %s\n", wave->Error());
#endif

	/* Create a semaphore to wait for end of play */
	done=SDL_CreateMutex();
	if ( done == NULL ) {
		fprintf(stderr, "%s\n", SDL_GetError());
		SDL_Quit();
		exit(255);
	}
	SDL_mutexP(done);	/* Prime it for blocking */

	/* Set the signals */
#ifdef SIGHUP
	signal(SIGHUP, CleanUp);
#endif
	signal(SIGINT, CleanUp);
#ifdef SIGQUIT
	signal(SIGQUIT, CleanUp);
#endif
	signal(SIGTERM, CleanUp);

	/* Show what audio format we're playing */
	printf("Playing %#.2f seconds (%d bit %s) at %lu Hz\n", 
		(double)(wave->DataLeft()/wave->SampleSize())/wave->Frequency(),
			wave->BitsPerSample(),
			wave->Stereo() ? "stereo" : "mono", wave->Frequency());

	/* Start the audio device */
	if ( SDL_OpenAudio(spec, NULL) < 0 ) {
		fprintf(stderr, "%s\n", SDL_GetError());
		CleanUp(255);
	}

	/* Let the audio run, waiting until finished */
	SDL_PauseAudio(0);
	SDL_mutexP(done);

	/* We're done! */
	CleanUp(0);
}
