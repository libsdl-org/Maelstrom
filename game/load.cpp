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

#include "Maelstrom_Globals.h"
#include "load.h"

#include "../utils/files.h"


UITexture *Load_Texture(FrameBuf *screen, const char *folder, const char *name)
{
	char file[256];

	// Use the game display area for determining which art set to use
	for (unsigned int i = gResolutionIndex; i < gResolutions.length(); ++i) {
		SDL_snprintf(file, sizeof(file), "%s%s/%s%s.png",
				folder, gResolutions[i].path_suffix, name, gResolutions[i].file_suffix);
		SDL_Surface *surface = SDL_LoadSurface_IO(OpenRead(file), true);
		if (surface) {
			SDL_Texture *texture = screen->LoadImage(surface);
			SDL_DestroySurface(surface);
			return new UITexture(texture, gResolutions[i].scale);
		}
	}
	return NULL;
}

void Free_Texture(FrameBuf *screen, UITexture *texture)
{
	screen->FreeImage(texture->Texture());
	delete texture;
}

UITexture *Load_Image(FrameBuf *screen, const char *name)
{
	return Load_Texture(screen, "Images", name);
}

UITexture *Load_Title(FrameBuf *screen, int title_id)
{
	char name[256];
	SDL_snprintf(name, sizeof(name), "Maelstrom_Titles#%d", title_id);
	return Load_Texture(screen, "Images", name);
}

UITexture *GetCIcon(FrameBuf *screen, short id)
{
	char name[256];
	SDL_snprintf(name, sizeof(name), "Maelstrom_Icon#%d", id);
	return Load_Texture(screen, "Images", name);
}

UITexture *GetSprite(FrameBuf *screen, short id, bool large)
{
	char name[256];
	SDL_snprintf(name, sizeof(name), "%s#%d", large ? "icl8" : "ics8", id);
	return Load_Texture(screen, "Sprites", name);
}
