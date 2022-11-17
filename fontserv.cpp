
/* The Maelstrom Font Server module */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "bitesex.h"
#include "fontserv.h"

#define copy_short(S, D)	memcpy(&S, D, 2); D += 2;
#define copy_long(L, D)		memcpy(&L, D, 4); D += 4;

/* The structure of the Macintosh 'FOND' resource */
struct Font_entry {
	unsigned short	size;
	unsigned short	style;
	unsigned short	ID;
	};
struct FOND {
	unsigned short	flags;
	unsigned short	ID;
	unsigned short	firstCH;
	unsigned short	lastCH;
	unsigned short	MaxAscent;	/* Maximum Font Ascent */
	unsigned short	MaxDescent;	/* Maximum Font Descent */
	unsigned short	MaxLead;	/* Maximum Font Leading */
	unsigned short	MaxWidth;	/* Maximum Font Glyph Width */
	unsigned long	WidthOff;	/* Width table offset */
	unsigned long	KernOff;	/* Kerning table offset */
	unsigned long	StyleOff;	/* Style mapping table offset */
	unsigned short	StyleProp[9];	/* 9 Style Properties */
	unsigned long	Intl_info;	/* International script info */
	unsigned short	Version;	/* The version of the FOND resource */

	/* The Font Association Table */
	unsigned short	num_fonts;	/* Number of fonts in table - 1 */
#ifdef SHOW_VARLENGTH_FIELDS
	struct Font_entry nfnts[0];
#endif

	/* The Offset Table */
	/* The Bounding Box Table */
	/* The Glyph Width Table */
	/* The Style Mapping Table */
	/* The Kerning Table */
};


FontServ:: FontServ(char *fontfile)
{
	fontres = new Mac_Resource(fontfile);
	if ( ! (num_fonds=fontres->get_num_resources("FOND")) ) {
		error("FontServ: No 'FOND' resources in %s\n", fontfile);
		exit(255);
	}
	fond_array = new short[num_fonds];
	fontres->get_resource_ids("FOND", fond_array);
}

FontServ:: ~FontServ()
{
	delete[] fond_array;
	delete   fontres;
}


MFont *
FontServ:: New_Font(char *fontname, int ptsize)
{
	Mac_ResData fond;
	MFont *font;
	struct FOND  Fond;
	struct Font_entry *Fent=NULL;
	unsigned char  *data;
	int nchars;		/* number of chars including 'missing char' */
	int nwords;		/* bit image size, in words */
	int    i;

	/* Get the font family */
	for ( i=0; i<num_fonds; ++i ) {
		if ( strcmp(fontname, fontres->
			get_resource_name("FOND", fond_array[i])) == 0 ) {
			/* We got the right font family  :-) */
			break;
		}
	}
	if ( fontres->get_resource("FOND", fond_array[i], &fond) < 0 ) {
		error("Warning: Font family '%s' not found\n", fontname);
		return(NULL);
	}

	/* Find out what font ID we need */
	data = fond.data;
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
	bytesexs(Fond.num_fonts);
	++Fond.num_fonts;
	for (i=0; i<Fond.num_fonts; ++i, data += sizeof(struct Font_entry)) {
		Fent=(struct Font_entry *)data;
		byteswap((unsigned short *)Fent, 3);
		if ( (Fent->size == ptsize) && ! Fent->style )
			break;
	} 
	if ( ! Fent ) {
		error(
"Warning: Font family '%s' has no fonts installed\n", fontname);
		return(NULL);
	}
	if ( i == Fond.num_fonts ) {
		error(
"Warning: Font family '%s' doesn't have %d pt fonts\n", fontname, ptsize);
		return(NULL);
	}

	/* Now, Fent->ID is the ID of the correct NFNT resource */
	font = new MFont;
	if ( fontres->get_resource("NFNT", Fent->ID, &font->nfnt) < 0 ) {
		error(
"Warning: Can't find NFNT resource for %d pt %s font\n", ptsize, fontname);
		return(NULL);
	}

	/* Now that we have the resource, fiddle with the font structure
	   so we can use it.  (Code taken from 'mac2bdf' -- Thanks! :)
	 */
	font->header = (struct FontHdr *)font->nfnt.data;
	byteswap((unsigned short *)font->header, 
				sizeof(struct FontHdr)/sizeof(short));

	/* Check magic number.
	   The low two bits are masked off; newer versions of the
	   Font Manager use these to indicate the presence of optional
	   'width' and 'height' tables.  These are for fractional
	   character spacing, which I don't see how to use in bdf format
	   anyway. */
	if (((font->header)->fontType & ~3) != PROPFONT &&
		((font->header)->fontType & ~3) != FIXEDFONT) {
		error("Warning: Bad font Magic number: 0x%04x\n", 
						(font->header)->fontType);
		delete font->nfnt.data;
		delete font;
		return(NULL);
	}
	nchars= ((font->header)->lastChar - (font->header)->firstChar + 1) + 1;
		/* One extra for "missing character image" */
	nwords= (font->header)->rowWords * (font->header)->fRectHeight;
	
	/* Read the tables.  They follow sequentially in the resource */
	font->bitImage = (unsigned short *)
			(font->nfnt.data+sizeof(*font->header));
	font->locTable = (unsigned short *)(font->bitImage+nwords);
	font->owTable = (short *)(font->locTable+nchars+1);
	
	/* Note -- there may be excess data at the end of the resource
	   (the optional width or height tables) */
	
	/* Byteswap the tables */
	byteswap(font->bitImage, nwords);
	byteswap(font->locTable, nchars+1);
	//byteswap((unsigned short *)font->owTable, nchars+1);
	byteswap((unsigned short *)font->owTable, nchars);

	delete[] fond.data;
	return(font);
}

