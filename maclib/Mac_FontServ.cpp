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

/* The Macintosh Font Server module */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "SDL_types.h"
#include "bitesex.h"
#include "../utils/files.h"
#include "Mac_FontServ.h"

#define copy_short(S, D)	memcpy(&S, D, 2); D += 2;
#define copy_long(L, D)		memcpy(&L, D, 4); D += 4;

/* The structure of the Macintosh 'FOND' resource */
struct Font_entry {
	Uint16	size;
	Uint16	style;
	Uint16	ID;
};
struct FOND {
	Uint16	flags;
	Uint16	ID;
	Uint16	firstCH;
	Uint16	lastCH;
	Uint16	MaxAscent;	/* Maximum Font Ascent */
	Uint16	MaxDescent;	/* Maximum Font Descent */
	Uint16	MaxLead;	/* Maximum Font Leading */
	Uint16	MaxWidth;	/* Maximum Font Glyph Width */
	Uint32	WidthOff;	/* Width table offset */
	Uint32	KernOff;	/* Kerning table offset */
	Uint32	StyleOff;	/* Style mapping table offset */
	Uint16	StyleProp[9];	/* 9 Style Properties */
	Uint32	Intl_info;	/* International script info */
	Uint16	Version;	/* The version of the FOND resource */

	/* The Font Association Table */
	Uint16	num_fonts;	/* Number of fonts in table - 1 */
#ifdef SHOW_VARLENGTH_FIELDS
	struct Font_entry nfnts[0];
#endif

	/* The Offset Table */
	/* The Bounding Box Table */
	/* The Glyph Width Table */
	/* The Style Mapping Table */
	/* The Kerning Table */
};


static TTF_Font *GetTrueTypeFont(const char *name, int ptsize)
{
	char file[128];
	SDL_RWops *src;

	SDL_snprintf(file, sizeof(file), "Fonts/%s.ttf", name);
	src = OpenRead(file);
	if (src) {
		return TTF_OpenFontRW(src, 1, ptsize);
	}
	return NULL;
}

static Uint8 *GetFontData(const char *type, int ID)
{
	char file[128];
	SDL_snprintf(file, sizeof(file), "Fonts/Maelstrom_%s#%d", type, ID);
	return (Uint8 *)LoadFile(file);
}

FontServ::FontServ(FrameBuf *_screen, const char *fontfile) : ErrorBase()
{
	screen = _screen;
	TTF_Init();
}

FontServ::~FontServ()
{
	TTF_Quit();
}


MFont *
FontServ::NewFont(const char *fontname, int ptsize)
{
	static struct {
		const char *fontname;
		int ID;
	} fonts[] = {
		{ "Chicago", 0 },
		{ "New York", 2 },
		{ "Geneva", 3 },
	};

	Uint8 *fond, *data;
	struct FOND Fond;
	struct Font_entry Fent;
	int nchars;		/* number of chars including 'missing char' */
	int nwords;		/* bit image size, in words */
	int i, swapfont;
	MFont *font;

	font = new MFont;
	SDL_zerop(font);

	/* See if this is a TrueType font */
	font->font = GetTrueTypeFont(fontname, ptsize);
	if ( font->font ) {
		/* That was easy :) */
		return font;
	}

	/* Get the font family */
	fond = NULL;
	for (i = 0; i < (int)SDL_arraysize(fonts); ++i) {
		if (SDL_strcmp(fontname, fonts[i].fontname) == 0) {
			fond = GetFontData("FOND", fonts[i].ID);
			break;
		}
	}
	if ( fond == NULL ) {
		delete font;
		SetError("Warning: Font family '%s' not found", fontname);
		return(NULL);
	}
	data = fond;

	/* Find out what font ID we need */
        copy_short(Fond.flags, data);
        copy_short(Fond.ID, data);
        copy_short(Fond.firstCH, data);
        copy_short(Fond.lastCH, data);
	copy_short(Fond.MaxAscent, data);
	copy_short(Fond.MaxDescent, data);
	copy_short(Fond.MaxLead, data);
	copy_short(Fond.MaxWidth, data);
	copy_long(Fond.WidthOff, data);
	copy_long(Fond.KernOff, data);
	copy_long(Fond.StyleOff, data);
	memcpy(Fond.StyleProp, data, 18); data += 18;
	copy_long(Fond.Intl_info, data);
	copy_short(Fond.Version, data);
	copy_short(Fond.num_fonts, data);
	bytesex16(Fond.num_fonts);
	++Fond.num_fonts;
	for (i=0; i<Fond.num_fonts; ++i, data += sizeof(struct Font_entry)) {
		memcpy(&Fent, data, sizeof(Fent));
		byteswap((Uint16 *)&Fent, 3);
		if ( (Fent.size == ptsize) && ! Fent.style )
			break;
	} 
	SDL_free(fond);
	if ( i == Fond.num_fonts ) {
		delete font;
		SetError(
		"Warning: Font family '%s' doesn't have %d pt fonts",
							fontname, ptsize);
		return(NULL);
	}

	/* Now, Fent.ID is the ID of the correct NFNT resource */
	size_t size = SDL_strlen(fontname)+1;
	font->name = new char[size];
	SDL_strlcpy(font->name, fontname, size);
	font->ptsize = ptsize;
	font->nfnt = GetFontData("NFNT", Fent.ID);
	if ( font->nfnt == NULL ) {
		delete font;
		SetError(
"Warning: Can't find NFNT resource for %d pt %s font", ptsize, fontname);
		return(NULL);
	}

	/* Now that we have the resource, fiddle with the font structure
	   so we can use it.  (Code taken from 'mac2bdf' -- Thanks! :)
	 */
	font->header = (struct FontHdr *)font->nfnt;
	if ( ((font->header->fontType & ~3) != PROPFONT) &&
			((font->header->fontType & ~3) != FIXEDFONT) ) {
		swapfont = 1;
	} else {
		swapfont = 0;
	}
	if ( swapfont ) {
		byteswap((Uint16 *)font->header,
				sizeof(struct FontHdr)/sizeof(Uint16));
	}

	/* Check magic number.
	   The low two bits are masked off; newer versions of the Font Manager
	   use these to indicate the presence of optional 'width' and 'height'
	   tables, which are for fractional character spacing (unused).
	 */
	if ( ((font->header->fontType & ~3) != PROPFONT) &&
			((font->header->fontType & ~3) != FIXEDFONT) ) {
		SetError("Warning: Bad font Magic number: 0x%04x", 
						(font->header)->fontType);
		delete font;
		return(NULL);
	}
	nchars= ((font->header)->lastChar - (font->header)->firstChar + 1) + 1;
		/* One extra for "missing character image" */
	nwords= (font->header)->rowWords * (font->header)->fRectHeight;
	
	/* Read the tables.  They follow sequentially in the resource */
	font->bitImage = (Uint16 *)(font->nfnt+sizeof(*font->header));
	font->locTable = (Uint16 *)(font->bitImage+nwords);
	font->owTable = (Sint16 *)(font->locTable+nchars+1);
	
	/* Note -- there may be excess data at the end of the resource
	   (the optional width or height tables) */
	
	/* Byteswap the tables */
	if ( swapfont ) {
		byteswap(font->bitImage, nwords);
		byteswap(font->locTable, nchars+1);
		byteswap((Uint16 *)font->owTable, nchars);
	}

	return(font);
}

