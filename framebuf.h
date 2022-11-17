
/* Graphics routines for Maelstrom!  (By Sam Lantinga) */

#ifndef _framebuf_h
#define _framebuf_h

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "mydebug.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define ODD_STRUCTURES
#include "list.tmpl"

#undef  ODD_STRUCTURES

/* From shared.cc */
extern void   select_usleep(unsigned long usec);

#include "Sprite.h"		/* For the Rect structure */

#define NUM_COLORS	256	// Pseudo-color display. :)
typedef struct {
	unsigned short red;
	unsigned short green;
	unsigned short blue;
	} Color;


typedef struct {
	unsigned short width;
	unsigned short height;
	unsigned int   numops;
	unsigned char *ops;
	unsigned int   numpixels;
	unsigned char *pixels;
	} CSprite;


#define FADE_NONE	0
#define FADE_FAKE	1
#define FADE_REAL	2

#define XFADE_OUT	0
#define XFADE_IN	1

/* Exists to fix compiler warnings -- can be safely removed */
static inline void Unused(...) { }

/* This class allocates a 640x480 frame-buffer and provides high-level
   routines to blit images into it.  Well, it also provides access to
   other drawing routines.
*/
class FrameBuf {

public:

	// Not a complete class -- only intended as a base class.
	FrameBuf(unsigned short width, unsigned short height, int bpp) {
		VWIDTH = pitch = WIDTH = width;
		HEIGHT = height;
		switch (bpp) {
			case 1:
				image_bpp = bpp;
				break;
			case 2:
			case 3:
			default:
				error(
			"FrameBuf doesn't support %d bytes-per-pixel!", bpp);
				exit(255);
		}
		SetBPP(1);		/* Assume 256 color video for now */
		DoFade = 1;
		faded  = 0;
		ourart = new List<struct FrameBuf::artwork>;
	}
	virtual ~FrameBuf() {
		delete ourart;
	}