void
FontServ:: Free_Font(MFont *font)
{
	delete font->nfnt.data;
	delete font;
}

void
FontServ:: Free_Text(BitMap *text)
{
	delete[] text->bits;
	delete   text;
}
	
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#define HiByte(word)		((word>>8)&0xFF)
#define LoByte(word)		(word&0xFF)
#define Pixel2Byte(Pwidth)	(Pwidth/8 + ((Pwidth%8 ? 1 : 0)))

/* The width of the specified text in pixels when displayed with the 
   specified font and style.
*/
int
FontServ:: TextWidth(char *text, MFont *font, unsigned short style)
{
	int	nchars, i;
	int	space_width;	/* The width of the whole character */
	int	extra_width;	/* Stylistic width */
	int	Width=0;

	switch (style) {
		case STYLE_NORM:	extra_width = 0;
					break;
		case STYLE_BOLD:	extra_width = 1;
					break;
		case STYLE_ULINE:	extra_width = 0;
					break;
		default:		return(0);
	}
	nchars = strlen(text);

	for ( i = 0; i < nchars; ++i ) {
		/* check to see if this character is defined */
		if (font->owTable[(unsigned char)text[i]] == -1)
			continue;
		
		space_width = LoByte(font->owTable[(unsigned char)text[i]]);
#ifdef WIDE_BOLD
		Width += (space_width+extra_width);
#else
		Width += space_width;
#endif
	}
	return(Width);
}

/* Get/Set bit i of a scan line */
#define GETBIT(scanline, i) \
		((scanline[(i)/16] >> (15 - (i)%16)) & 1)
#define SETBIT(scanline, i, bit) \
		(scanline[(i)/8] |= bit << (7 - (i)%8))

BitMap *
FontServ:: Text_to_BitMap(char *text, MFont *font, unsigned short style)
{
	BitMap *bitmap;
	int	nchars;
	int	bit_offset;	/* The current bit offset into a scanline */
	int	space_width;	/* The width of the whole character */
	int	space_offset;	/* The offset into the character of glyph */
	int	glyph_line_offset; /* The offset into scanline of glyph */
	int	glyph_width;	/* The width of brushed glyph */
	int     bold_offset, boldness;
	int     ascii, i, y;
	int	bit, len;

	switch (style) {
		case STYLE_NORM:	bold_offset = 0;
					break;
		case STYLE_BOLD:	bold_offset = 1;
					break;
		case STYLE_ULINE:	bold_offset = 0;
					break;
		case STYLE_ITALIC:	error(
					"FontServ: Italics not implemented!\n");
					return(NULL);
		default:		error(
					"FontServ: Unknown text style!\n");
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
	
	nchars = strlen((char *)text);
	bitmap = new BitMap;
	bitmap->height = (font->header)->fRectHeight;
	bitmap->width = TextWidth(text, font, style);
	len = Pixel2Byte(bitmap->width)*bitmap->height;
	bitmap->bits = new unsigned char[len];
	memset(bitmap->bits, 0, len);

	/* Print the individual characters */
	for ( boldness=0; boldness <= bold_offset; ++boldness ) {
		bit_offset=0;
		for ( i = 0; i < nchars; ++i ) {
			/* check to see if this character is defined */
			if (font->owTable[(unsigned char)text[i]] == -1)
				continue;
		
			space_width = LoByte(font->owTable[(unsigned char)text[i]]);
			space_offset = HiByte(font->owTable[(unsigned char)text[i]]);
			ascii = (unsigned char)text[i] - (font->header)->firstChar;
			glyph_line_offset = font->locTable[ascii]; 
			glyph_width = (font->locTable[ascii+1]-font->locTable[ascii]);
			for ( y=0; y<bitmap->height; ++y ) {
				int dst_offset = (y*bitmap->width +
						bit_offset + space_offset);
				unsigned short *src_scanline = 
					font->bitImage + y*(font->header)->rowWords;
			
				for ( bit = 0; bit<glyph_width; ++bit ) {
					SETBIT(bitmap->bits, dst_offset+bit+boldness, 
				  		GETBIT(src_scanline, glyph_line_offset+bit));
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
		y = (bitmap->height-(font->header)->descent+1);
		bit_offset = (y*bitmap->width);
		for ( bit=0; bit<bitmap->width; ++bit )
			SETBIT(bitmap->bits, bit_offset+bit, 0x01);
	}
	return(bitmap);
}
