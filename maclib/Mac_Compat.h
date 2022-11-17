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

#ifndef _Mac_Compat_h
#define _Mac_Compat_h

#include "SDL_timer.h"

/* Some simple inline Macintosh compatibility routines */

/* Delay(x) -- sleep for x number of 1/60 second slices */
#define Delay(x)	SDL_Delay(((x)*1000)/60)
/* Ticks -- a global variable containing current time in 1/60 second slices */
#define Ticks		((SDL_GetTicks()*60)/1000)

#endif /* _Mac_Compat_h */
