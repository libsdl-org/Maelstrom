
// The VGA graphics module:
//

#if defined(linux) && defined (USE_SVGALIB)
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>
#include <linux/vt.h>

#include "vga_framebuf.h"
#include "cursor_sprite.h"
#include "keyboard.h"
#include "vga_keys.h"

typedef struct rgb_triple {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	};

/* The following are almost IBM standard color codes */
static rgb_triple vga_colors[] = {
	{0x00,0x00,0x00},{0x18,0x18,0xB2},{0x18,0xB2,0x18},{0x18,0xB2,0xB2},
	{0xB2,0x18,0x18},{0xB2,0x18,0xB2},{0xB2,0x68,0x18},{0xB2,0xB2,0xB2},
	{0x68,0x68,0x68},{0x54,0x54,0xFF},{0x54,0xFF,0x54},{0x54,0xFF,0xFF},
	{0xFF,0x54,0x54},{0xFF,0x54,0xFF},{0xFF,0xFF,0x54},{0xFF,0xFF,0xFF}
};

extern "C" {  // Part of SVGAlib (undocumented)
	extern int  mouse_update(void);
	extern void mouse_seteventhandler(void (*handler)(int, int, int));
}

static VGA_FrameBuf *THIS=NULL;
void VGA_MouseHandler(int button, int x, int y)
{
	THIS->MoveMouse(button, x, y);
}

