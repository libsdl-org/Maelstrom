
// The X11 graphics module:
//
// This module has been greatly enhanced by the content of "Xlib by Example",
// by Cui-Qinq Yang and Mahir S. Ali.


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "x11_framebuf.h"
#include "cursor.h"

#ifndef True
#define	True	1
#endif
#ifndef False
#define False	0
#endif

// Not in any header file...
extern "C" {
	extern int XShmQueryExtension(Display *dpy);
}

/* Shared memory error handler routine */
static int haderror;
static int (*origerrorhandler)(Display *, XErrorEvent *);
static int shmerrorhandler(Display *d, XErrorEvent *e)
{
	haderror++;
	if(e->error_code==BadAccess)
		error("X: Warning: failed to attach shared memory\n");
	else (*origerrorhandler)(d,e);
	return(0);
} 

X11_FrameBuf:: X11_FrameBuf(unsigned short width, unsigned short height,
			int bpp, int FullScreen, char *title, char **icon) :
						FrameBuf(width, height, bpp)
{
	XWindowAttributes    rootattr;
	XSetWindowAttributes winattr;
	XVisualInfo     vinfo_return;
	XGCValues       gcv;
	XKeyboardState  kbd_state;
	char           *window_Name = title;
        XTextProperty   windowName;
	XWMHints       *wmhints;
        XClassHint     *classhints;
        XSizeHints     *sizehints;
	XpmAttributes   attributes;
	Pixmap		icon_pixmap;
	Pixmap		cursor_pixmap;
	Pixmap		cursor_mask;
	Pixmap		nocursor_pixmap;
	Pixmap		nocursor_mask;
	XColor		black, white;
        unsigned long   valuemask=0L;
	int		XpmErrorStatus;
	XEvent          event;
	char           *argv[2] = { title, NULL };
	int             bits_per_pixel;

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
	colormap = DefaultColormap(display, DefaultScreen(display));
	private_cmap = 0;

#ifdef FORCE_NOSHM
	use_mitshm = 0;
#else
	/* Check to see if the extensions are supported */
	use_mitshm = XShmQueryExtension(display);
#endif

	/* Create the main windows */
	Black = BlackPixel(display, DefaultScreen(display));
	White = WhitePixel(display, DefaultScreen(display));
#ifdef PIXEL_DOUBLING
	width *= 2;
	height *= 2;
#endif
	if ( FullScreen ) {	// All Full-Screen initialization done here.
		fullscreen = 1;
		if ( XGetWindowAttributes(display, DefaultRootWindow(display),
							&rootattr) != True ) {
			error(
		"X: Couldn't figure out the size of the root window!\n");
			exit(3);
		}
		if ( (rootattr.width < width) || (rootattr.height < height) ) {
			error(
		"X: Root window is too small! -- Need %dx%d resolution.\n",
								width, height);
			exit(3);
		}
		root_width = rootattr.width;
		root_height = rootattr.height;
		TopWin = XCreateSimpleWindow(display,
				DefaultRootWindow(display),
			0, 0, root_width, root_height, 1, Black,Black);
		MainWin = XCreateSimpleWindow(display, TopWin,
				((root_width/2)-(width/2)),
				((root_height/2)-(height/2)),
					width, height, 1, Black, Black);
	} else {
		fullscreen = 0;
		MainWin = TopWin = XCreateSimpleWindow(display,
				DefaultRootWindow(display),
				0, 0, width, height, 1, Black, Black);
	}

	/* Get our shared screen memory */
	shared_len = (width * height * video_bpp);
	backbuf = new unsigned char[shared_len];
	if ( use_mitshm ) {
		shminfo.shmid = shmget(IPC_PRIVATE, shared_len, IPC_CREAT|0777);
		if ( shminfo.shmid < 0 ) {
			error("Warning: Couldn't get X shared memory.\n");
			use_mitshm = 0;
		}
	}
	if ( use_mitshm ) {
		/* We need to protect our shared memory against orphanage if we
		   are killed in this function.  */

		shminfo.shmaddr = (char *) shmat(shminfo.shmid, 0, 0);
		shared_mem = (unsigned char *) shminfo.shmaddr;
		if ( shminfo.shmaddr == (char *)-1 ) {
			error(
			"Warning: Couldn't attach to X shared memory.\n");
			shmctl(shminfo.shmid, IPC_RMID, NULL);
			use_mitshm = 0;
		}
		shminfo.readOnly = False;
	}

	/* Now try to attach it to the X Server */
	if ( use_mitshm ) {
		haderror = False;
		origerrorhandler = XSetErrorHandler(shmerrorhandler);
		XShmAttach (display, &shminfo);
		XSync(display,True);  /* wait for error or ok */
		XSetErrorHandler(origerrorhandler);

		/* After the X server attaches, remove the public ID */
		if ( shmctl(shminfo.shmid, IPC_RMID, NULL) < 0 )
			perror("X: Warning: shmctl rmid() error");

		if( haderror /* In shared memory attach procedure */ ){
			if ( shmdt(shminfo.shmaddr) < 0 )
				perror("X shmdt() error");
			shmctl(shminfo.shmid, IPC_RMID, NULL);
			use_mitshm = 0;
		}
	}
#ifdef FORCE_XSHM
	if ( ! use_mitshm ) {
		error("X: Unable to create MIT shared memory.\n");
		exit(3);
	}
#endif

	/* Create the shared memory video buffer */
	if ( use_mitshm ) {
		X_image = XShmCreateImage(display, vinfo_return.visual,
				bits_per_pixel, ZPixmap, shminfo.shmaddr,
						&shminfo, width, height);
	} else {
		shared_mem = new unsigned char[shared_len];
		X_image = XCreateImage(display, vinfo_return.visual,
				bits_per_pixel, ZPixmap, 0, 
					(char *)shared_mem, width, height,
						8*video_bpp, video_bpp*width);
		XSync(display,True);  /* wait for error or ok */
	}
	if ( ! X_image ) {
		error("X: Unable to create X image\n");
		exit(3);
	}
	Clear(0);

	/* Create the icon window, and give it the icon colormap */
	wmhints = XAllocWMHints();
	if ( icon ) {
		/* Allocate the pixmap for the icon */
		attributes.valuemask = XpmColormap;
		attributes.colormap = colormap;
        	XpmErrorStatus = XpmCreatePixmapFromData(display, MainWin, 
					icon, &icon_pixmap, NULL, &attributes);
		/* Ignore a failed pixmap creation */
		if ( XpmErrorStatus != XpmSuccess ) {
			error("Note: Couldn't create color icon.\n");
		}
		XpmFreeAttributes(&attributes);

		wmhints->icon_pixmap 	= icon_pixmap;
		wmhints->flags = IconPixmapHint;
	} else
		wmhints->flags = 0;

	/* Create the normal cursor (Mac cursor) */
	cursor_pixmap = XCreatePixmapFromBitmapData(display, MainWin, 
			cursor_bits, cursor_width, cursor_height, 1L, 0L, 1);
	cursor_mask = XCreatePixmapFromBitmapData(display, MainWin, 
			cursorm_bits, cursorm_width, cursorm_height, 1L, 0L, 1);
	black.red   = 0x0000;
	black.green = 0x0000;
	black.blue  = 0x0000;
	white.red   = 0xFFFF;
	white.green = 0xFFFF;
	white.blue  = 0xFFFF;
	cursor  = XCreatePixmapCursor(display, cursor_pixmap,
					cursor_mask, &black, &white, 0, 0);
	XFreePixmap(display, cursor_pixmap);
	XFreePixmap(display, cursor_mask);

	/* Create the hidden cursor */
	black.red   = 0;
	black.green = 0;
	black.blue  = 0;
	attributes.valuemask = (XpmColormap|XpmDepth);
	attributes.colormap = colormap;
	attributes.depth = 1;
	XpmCreatePixmapFromData(display, MainWin, invis_cursor_xpm, 
				&nocursor_pixmap, &nocursor_mask, &attributes);
	XpmFreeAttributes(&attributes);
	nocursor  = XCreatePixmapCursor(display, nocursor_pixmap,
					nocursor_mask, &black, &black, 0, 0);
	XFreePixmap(display, nocursor_pixmap);
	XFreePixmap(display, nocursor_mask);

	/* Set the cursor */
	hidden_cursor = 1;
	Show_Cursor();

	/* Various window manager settings */
        wmhints->initial_state   = NormalState;
        wmhints->input           = True;     
        wmhints->flags |= StateHint | InputHint;

	/* Set the class for this program */
	classhints = XAllocClassHint();
	classhints->res_name     = title;
	classhints->res_class    = title;

        /* Setup the max and minimum size that the window will be */
	sizehints = XAllocSizeHints();
        sizehints->flags         = PSize | PMinSize | PMaxSize;
	if ( fullscreen ) {
        	sizehints->min_width     = root_width;
        	sizehints->min_height    = root_height;
        	sizehints->max_width     = root_width;
        	sizehints->max_height    = root_height;
	} else {
        	sizehints->min_width     = width;
        	sizehints->min_height    = height;
        	sizehints->max_width     = width;
        	sizehints->max_height    = height;
	}

	/* Create the window/icon name properties */
        if (XStringListToTextProperty(&window_Name, 1, &windowName) == 0) {
		error("X: Cannot create window name resource!\n");
		exit(3);
	}

        /* Now set the window manager properties */
        XSetWMProperties(display, TopWin, &windowName, &windowName,
				argv, 1, sizehints, wmhints, classhints);
	XFree((char *)wmhints);
	XFree((char *)classhints);
	XFree((char *)sizehints);

#define TRANSIENT_X
#ifdef TRANSIENT_X
	/* If we are running full-screen, remove the window borders */
	if ( fullscreen ) {
		XSetTransientForHint(display, TopWin,
						DefaultRootWindow(display));
	}
#endif

  	/*  Enable the delete window protocol (taken from 'rxvt' -- Thanks!) */
	wm_del_win = XInternAtom(display,"WM_DELETE_WINDOW",False);
	XSetWMProtocols(display, TopWin, &wm_del_win, 1);
  
        /* Check if the server allows backing store */
        if (DoesBackingStore(XDefaultScreenOfDisplay(display)) == Always)
        {
                /* Ok we want backing store as it is very useful */
                valuemask |= CWBackingStore;
                winattr.backing_store = Always;
        }
	XChangeWindowAttributes(display, MainWin, valuemask, &winattr);

	/* Set up graphics contexts */
	gcv.graphics_exposures = False;
        if (!(gc = XCreateGC(display, MainWin, GCGraphicsExposures, &gcv))) {
		error("X: Couldn't create graphics context.\n");
		exit(3);
	}

	/* Turn off autorepeat */
	XGetKeyboardControl(display, &kbd_state);
	was_repeating = (kbd_state.global_auto_repeat != AutoRepeatModeOff);
	XAutoRepeatOff(display);

	/* Set up the events we wait for */
	if ( fullscreen ) {
		Event_mask =    KeyPressMask | KeyReleaseMask |
                        	ExposureMask | StructureNotifyMask;
		XSelectInput(display, TopWin, Event_mask);
	}
	Event_mask =    KeyPressMask | KeyReleaseMask |
                        ButtonPressMask | ButtonReleaseMask |
                        PointerMotionMask |
                        ExposureMask | StructureNotifyMask;
	XSelectInput(display, MainWin, Event_mask);


	/* Actually map the main window */
	if ( fullscreen )
		XMapWindow(display, TopWin);
	XMapWindow(display, MainWin);

	/* Loop, waiting for our window to be mapped */
	do {
		/* Get the next event */
		XNextEvent(display, &event);
	}
	while (event.type != MapNotify);

	/* We're up! */
	RefreshAll();
}

