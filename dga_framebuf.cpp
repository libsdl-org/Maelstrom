
// The X11 DGA graphics module:
//

#if defined(linux) && defined(USE_DGA)

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "dga_framebuf.h"
#include "cursor_sprite.h"

#ifndef True
#define	True	1
#endif
#ifndef False
#define False	0
#endif


DGA_FrameBuf:: DGA_FrameBuf(unsigned short width, unsigned short height,
				int bpp) : FrameBuf(width, height, bpp)
{
	XVisualInfo     vinfo_return;
	int EventBase, ErrorBase;
	XF86VidModeModeInfo **modes;
	XSetWindowAttributes  xswa;
	int i, ram, nmodes = 0;
	int bits_per_pixel;

	/* Open our DISPLAY */
	if ( (display=XOpenDisplay(NULL)) == NULL ) {
		error("X: Can't open display...\n");
		exit(3);
	}

	/* Make sure all is destroyed if killed off */
	XSetCloseDownMode(display, DestroyAll);

	/* Try to allocate an 8-bit PseudoColor display.
	 * If one isn't available, try to get a 16 bit TrueColor visual.
	 * Lastly, try to get a 24/32 bit TrueColor visual.
	 */
	if ( XMatchVisualInfo(display, DefaultScreen(display), 
					8, PseudoColor, &vinfo_return) ) {
#ifdef DEBUG
		mesg("X: 8 bit PsuedoColor\n");
#endif
	} else if ( XMatchVisualInfo(display, DefaultScreen(display), 
					15, TrueColor, &vinfo_return) ) {
#ifdef DEBUG
		mesg("X: 15 bit TrueColor\n");
#endif
	} else if ( XMatchVisualInfo(display, DefaultScreen(display),
				     16, TrueColor, &vinfo_return) ) {
#ifdef DEBUG
		mesg("X: 16 bit HiColor\n");
#endif
	} else if ( XMatchVisualInfo(display, DefaultScreen(display),
				     24, TrueColor, &vinfo_return) ) {
#ifdef DEBUG
		mesg("X: 24 bit TrueColor\n");
#endif
	} else if ( XMatchVisualInfo(display, DefaultScreen(display),
				     32, TrueColor, &vinfo_return) ) {
#ifdef DEBUG
		mesg("X: 32 bit TrueColor\n");
#endif
	} else {
		error(
		"X: Screen doesn't support PseudoColor or TrueColor!\n");
		exit(3);
	}
	/* Figure out how many bits-per-pixel this depth uses */
	{
		XPixmapFormatValues *pix_format;
		int i, num_formats;

		bits_per_pixel = vinfo_return.depth;
		pix_format = XListPixmapFormats(display, &num_formats);
		if ( pix_format == NULL ) {
			error("Couldn't determine screen formats");
			exit(3);
		}
		for ( i=0; i<num_formats; ++i ) {
			if ( bits_per_pixel == pix_format[i].depth )
				break;
		}
		if ( i != num_formats )
			bits_per_pixel = pix_format[i].bits_per_pixel;
		XFree((char *)pix_format);
	}
	SetBPP((bits_per_pixel+7)/8);
	if ( truecolor ) {
		red_mask = vinfo_return.red_mask;
		green_mask = vinfo_return.green_mask;
		blue_mask = vinfo_return.blue_mask;
	}

	/* Set our X display resolution to width x height */
	zoom = 0;
	if (XF86VidModeQueryExtension(display, &EventBase, &ErrorBase)) {
		if (XF86VidModeGetAllModeLines(display, DefaultScreen(display),
							&nmodes, &modes)) {
			for ( i=0; i<nmodes; ++i ) {
#ifdef DEBUG
				mesg("Mode %d: %dx%d\n", i+1,
					modes[i]->hdisplay, modes[i]->vdisplay);
#endif
				if ( (modes[i]->hdisplay == width) &&
					(modes[i]->vdisplay == height) ) {
					zoom = i;
					break;
				}
			}
			if ( (zoom == 0) && (i == nmodes) ) {
				error(
		"X: Warning: no %dx%d mode defined in your XF86Config file.\n",
								width, height);
			}

			/* if zoom == 0, we are at width-height mode already */
			if ( zoom > 0 ) {
				for ( i=0; i<zoom; ++i ) {
					if (!XF86VidModeSwitchMode(display,
						DefaultScreen(display), 1)) {
						error(
				"X: Warning: couldn't switch video modes\n");
						zoom = i;
						break;
					}
				}
				XFlush(display);
#ifdef DEBUG
				mesg("Switched video modes...\n");
#endif
			}
		} else {
			error("X: Warning: Couldn't get video modes\n");
		}
	} else {
		error("X: Warning: Couldn't set display resolution\n");
	}

	/* See if DGA is supported by the X server */
	if (!XF86DGAQueryExtension(display, &EventBase, &ErrorBase)) {
		error("X: DGA not supported by this X server!\n");
		exit(3);
	}

	/* Create our main window */
	/* override redirect tells the window manger to just ignore us :-) */
	xswa.override_redirect = True;
	xswa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
				PointerMotionMask | ButtonPressMask |
							ButtonReleaseMask;

	/* we want to get everything including colormap */
	TopWin = XCreateWindow(display, DefaultRootWindow(display), 0, 0,
				WidthOfScreen(ScreenOfDisplay(display, 0)),
			        HeightOfScreen(ScreenOfDisplay(display, 0)), 0,
				CopyFromParent, InputOutput, CopyFromParent,
				CWEventMask|CWOverrideRedirect, &xswa);
	Black = BlackPixel(display, DefaultScreen(display));

	XMapWindow(display, TopWin);
	XRaiseWindow(display, TopWin);

	/* We want all the key presses */
	XGrabKeyboard(display, TopWin, True, GrabModeAsync, 
						GrabModeAsync, CurrentTime);

	/* and all the mouse moves */
	XGrabPointer(display, TopWin, True, PointerMotionMask |
		ButtonPressMask | ButtonReleaseMask, GrabModeAsync,
				GrabModeAsync, None,  None, CurrentTime);

	/*
	 * Lets go live -- this is a priviledged operation...
	 */
	XF86DGASetViewPort(display, DefaultScreen(display), 0, 0);
	{ // A ROOT canal. :)
		(void) seteuid(0);
		XF86DGAGetVideo(display, DefaultScreen(display),
					&vbase, (int *)&vwidth, &vpage, &ram);
		(void) seteuid(getuid());
	}
	vwidth *= video_bpp;
//printf("Bank size = %d, Line length = %d, Mem size = %dK\n",vpage,vwidth,ram);
	vlines = (vpage/vwidth);
//printf("vlines = %d\n", vlines);
	XF86DGADirectVideo(display, DefaultScreen(display),
		XF86DGADirectGraphics|XF86DGADirectMouse|XF86DGADirectKeyb);
	XF86DGASetVidPage(display, DefaultScreen(display), 0);

	/* Set up the blit functions */
	mode_linear = ( vlines >= HEIGHT );

	/* Warn that banked video memory is doggy :) */
#ifdef VERBOSE_ERRORS
	if ( ! mode_linear )
		printf("DGA: Warning: Banked video memory is slow!\n");
#endif

	/* Allocate our framebuffer memory */
	shared_len = VWIDTH*HEIGHT;
	shared_mem = new unsigned char[shared_len];
	backbuf = new unsigned char[shared_len];
	Clear(1);

	/* Allocate area refresh queue */
	Reefer = new Stack<Area>(ChunkSize);

	/* Set mouse handler */
	mouseSprite.width = *((unsigned short *)cursor_sprite);
	mouseSprite.height = *((unsigned short *)&cursor_sprite[2]);
	mouseSprite.pixels = (unsigned char *)&cursor_sprite[4];
	mouseSprite.mask = (unsigned char *)&cursor_sprite[
				(mouseSprite.width*mouseSprite.height)+4];
	behind_mouse = new unsigned char
			[mouseSprite.width*mouseSprite.height*video_bpp];
#ifdef CENTER_MOUSE
	mouseX = WIDTH/2;
	mouseY = HEIGHT/2;
#else
	mouseX = 0;
	mouseY = 0;
#endif
	mouse_accel = 1.0;

	/* Set the cursor */
	hidden_cursor = 1;
	Show_Cursor();
}

