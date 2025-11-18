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

#ifndef _load_h
#define _load_h

#include "../screenlib/SDL_FrameBuf.h"

/* Functions exported from load.cpp */
extern UITexture *Load_Texture(FrameBuf *screen, const char *folder, const char *name);
extern void Free_Texture(FrameBuf *screen, UITexture *texture);
extern UITexture *Load_Image(FrameBuf *screen, const char *name);
extern UITexture *Load_Title(FrameBuf *screen, int title_id);
extern UITexture *GetCIcon(FrameBuf *screen, short id);
extern UITexture *GetSprite(FrameBuf *screen, short id, bool large);

#endif /* _load_h */