X11_FrameBuf:: ~X11_FrameBuf()
{
	XUnmapWindow(display, TopWin);
	XDestroyImage(X_image);
	XFreeCursor(display, cursor);
	XFreeCursor(display, nocursor);
	if ( ! use_mitshm )
		delete[] shared_mem;
	delete[] backbuf;
	XSync(display, False);
	if ( use_mitshm ) {
		XShmDetach (display, &shminfo);
		/* need server to detach so we can remove shm id */
		XSync(display, False);
		if ( shmdt(shminfo.shmaddr) < 0 )
			perror("X shmdt() error");
	}
	if ( was_repeating )
		XAutoRepeatOn(display);
	XCloseDisplay(display);
}

/* Just try to allocate a full colormap */
/* This code assumes a psuedo-color (256 color) display */
int
X11_FrameBuf:: Alloc_Cmap(Color Cmap[NUM_COLORS])
{
	int i, c, r, g, b;
	static int alloct[NUM_COLORS], nalloct=0;
	unsigned long pix;
	XColor xcol, cmap[NUM_COLORS];

	/* Most of the mapping code is adapted from 'xv' -- Thanks! :) */
	int ri, gi, bi;			/* The color shades we want */
	int rd, gd, bd;			/* The color shades we have */
	long mdist, close, d;		/* Distance and closest pixel */

	/* Truecolor displays can get their own "colormap" */
	if ( truecolor )
		return(Alloc_Private_Cmap(Cmap));

	if ( nalloct ) {  /* Free a previously allocated colormap */
		for ( i=0; i<NUM_COLORS; ++i ) {
			if ( alloct[i] ) {
				pix = i;
				XFreeColors(display, colormap, &pix, 1, 0L);
			}
		}
	}

	/* Shed light on a standard colormap, if we can */
	for ( i=0; i<NUM_COLORS; ++i )
		alloct[i] = 0;

	for ( r=0; r<6; ++r ) {
		for ( g=0; g<6; ++g ) {
			for ( b=0; b<6; ++b ) {
				xcol.red =   (((r*255)/5)<<8);
				xcol.green = (((g*255)/5)<<8);
				xcol.blue =  (((b*255)/5)<<8);
				xcol.flags = (DoRed|DoGreen|DoBlue);
				if ( XAllocColor(display, colormap, &xcol) ) {
					alloct[xcol.pixel]=1;
					++nalloct;
				}
			}
		}
	}
#ifdef DEBUG
error("Phase 1: nalloct = %d\n", nalloct);
#endif

	/* If we couldn't allocate an orthogonal plane of color */
	if ( (nalloct < (6*6*6)) && (nalloct > 128) ) {
		/* Free allocated colors and allocate a more limited range */
		for ( i=0; i<NUM_COLORS; ++i ) {
			if ( alloct[i] ) {
				pix = i;
				XFreeColors(display, colormap, &pix, 1, 0L);
				alloct[i]=0;
				--nalloct;
			}
		}
		for ( r=0; r<4; ++r ) {
			for ( g=0; g<8; ++g ) {
				for ( b=0; b<4; ++b ) {
					xcol.red =   (((r*255)/3)<<8);
					xcol.green = (((g*255)/7)<<8);
					xcol.blue =  (((b*255)/3)<<8);
					xcol.flags = (DoRed|DoGreen|DoBlue);
					if ( XAllocColor(display,
							colormap, &xcol) ) {
						alloct[xcol.pixel]=1;
						++nalloct;
					}
				}
			}
		}
	}
#ifdef DEBUG
error("Phase 2: nalloct = %d\n", nalloct);
#endif

	/* Find out what kind of colormap we have now */
	for ( i=0; i<NUM_COLORS; ++i ) {
		cmap[i].pixel = i;
		cmap[i].flags = (DoRed|DoGreen|DoBlue);
	}
	XQueryColors(display, colormap, cmap, NUM_COLORS);

	/* Map the whole colormap */
	for ( i=0; i<NUM_COLORS; ++i ) {
		mdist = 100000; close=0;

		ri = (Cmap[i].red >> 8);
		gi = (Cmap[i].green >> 8);
		bi = (Cmap[i].blue >> 8);

		/* Cycle through the colors we have */
		for ( c=0; c<NUM_COLORS; ++c ) {
			rd = ri - (cmap[c].red >> 8);
			gd = gi - (cmap[c].green >> 8);
			bd = bi - (cmap[c].blue >> 8);
			d = rd*rd + gd*gd + bd*bd;

			if ( d < mdist ) {
				mdist = d;
				close = c;
			}
		}
#ifdef DEBUG
error("Desired RGB (%d,%d,%d) mapped to existing RGB (%d,%d,%d)\n",
				ri, gi, bi, cmap[close].red>>8,
				cmap[close].green>>8, cmap[close].blue>>8);
#endif
		/* Lock down and register the color we want */
		if ( ! XAllocColor(display, colormap, &cmap[close]) ) {
#ifdef VERBOSE_ERRORS
			error(
	"X11_FrameBuf: Warning: couldn't allocate an existing color!\n");
#else
			;
#endif
		}
		Pixel_colors[i] = cmap[close].pixel;
		Color_Map[i].red   = cmap[close].red;
		Color_Map[i].green = cmap[close].green;
		Color_Map[i].blue  = cmap[close].blue;

		/* Find 'Black' */
		if ( ! cmap[close].red && ! cmap[close].green &&
							! cmap[close].blue )
			Black = cmap[close].pixel;
	}
	RefreshAll();
	return(NUM_COLORS);
}

