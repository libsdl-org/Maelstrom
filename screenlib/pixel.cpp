/*
    SCREENLIB:  A framebuffer library based on the SDL library
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

#include "SDL.h"
#include "pixel.h"

void PutPixel1(Uint8 *screen_loc, SDL_Surface *screen, Uint32 pixel) {
	*((Uint8 *)screen_loc) = pixel;
}
void PutPixel2(Uint8 *screen_loc, SDL_Surface *screen, Uint32 pixel) {
	*((Uint16 *)screen_loc) = pixel;
}
void PutPixel3(Uint8 *screen_loc, SDL_Surface *screen, Uint32 pixel) {
	int shift;

	/* Gack - slow, but endian correct */
	shift = screen->format->Rshift;
	*(screen_loc+shift/8) = pixel>>shift;
	shift = screen->format->Gshift;
	*(screen_loc+shift/8) = pixel>>shift;
	shift = screen->format->Bshift;
	*(screen_loc+shift/8) = pixel>>shift;
}
void PutPixel4(Uint8 *screen_loc, SDL_Surface *screen, Uint32 pixel) {
	*((Uint32 *)screen_loc) = pixel;
}
