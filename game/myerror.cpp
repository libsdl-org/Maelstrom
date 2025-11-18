/*
    Maelstrom: Open Source version of the classic game by Ambrosia Software
    Copyright (C) 1997-2011  Sam Lantinga

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
    slouken@libsdl.org
*/

/* Generic error message routines */

#include <stdio.h>
#include <errno.h>

#include "SDL.h"

#include "myerror.h"

void error(const char *fmt, ...)
{
    char mesg[BUFSIZ];
    va_list ap;

    va_start(ap, fmt);
    SDL_vsnprintf(mesg, sizeof(mesg), fmt, ap);
    if (mesg[SDL_strlen(mesg)-1] == '\n') {
        mesg[SDL_strlen(mesg)-1] = '\0';
    }
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", mesg);
    va_end(ap);
}

void mesg(const char *fmt, ...)
{
    char mesg[BUFSIZ];
    va_list ap;

    va_start(ap, fmt);
    SDL_vsnprintf(mesg, sizeof(mesg), fmt, ap);
    if (mesg[SDL_strlen(mesg)-1] == '\n') {
        mesg[SDL_strlen(mesg)-1] = '\0';
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", mesg);
    va_end(ap);
}