/* Allocate a private colormap :) */
int
X11_FrameBuf:: Alloc_Private_Cmap(Color Cmap[NUM_COLORS])
{
	static int alloct=0;
	int i;
        XColor xcols[NUM_COLORS], white;

#if 0
	/* The world is black and white... */
	white.red = white.green = white.blue = 0;
	for (i = 0; i < NUM_COLORS; i++) {
		if ( !Cmap[i].red && !Cmap[i].green && !Cmap[i].blue )
			Black = i;
		if ( (white.red < Cmap[i].red) &&
				(white.green < Cmap[i].green) &&
						(white.blue < Cmap[i].blue) ) {
			white.red = Cmap[i].red;
			white.green = Cmap[i].green;
			white.blue = Cmap[i].blue;
			White = i;
		}
	}
#endif

	/* Oops, nope, it's color! :-) */
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
		if ( ! alloct ) {
			Clear(0);
			alloct = 1;
		}
		RefreshAll();
	} else {
		/* Create a custom visual colormap, if needed */
		if ( ! alloct ) {
			private_cmap = XCreateColormap(display, TopWin,
				DefaultVisual(display, XDefaultScreen(display)),
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
		}
		Clear(0);
		XStoreColors(display, private_cmap, xcols, NUM_COLORS);
		XInstallColormap(display, private_cmap);
		XSetWindowColormap(display, TopWin, private_cmap);
		RefreshAll();
	}
	return(NUM_COLORS);
}

