
#include <stdio.h>
#include <vga.h>
#include <vgagl.h>

#include "colortable.h"
#include "../bytesex.h"

#define Colors_64K
#define GR_MODE G640x480x64K
main()
{
	GraphicsContext *Screen, *Virtual;
	unsigned char row[32];
	unsigned short width, height;
	unsigned char red, green, blue;
	unsigned char c;
	int i, j;

        /* Set the video into graphics mode */
        vga_setmode(GR_MODE);
        /* Get the context for the virtual screen */
        gl_setcontextvgavirtual(GR_MODE);
        Virtual = gl_allocatecontext();
        gl_getcontext(Virtual);
        /* Get the context for the real screen */
        gl_setcontextvga(GR_MODE);
        Screen = gl_allocatecontext();
        gl_getcontext(Screen);
	gl_setrgbpalette();
        /* Now switch to the virtual context... */
        gl_setcontext(Virtual);

	/* Clear the screen */
	for ( i=0; i<640; ++i )
		for ( j=0; j<480; ++j )
			gl_setpixelrgb(i, j, 0, 0, 0);

	/* Display the picture */
	fread(&width, sizeof(unsigned short), 1, stdin);
	bytesexs(width);
	fread(&height, sizeof(unsigned short), 1, stdin);
	bytesexs(height);
printf("Width = %hu, Height = %hu\n", width, height);
	for ( i=0; i<height; ++i ) {
		for ( j=0; j<width; ++j ) {
			c = (unsigned char)fgetc(stdin);
			gl_setpixelrgb(j+(640-width)/2, i+(480-height)/2,
							full_colors[c].red>>8, 
							full_colors[c].green>>8,
							full_colors[c].blue>>8);
		}
	}
	gl_copyscreen(Screen);
	sleep(5);
	vga_setmode(TEXT);
}
