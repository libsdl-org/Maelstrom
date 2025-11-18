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

#include <stdio.h>

#include "../utils/files.h"
#include "SDL_FrameBuf.h"


#define LOWER_PREC(X)	((X)/16)	/* Lower the precision of a value */
#define RAISE_PREC(X)	((X)/16)	/* Raise the precision of a value */

#define MIN(A, B)	((A < B) ? A : B)
#define MAX(A, B)	((A > B) ? A : B)

/* Constructors cannot fail. :-/ */
FrameBuf::FrameBuf() : ErrorBase()
{
	/* Initialize various variables to null state */
	window = NULL;
	renderer = NULL;
	faded = 0;
	resizable = false;
	logicalScale = 0.0f;
}

int
FrameBuf::Init(int width, int height, Uint32 window_flags, Uint32 render_flags,
		SDL_Surface *icon)
{
	if (window_flags & SDL_WINDOW_RESIZABLE) {
		resizable = true;
	} else {
		resizable = false;
	}

	window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, window_flags);
	if (!window) {
		SetError("Couldn't create %dx%d window: %s", 
					width, height, SDL_GetError());
		return(-1);
	}

	renderer = SDL_CreateRenderer(window, -1, render_flags);
	if (!renderer) {
		SetError("Couldn't create renderer: %s", SDL_GetError());
		return(-1);
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

	/* Set the icon, if any */
	if ( icon ) {
		SDL_SetWindowIcon(window, icon);
	}

	/* Set the output area */
	if (Resizable()) {
		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		UpdateWindowSize(w, h);
	} else {
		SetLogicalSize(width, height);
	}

	return(0);
}

FrameBuf::~FrameBuf()
{
	if (renderer) {
		SDL_DestroyRenderer(renderer);
	}
	if (window) {
		SDL_DestroyWindow(window);
	}
}

void
FrameBuf::ProcessEvent(SDL_Event *event)
{
	switch (event->type) {
	case SDL_WINDOWEVENT:
		if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
			int w, h;
			SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);

			w = Width();
			h = Height();
			if (Resizable()) {
				// We'll accept this window size change
				SDL_GetWindowSize(window, &w, &h);
				SDL_RenderSetViewport(renderer, NULL);
			}
			if (logicalScale > 0.0f) {
				w = (int)(w/logicalScale);
				h = (int)(h/logicalScale);
				SetLogicalSize(w, h);
			} else {
				UpdateWindowSize(w, h);
			}
		}
		break;
	}
}

// This routine or something like it should probably go in SDL
bool
FrameBuf::ConvertTouchCoordinates(const SDL_TouchFingerEvent &finger, int *x, int *y)
{
	int window_w, window_h;
	float scale_x, scale_y;

	SDL_GetWindowSize(window, &window_w, &window_h);
	SDL_RenderGetScale(renderer, &scale_x, &scale_y);
	*x = (int)(finger.x*window_w/scale_x) - output.x;
	*y = (int)(finger.y*window_h/scale_y) - output.y;
	*x = (*x * rect.w) / output.w;
	*y = (*y * rect.h) / output.h;
	return true;
}

#ifdef __IPHONEOS__
extern "C" {
	extern int SDL_iPhoneKeyboardHide(SDL_Window * window);
	extern int SDL_iPhoneKeyboardShow(SDL_Window * window);
}
#endif

void
FrameBuf::GetCursorPosition(int *x, int *y)
{
	float scale_x, scale_y;

	SDL_GetMouseState(x, y);
	SDL_RenderGetScale(renderer, &scale_x, &scale_y);

	*x = (int)(*x/scale_x) - output.x;
	*y = (int)(*y/scale_y) - output.y;
	*x = (*x * rect.w) / output.w;
	*y = (*y * rect.h) / output.h;
}

void
FrameBuf::EnableTextInput()
{
	SDL_StartTextInput();
}

void
FrameBuf::DisableTextInput()
{
	SDL_StopTextInput();
}

