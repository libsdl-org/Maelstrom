
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

#include "Mac_Resource.h"
#include "Sprite.h"

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
       	unsigned short fontType;  /* PROPFONT or FIXEDFONT */
       	short   firstChar,
               	lastChar,
               	widMax,
               	kernMax,         /* Negative of max kern */
               	nDescent,        /* negative of descent */
               	fRectWidth,
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
        unsigned short *bitImage;	/* bitImage[rowWords][fRectHeight]; */
	unsigned short *locTable;	/* locTable[lastChar+3-firstChar]; */
	short *owTable;			/* owTable[lastchar+3-firstChar]; */ 

	/* The Raw Data */
	struct Mac_ResData nfnt;
} MFont;

class FontServ {

public:
	/* The "fontfile" parameter should be a Macintosh Resource fork file
	   that contains FOND and NFNT information for the desired fonts.
	*/
	FontServ(char *fontfile);
	~FontServ();
	
	/* This function causes disk accesses */
	MFont  *New_Font(char *fontname, int ptsize);
	void	Free_Font(MFont *font);

	int	TextWidth(char *text, MFont *font, unsigned short style);
	BitMap *Text_to_BitMap(char *text, MFont *font, unsigned short style);
	void    Free_Text(BitMap *text);

private:
	Mac_Resource *fontres;
	int           num_fonds;
	short        *fond_array;
};

#endif /* _fontserv_h */
