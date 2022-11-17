/*
    SND2WAV:  A program to convert Macintosh (tm) sound resources
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

/* A Macintosh sound resource converter */

#include <stdlib.h>
#include <string.h>

#include "Mac_Wave.h"

static Wave wave;

int main(int argc, char *argv[])
{
	Mac_Resource *macx;
	Mac_ResData  *snd;
	char wavname[128];
	Uint16 *ids, rate;
	int i;

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
	if ( argv[1] == NULL ) {
		fprintf(stderr,
		"Usage: %s [-rate <rate>] <snd_fork> [soundnum]\n", argv[0]);
		exit(1);
	}

	macx = new Mac_Resource(argv[1]);
	if ( macx->Error() ) {
		fprintf(stderr, "%s\n", macx->Error());
		delete macx;
		exit(255);
	}
	if ( macx->NumResources("snd ") == 0 ) {
		fprintf(stderr, "No sound resources in '%s'\n", argv[1]);
		delete macx;
		exit(1);
	}

	/* If a specific resource is requested, save it alone */
	if ( argv[2] ) {
		ids = new Uint16[2];
		ids[0] = atoi(argv[2]);
		ids[1] = 0xFFFF;
	} else
		ids = macx->ResourceIDs("snd ");

	for ( i=0; ids[i] != 0xFFFF; ++i ) {
		snd = macx->Resource("snd ", ids[i]);
		if ( snd == NULL ) {
			fprintf(stderr, "Warning: %s\n", macx->Error());
			continue;
		}
		wave.Load(snd, rate);
		sprintf(wavname, "snd_%d.wav", ids[i]);
		wave.Save(wavname);
	}
	delete macx;
	exit(0);
}