	virtual int ScreenDump(char *prefix, Rect *frame);
	virtual int Alloc_Cmap(Color Cmap[]) {
		Unused(Cmap);
		error("Unimplemented function (FrameBuf: Alloc_Cmap)\n");
		return(0);
	}
	virtual int Alloc_Private_Cmap(Color Cmap[]) {
		Unused(Cmap);
		error(
		"Unimplemented function (FrameBuf: Alloc_Private_Cmap)\n");
		return(0);
	}
	/* Once ReColor() is called on data, the original data is copied
	   into a private structure, the original data should be
	   deleted and the pointer pointed at by dest can be used.
	   The dest argument shouldn't point to data, otherwise you won't
	   be able to free data after the call to ReColor().
	   The return value of ReColor() should really only be used for 
	   verification that everything went okay in the ReColor call.
	   If the dest pointer is NULL, the re-colored data is still
	   returned, but the artwork is not registered with the FrameBuf
	   class. - Do this with extreme caution on Win95.
	   FreeArt() MUST be called on the new data pointer pointed to by
	   dest before it goes out of scope, otherwise when FrameBuf tries
	   to ReColor it on display depth change, the program may crash.

	   e.g.
		{
			unsigned char *pdata, *pixels;
			pdata = Load_Graphic(whatever);
			if ( ! FrameBuf::ReColor(pdata, &pixels, graphic_len) )
				error("Something went wrong!\n");
			delete[] pdata;
			... use pixels as the image ...
			FreeArt(pixels);
		}
	*/
	virtual unsigned char *ReColor(unsigned char *data,
						unsigned char **dest, int len);
	virtual int FreeArt(unsigned char *pixelptr);
	virtual unsigned long Map_Color(unsigned short red, 
				unsigned short green, unsigned short blue);
	virtual void Clear(int DoRefresh = 1);
	virtual void DrawBGPoint(unsigned int x, unsigned int y,
						unsigned long color);
	virtual void DrawPoint(unsigned int x, unsigned int y,
						unsigned long color);
	virtual void DrawLine(unsigned int X1, unsigned int Y1,
			unsigned int X2, unsigned int Y2, unsigned long color);
	virtual void DrawRectangle(int X, int Y, int width, int height, 
						unsigned long color);
	virtual void FillRectangle(int X, int Y, int width, int height, 
						unsigned long color);
	virtual unsigned char * Grab_Area(int x, int y, int width, int height,
							unsigned char **dest);
	virtual void Free_Area(unsigned char *areaptr);
	virtual void Set_Area(int x, int y, int width, int height,
							unsigned char *area);
	virtual  void Set_BlitClip(int left, int top, int right, int bottom);
	virtual void Blit_BitMap(int x, int y, int width, int height,
				unsigned char *bdata, unsigned long color);
	virtual void UnBlit_BitMap(int x, int y, int width, int height,
							unsigned char *bdata);
	virtual void Blit_Title(int x, int y, int width, int height,
							unsigned char *data);
	virtual void Blit_Icon(int x, int y, int width, int height,
				unsigned char *sdata, unsigned char *mdata);
	virtual void Blit_Sprite(int x, int y, int width, int height,
				unsigned char *sdata, unsigned char *mdata);
	virtual void UnBlit_Sprite(int x, int y, int width, int height,
						unsigned char *mdata);
	virtual void ClipBlit_Sprite(int x, int y, int width, int height,
				unsigned char *sdata, unsigned char *mdata);
	virtual void UnClipBlit_Sprite(int x, int y, int width, int height,
							unsigned char *mdata);
	virtual CSprite * Compile_Sprite(int width, int height, 
				unsigned char *sdata, unsigned char *mdata);
	virtual void Blit_CSprite(int x, int y, CSprite *sprite);
	virtual void UnBlit_CSprite(int x, int y, CSprite *sprite);
	virtual void Free_CSprite(CSprite *sprite);
	virtual void Hide_Cursor(void) {
		error("Unimplemented function (FrameBuf: Hide_Cursor)\n");
	}
	virtual void Show_Cursor(void) {
		error("Unimplemented function (FrameBuf: Show_Cursor)\n");
	}
	virtual void SetFade(int dofade) {
		DoFade = dofade;
	}
	virtual void Pixel_Fade(int state);
	virtual void Fade(int steps) {
		Unused(steps);
		error("Unimplemented function (FrameBuf: Fade)\n");
	}
	virtual void RefreshArea(int x0, int y0, int width, int height) {
		Unused(x0); Unused(y0); Unused(width); Unused(height);
		error("Unimplemented function (FrameBuf: RefreshArea)\n");
	}
	virtual void Refresh(void) {
		error("Unimplemented function (FrameBuf: Refresh)\n");
	}
	virtual void FlushEvents(void) {
		error("Unimplemented function (FrameBuf: FlushEvents)\n");
	}
	virtual int NumEvents(void) {
		error("Unimplemented function (FrameBuf: FlushEvents)\n");
		return(0);
	}
	virtual void GetEvent(XEvent *event) {
		Unused(event);
		error("Unimplemented function (FrameBuf: GetEvent)\n");
	}
	virtual int KeyToAscii(XEvent *event, char *buf, int buflen,
								KeySym *key) {
		Unused(event); Unused(buf); Unused(buflen); Unused(key);
		error("Unimplemented function (FrameBuf: KeyToAscii)\n");
		return(0);
	}
	virtual void Flush(int sync) {
		Unused(sync);
		error("Unimplemented function (FrameBuf: Flush)\n");
	}
	virtual char *DisplayType(void) {
		return("Unfinished FrameBuf class");
	}
	virtual int DisplayBPP(void) {
		return(video_bpp);
	}
// DEBUG
	virtual void DebugArt(void) {
		ReColorArt();
	}

protected:
	/* Recompile a CSprite, taking clipping into account */
	virtual CSprite * ClipSprite(int x, int y, CSprite *sprite);

	/* Lock and unlock video memory -- unnecessary here */
	virtual int  LockVideo(void) { return(1); }
	virtual void UnlockVideo(void) { }
	virtual int  VideoLocked(void) { return(shared_mem != NULL); }

	/* Some notes:
		shared_mem is the real frame buffer:
			logical height = HEIGHT
			logical width = WIDTH
			real width = pitch
		backbuf is an offscreen back buffer:
			logical height = HEIGHT
			logical width = WIDTH
			real width = VWIDTH
		shared_len is the length of backbuf: HEIGHT*VWIDTH
		video_bpp is the size of each logical pixel in bytes
		 - should not be modified, except when the hardware changes.
		image_bpp is only used by ReColor() to convert image data
		 - can be modified to affect ReColor() if saved and restored.
	*/
	unsigned char *shared_mem;	// The shared memory frame buffer
	unsigned char *backbuf;		// The background bits buffer
	unsigned long shared_len;	// The length of the frame buffer
	int WIDTH, HEIGHT;		// The width and height of the window
	int VWIDTH;			// The width in bytes of a scanline
	int clip_left;			// Left of Blit clipping rectangle
	int clip_top;			// Top of Blit clipping rectangle
	int clip_right;			// Right of Blit clipping rectangle
	int clip_bottom;		// Bottom of Blit clipping rectangle
	long pitch;			// Width of video memory scanline
	int video_bpp;			// Bytes per pixel of video hardware
	int image_bpp;			// Bytes per pixel of image data