VGA_FrameBuf:: VGA_FrameBuf(unsigned short width, unsigned short height, 
					int bpp) : FrameBuf(width, height, bpp)
{
	struct vt_stat vtstate;
	int    i;

	/* Make sure we only have one VGA framebuffer per process */
	if ( THIS ) {
		error("VGA_FrameBuf: VGA frame buffer already running!\n");
		exit(255);
	}

	/* Verify the width and height */
	if ( (width != 640) || (height != 480) ) {
		error("VGA_FrameBuf only supports 640x480 resolution!\n");
		exit(255);
	}

	/* Make sure we are connected to a linux console */
	if ( (kbd_fd=open("/dev/tty", O_RDONLY, 0)) < 0) {
		perror("VGA_FrameBuf: Can't open tty");
		exit(255);
	}
	if ( ioctl(kbd_fd, KDGKBMODE, &Old_KDmode) < 0 ) {
		if ( errno == EINVAL ) {
			error("VGA_FrameBuf: Not at a linux console!\n");
		} else {
			perror(
			"VGA_FrameBuf: Can't get console keyboard modes");
		}
		exit(255);
	}
	for ( i=0; i<256; ++i )
		pressed_keys[i] = 0;

	/* Find out what the current console is. */
	if ( ioctl(kbd_fd, VT_GETSTATE, &vtstate) < 0 ) {
		perror("ioctl(VT_GETSTATE)");
		exit(255);
	}
	console = vtstate.v_active;

#ifdef LOW_RES
#define GRAPHICS_MODE	G320x240x256
#undef VGA_16COLOR
#else
#ifdef VGA_16COLOR
#define GRAPHICS_MODE	G640x480x16
#else
#define GRAPHICS_MODE	G640x480x256
#endif
#endif /* LOW_RES */

	/* Turn on the SVGA Graphics mode */
	if ( ! vga_hasmode(GRAPHICS_MODE) ) {
		error(
	"VGA_FrameBuf: Video hardware doesn't support requested mode!\n");
		exit(255);
	}
	vga_setmousesupport(1);
	if ( vga_setmode(GRAPHICS_MODE) != 0 ) {
		error("VGA_FrameBuf: Couldn't initialize graphics!\n");
		exit(255);
	}
	gl_setcontextvga(GRAPHICS_MODE);
	screen = gl_allocatecontext();
	gl_getcontext(screen);

	/* Make sure we're at 8 bits per pixel... */
	if ( screen->bytesperpixel != 1 ) {
		vga_setmode(TEXT);
		error("VGA_FrameBuf: SVGA card uses %d bits per pixel!\n",
						screen->bytesperpixel*8);
		exit(255);
	}
#ifdef LOW_RES
	/* Allocate virtual context and associated buffer */
	low_res_mem = new unsigned char[320*240];
	gl_setcontextvirtual(320, 240, 1, 8, low_res_mem);
#endif /* LOW_RES */

#ifdef VGA_16COLOR			/* Hack, hack, hack. :-) */
	/* Create virtual screen. */
	gl_setcontextvgavirtual(GRAPHICS_MODE);
#define gl_setrgbpalette()
#define gl_rgbcolor(x,y,z)	0
#endif /* VGA_16COLOR */

#ifdef DOUBLE_BUFFER
	/* Enable page flipping for smooth animation on screen */
	gl_enablepageflipping(screen);
#endif
	/* Set basic color palette */
	gl_setrgbpalette();

	/* Get a black pixel and fill our pixel array with it */
	Black = gl_rgbcolor(0, 0, 0);
	for ( i=0; i<NUM_COLORS; ++i )
		Pixel_colors[i] = Black;

	/* Allocate a screen pixmap, and set it black */
	shared_len = (width * height);	// 8 bits per pixel
	backbuf = new unsigned char[shared_len];
	memset(backbuf, Black, shared_len);
	shared_mem = new unsigned char[shared_len];
	memset(shared_mem, Black, shared_len);

	/* Initialize the keyboard 
	   WARNING: if not properly terminated, this will fry the keyboard!
	*/
	if ( ioctl(kbd_fd, KDSKBMODE, K_MEDIUMRAW) < 0 ) {
		vga_setmode(TEXT);
		perror("Can't set console keyboard modes");
		exit(255);
	}
	system("stty raw; stty -echo");

	/* Initialize Event Queues */
	Events = new Queue<XEvent>(ChunkSize);
	key_state = 0;
	Reefer = new Stack<Area>(ChunkSize);

	/* Set mouse handler */
	mouseSprite.width = *((unsigned short *)cursor_sprite);
	mouseSprite.height = *((unsigned short *)&cursor_sprite[2]);
	mouseSprite.pixels = (unsigned char *)&cursor_sprite[4];
	mouseSprite.mask = (unsigned char *)&cursor_sprite[
				(mouseSprite.width*mouseSprite.height)+4];
	behind_mouse = new unsigned char
				[mouseSprite.width*mouseSprite.height];
#ifdef CENTER_MOUSE
	mouseX = WIDTH/2;
	mouseY = HEIGHT/2;
#else
	mouseX = 0;
	mouseY = 0;
#endif
	mouseButtons = 0;
	mouse_accel = 1.0;
	THIS = this;
	mouse_seteventhandler(VGA_MouseHandler);

	/* Set the cursor */
	hidden_cursor = 1;
	Show_Cursor();

	/* Allow switches to other consoles */
	vga_unlockvc();
}

VGA_FrameBuf:: ~VGA_FrameBuf()
{
	struct vt_stat vtstate;

	/* Free the video memory */
#ifdef LOW_RES
	delete[] low_res_mem;
#endif
	delete[] backbuf;
	delete[] shared_mem;
	delete   Reefer;
	delete   Events;

	/* Jump to the current console, if we aren't there yet.
		(prevents corrupt text console)
	*/
	if ( ioctl(kbd_fd, VT_GETSTATE, &vtstate) >= 0 ) {
		if ( vtstate.v_active != console )
			(void) ioctl(kbd_fd, VT_ACTIVATE, console);
	}

	/* Reset the keyboard and mouse --
		If these fail, what can we do?
	 */
	(void) ioctl(kbd_fd, KDSKBMODE, Old_KDmode);
	(void) system("stty -raw echo");

	/* Reset the display */
	vga_setmode(TEXT);
	THIS = NULL;
}

int
VGA_FrameBuf:: Alloc_Cmap(Color Cmap[NUM_COLORS])
{
	return(Alloc_Private_Cmap(Cmap));
}