DGA_FrameBuf:: ~DGA_FrameBuf()
{
	int i;

	/* Back to normal X11 access */
	XF86DGADirectVideo(display, DefaultScreen(display), 0);
	XUngrabPointer(display, CurrentTime);
	XUngrabKeyboard(display, CurrentTime);
	XUnmapWindow(display, TopWin);
	if ( zoom > 0 ) {
		for ( i=0; i<zoom; ++i ) {
			if (!XF86VidModeSwitchMode(display,
						DefaultScreen(display), -1)) {
				error(
				"X: Warning: couldn't switch video modes\n");
				break;
			}
		}
		XFlush(display);
	}
	delete[] behind_mouse;
	delete[] shared_mem;
	delete[] backbuf;
	XCloseDisplay(display);
}

/* Just try to allocate a full colormap */
int
DGA_FrameBuf:: Alloc_Cmap(Color Cmap[NUM_COLORS])
{
	return(Alloc_Private_Cmap(Cmap));
}

/* Allocate a private colormap :) */
int
DGA_FrameBuf:: Alloc_Private_Cmap(Color Cmap[NUM_COLORS])
{
	static int alloct=0;
	int i;
        XColor xcols[NUM_COLORS];

	if ( truecolor ) {
		unsigned long red_prec;		// Red color precision
		unsigned long green_prec;	// Green color precision
		unsigned long blue_prec;	// Blue color precision
		unsigned long red_shift;	// Red color shift
		unsigned long green_shift;	// Green color shift
		unsigned long blue_shift;	// Blue color shift

		/* Get color info for pixels */
		get_color_info(red_mask,&red_shift,&red_prec);
		get_color_info(green_mask, &green_shift, &green_prec);
		get_color_info(blue_mask,&blue_shift,&blue_prec);

		for (i = 0; i < NUM_COLORS; i++) {
			Color_Map[i].red = (Cmap[i].red << 8);
			Color_Map[i].green = (Cmap[i].green << 8);
			Color_Map[i].blue = (Cmap[i].blue << 8);

			Pixel_colors[i] =
				(((Cmap[i].red>>(red_prec+8))<<red_shift)+
				((Cmap[i].green>>(green_prec+8))<<green_shift)+
				((Cmap[i].blue>>(blue_prec+8))<<blue_shift));
		}
		Black = 0L;
		if ( ! alloct ) {
			Clear(0);
			alloct = 1;
		}
		Refresh();
	} else {
		/* Create a custom visual colormap, if needed */
		if ( ! alloct ) {
			colormap = XCreateColormap(display, TopWin,
				DefaultVisual(display, DefaultScreen(display)),
							AllocAll);
			alloct = 1;
		}

		/* Allocate custom colors... */
		for(i=0;i<NUM_COLORS;i++) {
			Pixel_colors[i] = xcols[i].pixel = i;
			Color_Map[i].red = xcols[i].red   = Cmap[i].red<<8;
			Color_Map[i].green = xcols[i].green = Cmap[i].green<<8;
			Color_Map[i].blue = xcols[i].blue  = Cmap[i].blue<<8;
			xcols[i].flags = (DoRed|DoGreen|DoBlue);

			if ( !Cmap[i].red && !Cmap[i].green && !Cmap[i].blue )
				Black = i;
		}
		Clear(0);
		XStoreColors(display, colormap, xcols, NUM_COLORS);
		XF86DGAInstallColormap(display,DefaultScreen(display),colormap);
		Refresh();
	}
	return(NUM_COLORS);
}

