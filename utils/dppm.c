
#include <stdio.h>
#include <vga.h>
#include <vgagl.h>

#define Colors_64K
#define GR_MODE G640x480x64K
main()
{
	GraphicsContext *Screen, *Virtual;
	unsigned char row[32];
	unsigned int width, height;
	unsigned char red, green, blue;
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
	for ( i=0; i<320; ++i )
		for ( j=0; j<200; ++j )
			gl_setpixelrgb(i, j, 0, 0, 0);

	/* Display the picture */
	scanf("P6 %u %u %d ", &width, &height, &i);
	for ( i=0; i<height; ++i ) {
		for ( j=0; j<width; ++j ) {
			fread(&red, 1, 1, stdin);
			fread(&green, 1, 1, stdin);
			fread(&blue, 1, 1, stdin);
			gl_setpixelrgb(j+(640-width)/2, i+(480-height)/2,
							red, green, blue);
		}
	}
	gl_copyscreen(Screen);
	pause();
}
