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

#include <stdio.h>

#include "SDL_FrameBuf.h"
#include "pixel.h"


#define LOWER_PREC(X)	((X)/16)	/* Lower the precision of a value */
#define RAISE_PREC(X)	((X)/16)	/* Raise the precision of a value */

#define MIN(A, B)	((A < B) ? A : B)
#define MAX(A, B)	((A > B) ? A : B)

#define ADJUSTX(X)							\
{									\
	if ( X < 0 ) X = 0;						\
	else								\
	if ( X > screen->w ) X = screen->w;				\
}
#define ADJUSTY(Y)							\
{									\
	if ( Y < 0 ) Y = 0;						\
	else								\
	if ( Y > screen->h ) Y = screen->h;				\
}

/* Constructors cannot fail. :-/ */
FrameBuf:: FrameBuf()
{
	/* Initialize various variables to null state */
	window = NULL;
	renderer = NULL;
	texture = NULL;
	staging = NULL;
	screenfg = NULL;
	screenbg = NULL;
	palette = NULL;
	blitQ = NULL;
	dirtymap = NULL;
	updatelist = NULL;
	errstr = NULL;
	faded = 0;
	images.next = NULL;
	itail = &images;
}

static void PrintSurface(const char *title, SDL_Surface *surface)
{
#ifdef FRAMEBUF_DEBUG
	fprintf(stderr, "%s:\n", title);
	fprintf(stderr, "\t%dx%d at %d bpp\n", surface->w, surface->h,
					surface->format->BitsPerPixel);
	if ( SDL_LockSurface(surface) == 0 ) {
		fprintf(stderr, "\tPixels at %p\n", surface->pixels);
		SDL_UnlockSurface(surface);
	}
#endif
	return;
}

int
FrameBuf:: Init(int width, int height, Uint32 video_flags,
					SDL_Color *colors, SDL_Surface *icon)
{
	window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, video_flags);
	if ( window == NULL )
	{
		SetError("Couldn't create window: %s", SDL_GetError());
		return(-1);
	}

	/* Set the icon, if any */
	if ( icon ) {
		SDL_SetWindowIcon(window, icon);
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
	if ( renderer == NULL ) {
		SetError("Couldn't create renderer: %s", SDL_GetError());
		return(-1);
	}

	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	if ( texture == NULL ) {
		SetError("Couldn't create texture: %s", SDL_GetError());
		return(-1);
	}
	SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);

	SDL_RenderSetLogicalSize(renderer, width, height);

	screenfg = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
	if ( screenfg == NULL ) {
		SetError("Couldn't create foreground: %s", SDL_GetError());
		return(-1);
	}
	FocusFG();
	PrintSurface("Created foreground", screenfg);

	/* Create the background */
	screenbg = SDL_CreateRGBSurface(screen->flags, screen->w, screen->h,
					screen->format->BitsPerPixel,
					screen->format->Rmask,
					screen->format->Gmask,
					screen->format->Bmask, 0);
	if ( screenbg == NULL ) {
		SetError("Couldn't create background: %s", SDL_GetError());
		return(-1);
	}
	PrintSurface("Created background", screenbg);

	/* Create the staging surface */
	staging = SDL_CreateRGBSurfaceWithFormatFrom(NULL, width, height, 32, 0, SDL_PIXELFORMAT_ARGB8888);
	if ( staging == NULL ) {
		SetError("Couldn't create staging surface: %s", SDL_GetError());
		return(-1);
	}
	PrintSurface("Created staging", screenbg);

	/* Create the palette */
	if ( colors ) {
		palette = SDL_AllocPalette(256);
		if ( palette == NULL ) {
			SetError("Couldn't create palette: %s", SDL_GetError());
			return(-1);
		}

		SDL_SetSurfacePalette(screenfg, palette);
		SDL_SetSurfacePalette(screenbg, palette);
	}
	
	/* Create a dirty rectangle map of the screen */
	dirtypitch = LOWER_PREC(width);
	dirtymaplen = LOWER_PREC(height)*dirtypitch;
	dirtymap   = new SDL_Rect *[dirtymaplen];

	/* Create the update list */
	updatelist = new SDL_Rect[UPDATE_CHUNK];
	ClearDirtyList();
	updatemax = UPDATE_CHUNK;

	/* Create the blit list */
	blitQ = new BlitQ[QUEUE_CHUNK];
	blitQlen = 0;
	blitQmax = QUEUE_CHUNK;
	
	/* Set the blit clipping rectangle */
	clip.x = 0;
	clip.y = 0;
	clip.w = screen->w;
	clip.h = screen->h;

	/* Copy the image colormap and set a black background */
	SetBackground(0, 0, 0);
	if ( colors ) {
		SetPalette(colors);
	}

	/* Figure out what putpixel routine to use */
	switch (screen->format->BytesPerPixel) {
		case 1:
			PutPixel = PutPixel1;
			break;
		case 2:
			PutPixel = PutPixel2;
			break;
		case 3:
			PutPixel = PutPixel3;
			break;
		case 4:
			PutPixel = PutPixel4;
			break;
	}
	return(0);
}

