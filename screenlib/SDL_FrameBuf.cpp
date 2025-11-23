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
}

int
FrameBuf::Init(int width, int height, Uint32 window_flags, SDL_Surface *icon)
{
	/* Create the window */
	window = SDL_CreateWindow(NULL, width, height, window_flags);
	if (!window) {
		SetError("Couldn't create %dx%d window: %s", 
					width, height, SDL_GetError());
		return(-1);
	}

	/* Set the icon, if any */
	if ( icon ) {
		SDL_SetWindowIcon(window, icon);
	}

	/* Create the renderer */
	renderer = SDL_CreateRenderer(window, NULL);
	if (!renderer) {
		SetError("Couldn't create renderer: %s", SDL_GetError());
		return(-1);
	}

	/* Set the output area */
	SDL_SetRenderLogicalPresentation(renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	UpdateWindowSize(width, height);

	/* Create the render target */
	target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_UNKNOWN, SDL_TEXTUREACCESS_TARGET, width, height);
	if (!target) {
		SetError("Couldn't create target: %s", SDL_GetError());
		return(-1);
	}
	//SDL_SetTextureScaleMode(target, SDL_SCALEMODE_PIXELART);

	SDL_SetRenderTarget(renderer, target);

	return(0);
}

FrameBuf::~FrameBuf()
{
	if (target) {
		SDL_DestroyTexture(target);
	}
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
	SDL_ConvertEventToRenderCoordinates(renderer, event);
}

bool
FrameBuf::ConvertTouchCoordinates(const SDL_TouchFingerEvent &finger, int *x, int *y)
{
	int w, h;

	SDL_GetRenderOutputSize(renderer, &w, &h);
	*x = (int)(finger.x * w);
	*y = (int)(finger.y * h);
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
	float mouse_x, mouse_y;

	SDL_GetMouseState(&mouse_x, &mouse_y);
	SDL_RenderCoordinatesFromWindow(renderer, mouse_x, mouse_y, &mouse_x, &mouse_y);

	*x = (int)mouse_x;
	*y = (int)mouse_y;
}

void
FrameBuf::EnableTextInput()
{
	SDL_StartTextInput(window);
}

void
FrameBuf::DisableTextInput()
{
	SDL_StopTextInput(window);
}

void
FrameBuf::QueueBlit(SDL_Texture *src,
			int srcx, int srcy, int srcw, int srch,
			int dstx, int dsty, int dstw, int dsth, clipval do_clip,
			float angle)
{
	SDL_FRect srcrect;
	SDL_FRect dstrect;

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

		if (!SDL_GetRectIntersectionFloat(&clip, &dstrect, &dstrect)) {
			return;
		}

		/* Adjust the source rectangle to match */
		srcrect.x += (int)((dstrect.x - dstx) * scaleX);
		srcrect.y += (int)((dstrect.y - dsty) * scaleY);
		srcrect.w = (int)(dstrect.w * scaleX);
		srcrect.h = (int)(dstrect.h * scaleY);
	}
	if (angle) {
		SDL_RenderTextureRotated(renderer, src, &srcrect, &dstrect, angle, NULL, SDL_FLIP_NONE);
	} else {
		SDL_RenderTexture(renderer, src, &srcrect, &dstrect);
	}
}

void
FrameBuf::StretchBlit(const SDL_Rect *_dstrect, SDL_Texture *src, const SDL_Rect *_srcrect)
{
	SDL_FRect *srcrect = NULL, cvtsrcrect;
	SDL_FRect *dstrect = NULL, cvtdstrect;

	if (_srcrect) {
		SDL_RectToFRect(_srcrect, &cvtsrcrect);
		srcrect = &cvtsrcrect;
	}

	if (_dstrect) {
		SDL_RectToFRect(_dstrect, &cvtdstrect);
		dstrect = &cvtdstrect;
	}

	SDL_RenderTexture(renderer, src, srcrect, dstrect);
}

void
FrameBuf::Update(void)
{
	if (target) {
		/* Make sure resize events are seen before drawing to the screen */
		SDL_PumpEvents();

		SDL_SetRenderTarget(renderer, NULL);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);

		SDL_RenderTexture(renderer, target, NULL, NULL);
		SDL_RenderPresent(renderer);

		SDL_SetRenderTarget(renderer, target);
	} else {
		SDL_RenderPresent(renderer);
	}
}

void
FrameBuf::Fade(void)
{
	const int max = 32;
	Uint8 value;

	for ( int i = 1; i <= max; ++i ) {
		int v = faded ? i : max - i;
		value = (Uint8)(255 * v / max);
		SDL_SetTextureColorMod(target, value, value, value);
		Update();
		SDL_Delay(10);
	}
	faded = !faded;
} 

int
FrameBuf::ScreenDump(const char *prefix, int x, int y, int w, int h)
{
	SDL_Rect rect;
	SDL_Surface *dump;
	char file[1024];
	int retval = -1;

	if (!w) {
		w = Width();
	}
	if (!h) {
		h = Height();
	}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	dump = SDL_RenderReadPixels(renderer, &rect);
	if (!dump) {
		SetError("%s", SDL_GetError());
		return -1;
	}

	/* Get a suitable new filename */
	SDL_Storage *storage = OpenUserStorage();
	if (storage) {
		bool available = false;
		for ( int which = 0; !available; ++which ) {
			SDL_snprintf(file, sizeof(file), "%s%d.png", prefix, which);
			if (!SDL_GetStorageFileSize(storage, file, NULL)) {
				available = true;
			}
		}
		SDL_CloseStorage(storage);
	}

	SDL_IOStream *fp = SDL_IOFromDynamicMem();
	if (SDL_SavePNG_IO(dump, fp, false)) {
		if (SaveUserFile(file, fp)) {
			retval = 0;
		}
	} else {
		/* Couldn't save the screenshot */
		SDL_CloseIO(fp);
	}
	if (retval < 0) {
		SetError("%s", SDL_GetError());
	}
	SDL_DestroySurface(dump);

	return(retval);
}

SDL_Texture *
FrameBuf::LoadImage(const char *file)
{
	SDL_Surface *surface;
	SDL_Texture *texture;
	
	texture = NULL;
	surface = SDL_LoadBMP_IO(OpenRead(file), 1);
	if (surface) {
		texture = LoadImage(surface);
		SDL_DestroySurface(surface);
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

	if (!SDL_UpdateTexture(texture, NULL, pixels, w*sizeof(Uint32))) {
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