void
X11_FrameBuf:: Hide_Cursor(void)
{
	if ( hidden_cursor ==  0 ) {
		XDefineCursor(display, TopWin, nocursor);
		hidden_cursor = 1;
	}
}

void
X11_FrameBuf:: Show_Cursor(void)
{
	if ( hidden_cursor == 1 ) {
		XDefineCursor(display, TopWin, cursor);
		hidden_cursor = 0;
	}
}


/* This function was adapted from 'xscreensaver', by Jamie Zawinski 

	-- Thanks! :)
*/
void
X11_FrameBuf:: Fade(int steps)
{
	static int           state = XFADE_IN;
	static XColor        orig_colors[NUM_COLORS];
	XColor               curr_colors[NUM_COLORS];
	int                  i, j;
	const int            true_slowfactor=4;
	static Colormap     *fade_cmap, new_cmap;

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
	if ( private_cmap ) {
		if ( state == XFADE_OUT ) {
			XQueryColors(display, private_cmap, orig_colors, 
								NUM_COLORS);
			fade_cmap = &private_cmap;
		}
	} else {
		XQueryColors(display, colormap, orig_colors, NUM_COLORS);
		if ( state == XFADE_OUT ) {
			new_cmap = XCreateColormap(display,
				DefaultRootWindow(display),
				DefaultVisual(display, XDefaultScreen(display)),
								AllocAll);
			XStoreColors(display,new_cmap,orig_colors,NUM_COLORS);
			XGrabServer(display);
			XInstallColormap(display, new_cmap);
    			XSetWindowColormap(display, TopWin, new_cmap);
			fade_cmap = &new_cmap;
		}
	}
	XSync(display, False);

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
		if ( private_cmap ) {
			XStoreColors (display, private_cmap, 
						orig_colors, NUM_COLORS);
		} else {
    			XSetWindowColormap(display, TopWin, colormap);
			XUngrabServer(display);
			XFreeColormap(display, *fade_cmap);
		}
		XSync(display, False);
	}
}