void
FrameBuf::GetDesktopSize(int &w, int &h) const
{
	SDL_DisplayMode mode;

	if (SDL_GetDesktopDisplayMode(0, &mode) < 0) {
		w = 0;
		h = 0;
	} else {
		w = mode.w;
		h = mode.h;
	}
}

void
FrameBuf::GetDisplaySize(int &w, int &h) const
{
	SDL_GetWindowSize(window, &w, &h);
}

void
FrameBuf::GetLogicalSize(int &w, int &h) const
{
	SDL_RenderGetLogicalSize(renderer, &w, &h);
	if (!w || !h) {
		w = Width();
		h = Height();
	}
}

void
FrameBuf::SetLogicalSize(int w, int h)
{
	SDL_RenderSetLogicalSize(renderer, w, h);
	UpdateWindowSize(w, h);
}

void
FrameBuf::SetLogicalScale(float scale)
{
	int w, h;
	SDL_Texture *target;

	target = SDL_GetRenderTarget(renderer);
	if (target) {
		// This is a temporary scale change
		SDL_QueryTexture(target, NULL, NULL, &w, &h);
		if (scale > 0.0f) {
			w = (int)(w/scale);
			h = (int)(h/scale);
		}
		SDL_RenderSetLogicalSize(renderer, w, h);
	} else {
		logicalScale = scale;

		if (Resizable()) {
			SDL_GetWindowSize(window, &w, &h);
		} else {
			w = Width();
			h = Height();
		}
		if (logicalScale > 0.0f) {
			w = (int)(w/logicalScale);
			h = (int)(h/logicalScale);
			SetLogicalSize(w, h);
		} else {
			UpdateWindowSize(w, h);
		}
	}
}

void
FrameBuf::QueueBlit(SDL_Texture *src,
			int srcx, int srcy, int srcw, int srch,
			int dstx, int dsty, int dstw, int dsth, clipval do_clip,
			float angle)
{
	SDL_Rect srcrect;
	SDL_Rect dstrect;

	srcrect.x = srcx;
	srcrect.y = srcy;
	srcrect.w = srcw;
	srcrect.h = srch;
	dstrect.x = dstx;
	dstrect.y = dsty;
	dstrect.w = dstw;
	dstrect.h = dsth;
	if (do_clip == DOCLIP) {
		float scaleX = (float)srcrect.w / dstrect.w;
		float scaleY = (float)srcrect.h / dstrect.h;

		if (!SDL_IntersectRect(&clip, &dstrect, &dstrect)) {
			return;
		}

		/* Adjust the source rectangle to match */
		srcrect.x += (int)((dstrect.x - dstx) * scaleX);
		srcrect.y += (int)((dstrect.y - dsty) * scaleY);
		srcrect.w = (int)(dstrect.w * scaleX);
		srcrect.h = (int)(dstrect.h * scaleY);
	}
	if (angle) {
		SDL_RenderCopyEx(renderer, src, &srcrect, &dstrect, angle, NULL, SDL_FLIP_NONE);
	} else {
		SDL_RenderCopy(renderer, src, &srcrect, &dstrect);
	}
}

void
FrameBuf::StretchBlit(const SDL_Rect *dstrect, SDL_Texture *src, const SDL_Rect *srcrect)
{
	SDL_RenderCopy(renderer, src, srcrect, dstrect);
}

void
FrameBuf::Update(void)
{
	SDL_RenderPresent(renderer);
}

void
FrameBuf::Fade(void)
{
#ifdef FAST_ITERATION
	return;
#else
	const int max = 32;
	Uint16 ramp[256];   

	for ( int j = 1; j <= max; j++ ) {
		int v = faded ? j : max - j + 1;
		for ( int i = 0; i < 256; i++ ) {
			ramp[i] = (i * v / max) << 8;
		}
		SDL_SetWindowGammaRamp(window, ramp, ramp, ramp);
		SDL_Delay(10);
	}
	faded = !faded;

	if ( faded ) {
		for ( int i = 0; i < 256; i++ ) {
			ramp[i] = 0;
		}
		SDL_SetWindowGammaRamp(window, ramp, ramp, ramp);
	}
#endif
} 

