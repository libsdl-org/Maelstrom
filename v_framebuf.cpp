
// The virtual graphics module:
//

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

#include "v_framebuf.h"


Virtual_FrameBuf::Virtual_FrameBuf(unsigned short width, unsigned short height,
					int bpp) : FrameBuf(width, height, bpp)
{
	/* Allocate a screen pixmap and make it black. */
	Black = 0;
	shared_len = (width * height);	// 8 bits per pixel
	backbuf = new unsigned char[shared_len];
	memset(backbuf, Black, shared_len);
	shared_mem = new unsigned char[shared_len];
	memset(shared_mem, Black, shared_len);
}

Virtual_FrameBuf:: ~Virtual_FrameBuf()
{
	/* Free the video memory */
	delete[] backbuf;
	delete[] shared_mem;
}

int
Virtual_FrameBuf:: Alloc_Cmap(Color Cmap[NUM_COLORS])
{
	return(Alloc_Private_Cmap(Cmap));
}

/* Allocate a private 256 color colormap :) */
int
Virtual_FrameBuf:: Alloc_Private_Cmap(Color Cmap[NUM_COLORS])
{
	int        c;

	/* Just copy them in */
	for ( c=0; c<NUM_COLORS; ++c ) {
		Color_Map[c].red   = Cmap[c].red<<8;
		Color_Map[c].green = Cmap[c].green<<8;
		Color_Map[c].blue  = Cmap[c].blue<<8;
		Pixel_colors[c]    = c;

		if ( !Cmap[c].red && !Cmap[c].green && !Cmap[c].blue )
			Black = c;
	}
	memset(backbuf, Black, shared_len);
	memset(shared_mem, Black, shared_len);
	Refresh();
	return(NUM_COLORS);
}

void
Virtual_FrameBuf:: Hide_Cursor(void)
{
	if ( hidden_cursor ==  0 ) {
		hidden_cursor = 1;
	}
}

void
Virtual_FrameBuf:: Show_Cursor(void)
{
	if ( hidden_cursor == 1 ) {
		hidden_cursor = 0;
	}
}

/* This function was adapted from 'xscreensaver', by Jamie Zawinski 

	-- Thanks! :)
*/
void
Virtual_FrameBuf:: Fade(int steps)
{
	static int state = XFADE_IN;

	/* Find out the state of the fade */
	state = ( (state == XFADE_IN) ? XFADE_OUT : XFADE_IN );

	/* We don't really fade.. */
	Pixel_Fade(state);
	Unused(steps);
}
	
void
Virtual_FrameBuf:: RefreshArea(int x0, int y0, int width, int height)
{
	Unused(x0); Unused(y0); Unused(width); Unused(height);
	return;
}

void
Virtual_FrameBuf:: Refresh(void)
{
	return;
}

void
Virtual_FrameBuf:: FlushEvents(void)
{
	return;
}


int
Virtual_FrameBuf:: NumEvents(void)
{
	return(0);
}

void
Virtual_FrameBuf:: GetEvent(XEvent *event)
{
	Unused(event);
	return;
}

int
Virtual_FrameBuf:: KeyToAscii(XEvent *event, char *buf, int buflen, KeySym *key)
{
	Unused(event); Unused(buf); Unused(buflen); Unused(key);
	return(0);
}

void
Virtual_FrameBuf:: Flush(int sync)
{
	Unused(sync);
	return;
}