FrameBuf:: ~FrameBuf()
{
	image_list *ielem, *iold;

	for ( ielem = images.next; ielem; ) {
		iold = ielem;
		ielem = ielem->next;
		SDL_FreeSurface(iold->image);
		delete iold;
	}
	if ( palette )
		SDL_FreePalette(palette);
	if ( screenfg )
		SDL_FreeSurface(screenfg);
	if ( screenbg )
		SDL_FreeSurface(screenbg);
	if ( staging )
		SDL_FreeSurface(staging);
	if ( blitQ )
		delete[] blitQ;
	if ( dirtymap )
		delete[] dirtymap;
	if ( updatelist )
		delete[] updatelist;
	if ( texture )
		SDL_DestroyTexture(texture);
	if ( renderer )
		SDL_DestroyRenderer(renderer);
	if ( window )
		SDL_DestroyWindow(window);
}

/* Setup routines */
void
FrameBuf:: SetPalette(SDL_Color *colors)
{
	SDL_SetPaletteColors(palette, colors, 0, 256);

	for ( int i = 0; i < 256; ++i ) {
		Uint8 r = palette->colors[i].r;
		Uint8 g = palette->colors[i].g;
		Uint8 b = palette->colors[i].b;
		colormap[i] = SDL_MapRGB(staging->format, r, g, b);
	}

	SetBackground(BGrgb[0], BGrgb[1], BGrgb[2]);
}
void
FrameBuf:: SetBackground(Uint8 R, Uint8 G, Uint8 B)
{
	BGrgb[0] = R;
	BGrgb[1] = G;
	BGrgb[2] = B;
	BGcolor = SDL_MapRGB(screenfg->format, R, G, B);
	FocusBG();
	Clear();
	FocusFG();
}
Uint32
FrameBuf:: MapRGB(Uint8 R, Uint8 G, Uint8 B)
{
	return(SDL_MapRGB(screenfg->format, R, G, B));
}
void
FrameBuf:: ClipBlit(SDL_Rect *cliprect)
{
	clip = *cliprect;
}