	virtual void SetBPP(int bpp) {
		video_bpp = bpp;
		if ( video_bpp > 1 )
			truecolor = 1;
		else
			truecolor = 0;
		VWIDTH = (WIDTH * bpp);
	}

/* Ultra-fast routines for several types of pixel-poking :-) */
#define POKE_1(dest, pixel)	*((char *)dest) = (char)pixel
#define POKE_2(dest, pixel)	*((short *)(dest)) = (short)pixel
#define POKE_4(dest, pixel)	memcpy(dest, &pixel, video_bpp)

	virtual void PixelPoke(unsigned char *dest, int offset, int Pitch,
								long pixel) {
#ifdef PIXEL_DOUBLING
		dest += ((offset/Pitch)*Pitch*4)+((offset%Pitch)*2);
		switch (video_bpp) {
			case 1:
				POKE_1(dest, pixel);
				POKE_1(dest+1, pixel);
#ifndef INTERLACE_DOUBLED
				dest += Pitch*2;
				POKE_1(dest, pixel);
				POKE_1(dest+1, pixel);
#endif
				break;
			case 2:
				POKE_2(dest, pixel);
				POKE_2(dest+2, pixel);
#ifndef INTERLACE_DOUBLED
				dest += Pitch*2;
				POKE_2(dest, pixel);
				POKE_2(dest+2, pixel);
#endif
				break;
			case 3:
			case 4:
				POKE_4(dest, pixel);
				POKE_4(dest+video_bpp, pixel);
#ifndef INTERLACE_DOUBLED
				dest += Pitch*2;
				POKE_4(dest, pixel);
				POKE_4(dest+video_bpp, pixel);
#endif
				break;
		}
#else
		switch (video_bpp) {
			case 1:
				POKE_1(dest+offset, pixel);
				break;
			case 2:
				POKE_2(dest+offset, pixel);
				break;
			case 3:
			case 4:
				POKE_4(dest+offset, pixel);
				break;
		}
#endif
	}
	/* We assume that:
	   'Pitch' is the raw byte-width of the destination surface,
	   'length' is the raw byte-length of the data being copied,
	   and that 'src' and 'dest' have the same pixel format.
	*/
	virtual void LinePoke(unsigned char *dest, int offset, int Pitch,
						unsigned char *src, int length) {
#ifdef PIXEL_DOUBLING
		dest += ((offset/Pitch)*Pitch*4)+((offset%Pitch)*2);
		while ( length > 0 ) {
			register unsigned char *dptr;
			/* Upper left */
			dptr = dest;
			memcpy(dptr, src, video_bpp);
			/* Upper right */
			dptr += video_bpp;
			memcpy(dptr, src, video_bpp);
#ifndef INTERLACE_DOUBLED
			/* Lower right */
			dptr += (Pitch*2);
			memcpy(dptr, src, video_bpp);
			/* Lower left */
			dptr -= video_bpp;
			memcpy(dptr, src, video_bpp);
#endif
			/* Update loop */
			src += video_bpp;
			dest += (video_bpp*2);
			length -= video_bpp;
		}
#else
		memcpy(dest+offset, src, length);
#endif
	}

	/* All colors in Color_Map are shifted << 8 from 0xFFFF format */
	unsigned long Black;			// The black pixel color 
	int           truecolor;		// Are we in truecolor mode?
	Color         Color_Map[NUM_COLORS];	// The colormap we actually use
	unsigned long Pixel_colors[NUM_COLORS];	// The pixel->color table

	/* Keep track of all translated artwork */
	virtual void ReColorArt(void);
	struct artwork {
		unsigned char  *original;
		unsigned char  *ops;		// NULL unless CSprite
		int             ops_bpp;
		int             len;
		int		bpp;
		unsigned char **newptr;
	};
	List<struct FrameBuf::artwork> *ourart;

	/* Misc. state variables */
	int    hidden_cursor;		// Is the cursor hidden?
	int    DoFade;			// Do we actually perform fades?
	int    faded;			// Are we faded?
};
#endif /* _framebuf_h */
