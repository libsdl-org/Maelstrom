/*
  maclib:  A companion library to SDL for working with Macintosh (tm) data
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
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

#include "SDL_ttf.h"

#include "../utils/ErrorBase.h"
#include "../screenlib/SDL_FrameBuf.h"

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

typedef struct MFont {
	char *name;
	int ptsize;
	struct FontHdr *header;		/* The NFNT header! */

	/* Variable-length tables */
        Uint16 *bitImage;	/* bitImage[rowWords][fRectHeight]; */
	Uint16 *locTable;	/* locTable[lastChar+3-firstChar]; */
	Sint16 *owTable;	/* owTable[lastchar+3-firstChar]; */ 

	/* The Raw Data */
	Uint8 *nfnt;

	/* TrueType font information */
	TTF_Font *font;
} MFont;

class FontServ : public ErrorBase {

public:
	/* The "fontfile" parameter should be a Macintosh Resource fork file
	   that contains FOND and NFNT information for the desired fonts, or
	   a TrueType font file.
	*/
	FontServ(FrameBuf *screen, const char *fontfile);
	virtual ~FontServ();
	
	/* The font returned by NewFont() should be freed with FreeFont() */
	MFont  *NewFont(const char *fontname, int ptsize);
	void FreeFont(MFont *font);

	/* Determine the final width/height of a text block (in pixels) */
	Uint16	TextWidth(const char *text, MFont *font, Uint8 style);
	Uint16	TextHeight(MFont *font);

	/* Returns a bitmap image filled with the requested text.
	   The text should be freed with FreeText() after it is used.
	 */
	SDL_Texture *TextImage(const char *text, MFont *font, Uint8 style,
						SDL_Color foreground);
	SDL_Texture *TextImage(const char *text, MFont *font, Uint8 style,
						Uint32 rgb) {
		SDL_Color foreground;

		foreground.r = (rgb >> 16) & 0xFF;
		foreground.g = (rgb >>  8) & 0xFF;
		foreground.b = (rgb >>  0) & 0xFF;
		return(TextImage(text, font, style, foreground));
	}
	SDL_Texture *TextImage(const char *text, MFont *font, Uint8 style,
						Uint8 R, Uint8 G, Uint8 B) {
		SDL_Color foreground;

		foreground.r = R;
		foreground.g = G;
		foreground.b = B;
		return(TextImage(text, font, style, foreground));
	}
	void FreeText(SDL_Texture *text);

	/* Returns NULL if everything is okay, or an error message if not */
	char *Error(void) {
		return(errstr);
	}

private:
	FrameBuf *screen;
};

#endif /* _fontserv_h */