/* Locking routines */
void
FrameBuf:: Lock(void)
{
	screen_mem = (Uint8 *)screen->pixels;
}
void
FrameBuf:: Unlock(void)
{
}
void
FrameBuf:: PerformBlits(void)
{
	if ( blitQlen > 0 ) {
		/* Blast and free the queued blits */
		for ( int i=0; i<blitQlen; ++i ) {
			SDL_LowerBlit(blitQ[i].src, &blitQ[i].srcrect,
						screen, &blitQ[i].dstrect);
			SDL_FreeSurface(blitQ[i].src);
		}
		blitQlen = 0;
	}
}
void
FrameBuf:: Update(int auto_update)
{
	int i;

	/* Blit and update the changed rectangles */
	PerformBlits();
	if ( (screen == screenbg) && auto_update ) {
		for ( i=0; i<updatelen; ++i ) {
			SDL_LowerBlit(screenbg, &updatelist[i],
			 		screenfg, &updatelist[i]);
		}
		UpdateScreen();
	} else if ( screen == screenfg ) {
		UpdateScreen();
	}
	ClearDirtyList();
}
void
FrameBuf:: UpdateScreen(void)
{
	if ( SDL_LockTexture(texture, NULL, &staging->pixels, &staging->pitch) == 0 ) {
		int w = staging->w;
		int h = staging->h;
		int row, col;

		for ( row = 0; row < h; ++row ) {
			Uint8 *src = (Uint8 *)screenfg->pixels + row * screenfg->pitch;
			Uint32 *dst = (Uint32 *)((Uint8 *)staging->pixels + row * staging->pitch);
			for ( col = 0; col < w; ++col ) {
				*dst++ = colormap[ *src++ ];
			}
		}
		SDL_UnlockTexture(texture);
	}
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

/* Drawing routines */
void
FrameBuf:: Clear(Sint16 x, Sint16 y, Uint16 w, Uint16 h, clipval do_clip)
{
	/* If we're focused on the foreground, copy from background */
	if ( screen == screenfg ) {
		QueueBlit(x, y, screenbg, x, y, w, h, do_clip);
	} else {
		Uint8 screen_bpp;
		Uint8 *screen_loc;

		screen_bpp = screen->format->BytesPerPixel;
		screen_loc = screen_mem + y*screen->pitch + x*screen_bpp;
		w *= screen_bpp;
		while ( h-- ) {
			/* Note that BGcolor is a 32 bit quantity while memset()
			   fills with an 8-bit quantity.  This only matters if
			   the background is a different color than black on a
			   HiColor or TrueColor display.
			 */
			memset(screen_loc, BGcolor, w);
			screen_loc += screen->pitch;
		}
	}
}
void
FrameBuf:: DrawPoint(Sint16 x, Sint16 y, Uint32 color)
{
	SDL_Rect dirty;

	/* Adjust the bounds */
	if ( x < 0 ) return;
	if ( x > screen->w ) return;
	if ( y < 0 ) return;
	if ( y > screen->h ) return;

	PerformBlits();
	PutPixel(screen_mem+y*screen->pitch+x*screen->format->BytesPerPixel,
								screen, color);
	dirty.x = x;
	dirty.y = y;
	dirty.w = 1;
	dirty.h = 1;
	AddDirtyRect(&dirty);
}
/* Simple, slow, line drawing algorithm.  Improvement, anyone? :-) */
void
FrameBuf:: DrawLine(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 color)
{
	SDL_Rect dirty;
	Sint16   x , y;
	Sint16   lo, hi;
	double slope, b;
	Uint8  screen_bpp;
	Uint8 *screen_loc;

	/* Adjust the bounds */
	ADJUSTX(x1); ADJUSTY(y1);
	ADJUSTX(x2); ADJUSTY(y2);
	
	PerformBlits();
	screen_bpp = screen->format->BytesPerPixel;
	if ( y1 == y2 )  {  /* Horizontal line */
		if ( x1 < x2 ) {
			lo = x1;
			hi = x2;
		} else {
			lo = x2;
			hi = x1;
		}
		screen_loc = screen_mem + y1*screen->pitch + lo*screen_bpp;
		for ( x=lo; x<=hi; ++x ) {
			PutPixel(screen_loc, screen, color);
			screen_loc += screen_bpp;
		}
		dirty.x = lo;
		dirty.y = y1;
		dirty.w = (Uint16)(hi-lo+1);
		dirty.h = 1;
		AddDirtyRect(&dirty);
	} else if ( x1 == x2 ) {  /* Vertical line */
		if ( y1 < y2 ) {
			lo = y1;
			hi = y2;
		} else {
			lo = y2;
			hi = y1;
		}
		screen_loc = screen_mem + lo*screen->pitch + x1*screen_bpp;
		for ( y=lo; y<=hi; ++y ) {
			PutPixel(screen_loc, screen, color);
			screen_loc += screen->pitch;
		}
		dirty.x = x1;
		dirty.y = lo;
		dirty.w = 1;
		dirty.h = (Uint16)(hi-lo+1);
		AddDirtyRect(&dirty);
	} else {
		/* Equation:  y = mx + b */
		slope = ((double)((int)(y2 - y1)) / 
					(double)((int)(x2 - x1)));
		b = (double)(y1 - slope*(double)x1);
		if ( ((slope < 0) ? slope > -1 : slope < 1) ) {
			if ( x1 < x2 ) {
				lo = x1;
				hi = x2;
			} else {
				lo = x2;
				hi = x1;
			}
			for ( x=lo; x<=hi; ++x ) {
				y = (int)((slope*(double)x) + b);
				screen_loc = screen_mem + 
						y*screen->pitch + x*screen_bpp;
				PutPixel(screen_loc, screen, color);
			}
		} else {
			if ( y1 < y2 ) {
				lo = y1;
				hi = y2;
			} else {
				lo = y2;
				hi = y1;
			}
			for ( y=lo; y<=hi; ++y ) {
				x = (int)(((double)y - b)/slope);
				screen_loc = screen_mem + 
						y*screen->pitch + x*screen_bpp;
				PutPixel(screen_loc, screen, color);
			}
		}
		dirty.x = MIN(x1, x2);
		dirty.y = MIN(y1, y2);
		dirty.w = (Uint16)(MAX(x1, x2)-dirty.x+1);
		dirty.h = (Uint16)(MAX(y1, y2)-dirty.y+1);
		AddDirtyRect(&dirty);
	}
}
void
FrameBuf:: DrawRect(Sint16 x, Sint16 y, Uint16 w, Uint16 h, Uint32 color)
{
	SDL_Rect dirty;
	int i;
	Uint8  screen_bpp;
	Uint8 *screen_loc;

	/* Adjust the bounds */
	ADJUSTX(x); ADJUSTY(y);
	if ( (x+w) > screen->w ) w = (Uint16)(screen->w-x);
	if ( (y+h) > screen->h ) h = (Uint16)(screen->h-y);

	PerformBlits();
	screen_bpp = screen->format->BytesPerPixel;

	/* Horizontal lines */
	screen_loc = screen_mem + y*screen->pitch + x*screen_bpp;
	for ( i=0; i<w; ++i ) {
		PutPixel(screen_loc, screen, color);
		screen_loc += screen_bpp;
	}
	screen_loc = screen_mem + (y+h-1)*screen->pitch + x*screen_bpp;
	for ( i=0; i<w; ++i ) {
		PutPixel(screen_loc, screen, color);
		screen_loc += screen_bpp;
	}

	/* Vertical lines */
	screen_loc = screen_mem + y*screen->pitch + x*screen_bpp;
	for ( i=0; i<h; ++i ) {
		PutPixel(screen_loc, screen, color);
		screen_loc += screen->pitch;
	}
	screen_loc = screen_mem + y*screen->pitch + (x+w-1)*screen_bpp;
	for ( i=0; i<h; ++i ) {
		PutPixel(screen_loc, screen, color);
		screen_loc += screen->pitch;
	}

	/* Update rectangle */
	dirty.x = x;
	dirty.y = y;
	dirty.w = w;
	dirty.h = h;
	AddDirtyRect(&dirty);
}
void
FrameBuf:: FillRect(Sint16 x, Sint16 y, Uint16 w, Uint16 h, Uint32 color)
{
	SDL_Rect dirty;
	Uint16 i, skip;
	Uint8 screen_bpp;
	Uint8 *screen_loc;

	/* Adjust the bounds */
	ADJUSTX(x); ADJUSTY(y);
	if ( (x+w) > screen->w ) w = (screen->w-x);
	if ( (y+h) > screen->h ) h = (screen->h-y);

	/* Set the dirty rectangle */
	dirty.x = x;
	dirty.y = y;
	dirty.w = w;
	dirty.h = h;

	/* Semi-efficient, for now. :) */
	screen_bpp = screen->format->BytesPerPixel;
	screen_loc = screen_mem + y*screen->pitch + x*screen_bpp;
	skip = screen->pitch - (w*screen_bpp);
	while ( h-- ) {
		for ( i=w; i!=0; --i ) {
			PutPixel(screen_loc, screen, color);
			screen_loc += screen_bpp;
		}
		screen_loc += skip;
	}
	AddDirtyRect(&dirty);
}

static inline void memswap(Uint8 *dst, Uint8 *src, Uint8 len)
{
#ifdef SWAP_XOR
	/* Swap two buffers using ye ol' xor trick :-) */
	while ( len-- ) {
		*dst ^= *src;
		*src ^= *dst;
		*dst ^= *src;
		++src; ++dst;
	}
#else
	/* Swap two buffers using a temporary variable */
	register Uint8 tmp;

	while ( len-- ) {
		tmp = *dst;
		*dst = *src;
		*src = tmp;
		++src; ++dst;
	}
#endif
}

void
FrameBuf:: Fade(void)
{
	const int max = 32;
	Uint8 ramp[256];
	int i, j;

	if ( palette ) {
		for ( j = 1; j <= max; j++ ) {
			int v = faded ? j : max - j;

			for ( i = 0; i < 256; ++i ) {
				ramp[i] = (Uint8)(i * v / max);
			}
			for ( i = 0; i < 256; ++i ) {
				Uint8 r = ramp[palette->colors[i].r];
				Uint8 g = ramp[palette->colors[i].g];
				Uint8 b = ramp[palette->colors[i].b];
				colormap[i] = SDL_MapRGB(staging->format, r, g, b);
			}
			UpdateScreen();

			// Prevent the "busy" cursor on macOS
			SDL_PumpEvents();
		}
	}

	faded = !faded;
} 

SDL_Surface *
FrameBuf:: GrabArea(Uint16 x, Uint16 y, Uint16 w, Uint16 h)
{
	SDL_Surface *area;

	/* Clip the area we are grabbing */
	if ( x >= screen->w ) {
		return(NULL);
	}
	if ( (x+w) > screen->w ) {
		w -= (screen->w-(x+w));
	}
	if ( y >= screen->h ) {
		return(NULL);
	}
	if ( (y+h) > screen->h ) {
		h -= (screen->h-(y+h));
	}

	/* Allocate an area of the same pixel format */
	area = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h,
				screen->format->BitsPerPixel, 
					screen->format->Rmask,
					screen->format->Gmask,
					screen->format->Bmask, 0);
	if ( area ) {
		Uint8 *area_mem;
		Uint8 *scrn_mem;
		int cursor_shown;

		if ( area->format->palette ) {
			memcpy(area->format->palette->colors,
				screen->format->palette->colors,
				screen->format->palette->ncolors
						*sizeof(SDL_Color));
		}
		cursor_shown = SDL_ShowCursor(0);
		Lock();
		area_mem = (Uint8 *)area->pixels;
		scrn_mem = screen_mem + y*screen->pitch +
					x*screen->format->BytesPerPixel;
		w *= screen->format->BytesPerPixel;
		while ( h-- ) {
			memcpy(area_mem, scrn_mem, w);
			area_mem += area->pitch;
			scrn_mem += screen->pitch;
		}
		Unlock();
		SDL_ShowCursor(cursor_shown);
	}

	/* Add the area to the list of images */
	itail->next = new image_list;
	itail = itail->next;
	itail->image = area;
	itail->next = NULL;
	return(area);
}