/* Refresh the toplevel window and the main window */
void
X11_FrameBuf:: RefreshAll(void)
{
	if ( fullscreen ) {
		/* Clear the background */
		XSetForeground(display, gc, Black);
		XFillRectangle(display, TopWin, gc, 0, 0,
						root_width, root_height);
		
		/* Draw a nifty white border */
		if ( (root_width >= (WIDTH+2)) &&
					(root_height >= (HEIGHT+2)) ) {
			XSetForeground(display, gc, White);
			XDrawRectangle(display, TopWin, gc, 
					((root_width/2)-(WIDTH/2))-1,
					((root_height/2)-(HEIGHT/2))-1,
							WIDTH+3, HEIGHT+3);
		}
	}
	Refresh();
}

/* BTW, you _can_ pass negative x0 and y0 values to this function */
void
X11_FrameBuf:: RefreshArea(int x0, int y0, int width, int height)
{
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

#ifdef PIXEL_DOUBLING
	x0 *= 2;
	y0 *= 2;
	width *= 2;
	height *= 2;
#endif
#ifdef FORCE_XSHM
	XShmPutImage(display, MainWin, gc, X_image, x0, y0, x0, y0,
             						width, height, False);
#else
	if ( use_mitshm) {
		XShmPutImage(display, MainWin, gc, X_image, x0, y0, x0, y0,
             						width, height, False);
	} else {
		XPutImage(display, MainWin, gc, X_image, x0, y0, x0, y0,
             							width, height);
	}
#endif /* FORCE_XSHM */
}

