
/* X11 Graphics routines for Maelstrom!  (By Sam Lantinga) */

#ifndef _x11_framebuf_h
#define _x11_framebuf_h

#include "framebuf.h"

#include <X11/Xutil.h>
#include <X11/xpm.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

/* This class allocates a 640x480 frame-buffer and provides high-level
   routines to blit images into it.  Well, it also provides access to
   other X11 based drawing routines.
*/
class X11_FrameBuf : public FrameBuf {

public:
	X11_FrameBuf(unsigned short width, unsigned short height, int bpp,
				int FullScreen, char *title, char **icon);
	~X11_FrameBuf();

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
		switch (video_bpp) {
			case 1:
				return("X11 8-bit PseudoColor Display");
			case 2:
				return("X11 16-bit HiColor Display");
			case 3:
				return("X11 24-bit TrueColor Display");
			case 4:
				return("X11 32-bit TrueColor Display");
			default:
				return("X11 unknown depth Display");
		}
	}

protected:
	void get_color_info (unsigned long mask, unsigned long *shift, 
							unsigned long *prec);
	void RefreshAll(void);		// Refresh the full-screen too

	Display *display;		// The X11 display connection
	Window   MainWin;		// Our window on the display
	unsigned long Event_mask;	// The events we wait for
	Colormap colormap;		// The current colormap
	Colormap private_cmap;		// A private colormap?
	Atom     wm_del_win;		// The delete window flag
	GC       gc;			// The current graphics context
	int      was_repeating;		// Was the keyboard repeating?

	XShmSegmentInfo shminfo;	// The shared memory segment info
	XImage *X_image;		// The shared memory screen image
	int use_mitshm;			// Can we use mit shared memory?

	Cursor cursor, nocursor;

	int fullscreen;			// Are we full-screen?
	int root_width;			// The width of the root screen
	int root_height;		// The height of the root screen
	Window TopWin;			// The top-level Maelstrom window
	unsigned long White;		// The white pixel

	void SetBPP(int bpp) {
		FrameBuf::SetBPP(bpp);
		pitch = WIDTH * video_bpp;
	}
	unsigned long red_mask;		// Red pixel mask
	unsigned long green_mask;	// Green pixel mask
	unsigned long blue_mask;	// Blue pixel mask
};
#endif /* _x11_framebuf_h */
