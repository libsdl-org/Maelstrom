/*
  screenlib:  A simple window and UI library based on the SDL library
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

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

#ifndef _SDL_FrameBuf_h
#define _SDL_FrameBuf_h

/* A simple display management class based on SDL:

   It supports line drawing, rectangle filling, and fading,
   and it supports loading 8 bits-per-pixel masked images.
*/

// Define this if you're rapidly iterating on UI screens
//#define FAST_ITERATION

#include <stdio.h>

#include <SDL3/SDL.h>
#include "../utils/ErrorBase.h"

typedef enum {
	DOCLIP,
	NOCLIP
} clipval;

class FrameBuf : public ErrorBase {

public:
	FrameBuf();
	int Init(int width, int height, Uint32 window_flags, const char *title = nullptr, SDL_Surface *icon = nullptr);
	virtual ~FrameBuf();

	/* Setup routines */
	/* Map an RGB value to a color pixel */
	Uint32 MapRGB(Uint8 R, Uint8 G, Uint8 B) {
		return (0xFF000000 | ((Uint32)R << 16) | ((Uint32)G << 8) | B);
	}
	void GetRGB(Uint32 color, Uint8 *R, Uint8 *G, Uint8 *B) {
		*R = (Uint8)((color >> 16) & 0xFF);
		*G = (Uint8)((color >>  8) & 0xFF);
		*B = (Uint8)((color >>  0) & 0xFF);
	}
	/* Set the blit clipping rectangle */
	void ClipBlit(SDL_Rect *cliprect) {
		m_clip.x = (float)cliprect->x;
		m_clip.y = (float)cliprect->y;
		m_clip.w = (float)cliprect->w;
		m_clip.h = (float)cliprect->h;
	}

	/* Event Routines */
	void ProcessEvent(SDL_Event *event);

	bool ConvertTouchCoordinates(const SDL_TouchFingerEvent &finger, int *x, int *y);

	void EnableTextInput();
	void DisableTextInput();

	void ToggleFullScreen(void) {
		if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) {
			SDL_SetWindowFullscreen(m_window, false);
		} else {
			SDL_SetWindowFullscreen(m_window, true);
		}
	}

	/* Information routines */
	SDL_Window *GetWindow() const {
		return m_window;
	}
	int Width() const {
		return m_width;
	}
	int Height() const {
		return m_height;
	}

	/* Blit and update routines */
	void QueueBlit(SDL_Texture *src,
			int srcx, int srcy, int srcw, int srch,
			int dstx, int dsty, int dstw, int dsth, clipval do_clip, float angle = 0.0f);
	void QueueBlit(SDL_Texture *src, int x, int y, int w, int h, clipval do_clip, float angle = 0.0f) {
		QueueBlit(src, 0, 0, src->w, src->h, x, y, w, h, do_clip, angle);
	}
	void QueueBlit(SDL_Texture *src, int x, int y, clipval do_clip, float angle = 0.0f) {
		QueueBlit(src, 0, 0, src->w, src->h, x, y, src->w, src->h, do_clip, angle);
	}
	void StretchBlit(const SDL_Rect *dstrect, SDL_Texture *src, const SDL_Rect *srcrect);

	void Update(void);
	void Update(SDL_Texture *texture);
	void FadeOut(void) {
		if (!m_faded) {
			Fade();
		}
	}
	void FadeIn(void) {
		if (m_faded) {
			Fade();
		}
	}
	void Fade(void);		/* Fade screen out, then in */
	bool Fading(void) {
		return m_fadeTexture ? true : false;
	}
	void FadeStep(void);

	/* Drawing routines */
	void Clear(int x, int y, int w, int h) {
		FillRect(x, y, w, h, 0);
	}
	void Clear(Uint32 color = 0) {
		UpdateDrawColor(color);
		SDL_RenderClear(m_renderer);
	}
	void DrawPoint(int x, int y, Uint32 color) {
		UpdateDrawColor(color);
		SDL_RenderPoint(m_renderer, (float)x, (float)y);
	}
	void DrawLine(int x1, int y1, int x2, int y2, Uint32 color) {
		UpdateDrawColor(color);
		SDL_RenderLine(m_renderer, (float)x1, (float)y1, (float)x2, (float)y2);
	}
	void DrawRect(int x1, int y1, int w, int h, Uint32 color) {
		UpdateDrawColor(color);

		SDL_FRect rect;
		rect.x = (float)x1;
		rect.y = (float)y1;
		rect.w = (float)w;
		rect.h = (float)h;
		SDL_RenderRect(m_renderer, &rect);
	}
	void FillRect(int x1, int y1, int w, int h, Uint32 color) {
		UpdateDrawColor(color);

		SDL_FRect rect;
		rect.x = (float)x1;
		rect.y = (float)y1;
		rect.w = (float)w;
		rect.h = (float)h;
		SDL_RenderFillRect(m_renderer, &rect);
	}

	/* Load a texture image */
	SDL_Texture *LoadImage(const char *file);
	SDL_Texture *LoadImage(SDL_Surface *surface);
	SDL_Texture *LoadImage(int w, int h, Uint32 *pixels);
	int GetImageWidth(SDL_Texture *image) {
		return image->w;
	}
	int GetImageHeight(SDL_Texture *image) {
		return image->h;
	}
	void FreeImage(SDL_Texture *image);

	/* Screen dump routines */
	int ScreenDump(const char *prefix, int x, int y, int w, int h);

	/* Cursor handling routines */
	void ShowCursor(void) {
		SDL_ShowCursor();
	}
	void HideCursor(void) {
		SDL_HideCursor();
	}
	void GetCursorPosition(int *x, int *y);
	void SetCaption(const char *caption, const char *icon = NULL) {
		SDL_SetWindowTitle(m_window, caption);
	}

private:
	/* The current display */
	SDL_Window *m_window = nullptr;
	SDL_Renderer *m_renderer = nullptr;
	SDL_Texture *m_target = nullptr;
	SDL_Texture *m_fadeTexture = nullptr;
	int m_fadeStep = 0;
	bool m_faded = false;
	SDL_FRect m_clip;
	int m_width;
	int m_height;

	void UpdateDrawColor(Uint32 color) {
		Uint8 r, g, b;
		r = (color >> 16) & 0xFF;
		g = (color >>  8) & 0xFF;
		b = (color >>  0) & 0xFF;
		SDL_SetRenderDrawColor(m_renderer, r, g, b, 0xFF);
	}
};

#endif /* _SDL_FrameBuf_h */
