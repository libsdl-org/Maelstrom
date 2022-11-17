/*
    SCREENLIB:  A framebuffer library based on the SDL library
    Copyright (C) 1997-2021 Sam Lantinga <slouken@libsdl.org>

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
*/

#ifndef _SDL_FrameBuf_h
#define _SDL_FrameBuf_h

/* A simple display management class based on SDL:

   It supports medium-slow line drawing, rectangle filling, and fading,
   and it supports loading 8 bits-per-pixel masked images.
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "SDL.h"

typedef enum {
	DOCLIP,
	NOCLIP
} clipval;

class FrameBuf {

public:
	FrameBuf();
	int Init(int width, int height, Uint32 video_flags,
			SDL_Color *colors = NULL, SDL_Surface *icon = NULL);
	~FrameBuf();

	/* Setup routines */
	/* Set the image palette -- 256 entries */
	void SetPalette(SDL_Color *colors);
	/* Set the background color -- used by Clear() */
	void   SetBackground(Uint8 r, Uint8 g, Uint8 b);
	/* Map an RGB value to a color pixel */
	Uint32 MapRGB(Uint8 R, Uint8 G, Uint8 B);
	/* Set the blit clipping rectangle */
	void   ClipBlit(SDL_Rect *cliprect);

	/* Event Routines */
	int PollEvent(SDL_Event *event) {
		return(SDL_PollEvent(event));
	}
	int WaitEvent(SDL_Event *event) {
		return(SDL_WaitEvent(event));
	}
	void ToggleFullScreen(void) {
		if (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) {
			SDL_SetWindowFullscreen(window, 0);
		} else {
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
	}

	/* Locking blitting and update routines */
	void Lock(void);
	void Unlock(void);
	void QueueBlit(int dstx, int dsty, SDL_Surface *src,
			int srcx, int srcy, int w, int h, clipval do_clip);
	void QueueBlit(int x, int y, SDL_Surface *src, clipval do_clip) {
		QueueBlit(x, y, src, 0, 0, src->w, src->h, do_clip);
	}
	void QueueBlit(int x, int y, SDL_Surface *src) {
		QueueBlit(x, y, src, DOCLIP);
	}
	void PerformBlits(void);
	void Update(int auto_update = 0);
	void UpdateScreen(void);
	void Fade(void);		/* Fade screen out, then in */

	/* Informational routines */
	Uint16 Width(void) {
		return(screen->w);
	}
	Uint16 Height(void) {
		return(screen->h);
	}
	SDL_PixelFormat *Format(void) {
		return(screenfg->format);
	}

	/* Set the drawing focus (foreground or background) */
	void FocusFG(void) {
		screen = screenfg;
		screen_mem = (Uint8 *)screen->pixels;
	}
	void FocusBG(void) {
		screen = screenbg;
		screen_mem = (Uint8 *)screen->pixels;
	}

	/* Drawing routines */
	/* These drawing routines must be surrounded by Lock()/Unlock() calls */
	void Clear(Sint16 x, Sint16 y, Uint16 w, Uint16 h,
						clipval do_clip = NOCLIP);
	void Clear(void) {
		Clear(0, 0, screen->w, screen->h);
	}
	void DrawPoint(Sint16 x, Sint16 y, Uint32 color);
	void DrawLine(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color);
	void DrawRect(Sint16 x1, Sint16 y1, Uint16 w, Uint16 h, Uint32 color);
	void FillRect(Sint16 x1, Sint16 y1, Uint16 w, Uint16 h, Uint32 color);

	/* Load and convert an 8-bit image with the given mask */
	SDL_Surface *LoadImage(Uint16 w, Uint16 h, Uint8 *pixels,
							Uint8 *mask = NULL);
	void FreeImage(SDL_Surface *image);

	/* Area copy/dump routines */
	SDL_Surface *GrabArea(Uint16 x, Uint16 y, Uint16 w, Uint16 h);
	int ScreenDump(const char *prefix, Uint16 x, Uint16 y, Uint16 w, Uint16 h);

	/* Cursor handling routines */
	void ShowCursor(void) {
		SDL_ShowCursor(1);
	}
	void HideCursor(void) {
		SDL_ShowCursor(0);
	}
	void SetCaption(const char *caption) {
		SDL_SetWindowTitle(window, caption);
	}

	/* Error message routine */
	char *Error(void) {
		return(errstr);
	}

private:
	/* The current display and background */
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Surface *staging;
	SDL_Surface *screen;
	SDL_Surface *screenfg;
	SDL_Surface *screenbg;
	SDL_Palette *palette;
	Uint8 *screen_mem;
	Uint32 colormap[256];
	int faded;

	/* Error message */
	void SetError(const char *fmt, ...) {
		va_list ap;

		va_start(ap, fmt);
		SDL_vsnprintf(errbuf, sizeof(errbuf), fmt, ap);
		va_end(ap);
		errstr = errbuf;
	}
	char *errstr;
	char  errbuf[1024];

	/* Blit queue list */
#define QUEUE_CHUNK	16
	typedef struct {
		SDL_Surface *src;
		SDL_Rect srcrect;
		SDL_Rect dstrect;
	} BlitQ;
	BlitQ *blitQ;
	int blitQlen;
	int blitQmax;

	/* Rectangle update list */
#define UPDATE_CHUNK	QUEUE_CHUNK*2
	void AddDirtyRect(SDL_Rect *rect);
	int updatelen;
	int updatemax;
	SDL_Rect *updatelist;
	Uint16 dirtypitch;
	SDL_Rect **dirtymap;
	Uint16 dirtymaplen;
	void ClearDirtyList(void) {
		updatelen = 0;
		memset(dirtymap, 0, dirtymaplen*sizeof(SDL_Rect *));
	}

	/* Background color */
	Uint8  BGrgb[3];
	Uint32 BGcolor;

	/* Blit clipping rectangle */
	SDL_Rect clip;

	/* List of loaded images */
	typedef struct image_list {
		SDL_Surface *image;
		struct image_list *next;
	} image_list;
	image_list images, *itail;
	
	/* Function to write to the display surface */
	void (*PutPixel)(Uint8 *screen_loc, SDL_Surface *screen, Uint32 pixel);
};

#endif /* _SDL_FrameBuf_h */
