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

#include "SDL.h"

#include "SDL_FrameBuf.h"
#include "UITexture.h"


UITexture::UITexture(SDL_Texture *texture, float scale)
{
	m_texture = texture;
	m_scale = scale;
	m_angle = 0.0f;
	SDL_QueryTexture(texture, NULL, NULL, &m_textureWidth, &m_textureHeight);
	m_stretch = false;
	m_locked = false;
}

void
UITexture::CalculateStretchAreas(int cornerSize, int x, int y, int w, int h,
				      SDL_Rect areas[NUM_STRETCH_AREAS])
{
	SDL_Rect rect;

	rect.x = x;
	rect.y = y;
	rect.w = cornerSize;
	rect.h = cornerSize;
	areas[STRETCH_UPPER_LEFT] = rect;

	rect.x = x + (w - cornerSize);
	areas[STRETCH_UPPER_RIGHT] = rect;

	rect.y = y + (h - cornerSize);
	areas[STRETCH_LOWER_RIGHT] = rect;

	rect.x = x;
	areas[STRETCH_LOWER_LEFT] = rect;

	rect.x = x;
	rect.y = y + cornerSize;
	rect.w = cornerSize;
	rect.h = (h - (2*cornerSize));
	areas[STRETCH_LEFT] = rect;

	rect.x = x + (w - cornerSize);
	areas[STRETCH_RIGHT] = rect;

	rect.x = x + cornerSize;
	rect.y = y;
	rect.w = (w - (2*cornerSize));
	rect.h = cornerSize;
	areas[STRETCH_TOP] = rect;

	rect.y = y + (h - cornerSize);
	areas[STRETCH_BOTTOM] = rect;

	rect.x = x + cornerSize;
	rect.y = y + cornerSize;
	rect.w = (w - (2*cornerSize));
	rect.h = (h - (2*cornerSize));
	areas[STRETCH_CENTER] = rect;
}

void
UITexture::SetStretchGrid(int cornerSize)
{
	m_stretch = true;
	m_stretchCornerSize = (int)(cornerSize * m_scale);

	// Slide the texture into a grid based on the center
	//
	// For example, if we have a 9x9 texture with a corner size of 1
	// we would have a set of stretch areas like this:
	// 	STRETCH_CENTER		1,1 1x1
	//	STRETCH_LEFT		0,1 1x1
	//	STRETCH_RIGHT		2,1 1x1
	//	STRETCH_TOP		1,0 1x1
	//	STRETCH_BOTTOM		1,2 1x1
	//	STRETCH_UPPER_LEFT	0,0 1x1
	//	STRETCH_UPPER_RIGHT	2,0 1x1
	//	STRETCH_LOWER_LEFT	0,2 1x1
	//	STRETCH_LOWER_RIGHT	2,2 1x1
	CalculateStretchAreas(m_stretchCornerSize, 0, 0, m_textureWidth, m_textureHeight, m_stretchAreas);
}

void
UITexture::Draw(FrameBuf *screen, int x, int y, int w, int h)
{
	if (m_stretch) {
		SDL_Rect dstAreas[NUM_STRETCH_AREAS];

		CalculateStretchAreas((int)(m_stretchCornerSize / m_scale), x, y, w, h, dstAreas);
		// Draw the grid
		for (int i = 0; i < NUM_STRETCH_AREAS; ++i) {
			const SDL_Rect *src, *dst;
			src = &m_stretchAreas[i];
			dst = &dstAreas[i];
			screen->QueueBlit(m_texture,
					src->x, src->y, src->w, src->h,
					dst->x, dst->y, dst->w, dst->h, NOCLIP);
		}
	} else {
		screen->QueueBlit(m_texture, x, y, w, h, NOCLIP, m_angle);
	}
}