void
DGA_FrameBuf:: Hide_Cursor(void)
{
	if ( hidden_cursor ==  0 ) {
		showmouse = 0;
		EraseMouse();
		hidden_cursor = 1;
	}
}

void
DGA_FrameBuf:: Show_Cursor(void)
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
DGA_FrameBuf:: Fade(int steps)
{
	static int      state = XFADE_IN;
	static XColor   orig_colors[NUM_COLORS];
	XColor          curr_colors[NUM_COLORS];
	static long     orig_truecolors[NUM_COLORS];
	int             i, j;
	const int       true_slowfactor=4;
	static Colormap *fade_cmap, new_cmap;

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
			/* Continue... */
			break;
	}

	/* Truecolor screens don't fade using a colormap */
	if ( truecolor ) {  /* Do it a different way... */
		Pixel_Fade(state);
		return;
	}

	/* Set the requested pixels */
	for ( i=0; i<NUM_COLORS; ++i )
		orig_colors[i].pixel = i;

	/* Make a copy of the default colormap and use that in fade */
	if ( state == XFADE_OUT ) {
		XQueryColors(display, colormap, orig_colors, NUM_COLORS);
		fade_cmap = &colormap;
	}

	memcpy(curr_colors, orig_colors, NUM_COLORS*sizeof(XColor));
	if ( state == XFADE_OUT ) {
		for ( i=steps-1; i >= 0; i-- ) {
			for (j = 0; j < NUM_COLORS; j++) {
				curr_colors[j].red   = 
					orig_colors[j].red   * i / steps;
				curr_colors[j].green = 
					orig_colors[j].green * i / steps;
				curr_colors[j].blue  = 
					orig_colors[j].blue  * i / steps;
			}
			XStoreColors (display, *fade_cmap, 
						curr_colors, NUM_COLORS);
			select_usleep(5);
			XSync (display, False);
		}
	} else {
		Refresh();
		for ( i=0; i < steps; i++ ) {
			for (j = 0; j < NUM_COLORS; j++) {
				curr_colors[j].red   = 
					orig_colors[j].red   * i / steps;
				curr_colors[j].green = 
					orig_colors[j].green * i / steps;
				curr_colors[j].blue  = 
					orig_colors[j].blue  * i / steps;
			}
			XStoreColors (display, *fade_cmap, 
						curr_colors, NUM_COLORS);
			select_usleep(5);
			XSync (display, False);
		}
	}
	
	if ( state == XFADE_IN ) {
		/* Restore the original colormap */
		XStoreColors (display, colormap, orig_colors, NUM_COLORS);
		XSync(display, False);
	}
}

