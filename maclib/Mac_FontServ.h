/*
    MACLIB:  A companion library to SDL for working with Macintosh (tm) data
    Copyright (C) 1997  Sam Lantinga

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    5635-34 Springhouse Dr.
    Pleasanton, CA 94588 (USA)
    slouken@devolution.com
*/

#ifndef _fontserv_h
#define _fontserv_h

/* This documents the FontServ C++ class */

/* The FontServ takes a Macintosh NFNT resource file and extracts
   the fonts for use with any blit type display.  Specifically 
   designed for the Maelstrom port to Linux. :)

   The specs for the NFNT resource were finally understood by looking
   at mac2bdf, written by Guido van Rossum

	-Sam Lantinga			(slouken@devolution.com)
*/

#include <stdio.h>
#include <stdarg.h>

#include "Mac_Resource.h"
#include "SDL_FrameBuf.h"

/* Different styles supported by the font server */
#define STYLE_NORM	0x00
#define STYLE_BOLD	0x01
#define STYLE_ULINE	0x02
#define STYLE_ITALIC	0x04		/* Unimplemented */

#define WIDE_BOLD	/* Bold text is widened porportionally */

/* Macintosh font magic numbers */
#define PROPFONT	0x9000
#define FIXEDFONT	0xB000

/* Lay-out of a Font Record header */
struct FontHdr {
       	Uint16 fontType;  /* PROPFONT or FIXEDFONT */
       	Sint16 firstChar,
               lastChar,
               widMax,
               kernMax,         /* Negative of max kern */
               nDescent;        /* negative of descent */
        Uint16 fRectWidth,
	       fRectHeight,
               owTLoc,          /* Offset in words from itself to
				   the start of the owTable */
               ascent,
               descent,
               leading,
               rowWords;        /* Row width of bit image in words */
};

typedef struct {
	struct FontHdr *header;		/* The NFNT header! */

	/* Variable-length tables */
        Uint16 *bitImage;	/* bitImage[rowWords][fRectHeight]; */
	Uint16 *locTable;	/* locTable[lastChar+3-firstChar]; */
	Sint16 *owTable;	/* owTable[lastchar+3-firstChar]; */ 

	/* The Raw Data */
	Mac_ResData *nfnt;
} MFont;

class FontServ {

public:
	/* The "fontfile" parameter should be a Macintosh Resource fork file
	   that contains FOND and NFNT information for the desired fonts.
	*/
	FontServ(const char *fontfile);
	~FontServ();
	
	/* The font returned by NewFont() should be delete'd */
	MFont  *NewFont(const char *fontname, int ptsize);

	/* Determine the final width/height of a text block (in pixels) */
	Uint16	TextWidth(const char *text, MFont *font, Uint8 style);
	Uint16	TextHeight(MFont *font);

	/* Returns a bitmap image filled with the requested text.
	   The text should be freed with FreeText() after it is used.
	 */
	SDL_Surface *TextImage(const char *text, MFont *font, Uint8 style,
				SDL_Color background, SDL_Color foreground);
	SDL_Surface *TextImage(const char *text, MFont *font, Uint8 style,
						Uint8 R, Uint8 G, Uint8 B) {
		SDL_Color background = { 0xFF, 0xFF, 0xFF, 0 };
		SDL_Color foreground;

		foreground.r = R;
		foreground.g = G;
		foreground.b = B;
		return(TextImage(text, font, style, foreground, background));
	}
	void FreeText(SDL_Surface *text);

	/* Inverts the color of the text image */
	int InvertText(SDL_Surface *text);

	/* Returns NULL if everything is okay, or an error message if not */
	char *Error(void) {
		return(errstr);
	}

private:
	Mac_Resource *fontres;
	int text_allocated;

	/* Useful for getting error feedback */
	void SetError(char *fmt, ...) {
		va_list ap;

		va_start(ap, fmt);
		vsprintf(errbuf, fmt, ap);
		va_end(ap);
		errstr = errbuf;
	}
	char *errstr;
	char  errbuf[BUFSIZ];
};

#endif /* _fontserv_h */