/* Allocate a private 256 color colormap :) */
/* Note: The orig_colors array is here to keep a record of the _actual_
   contents of the VGA palette.  This is used during the Fade, and
   differs from Color_Map because the Trident SVGA card requires the
   zero'th element of the VGA palette to be the background color, black.
*/
static rgb_triple orig_colors[NUM_COLORS];
int
VGA_FrameBuf:: Alloc_Private_Cmap(Color Cmap[NUM_COLORS])
{
	int        c;

#ifdef VGA_16COLOR
	/* 16 color VGA? - Ick! */
	/* Most of the mapping code is adapted from 'xv' */
	int ri, gi, bi;			/* The color shades we want */
	int rd, gd, bd;			/* The color shades we have */
	int mdist, close, i, d;		/* Distance and closest pixel */

	/* Map the whole colormap */
	for ( i=0; i<NUM_COLORS; ++i ) {
		mdist = 100000; close=0;

		ri = (Cmap[i].red >> 8);
		gi = (Cmap[i].green >> 8);
		bi = (Cmap[i].blue >> 8);

		/* Cycle through the colors we have */
		for ( c=0; c<16; ++c ) {
			rd = ri - vga_colors[c].red;
			gd = gi - vga_colors[c].green;
			bd = bi - vga_colors[c].blue;
			d = rd*rd + gd*gd + bd*bd;

			if ( d < mdist ) {
				mdist = d;
				close = c;
			}
		}
#ifdef DEBUG
error("Desired RGB (%d,%d,%d) mapped to existing RGB (%d,%d,%d)\n",
				ri, gi, bi, vga_colors[close].red,
			vga_colors[close].green, vga_colors[close].blue);
#endif
		Pixel_colors[i] = close;
		Color_Map[i].red   = (vga_colors[close].red<<8);
		Color_Map[i].green = (vga_colors[close].green<<8);
		Color_Map[i].blue  = (vga_colors[close].blue<<8);
	}
	return(16);
#endif /* VGA_16COLOR */

	// Gently refresh a black screen in the new colormap
	memset(orig_colors, 0, NUM_COLORS*sizeof(rgb_triple));
	gl_setpalette(orig_colors);
	for ( c=0; c<NUM_COLORS; ++c ) {
		Color_Map[c].red   = Cmap[c].red<<8;
		Color_Map[c].green = Cmap[c].green<<8;
		Color_Map[c].blue  = Cmap[c].blue<<8;
		orig_colors[c].red      = (Cmap[c].red>>2)&0x3F;
		orig_colors[c].green    = (Cmap[c].green>>2)&0x3F;
		orig_colors[c].blue     = (Cmap[c].blue>>2)&0x3F;
		Pixel_colors[c]    = c;

		if ( !Cmap[c].red && !Cmap[c].green && !Cmap[c].blue )
			Black = c;
	}
	orig_colors[Black].red = orig_colors[0].red;
	orig_colors[Black].green = orig_colors[0].green;
	orig_colors[Black].blue = orig_colors[0].blue;
	orig_colors[0].red = orig_colors[0].green = orig_colors[0].blue = 0;
	Pixel_colors[Black] = 0;
	Pixel_colors[0] = Black;
	Black = 0;
	memset(backbuf, Black, shared_len);
	memset(shared_mem, Black, shared_len);
	Refresh();
	gl_setpalette(orig_colors);
	return(NUM_COLORS);
}

void
VGA_FrameBuf:: Hide_Cursor(void)
{
	if ( hidden_cursor ==  0 ) {
		showmouse = 0;
		EraseMouse();
		hidden_cursor = 1;
	}
}

void
VGA_FrameBuf:: Show_Cursor(void)
{
	if ( hidden_cursor == 1 ) {
		DrawMouse();
		showmouse = 1;
		hidden_cursor = 0;
	}
}