void
X11_FrameBuf:: Refresh(void)
{ 
	/* Don't refresh while faded */
	if ( faded ) return;

	/* Do it all! */
#ifdef PIXEL_DOUBLING
	WIDTH *= 2;
	HEIGHT *= 2;
#endif
#ifdef FORCE_XSHM
	XShmPutImage(display, MainWin, gc, X_image, 0, 0, 0, 0,
             						WIDTH, HEIGHT, False);
#else
	if ( use_mitshm) {
		XShmPutImage(display, MainWin, gc, X_image, 0, 0, 0, 0,
             						WIDTH, HEIGHT, False);
	} else {
		XPutImage(display, MainWin, gc, X_image, 0, 0, 0, 0,
             							WIDTH, HEIGHT);
	}
#endif /* FORCE_XSHM */
	XSync(display, False);
}

void
X11_FrameBuf:: FlushEvents(void)
{
	int    numevents;
	XEvent event;

	for ( numevents=XPending(display); numevents > 0; --numevents )
		GetEvent(&event);
}
	
int
X11_FrameBuf:: NumEvents(void)
{
	return(XPending(display));
}

void
X11_FrameBuf:: GetEvent(XEvent *event)
{
	XNextEvent(display, event);

	/* Handle some events internally */
	switch (event->type) {
		/* Are we being sent a message? */
		case ClientMessage:
			if ( event->xclient.format == 32 && 
					event->xclient.data.l[0] == wm_del_win )
			{ /* Window manager wants to delete us */
			  /* Assume that atexit() handles all cleanup */
				exit(0);
			}
			break;

		/* Are we iconified? */
		case UnmapNotify:
			{  /* Wait until we are un-iconified */
				int iconified = 1;
				while ( iconified ) {
					GetEvent(event);
					if ( event->type == MapNotify )
						iconified = 0;
				}
			}
			break;

		/* Cover yourself! :) */
		case Expose:
			/* If there are no more Exposes, refresh completely */
			if ( event->xexpose.count == 0 )
				RefreshAll();
			event->type = 0;
			break;
#ifdef PIXEL_DOUBLING
		/* Map the large screen back down to normal coordinates */
		case ButtonPress:
			event->xbutton.x /= 2;
			event->xbutton.y /= 2;
			break;
#endif
		default:
			break;
	}
}

int
X11_FrameBuf:: KeyToAscii(XEvent *event, char *buf, int buflen, KeySym *key)
{
	return(XLookupString(&event->xkey, buf, buflen, key, NULL));
}

void
X11_FrameBuf:: Flush(int sync)
{
	if ( sync )
		XSync(display, False);
	else
		XFlush(display);
}

void
X11_FrameBuf:: get_color_info(unsigned long mask, unsigned long *shift, 
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