int
FrameBuf::ScreenDump(const char *prefix, int x, int y, int w, int h)
{
	float scale_x, scale_y;
	SDL_Rect rect;
	SDL_Surface *dump;
	int which, found;
	char file[1024];
	int retval;

	if (!w) {
		w = Width();
	}
	if (!h) {
		h = Height();
	}

	// Convert to real output coordinates
	SDL_RenderGetScale(renderer, &scale_x, &scale_y);

	x = (x * output.w) / this->rect.w;
	y = (y * output.h) / this->rect.h;
	x = (int)((x + output.x) * scale_x);
	y = (int)((y + output.y) * scale_y);

	w = (w * output.w) / this->rect.w;
	h = (h * output.h) / this->rect.h;
	w = (int)(w * scale_x);
	h = (int)(h * scale_y);

	/* Create a BMP format surface */
	dump = SDL_CreateRGBSurface(0, w, h, 24, 
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                   0x00FF0000, 0x0000FF00, 0x000000FF, 0);
#else
                   0x000000FF, 0x0000FF00, 0x00FF0000, 0);
#endif
	if (!dump) {
		SetError("%s", SDL_GetError());
		return -1;
	}

	/* Read the screen into it */
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	if (SDL_RenderReadPixels(renderer, &rect, SDL_PIXELFORMAT_BGR24, dump->pixels, dump->pitch) < 0) {
		SetError("%s", SDL_GetError());
		return -1;
	}

	/* Get a suitable new filename */
	found = 0;
	for ( which=0; !found; ++which ) {
		SDL_RWops *fp;
		SDL_snprintf(file, sizeof(file), "%s%d.bmp", prefix, which);
		fp = OpenRead(file);
		if (fp) {
			SDL_RWclose(fp);
		} else {
			found = 1;
		}
	}
	retval = SDL_SaveBMP_RW(dump, OpenWrite(file), 1);
	if ( retval < 0 ) {
		SetError("%s", SDL_GetError());
	}
	SDL_FreeSurface(dump);

	return(retval);
}

SDL_Texture *
FrameBuf::LoadImage(const char *file)
{
	SDL_Surface *surface;
	SDL_Texture *texture;
	
	texture = NULL;
	surface = SDL_LoadBMP_RW(OpenRead(file), 1);
	if (surface) {
		texture = LoadImage(surface);
		SDL_FreeSurface(surface);
	}
	return texture;
}

SDL_Texture *
FrameBuf::LoadImage(SDL_Surface *surface)
{
	return SDL_CreateTextureFromSurface(renderer, surface);
}

SDL_Texture *
FrameBuf::LoadImage(int w, int h, Uint32 *pixels)
{
	SDL_Texture *texture;

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
	if (!texture) {
		SetError("%s", SDL_GetError());
		return NULL;
	}

	if (SDL_UpdateTexture(texture, NULL, pixels, w*sizeof(Uint32)) < 0) {
		SetError("%s", SDL_GetError());
		SDL_DestroyTexture(texture);
		return NULL;
	}
	return(texture);
}

void
FrameBuf::FreeImage(SDL_Texture *image)
{
	SDL_DestroyTexture(image);
}

SDL_Texture *
FrameBuf::CreateRenderTarget(int w, int h)
{
	SDL_Texture *texture;

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, w, h);
	if (!texture) {
		SetError("Couldn't create target texture: %s", SDL_GetError());
		return NULL;
	}
	return texture;
}

int
FrameBuf::SetRenderTarget(SDL_Texture *texture)
{
	if (SDL_SetRenderTarget(renderer, texture) < 0) {
		SetError("Couldn't set render target: %s", SDL_GetError());
		return(-1);
	}
	return 0;
}

void
FrameBuf::FreeRenderTarget(SDL_Texture *texture)
{
	SDL_DestroyTexture(texture);
}