/* This function was adapted from 'xscreensaver', by Jamie Zawinski 

	-- Thanks! :)
*/
void
VGA_FrameBuf:: Fade(int steps)
{
	static int state = XFADE_IN;
	rgb_triple Colors[NUM_COLORS];
	int        i, c;

	/* Find out the state of the fade */
	state = ( (state == XFADE_IN) ? XFADE_OUT : XFADE_IN );

	/* Do we really fade? */
	switch (DoFade) {
		case FADE_FAKE:
			/* Do a pixellated fade */
			Pixel_Fade(state);
			
		case FADE_NONE:
			/* We're done */
			return;

		case FADE_REAL:
#ifdef VGA_16COLOR
			/* Do a pixellated fade */
			Pixel_Fade(state);
			return;
#else
			/* Continue... */
			break;
#endif
	}

	if ( state == XFADE_OUT ) {
		for ( i=steps; i > 0; i-- ) {
			for (c = 0; c < NUM_COLORS; c++) {
				Colors[c].red   =
					((orig_colors[c].red   * i) / steps);
				Colors[c].green =
					((orig_colors[c].green * i) / steps);
				Colors[c].blue  =
					((orig_colors[c].blue  * i) / steps);
			}
			gl_setpalette(Colors);
			select_usleep(5);
		}
	} else {
		Refresh();
		for ( i=0; i < steps; i++ ) {
			for (c = 0; c < NUM_COLORS; c++) {
				Colors[c].red   =
					((orig_colors[c].red   * i) / steps);
				Colors[c].green =
					((orig_colors[c].green * i) / steps);
				Colors[c].blue  =
					((orig_colors[c].blue  * i) / steps);
			}
			gl_setpalette(Colors);
			select_usleep(5);
		}
	}
	
	if ( state == XFADE_IN ) {
		/* Restore the original colormap */
		for (c = 0; c < NUM_COLORS; c++) {
			Colors[c].red   = orig_colors[c].red;
			Colors[c].green = orig_colors[c].green;
			Colors[c].blue  = orig_colors[c].blue;
		}
		gl_setpalette(Colors);
	}
}
	
/* BTW, you _can_ pass negative x0 and y0 values to this function */
void
VGA_FrameBuf:: RefreshArea(int x0, int y0, int width, int height)
{
	Area area;

	/* Do bounds checking */
	if ( y0 < 0 ) {
		if ( (height += y0) <= 0 )
			return;
		y0 = 0;
	}
	if ( (y0 + height) >= HEIGHT ) {
		if ( y0 > HEIGHT )
			return;
		height = (HEIGHT-y0);
	}
	if ( x0 < 0 ) {
		if ( (width += x0) <= 0 )
			return;
		x0 = 0;
	}
	if ( (x0 + width) >= WIDTH ) {
		if ( x0 > WIDTH )
			return;
		width = (WIDTH-x0);
	}
	/* Set up the area and queue it */
	area.x = x0;
	area.y = y0;
	area.width = width;
	area.height = height;
	Reefer->Add(&area);
}

#ifdef LOW_RES
void
VGA_FrameBuf:: shrinkwrap(void)
{
	unsigned char *from = shared_mem;
	unsigned char *to   = low_res_mem;
	int i, j;

	for ( i=HEIGHT/2; i; --i ) {
		for ( j=WIDTH; j; j -= 2 ) {
			*(to++) = *from;
			from += 2;
		}
		from += WIDTH;
	}
}
void
VGA_FrameBuf:: shrinkwrap(int x, int y, int w, int h, unsigned char *data)
{
	unsigned char *to = low_res_mem;
	int i, j;

	/* Adjust entry offsets */
	if ( y%2 ) {
		data += w;
		++y;
	}
	if ( x%2 ) {
		++data;
		++x;
	}
	to += (((y/2)*(WIDTH/2))+(x/2));

	for ( i=h/2; i; --i ) {
		for ( j=w/2; j; --j ) {
			*(to++) = *data;
			data += 2;
		}
		to += (WIDTH/2)-(w/2);
		data += w;
	}
}
#endif /* LOW_RES */