void
FontServ::FreeFont(MFont *font)
{
	if ( font->font ) {
		TTF_CloseFont(font->font);
	}
	if ( font->name ) {
		delete[] font->name;
	}
	if ( font->nfnt ) {
		SDL_free(font->nfnt);
	}
	delete font;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define HiByte(word)		((word>>8)&0xFF)
#define LoByte(word)		(word&0xFF)

/* The width of the specified text in pixels when displayed with the 
   specified font and style.
*/
Uint16
FontServ::TextWidth(const char *text, MFont *font, Uint8 style)
{
	int nchars, i;
	int space_width;	/* The width of the whole character */
	int extra_width;	/* Stylistic width */
	Uint16 Width;

	if ( font->font ) {
		int w = 0, h = 0;

		TTF_SizeText(font->font, text, &w, &h);
		return w;
	}

	switch (style) {
		case STYLE_NORM:	extra_width = 0;
					break;
		case STYLE_BOLD:	extra_width = 1;
					break;
		case STYLE_ULINE:	extra_width = 0;
					break;
		default:		return(0);
	}
	nchars = SDL_strlen(text);

	Width = 0;
	for ( i = 0; i < nchars; ++i ) {
		/* check to see if this character is defined */
		if (font->owTable[(Uint8)text[i]] <= 0)
			continue;
		
		space_width = LoByte(font->owTable[(Uint8)text[i]]);
#ifdef WIDE_BOLD
		Width += (space_width+extra_width);
#else
		Width += space_width;
#endif
	}
	return(Width);
}
Uint16
FontServ::TextHeight(MFont *font)
{
	if ( font->font ) {
		return TTF_FontHeight(font->font);
	}
	return((font->header)->fRectHeight);
}

/* Get/Set bit i of a scan line */
#define GETBIT(scanline, i) \
		((scanline[(i)/16] >> (15 - (i)%16)) & 1)

SDL_Texture *
FontServ::TextImage(const char *text, MFont *font, Uint8 style, SDL_Color fg)
{
	int width, height;
	SDL_Texture *image;
	Uint32 *bitmap;
	Uint32 color;
	int nchars;
	int bit_offset;		/* The current bit offset into a scanline */
	int space_width;	/* The width of the whole character */
	int space_offset;	/* The offset into the character of glyph */
	int glyph_line_offset;	/* The offset into scanline of glyph */
	int glyph_width;	/* The width of brushed glyph */
	int bold_offset, boldness;
	int ascii, i, y;
	int bit;

	if ( font->font ) {
		SDL_Color white = { 0xff, 0xff, 0xff, 0xff };
		SDL_Surface *surface;

		switch (style) {
		case STYLE_NORM:
			TTF_SetFontStyle(font->font, TTF_STYLE_NORMAL);
			break;
		case STYLE_BOLD:
			TTF_SetFontStyle(font->font, TTF_STYLE_BOLD);
			break;
		case STYLE_ULINE:
			TTF_SetFontStyle(font->font, TTF_STYLE_UNDERLINE);
			break;
		case STYLE_ITALIC:
			TTF_SetFontStyle(font->font, TTF_STYLE_ITALIC);
			break;
		default:		SetError(
					"FontServ: Unknown text style!");
					return(NULL);
		}

		surface = TTF_RenderText_Blended(font->font, text, white);
		if (!surface) {
			SetError(SDL_GetError());
			return(NULL);
		}

		image = screen->LoadImage(surface);
		SDL_FreeSurface(surface);
		SDL_SetTextureBlendMode(image, SDL_BLENDMODE_BLEND);
		SDL_SetTextureColorMod(image, fg.r, fg.g, fg.b);
		return image;
	}

	switch (style) {
		case STYLE_NORM:	bold_offset = 0;
					break;
		case STYLE_BOLD:	bold_offset = 1;
					break;
		case STYLE_ULINE:	bold_offset = 0;
					break;
		case STYLE_ITALIC:	SetError(
					"FontServ: Italics not implemented!");
					return(NULL);
		default:		SetError(
					"FontServ: Unknown text style!");
					return(NULL);
	}

	/* Notes on the tables.
	
	   Table 'bits' contains a bitmap image of the entire font.
	   There are fRectHeight rows, each rowWords long.
	   The high bit of a word is leftmost in the image.
	   The characters are placed in this image in order of their
	   ASCII value.  The last image is that of the "missing
	   character"; every Mac font must have such an image
	   (traditionally a maximum-sized block).
	   
	   The location table (loctab) and offset/width table (owtab)
	   have one entry per character in the range firstChar..lastChar,
	   plus two extra entries: one for the "missing character" image
	   and a terminator.  They describe, respectively, where to
	   find the character in the bitmap and how to interpret it with
	   respect to the "character origin" (pen position on the base
	   line).
	   
	   The location table entry for a character contains the bit (!)
	   offset of the start of its image data in the font's bitmap.
	   The image data's width is computed by subtracting the start
	   from the start of the next character (hence the terminator).
	   
	   The offset/width table contains -1 for undefined characters;
	   for defined characters, the high byte contains the character
	   offset (distance between left of character image and
	   character origin), and the low byte contains the character
	   width (distance between the character origin and the origin
	   of the next character on the line).
	 */
	
	/* Figure out how big the text image will be */
	width = TextWidth(text, font, style);
	if ( width == 0 ) {
		SetError("No text to convert");
		return(NULL);
	}
	height = (font->header)->fRectHeight;

	/* Allocate the text pixels */
	bitmap = new Uint32[width*height];
	memset(bitmap, 0, width*height*sizeof(Uint32));
	color = screen->MapRGB(fg.r, fg.g, fg.b);

	/* Print the individual characters */
	/* Note: this could probably be optimized.. eh, who cares. :) */
	nchars = SDL_strlen(text);
	for ( boldness=0; boldness <= bold_offset; ++boldness ) {
		bit_offset=0;
		for ( i = 0; i < nchars; ++i ) {
			/* check to see if this character is defined */
			/* According to the above comment, we should */
			/* check if the table contains -1, but this  */
			/* change seems to fix a SIGSEGV that would  */
			/* otherwise occur in some cases.            */
			if (font->owTable[(Uint8)text[i]] <= 0)
				continue;

			space_width = LoByte(font->owTable[(Uint8)text[i]]);
			space_offset = HiByte(font->owTable[(Uint8)text[i]]);
			ascii = (Uint8)text[i] - (font->header)->firstChar;
			glyph_line_offset = font->locTable[ascii]; 
			glyph_width = (font->locTable[ascii+1] -
							font->locTable[ascii]);
			for ( y=0; y<height; ++y ) {
				int     dst_offset;
				Uint16 *src_scanline;
			
				dst_offset = (y*width+bit_offset+space_offset);
				src_scanline = font->bitImage + 
						y*(font->header)->rowWords;
				for ( bit = 0; bit<glyph_width; ++bit ) {
					bitmap[dst_offset+bit+boldness] |=
				  		GETBIT(src_scanline, glyph_line_offset+bit)*0xFFFFFFFF;
				}
			}
#ifdef WIDE_BOLD
			bit_offset += (space_width+bold_offset);
#else
			bit_offset += space_width;
#endif
		}
	}
	if ( (style&STYLE_ULINE) == STYLE_ULINE ) {
		y = (height-(font->header)->descent+1);
		bit_offset = (y*width);
		for ( bit=0; bit<width; ++bit )
			bitmap[bit_offset++] = 0xFFFFFFFF;
	}

	/* Create the image */
	image = screen->LoadImage(width, height, bitmap);
	delete[] bitmap;
	SDL_SetTextureBlendMode(image, SDL_BLENDMODE_BLEND);
	SDL_SetTextureColorMod(image, fg.r, fg.g, fg.b);

	return(image);
}

void
FontServ::FreeText(SDL_Texture *text)
{
	screen->FreeImage(text);
}