/* Adapted thankfully from "DeathRoids Source!" by Augusto Roman!
   I think this code is broken -- See the SDL implementation for good code.
   (http://www.devolution.com/~slouken/SDL/)
 */
void
DGA_FrameBuf:: DGA_BankedBlit(int x0, int y0, int width, int height)
{
	char *vidaddr;
	char *memaddr;
	int   curr_bank, next_bank;
	int   i;

	/* Set the graphics offset: */
	x0 *= video_bpp;
	curr_bank = (y0/vlines);
	next_bank = vlines-(y0%vlines);
	vidaddr = vbase+(((y0%vlines)*vwidth)+x0);
	memaddr = (char *)shared_mem+((y0*pitch)+x0);

	XF86DGASetVidPage(display, DefaultScreen(display), curr_bank);
	width *= video_bpp;
	for ( i=height; i; --i ) {
		if ( next_bank-- == 0 ) {
			XF86DGASetVidPage(display, DefaultScreen(display),
								++curr_bank);
			vidaddr = vbase+x0;
			next_bank = (vpage/vwidth)-1;
		}
		memcpy(vidaddr, memaddr, width);
		memaddr += pitch;
		vidaddr += vwidth;
	}
}
void
DGA_FrameBuf:: DGA_LinearBlit(int x0, int y0, int width, int height)
{
	char *vidaddr;
	char *memaddr;

	/* Set the graphics offset: */
	x0 *= video_bpp;
	vidaddr = vbase+((y0*vwidth)+x0);
	memaddr = (char *)shared_mem+((y0*pitch)+x0);

	width *= video_bpp;
	while ( height-- ) {
		memcpy(vidaddr, memaddr, width);
		memaddr += pitch;
		vidaddr += vwidth;
	}
}
/* This code works with non-banked video cards as well, it's just potentially
   slower than it needs to be. :)  Normally, when the mouse is enabled, speed
   is not a prime consideration, so this routine doesn't need to be speedy.
*/
void
DGA_FrameBuf:: DGA_BlitBuf(int x0, int y0, int width, int height, char *memaddr)
{
	char *vidaddr;
	int   curr_bank, next_bank;
	int   i;

	/* Set the graphics offset: */
	x0 *= video_bpp;
	curr_bank = (y0/vlines);
	next_bank = vlines-(y0%vlines);
	vidaddr = vbase+(((y0%vlines)*vwidth)+x0);

	XF86DGASetVidPage(display, DefaultScreen(display), curr_bank);
	width *= video_bpp;
	for ( i=height; i; --i ) {
		if ( next_bank-- == 0 ) {
			XF86DGASetVidPage(display, DefaultScreen(display),
								++curr_bank);
			vidaddr = vbase+x0;
			next_bank = (vpage/vwidth)-1;
		}
		memcpy(vidaddr, memaddr, width);
		memaddr += width;
		vidaddr += vwidth;
	}
}