void
VGA_FrameBuf:: Refresh(void)
{
	/* Don't refresh during fake pixel fade */
	if ( faded ) return;

	Reefer->Flush();
#ifdef LOW_RES
	shrinkwrap();
	if ( showmouse )
		DrawMouse();
	else
		gl_copyscreen(screen);
#else
	gl_putbox(0, 0, WIDTH, HEIGHT, shared_mem);
	if ( showmouse )
		DrawMouse();
#ifdef VGA_16COLOR
	gl_copyscreen(screen);
#endif
#endif /* LOW_RES */
}

void
VGA_FrameBuf:: FlushEvents(void)
{
	int    nevents;
	XEvent event;

	for ( nevents=NumEvents(); nevents; --nevents )
		GetEvent(&event);
}


/* Stuff to help map keycodes to ascii */
#define RSHIFT_BIT	0x01
#define LSHIFT_BIT	0x02
#define RCTRL_BIT	0x04
#define LCTRL_BIT	0x08
#define RALT_BIT	0x10
#define LALT_BIT	0x20
#define CAPSLK_BIT	0x40
#define INSERT_BIT	0x80
#define STATE_SHIFT(X)	( (X&CAPSLK_BIT) ? (!(X&(RSHIFT_BIT|LSHIFT_BIT))) : \
						(X&(RSHIFT_BIT|LSHIFT_BIT)) )
#define STATE_CTRL(X)	(X&(RCTRL_BIT|LCTRL_BIT))
#define STATE_ALT(X)	(X&(RALT_BIT|LALT_BIT))

#define UP_BIT	0x80
int
VGA_FrameBuf:: NumEvents(void)
{
	XEvent event;
	struct timeval tv;
	fd_set fdset;
	int    i, len;
	unsigned char key, buffer[1024];

	/* Check for mouse events (before calling select()!) */
	mouse_update();

	/* Check for keyboard events */
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&fdset);
	FD_SET(kbd_fd, &fdset);

	if ( select(kbd_fd+1, &fdset, NULL, NULL, &tv) != 1 )
		return(Events->Size());

	if ( (len=read(kbd_fd, buffer, 1024)) < 0 ) {
		perror("VGA_FrameBuf: read event error");
		return(Events->Size());
	}

	for ( i=0; i<len; ++i ) {
		key = buffer[i];

		if ( key & UP_BIT ) {
			key &= ~UP_BIT;
			if ( pressed_keys[key] ) {
				event.type = KeyRelease;
				switch (vga_keys[key]) {
					case XK_Caps_Lock:
						key_state &= ~CAPSLK_BIT;
						break;
					case XK_Shift_L:
						key_state &= ~LSHIFT_BIT;
						break;
					case XK_Shift_R:
						key_state &= ~RSHIFT_BIT;
						break;
					case XK_Control_L:
						key_state &= ~LCTRL_BIT;
						break;
					case XK_Control_R:
						key_state &= ~RCTRL_BIT;
						break;
					case XK_Alt_L:
						key_state &= ~LALT_BIT;
						break;
					case XK_Alt_R:
						key_state &= ~RALT_BIT;
						break;
					default:
						break;
				}
				event.xkey.state = key_state;
				event.xkey.keycode = key;
				Events->Push(&event);
				pressed_keys[key] = 0;
			} else {
				/* Huh?  Key release with no press? */;
			}
		} else {
			if ( ! pressed_keys[key] ) {
				event.type = KeyPress;
				switch (vga_keys[key]) {
					case XK_Caps_Lock:
						key_state |= CAPSLK_BIT;
						break;
					case XK_Shift_L:
						key_state |= LSHIFT_BIT;
						break;
					case XK_Shift_R:
						key_state |= RSHIFT_BIT;
						break;
					case XK_Control_L:
						key_state |= LCTRL_BIT;
						break;
					case XK_Control_R:
						key_state |= RCTRL_BIT;
						break;
					case XK_Alt_L:
						key_state |= LALT_BIT;
						break;
					case XK_Alt_R:
						key_state |= RALT_BIT;
						break;
					// Check for console switch
					case XK_F1:
					case XK_F2:
					case XK_F3:
					case XK_F4:
					case XK_F5:
					case XK_F6:
					case XK_F7:
					case XK_F8:
						if ( STATE_ALT(key_state) ) {
							Switch(vga_keys[key]-
								XK_F1+1);
							continue;
						}
						break;
					default:
						break;
				}
				event.xkey.state = key_state;
				event.xkey.keycode = key;
				Events->Push(&event);
				pressed_keys[key] = 1;
			}
		}
	}
	return(Events->Size());
}

