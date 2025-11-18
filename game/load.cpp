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

#include "SDL_image.h"

#include "Maelstrom_Globals.h"
#include "load.h"

#include "../utils/files.h"


UITexture *Load_Texture(FrameBuf *screen, const char *folder, const char *name)
{
	static const char *extensions[] = {
		"png",
		"bmp",
	};
	char file[256];

	// Use the game display area for determining which art set to use
	for (int i = gResolutionIndex; i < gResolutions.length(); ++i) {
		for (int j = 0; j < SDL_arraysize(extensions); ++j) {
			SDL_snprintf(file, sizeof(file), "%s%s/%s%s.%s",
					folder, gResolutions[i].path_suffix, name, gResolutions[i].file_suffix, extensions[j]);
			SDL_Surface *surface = IMG_Load_RW(OpenRead(file), 1);
			if (surface) {
				SDL_Texture *texture = screen->LoadImage(surface);
				SDL_FreeSurface(surface);
				return new UITexture(texture, gResolutions[i].scale);
			}
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
	SDL_snprintf(name, sizeof(name), "Maelstrom_%s#%d", large ? "icl" : "ics", id);
	return Load_Texture(screen, "Sprites", name);
}
