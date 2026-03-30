/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

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

/* Generic error message routines */

#include <SDL3/SDL.h>

#include "myerror.h"

void error(const char *fmt, ...)
{
    char mesg[1024];
    va_list ap;

    va_start(ap, fmt);
    SDL_vsnprintf(mesg, sizeof(mesg), fmt, ap);
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", mesg);
    va_end(ap);
}

void mesg(const char *fmt, ...)
{
    char mesg[1024];
    va_list ap;

    va_start(ap, fmt);
    SDL_vsnprintf(mesg, sizeof(mesg), fmt, ap);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", mesg);
    va_end(ap);
}