int
FrameBuf:: ScreenDump(const char *prefix, Uint16 x, Uint16 y, Uint16 w, Uint16 h)
{
	SDL_Surface *dump;
	int retval;

	/* Get a suitable new filename */
	dump = GrabArea(x, y, w, h);
	if ( dump ) {
		int which, found;
		char file[1024];
		FILE *fp;

		found = 0;
		for ( which=0; !found; ++which ) {
			SDL_snprintf(file, sizeof(file), "%s%d.bmp", prefix, which);
			if ( ((fp=fopen(file, "r")) == NULL) &&
			     ((fp=fopen(file, "w")) != NULL) ) {
				found = 1;
			}
			if ( fp != NULL ) {
				fclose(fp);
			}
		}
		retval = SDL_SaveBMP(dump, file);
		if ( retval < 0 ) {
			SetError("%s", SDL_GetError());
		}
		FreeImage(dump);
	} else {
		retval = -1;
	}
	return(retval);
}

SDL_Surface *
FrameBuf:: LoadImage(Uint16 w, Uint16 h, Uint8 *pixels, Uint8 *mask)
{
	SDL_Surface *artwork;
	int i, j, pad;
	Uint8 *art_mem, *pix_mem;

	/* Assume 8-bit artwork using the current palette */
	artwork = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h,
				screenfg->format->BitsPerPixel, 
					screenfg->format->Rmask,
					screenfg->format->Gmask,
					screenfg->format->Bmask, 0);
	if ( artwork == NULL ) {
		SetError("Couldn't create artwork: %s", SDL_GetError());
		return(NULL);
	}

	/* Set the palette and copy pixels, checking for colorkey */
	if ( artwork->format->palette != NULL ) {
		memcpy(artwork->format->palette->colors,
		       screenfg->format->palette->colors,
		       screenfg->format->palette->ncolors*sizeof(SDL_Color));
	}
	pad  = ((w%4) ? (4-(w%4)) : 0);
	if ( mask ) {
		int used[256];
		Uint8 colorkey, m;

		/* Look for an unused palette entry */
		memset(used, 0, 256);
		pix_mem = pixels;
		for ( i=(w*h); i!=0; --i ) {
			++used[*pix_mem];
			++pix_mem;
		}
		for ( i=0; used[i] && i<256; ++i ) {
			/* Keep looking */;
		}
		colorkey = (Uint8)i;
	
		/* Copy over the pixels */
		pix_mem = pixels;
		for ( i=0; i<h; ++i ) {
			art_mem = (Uint8 *)artwork->pixels + i*artwork->pitch;
			for ( j=0; j<w; ++j ) {
				if ( (j%8) == 0 ) {
					m = *mask++;
				}
				if ( m & 0x80 ) {
					PutPixel(art_mem, screenfg, *pix_mem);
				} else {
					PutPixel(art_mem, screenfg, colorkey);
				}
				m <<= 1;
				art_mem += artwork->format->BytesPerPixel;
				pix_mem += 1;
			}
			pix_mem += pad;
		}
		SDL_SetColorKey(artwork, SDL_RLEACCEL, colorkey);
	} else {
		/* Copy over the pixels */
		pix_mem = pixels;
		for ( i=0; i<h; ++i ) {
			art_mem = (Uint8 *)artwork->pixels + i*artwork->pitch;
			for ( j=0; j<w; ++j ) {
				PutPixel(art_mem, screenfg, *pix_mem);
				art_mem += artwork->format->BytesPerPixel;
				pix_mem += 1;
			}
			pix_mem += pad;
		}
	}
	/* Add the image to the list of images */
	itail->next = new image_list;
	itail = itail->next;
	itail->image = artwork;
	itail->next = NULL;
	return(artwork);
}
void
FrameBuf:: FreeImage(SDL_Surface *image)
{
	image_list *ielem, *iold;

	/* Remove the image from the list of images */
	for ( ielem=&images; ielem->next; ) {
		iold = ielem->next;
		if ( iold->image == image ) {
			if ( iold == itail ) {
				itail = ielem;
			}
			ielem->next = iold->next;
			SDL_FreeSurface(iold->image);
			delete iold;
			return;
		} else {
			ielem = ielem->next;
		}
	}
	fprintf(stderr, "Warning: image to be freed not in list\n");
}