/* BTW, you _can_ pass negative x0 and y0 values to this function */
void
DGA_FrameBuf:: RefreshArea(int x0, int y0, int width, int height)
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

	area.x = x0;
	area.y = y0;
	area.width = width;
	area.height = height;
	Reefer->Add(&area);
}

void
DGA_FrameBuf:: Refresh(void)
{ 
	/* Don't refresh during fake pixel fade */
	if ( faded ) return;

	if ( mode_linear ) {
		DGA_LinearBlit(0, 0, WIDTH, HEIGHT);
	} else {
		DGA_BankedBlit(0, 0, WIDTH, HEIGHT);
	}
}

void
DGA_FrameBuf:: FlushEvents(void)
{
	int    numevents;
	XEvent event;

	for ( numevents=XPending(display); numevents > 0; --numevents )
		GetEvent(&event);
}
	
int
DGA_FrameBuf:: NumEvents(void)
{
	return(XPending(display));
}

void
DGA_FrameBuf:: GetEvent(XEvent *event)
{
	Flush(0);
	XNextEvent(display, event);

	/* Handle some events internally */
	switch (event->type) {
		case MotionNotify:
			MoveMouse(event->xmotion.x_root, event->xmotion.y_root,
									event);
			break;
		case ButtonPress:
			event->xbutton.x = event->xbutton.x_root = (int)mouseX;
			event->xbutton.y = event->xbutton.y_root = (int)mouseY;
			break;
		case ButtonRelease:
			event->xbutton.x = event->xbutton.x_root = (int)mouseX;
			event->xbutton.y = event->xbutton.y_root = (int)mouseY;
			break;
		case Expose:
			/* If there are no more Exposes, refresh completely */
			if ( event->xexpose.count == 0 )
				Refresh();
			event->type = 0;
			break;
		default:
			break;
	}
}

int
DGA_FrameBuf:: KeyToAscii(XEvent *event, char *buf, int buflen, KeySym *key)
{
	return(XLookupString(&event->xkey, buf, buflen, key, NULL));
}

/* Sorting routine for refresh area stack */
int sort_areas(DGA_FrameBuf::Area *item1, DGA_FrameBuf::Area *item2) {
	return(item1->y - item2->y);
}

void
DGA_FrameBuf:: Flush(int sync)
{
	int nreefs;
	Area *area;

#ifdef SORT_REFRESH
	Reefer->Sort(sort_areas);
#endif
	for ( nreefs = Reefer->Size(); nreefs; --nreefs ) {
		area = Reefer->Pop();	// This should never be NULL
		if ( mode_linear ) {
			DGA_LinearBlit(area->x,area->y,
					area->width,area->height);
		} else {
			DGA_BankedBlit(area->x,area->y,
					area->width,area->height);
		}
	}
	if ( showmouse )
		DrawMouse();
	Unused(sync);
}

void
DGA_FrameBuf:: get_color_info(unsigned long mask, unsigned long *shift, 
			       				unsigned long *prec)
{
  	*shift = 0;
  	*prec = 8;

	while (! (mask & 0x1)) {
		(*shift)++;
		mask >>= 1;
	}
	while (mask & 0x1) {
		(*prec)--;
		mask >>= 1;
	}
}

#endif /* Linux and use DGA */
