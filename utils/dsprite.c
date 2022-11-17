
#include <stdio.h>
#include <vga.h>
#include <vgagl.h>

#include "colortable.h"
#include "Sprite.h"

#define GR_MODE G640x480x64K
main()
{
	GraphicsContext *Screen, *Virtual;
	unsigned char pixel, red, green, blue;
	Sprite sprite;
	int i, j, offset;

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
	for ( i=0; i<640; ++i ) {
		for ( j=0; j<480; ++j )
			gl_setpixelrgb(i, j, 0, 0, 0);
	}

	/* Display the picture */
	fread(&sprite.width, sizeof(sprite.width), 1, stdin);
	fread(&sprite.height, sizeof(sprite.width), 1, stdin);
	sprite.pixels = (unsigned char *)malloc(sprite.width*sprite.height);
	fread(sprite.pixels, 1, sprite.width*sprite.height, stdin);
	sprite.mask = (unsigned char *)malloc((sprite.width/8)*sprite.height);
	fread(sprite.mask, 1, (sprite.width/8)*sprite.height, stdin);

	fprintf(stderr, "Reading sprite %dx%d\n", sprite.width, sprite.height);
	for ( i=0; i<sprite.height; ++i ) {
		for ( j=0; j<sprite.width; ++j ) {
			offset = ((i*sprite.width)+j);
			if ((sprite.mask[offset/8]>>(7-(offset%8))) & 0x01) {
				pixel = sprite.pixels[(i*sprite.width)+j];
				red = full_colors[pixel].red;
				green = full_colors[pixel].green;
				blue = full_colors[pixel].blue;
				gl_setpixelrgb(j+(640-sprite.width)/2, 
						i+(480-sprite.height)/2, 
							red, green, blue);
			}
		}
	}
	gl_copyscreen(Screen);
	sleep(5);
	vga_setmode(TEXT);
}
