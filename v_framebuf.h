
/* Framebuffer graphics routines for Maelstrom!  (By Sam Lantinga) */

#ifndef _v_framebuf_h
#define _v_framebuf_h

#include "framebuf.h"
#include "Sprite.h"

/* This class allocates a frame-buffer and provides high-level
   routines to blit images into it.
*/
class Virtual_FrameBuf : public FrameBuf {

public:
	Virtual_FrameBuf(unsigned short width, unsigned short height, int bpp);
	~Virtual_FrameBuf();

	int            Alloc_Cmap(Color Cmap[]);
	int            Alloc_Private_Cmap(Color Cmap[]);
	void           RefreshArea(int x0, int y0, int width, int height);
	void           Refresh(void);
	void           Hide_Cursor(void);
	void           Show_Cursor(void);
	void           Fade(int steps);
	void           FlushEvents(void);
	int            NumEvents(void);
	void           GetEvent(XEvent *event);
	int            KeyToAscii(XEvent *event, char *buf, int buflen,
								KeySym *key);
	void           Flush(int sync);

	char *DisplayType(void) {
		return("Virtual Framebuffer Display");
	}

};

#endif /* _v_framebuf_h */
