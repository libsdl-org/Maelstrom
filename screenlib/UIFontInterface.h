/*
  screenlib:  A simple window and UI library based on the SDL library
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

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

#ifndef _UIFontInterface_h
#define _UIFontInterface_h

#include "SDL.h"

enum UIFontStyle {
	UIFONT_STYLE_NORMAL,
	UIFONT_STYLE_BOLD,
	UIFONT_STYLE_UNDERLINE,
	UIFONT_STYLE_ITALIC
};

class UIFontInterface
{
public:
	virtual SDL_Texture *CreateText(const char *text, const char *fontName, int fontSize, UIFontStyle fontStyle, Uint32 color) = 0;
	virtual void FreeText(SDL_Texture *texture) = 0;
};

#endif // _UIFontInterface_h
