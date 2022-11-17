
/* VGA Graphics routines for Maelstrom!  (By Sam Lantinga) */

#ifndef _vga_framebuf_h
#define _vga_framebuf_h

#if defined(linux) && defined(USE_SVGALIB)
#include <linux/kd.h>
#include <vga.h>
#include <vgagl.h>

/* Some features to enable... */
#define DOUBLE_BUFFER

/* Undefine some of the defines in vga.h/vgagl.h */
#undef WIDTH
#undef HEIGHT

#include "framebuf.h"
#include "Sprite.h"

/* Include the stack template for the Refresh stack */
#include "stack.tmpl"

/* Include the queue template for the Events queue */
#include "queue.tmpl"


/* This class allocates a 640x480 frame-buffer and provides high-level
   routines to blit images into it.  Well, it also provides access to
   other VGA based drawing routines.
*/
class VGA_FrameBuf : public FrameBuf {

public:
	VGA_FrameBuf(unsigned short width, unsigned short height, int bpp);
	~VGA_FrameBuf();

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
		return("Linux console SVGA Display");
	}

protected:
	GraphicsContext *screen;
	int		 console;
	int              kbd_fd;
	unsigned long    Old_KDmode;

#ifdef LOW_RES
	/* Low resolution SVGA hack */
	unsigned char   *low_res_mem;
	void             shrinkwrap(void);
	void             shrinkwrap(int x, int y, int w, int h,
							unsigned char *data);
#endif
	/* Keypress event queue structures */
	unsigned char  pressed_keys[256];
	int            key_state;
	Queue<XEvent> *Events;
	friend void VGA_MouseHandler(int button, int x, int y);
	
	/* Refresh-event queue structures */
	typedef struct {
		int x, y;
		int width, height;
	} Area;
	Stack<Area> *Reefer;
	friend int sort_areas(Area *item1, Area *item2);

	/* Chunk size in which the stack is allocated */
	static const int	 ChunkSize = 32;

	/* Mouse stuff */
	double mouseX, mouseY;
	double mouse_accel;
	int mouseButtons;
	int showmouse;
	CIcon mouseSprite;
	unsigned char *behind_mouse;

	void DrawMouse(void) {
		unsigned char *sdata = mouseSprite.pixels;
		unsigned char *mdata = mouseSprite.mask;
		int row, col, offset;
		int m_x=(int)mouseX, m_y=(int)mouseY;
		int m_width = mouseSprite.width;
		int m_height = mouseSprite.height;
		unsigned char new_area[m_width*m_height];

		/* Save what's behind the mouse */
		for ( row=0; row<m_height; ++row ) {
			memcpy(&behind_mouse[row*m_width], 
				&shared_mem[(m_y+row)*WIDTH+m_x], m_width);
		}
		memcpy(new_area, behind_mouse, m_width*m_height);

		/* Draw the mouse in the new area */
		for ( row=0; row<m_height; ++row ) {
			for ( col=0; col<m_width; ++col, ++sdata ) {
				offset = ((row*m_width)+col);
				if ((mdata[offset/8]>>(7-(offset%8))) & 0x01) {
					new_area[offset] = Pixel_colors[*sdata];
				}
			}
		}
#ifdef LOW_RES
		shrinkwrap(m_x, m_y, m_width, m_height, new_area);
		gl_copyscreen(screen);
#else
		gl_putbox(m_x, m_y, m_width, m_height, new_area);
#endif
	}
	void EraseMouse(void) {
#ifdef LOW_RES
		shrinkwrap((int)mouseX, (int)mouseY, mouseSprite.width,
					mouseSprite.height, behind_mouse);
		gl_copyscreen(screen);
#else
		gl_putbox((int)mouseX, (int)mouseY, mouseSprite.width,
					mouseSprite.height, behind_mouse);
#endif
	}
	void MoveMouse(int button, int x, int y) {
		XEvent event;

		if ( showmouse )
			EraseMouse();
		mouseX += (mouse_accel*x);
		if ( mouseX < 0 )
			mouseX = 0;
		if ( mouseX > (WIDTH-mouseSprite.width) )
			mouseX = (WIDTH-mouseSprite.width);
		mouseY += (mouse_accel*y);
		if ( mouseY < 0 )
			mouseY = 0;
		if ( mouseY > (HEIGHT-mouseSprite.height) )
			mouseY = (HEIGHT-mouseSprite.height);

		/* Handle button press events (not yet robust) */
		if ( button != mouseButtons ) {
			if ( button )
				event.xbutton.type = ButtonPress;
			else
				event.xbutton.type = ButtonRelease;
			event.xbutton.x = event.xbutton.x_root = (int)mouseX;
			event.xbutton.y = event.xbutton.y_root = (int)mouseY;
			event.xbutton.button = 1;
			Events->Push(&event);
			mouseButtons = button;
		}
		if ( showmouse )
			DrawMouse();
	}

	/* Routine that switches to another console and suspends
	   the current process until the console is switched back.
	*/
	void Switch(int which);
};
#else
#define vga_init()	setuid(getuid())
#endif /* linux && USE_SVGALIB */

extern int On_Console(void);	// Returns true, if we're at the console.
#endif /* _vga_framebuf_h */
