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