void
FrameBuf:: QueueBlit(int dstx, int dsty, SDL_Surface *src,
			int srcx, int srcy, int w, int h, clipval do_clip)
{
	int diff;

	/* Perform clipping */
	if ( do_clip == DOCLIP ) {
		diff = (int)clip.x - dstx;
		if ( diff > 0 ) {
			w -= diff;
			if ( w <= 0 )
				return;
			srcx += diff;
			dstx = clip.x;
		}
		diff = (int)clip.y - dsty;
		if ( diff > 0 ) {
			h -= diff;
			if ( h <= 0 )
				return;
			srcy += diff;
			dsty = clip.y;
		}
		diff = (int)(dstx+w) - (clip.x+clip.w);
		if ( diff > 0 ) {
			w -= diff;
			if ( w <= 0 )
				return;
		}
		diff = (int)(dsty+h) - (clip.y+clip.h);
		if ( diff > 0 ) {
			h -= diff;
			if ( h <= 0 )
				return;
		}
	}

	/* Lengthen the queue if necessary */
	if ( blitQlen == blitQmax ) {
		BlitQ *newq;

		blitQmax += QUEUE_CHUNK;
		newq = new BlitQ[blitQmax];
		memcpy(newq, blitQ, blitQlen*sizeof(BlitQ));
		delete[] blitQ;
		blitQ = newq;
	}

	/* Add the blit to the queue */
	++src->refcount;
	blitQ[blitQlen].src = src;
	blitQ[blitQlen].srcrect.x = srcx;
	blitQ[blitQlen].srcrect.y = srcy;
	blitQ[blitQlen].srcrect.w = w;
	blitQ[blitQlen].srcrect.h = h;
	blitQ[blitQlen].dstrect.x = dstx;
	blitQ[blitQlen].dstrect.y = dsty;
	blitQ[blitQlen].dstrect.w = w;
	blitQ[blitQlen].dstrect.h = h;
	AddDirtyRect(&blitQ[blitQlen].dstrect);
	++blitQlen;
}