void
VGA_FrameBuf:: GetEvent(XEvent *event)
{
	Flush(0);
	while ( ! NumEvents() )
		;
	Events->Pull(event);
}

/* Macro to 'controlify' a lower case letter */
#define toctrl(X)	(toupper(X)-'@')

int
VGA_FrameBuf:: KeyToAscii(XEvent *event, char *buf, int buflen, KeySym *key)
{
	*key = vga_keys[event->xkey.keycode];
	for ( int i=0; keycodes[i].name; ++i ) {
		if ( *key == keycodes[i].key ) {
			if ( (strlen(keycodes[i].ascii)+1) > buflen ) {
				strncpy(buf, keycodes[i].ascii, buflen-1);
				buf[buflen-1] = '\0';
			} else
				strcpy(buf, keycodes[i].ascii);

			/* We rely on function strings starting with ESC */
			if ( STATE_CTRL(event->xkey.state) ) {
				if ( isalpha(*buf) )
					*buf = toctrl(*buf);
			} else if ( STATE_SHIFT(event->xkey.state) ) {
				if ( isalnum(*buf) )
					*buf = toupper(*buf);
			}
			return(strlen(buf));
		}
	}
	return(0);
}

/* Sorting routine for refresh area stack */
int sort_areas(VGA_FrameBuf::Area *item1, VGA_FrameBuf::Area *item2) {
	return(item1->y - item2->y);
}

void
VGA_FrameBuf:: Flush(int sync)
{
	int nreefs;
	Area *area;

#if defined(LOW_RES) || defined(VGA_16COLOR)
	Refresh();
	return;
#endif
#ifdef SORT_REFRESH
	Reefer->Sort(sort_areas);
#endif
	for ( nreefs = Reefer->Size(); nreefs; --nreefs ) {
		area = Reefer->Pop();	// This should never be NULL
		gl_putboxpart(area->x, area->y, area->width, area->height,
				WIDTH, HEIGHT, shared_mem, area->x, area->y);
	}
	if ( showmouse )
		DrawMouse();
	Unused(sync);
}

/* Routine that switches to another console and suspends
   the current process until the console is switched back.
*/
void
VGA_FrameBuf:: Switch(int which)
{
	struct termios ttstate, *ttptr=&ttstate;

	/* Don't do a switch to current console -- redundant */
	if ( which == console )
		return;

	/* Due to a bug (feature?), we need to save and restore the
	   tty modes.  For some reason, when the console is switched
	   back, input signal processing is re-enabled.  We don't want this. :)
	*/
	if ( tcgetattr(kbd_fd, ttptr) < 0 ) {
		/* Huh?  We probably shouldn't call tcsetattr() later. */
		error("tcgetattr() failed.  Never happens. ;-)\r\n");
		ttptr = NULL;
	}	
	
	/* Switch to a different console and wait for switch back */
	vga_runinbackground(0);
	if ( ioctl(kbd_fd, VT_ACTIVATE, which) < 0 ) {
		perror("ioctl(VT_ACTIVATE)");
		return;
	}
	/* We're back. :) */

	/* Restore the terminal attributes */	
	if ( ttptr )
		(void) tcsetattr(kbd_fd, TCSANOW, ttptr);
	return;
}

// Miscellaneous function:
/* Tell whether we are on the console... */
int On_Console(void)
{
	struct vt_stat vtstate;

	return((ioctl(2, VT_GETSTATE, &vtstate) == 0) ? 1 : 0);
}
#else
int On_Console(void)
{
	return(0);
}
#endif /* linux && USE_SVGALIB */