/* Maintenance routines */
/* Add a rectangle to the update list
   This is a little bit smart -- if the center nearly overlaps the center
   of another rectangle update, expand the existing rectangle to include
   the new area, instead adding another update rectangle.
*/
void
FrameBuf:: AddDirtyRect(SDL_Rect *rect)
{
	Uint16  mapoffset;
	SDL_Rect *newrect;

	/* The dirty map offset is the center of the rectangle */
	mapoffset = LOWER_PREC(rect->y+(rect->h/2)) * dirtypitch +
		    LOWER_PREC(rect->x+(rect->w/2));

	if ( dirtymap[mapoffset] == NULL ) {
		/* New dirty rectangle */
		if ( updatelen == updatemax ) {
			/* Expand the updatelist */
			SDL_Rect *newlist;
			int       i;

			updatemax += UPDATE_CHUNK;
			newlist = new SDL_Rect[updatemax+UPDATE_CHUNK];
			memcpy(newlist,updatelist,updatelen*sizeof(SDL_Rect));
			/* Update the dirty rectangle map with the new list */
			for ( i=0; i<dirtymaplen; ++i ) {
				if ( dirtymap[i] != NULL ) {
					dirtymap[i] = (SDL_Rect *)(
					((intptr_t)dirtymap[i]-(intptr_t)updatelist) + (intptr_t)newlist
					);
				}
			}
			delete[] updatelist;
			updatelist = newlist;
		}
		newrect = &updatelist[updatelen];
		++updatelen;
		memcpy(newrect, rect, sizeof(*newrect));
		dirtymap[mapoffset] = newrect;
	} else {
		Sint16 x1, y1, x2, y2;

		/* Overlapping dirty rectangle -- expand it */
		newrect = dirtymap[mapoffset];
		x1 = MIN(rect->x, newrect->x);
		y1 = MIN(rect->y, newrect->y);
		x2 = MAX(rect->x+rect->w, newrect->x+newrect->w);
		y2 = MAX(rect->y+rect->h, newrect->y+newrect->h);
		newrect->x = x1;
		newrect->y = y1;
		newrect->w = (Uint16)(x2 - x1);
		newrect->h = (Uint16)(y2 - y1);
	}
}
