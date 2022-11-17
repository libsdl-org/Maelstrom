

/* Graphics routines for Maelstrom!  (By Sam Lantinga) */

#include <stdlib.h>

#include "framebuf.h"
#include "imagedump.h"

#ifdef __WIN95__
/* The depth of the display can change on the fly, so keep track of all art */
/* Conceptually, if we don't need to keep track of the artwork, each sprite
   has only one new copy -- the one ReColor()'d.
   If the display depth can change, we also need to keep copies of the 
   original pixels so we can remap them to the new display depth.
*/
#define VARIABLE_DEPTH
#endif

/* Opcodes for the compiled sprite: */
#define OP_SKIP		0x00
#define OP_DRAW		0x01
#define OP_NEXT_LINE	0x80
#define OP_TERMINATE	0xF0
#define MAX_NOPS	1024		/* Max ops in a clipped sprite */
#define MAX_NPIXELS	4096		/* Max pixels in a clipped sprite */


/* This class allocates a 640x480 frame-buffer and provides high-level
   routines to blit images into it.  Well, it also provides access to
   other drawing routines.
*/


/* This function dumps a portion of the screen into an image file.  */

int
FrameBuf:: ScreenDump(char *prefix, Rect *frame)
{
	Rect fullscreen = { 0, 0, HEIGHT, WIDTH };
	int i, width, height, len;
	int perm[NUM_COLORS], mapped[NUM_COLORS];
	unsigned long new_colors[NUM_COLORS];
	int index, ncolors;
	struct stat sb;
	FILE *dumpfp;
	char filename[BUFSIZ];
	unsigned char *data;

	/* If no frame is specified, dump the entire screen */
	if ( ! frame )
		frame = &fullscreen;
	width = frame->right - frame->left;
	height = frame->bottom - frame->top;

	/* Find an unused filename to dump to */
	i = 0;
	do {
		sprintf(filename, "%s%d.%s", prefix, ++i, IMAGE_TYPE);
	} while ( stat(filename, &sb) == 0 );

	if ( (dumpfp=fopen(filename, "w")) == NULL )
		return(-1);

	/* Get the screen area, remap and dump it. :) */
	if ( !Grab_Area(frame->left, frame->top, width, height, &data) )
		return(-1);
	len = (height*width);

	if ( video_bpp > 1 ) {
		ImageDump(dumpfp, width, height, video_bpp, data);
	} else {
		/* Extract color information */
	
		/* Map sprite pixel "i" into new colortable */
		for ( i=0; i<NUM_COLORS; ++i ) {
			mapped[i] = -1;
			perm[Pixel_colors[i]] = i;
		}
		for ( i=0, ncolors=0; i<len; ++i ) {
			index = perm[data[i]];
			if ( mapped[index] < 0 ) {
				if ( ncolors == NUM_COLORS ) {
					error(
"Warning: image has more than %d colors!  (Corrupt?)\n", NUM_COLORS);
					continue;
				}
				new_colors[ncolors] = 0L;
				new_colors[ncolors] |=
					(Color_Map[index].blue&0xFF00)>>8;
				new_colors[ncolors] |=
					(Color_Map[index].green&0xFF00);
				new_colors[ncolors] |=
					(Color_Map[index].red&0xFF00)<<8;
				mapped[index] = ncolors;
				++ncolors;
			}
			data[i] = mapped[index];
		}
		/* Now we have our colormap -- Don't sort it! */

		/* Dump the image to the file */
		ImageDump(dumpfp, width, height, ncolors, new_colors, data);
	}

	/* Clean up and exit */
	Free_Area(data);
	fclose(dumpfp);
	return(0);
}

unsigned char *
FrameBuf:: ReColor(unsigned char *odata, unsigned char **dest, int len)
{
	struct FrameBuf::artwork colored;
	unsigned char *data, *newpixels, *Data;

	/* Create the new pixel data area */
	newpixels = Data = new unsigned char[len*video_bpp];
#ifdef VARIABLE_DEPTH
	data = new unsigned char[len*image_bpp];
	memcpy(data, odata, len*image_bpp);
	if ( dest ) {
		*dest = newpixels;

		/* Save the state of the artwork */
		colored.original = data;
		colored.ops = NULL;
		colored.bpp = image_bpp;
		colored.len = len;
		colored.newptr = dest;
		ourart->Add(colored);
	}
#else
	data = odata;
	*dest = newpixels;
#endif

	/* Translate pixel data .. oh my god... */
	switch (image_bpp) {
		case 1: /* Easy: 8-bit image to anything */
			switch (video_bpp) {
				case 1:
					while ( len-- ) {
						POKE_1(Data,
							Pixel_colors[*data]);
						++Data;
						++data;
					}
					break;
				case 2:
					while ( len-- ) {
						POKE_2(Data,
							Pixel_colors[*data]);
						Data += 2;
						data += 1;
					}
					break;
				case 3:
				case 4:
					while ( len-- ) {
						POKE_4(Data,
							Pixel_colors[*data]);
						Data += video_bpp;
						data += 1;
					}
					break;
				default:
					/* Huh? */
					error(
				"FrameBuf: ReColor: Unknown video bpp!\n");
					break;
			}
			break;
		case 2:
			/* Assuming 5/6/5 pixel format */
			switch (video_bpp) {
				case 1: { /* 16 bit image, 8 bit video */
					unsigned long  newpixel;
					unsigned short r, g, b;
					while ( len-- ) {
						memcpy(&newpixel, data,
								image_bpp);
						r = (((newpixel>>11)&0x1F)<<8);
						g = (((newpixel>>5)&0x2F)<<8);
						b = (((newpixel)&0x1F)<<8);
						newpixel = Map_Color(r, g, b);
						POKE_1(Data, newpixel);
						++Data;
						data += 2;
					}
				}
				break;
				case 2: /* 16 bit image, 16 bit video */
					memcpy(Data, data, len*video_bpp);
				break;
				case 3:
				case 4: { /* 16 bit image, 24/32 bit video */
					unsigned long  newpixel;
					unsigned short r, g, b;
					while ( len-- ) {
						memcpy(&newpixel, data,
								image_bpp);
						r = (newpixel>>11)&0x1F;
						g = (newpixel>>5)&0x2F;
						b = (newpixel)&0x1F;
						newpixel = ((r<<16)|(g<<8)|b);
						memcpy(Data, &newpixel,
								video_bpp);
						Data += video_bpp;
						data += image_bpp;
					}
				}
				break;
				default: /* Huh? */
					error(
				"FrameBuf: ReColor: Unknown video bpp!\n");
				break;
			}
			break;
		case 3:
			switch (video_bpp) {
				case 1: { /* 24 bit image, 8 bit video */
					unsigned long  newpixel;
					unsigned short r, g, b;
					while ( len-- ) {
						memcpy(&newpixel, data,
								image_bpp);
						r = ((newpixel>>8)&0xFF00);
						g = (newpixel&0xFF00);
						b = ((newpixel<<8)&0xFF00);
						POKE_1(Data,Map_Color(r, g, b));
						Data += 1;
						data += image_bpp;
					}
				}
				break;
				case 2: { /* 24 bit image, 16 bit video */
					unsigned long  newpixel;
					unsigned short r, g, b;
					while ( len-- ) {
						memcpy(&newpixel, data,
								image_bpp);
						r = (newpixel>>16)&0xFF;
						g = (newpixel>>8)&0xFF;
						b = (newpixel)&0xFF;
						newpixel =( ((r<<11)&0xF800) |
							    ((g<<5)&0x03E0)  |
							    (b&0x001F) );
						memcpy(Data, &newpixel,
								video_bpp);
						Data += video_bpp;
						data += image_bpp;
					}
				}
				break;
				case 3: /* 24 bit image, 24 bit video */
					memcpy(Data, data, len*video_bpp);
				break;
				case 4: { /* 24 bit image, 32 bit video */
					while ( len-- ) {
						memcpy(Data, data, image_bpp);
						Data += video_bpp;
						data += image_bpp;
					}
				}
				break;
				default: /* Huh? */
					error(
				"FrameBuf: ReColor: Unknown video bpp!\n");
				break;
			}
			break;
		case 4:
			switch (video_bpp) {
				case 1: { /* 32 bit image, 8 bit video -ack! */
					unsigned long  newpixel;
					unsigned short r, g, b;
					while ( len-- ) {
						memcpy(&newpixel, data,
								image_bpp);
						r = ((newpixel>>8)&0xFF00);
						g = (newpixel&0xFF00);
						b = ((newpixel<<8)&0xFF00);
						POKE_1(Data,Map_Color(r, g, b));
						Data += 1;
						data += image_bpp;
					}
				}
				break;
				case 2: { /* 32 bit image, 16 bit video */
					unsigned long  newpixel;
					unsigned short r, g, b;
					while ( len-- ) {
						memcpy(&newpixel, data,
								image_bpp);
						r = ((newpixel>>16)&0xFF);
						g = ((newpixel>>8)&0xFF);
						b = ((newpixel)&0xFF);
						newpixel =( ((r<<11)&0xF800) |
							    ((g<<5)&0x03E0)  |
							    (b&0x001F) );
						memcpy(Data, &newpixel,
								video_bpp);
						Data += video_bpp;
						data += image_bpp;
					}
				}
				break;
				case 3: { /* 32 bit image, 24 bit video */
					while ( len-- ) {
						memcpy(Data, data, image_bpp);
						Data += video_bpp;
						data += image_bpp;
					}
				}
				break;
				case 4: /* 32 bit image, 32 bit video */
					memcpy(Data, data, len*video_bpp);
				break;
				default: /* Huh? */
					error(
				"FrameBuf: ReColor: Unknown video bpp!\n");
				break;
			}
			break;
		default: /* Huh? */
			error("FrameBuf: ReColor: Unknown image bpp!\n");
			break;
	}
	return(newpixels);
}

/* This function frees any memory associated with ReColor()'d artwork */
int
FrameBuf:: FreeArt(unsigned char *pixelptr)
{
#ifdef VARIABLE_DEPTH
	struct FrameBuf::artwork *piece;

	ourart->InitIterator();
	while ( (piece=ourart->Iterate()) != NULL ) {
		if ( *(piece->newptr) == pixelptr ) {
			delete[] piece->original;
			delete[] *(piece->newptr);
			if ( ! ourart->Remove(piece) ) {
				error(
	"FrameBuf: Consistency error: Can't remove Artwork in list!\n");
				return(-1);
			}
			return(0);
		}
	}
	return(-1);
#else
	delete[] pixelptr;
	return(0);
#endif
}

/* This function is called when our colormap or display depth changes,
   and we have to go through and ReColor() all of our artwork.
 */
void
FrameBuf:: ReColorArt(void)
{
#ifdef VARIABLE_DEPTH
	int orig_imagebpp;
	List<struct FrameBuf::artwork> *origart;
	struct FrameBuf::artwork *piece;

	/* Save the official image BytesPerPixel */
	orig_imagebpp = image_bpp;

	/* Create a new set of artwork */
	origart = ourart;
	ourart = new List<struct FrameBuf::artwork>;

	/* ReColor everything */
	origart->InitIterator();
	while ( (piece=origart->Iterate()) != NULL ) {
		delete[] *(piece->newptr);
		image_bpp = piece->bpp;
		ReColor(piece->original, piece->newptr, piece->len);
		if ( piece->ops ) {
			/* We need to rewrite the compiled sprite pixels */
			unsigned char *ops = piece->ops;
			while ( *ops != OP_TERMINATE ) {
				switch (*(ops++)) {
					case OP_SKIP:
					case OP_DRAW:
						*ops = ((*ops/piece->ops_bpp)
								* video_bpp);
						++ops;
						break;
					default:
						break;
				}
			}
			(ourart->Last())->ops = piece->ops;
			(ourart->Last())->ops_bpp = video_bpp;
		}
		delete[] piece->original;
	}
	delete origart;

	/* Restore the bytes per pixel */
	image_bpp = orig_imagebpp;
#else
	error("This program cannot handle display depth changes\n");
#endif
}

unsigned long
FrameBuf:: Map_Color(unsigned short red, 
			unsigned short green, unsigned short blue)
{
	/* Allocate custom colors... */
	/* Most of this code is adapted from 'xv' -- Thanks! :) */
	int rd, gd, bd;			/* The color shades we have */
	int mdist, close, d;		/* Distance and closest pixel */
	int c;

	/* Initialize for this pass */
	mdist = 1000000; close=0;

	/* Cycle through the colors we have */
	for ( c=0; c<NUM_COLORS; ++c ) {
		rd = ((red >> 8) - (Color_Map[c].red >> 8));
		gd = ((green >> 8) - (Color_Map[c].green >> 8));
		bd = ((blue >> 8) - (Color_Map[c].blue >> 8));
		d = rd*rd + gd*gd + bd*bd;

		if ( d == 0 )  // Perfect match!
			return(Pixel_colors[c]);

		// Work slowly closer to a matching color..
		if ( d < mdist ) {
			mdist = d;
			close = c;
		}
	}
	return(Pixel_colors[close]);
}

void
FrameBuf:: Clear(int DoRefresh)
{
	if ( ! LockVideo() ) return;
	for ( int row=0; row<HEIGHT; ++row ) {
		int roff = row*pitch;
		for ( int col=0; col<VWIDTH; col += video_bpp )
			PixelPoke(shared_mem, roff+col, pitch, Black);
	}
	UnlockVideo();
	for ( int i=0; i<(WIDTH*HEIGHT*video_bpp); i += video_bpp )
		PixelPoke(backbuf, i, VWIDTH, Black);

	if ( DoRefresh )
		Refresh();
}

void
FrameBuf:: DrawBGPoint(unsigned int x, unsigned int y, 
					unsigned long color)
{
	DrawPoint(x, y, color);
	if ( ! faded )
		RefreshArea(x, y, 1, 1);
	PixelPoke(backbuf, y*VWIDTH+x*video_bpp, VWIDTH, color);
}

void
FrameBuf:: DrawPoint(unsigned int x, unsigned int y,
					unsigned long color)
{
	if ( ! LockVideo() ) return;
	PixelPoke(shared_mem, y*pitch+x*video_bpp, pitch, color);
	UnlockVideo();
}

void
FrameBuf:: DrawLine(unsigned int X1, unsigned int Y1, 
		unsigned int X2, unsigned int Y2, unsigned long color)
{
	int x, y;
	int lo, hi;
	double slope, b;

	if ( ! LockVideo() ) return;
	if ( Y1 == Y2 )  {  /* Horizontal line */
		lo = (X1 < X2 ? X1 : X2);
		hi = (X1 > X2 ? X1 : X2);
		y = Y1;
		for ( x=lo; x<hi; ++x )
			PixelPoke(shared_mem, y*pitch+x*video_bpp, pitch,color);
	} else if ( X1 == X2 ) {  /* Vertical line */
		x = X1;
		lo = (Y1 < Y2 ? Y1 : Y2);
		hi = (Y1 > Y2 ? Y1 : Y2);
		for ( y=lo; y<hi; ++y ) 
			PixelPoke(shared_mem, y*pitch+x*video_bpp ,pitch,color);
	} else {
		/* Equation:  y = mx + b */
		slope = ((double)((int)(Y2 - Y1)) / 
					(double)((int)(X2 - X1)));
		b = (double)(Y1 - slope*(double)X1);
		if ( ((slope < 0) ? slope > -1 : slope < 1) ) {
			lo = (X1 < X2 ? X1 : X2);
			hi = (X1 > X2 ? X1 : X2);
			for ( x=lo; x<hi; ++x ) {
				y = (int)((slope*(double)x) + b);
				PixelPoke(shared_mem, y*pitch+x*video_bpp,
								pitch,color);
			}
		} else {
			lo = (Y1 < Y2 ? Y1 : Y2);
			hi = (Y1 > Y2 ? Y1 : Y2);
			for ( y=lo; y<hi; ++y ) {
				x = (int)(((double)y - b)/slope);
				PixelPoke(shared_mem, y*pitch+x*video_bpp,
								pitch,color);
			}
		}
	}
	UnlockVideo();
}

void
FrameBuf:: DrawRectangle(int X, int Y, int width, int height, 
					unsigned long color)
{
	int x, maxx, y, maxy;

	maxx = X+width;
	maxy = Y+height;
	if ( ! LockVideo() ) return;
	for ( x=X; x<=maxx; ++x )
		PixelPoke(shared_mem, Y*pitch+x*video_bpp, pitch, color);
	for ( x=X; x<=maxx; ++x )
		PixelPoke(shared_mem, maxy*pitch+x*video_bpp, pitch, color);
	for ( y=Y; y<=maxy; ++y )
		PixelPoke(shared_mem, y*pitch+X*video_bpp, pitch, color);
	for ( y=Y; y<=maxy; ++y )
		PixelPoke(shared_mem, y*pitch+maxx*video_bpp, pitch, color);
	UnlockVideo();
}

void
FrameBuf:: FillRectangle(int X, int Y, int width, int height, 
					unsigned long color)
{
	int offset, loffset, sideways;

	if ( ! LockVideo() ) return;
	/* Semi-efficient, for now. :) */
	offset = ((Y*pitch)+X*video_bpp);
	loffset = pitch - (width*video_bpp);
	while ( height-- > 0 ) {
		for ( sideways=width; sideways; --sideways ) {
			PixelPoke(shared_mem, offset, pitch, color);
			offset += video_bpp;
		}
		offset += loffset;
	}
	UnlockVideo();
}

unsigned char *
FrameBuf:: Grab_Area(int x, int y, int width, int height, unsigned char **dest)
{
	struct FrameBuf::artwork colored;
	int            row;
	unsigned char *area, *newarea;

	newarea = new unsigned char[width*height*video_bpp];
#ifdef VARIABLE_DEPTH
	area = new unsigned char[width*height*video_bpp];
#endif
	if ( ! LockVideo() ) return(NULL);
	for ( row=0; row<height; ++row ) {
#ifdef PIXEL_DOUBLING
		int col;
#ifdef VARIABLE_DEPTH
		for ( col=0; col<width; ++col ) {
			memcpy(&area[row*width*video_bpp+col*video_bpp],
				&shared_mem[(row+y)*pitch*4+col*video_bpp*2],
								video_bpp);
		}
#endif
		for ( col=0; col<width; ++col ) {
			memcpy(&newarea[row*width*video_bpp+col*video_bpp],
				&shared_mem[
					(row+y)*pitch*4+(col+x)*video_bpp*2],
								video_bpp);
		}
#else
#ifdef VARIABLE_DEPTH
		memcpy(&area[row*width*video_bpp],
			&shared_mem[(row+y)*pitch+x*video_bpp],
							width*video_bpp);
#endif
		memcpy(&newarea[row*width*video_bpp],
			&shared_mem[(row+y)*pitch+x*video_bpp],
							width*video_bpp);
#endif /* PIXEL_DOUBLING */
	}
	UnlockVideo();

#ifdef VARIABLE_DEPTH
	/* Add it to our list of artwork */
	if ( dest ) {
		colored.original = area;
		colored.ops = NULL;
		colored.bpp = video_bpp;
		colored.len = width*height;
		colored.newptr = dest;
		*dest = newarea;
		ourart->Add(colored);
	}
#else
	*dest = newarea;
#endif
	return(newarea);
}

void
FrameBuf:: Set_Area(int x, int y, int width, int height, unsigned char *area)
{
	if ( ! LockVideo() ) return;
	for ( int row=0; row<height; ++row ) {
		LinePoke(shared_mem, (row+y)*pitch+x*video_bpp, pitch,
				&area[row*width*video_bpp], width*video_bpp);
	}
	UnlockVideo();
}

void
FrameBuf:: Free_Area(unsigned char *areaptr)
{
	if ( FreeArt(areaptr) < 0 )
		error("FrameBuf::Free_Area: Tried to free bad area pointer!\n");
}
 
void
FrameBuf:: Set_BlitClip(int left, int top, int right, int bottom)
{
	clip_left = left;
	clip_top = top;
	clip_right = right;
	clip_bottom = bottom;
}

void
FrameBuf:: Blit_BitMap(int x, int y, int width, int height,
			unsigned char *bdata, unsigned long color)
{
	int row, col, offset;

	/* Inefficient, for now. :) */
	if ( ! LockVideo() ) return;
	for ( row=y; row<(y+height); ++row ) {
		for ( col=x; col<(x+width); ++col ) {
			offset = (((row-y)*width)+(col-x));
			if ((bdata[offset/8]>>(7-(offset%8))) & 0x01) {
				PixelPoke(shared_mem, row*pitch+col*video_bpp,
								pitch, color);
			}
		}
	}
	UnlockVideo();
	if ( ! faded )
		RefreshArea(x, y, width, height);
}

void
FrameBuf:: UnBlit_BitMap(int x, int y, int width, int height,
						unsigned char *bdata)
{
	int row, col, offset;

	/* Inefficient, for now. :) */
	if ( ! LockVideo() ) return;
	for ( row=y; row<(y+height); ++row ) {
		for ( col=x; col<(x+width); ++col ) {
			offset = (((row-y)*width)+(col-x));
			if ((bdata[offset/8]>>(7-(offset%8))) & 0x01) {
				PixelPoke(shared_mem, row*pitch+col*video_bpp,
								pitch, Black);
			}
		}
	}
	UnlockVideo();
	if ( ! faded )
		RefreshArea(x, y, width, height);
}

void
FrameBuf:: Blit_Title(int x, int y, int width, int height,
						unsigned char *data)
{
	int offset, vwidth, vheight;

	/* We don't handle TrueColor image data yet */
	if ( image_bpp > 1 )
		return;

	if ( ! LockVideo() ) return;
	/* Efficient, for now. :) */
	offset = ((y*pitch)+x*video_bpp);
	vwidth = width*video_bpp;
	vheight = height;
	while ( vheight-- > 0 ) {
		LinePoke(shared_mem, offset, pitch, data, vwidth);
		offset += pitch;
		data += vwidth;
	}
	UnlockVideo();
	if ( ! faded ) {
		RefreshArea(x, y, width, height);
		Flush(1);
	}
}

void
FrameBuf:: Blit_Icon(int x, int y, int width, int height,
			unsigned char *sdata, unsigned char *mdata)
{
	int row, col, offset;

	/* Inefficient, for now. :) */
	if ( ! LockVideo() ) return;
	for ( row=y; row<(y+height); ++row ) {
		for ( col=x; col<(x+width); ++col, sdata += video_bpp ) {
			offset = (((row-y)*width)+(col-x));
			if ((mdata[offset/8]>>(7-(offset%8))) & 0x01) {
				LinePoke(shared_mem, row*pitch+col*video_bpp,
						pitch, sdata, video_bpp);
			}
		}
	}
	UnlockVideo();
}

void
FrameBuf:: Blit_Sprite(int x, int y, int width, int height,
			unsigned char *sdata, unsigned char *mdata)
{
	int row, col, offset;

	/* Inefficient, for now. :) */
	if ( ! LockVideo() ) return;
	for ( row=y; row<(y+height); ++row ) {
		for ( col=x; col<(x+width); ++col, sdata += video_bpp ) {
			offset = (((row-y)*width)+(col-x));
			if ((mdata[offset/8]>>(7-(offset%8))) & 0x01) {
				LinePoke(shared_mem, row*pitch+col*video_bpp,
						pitch, sdata, video_bpp);
			}
		}
	}
	UnlockVideo();
	RefreshArea(x, y, width, height);
}

void
FrameBuf:: UnBlit_Sprite(int x, int y, int width, int height,
					unsigned char *mdata)
{
	int row, col, offset;

	/* Inefficient, for now. :) */
	if ( ! LockVideo() ) return;
	for ( row=y; row<(y+height); ++row ) {
		for ( col=x; col<(x+width); ++col ) {
			offset = (((row-y)*width)+(col-x));
			if ((mdata[offset/8]>>(7-(offset%8))) & 0x01) {
				PixelPoke(shared_mem, row*pitch+col*video_bpp,
								pitch, Black);
			}
		}
	}
	UnlockVideo();
	RefreshArea(x, y, width, height);
}

void
FrameBuf:: ClipBlit_Sprite(int x, int y, int width, int height,
			unsigned char *sdata, unsigned char *mdata)
{
	int row, col;
	int doffset, soffset;
	int xoff, maxx, maxy;

	/* Inefficient, for now. :) */
	maxx = (x+width);
	if ( maxx > clip_right )
		maxx = clip_right;
	xoff = 0;
	if ( x < clip_left )
		xoff = clip_left-x;
	maxy = (y+height);
	if ( maxy > clip_bottom )
		maxy = clip_bottom;
	if ( y < clip_top ) {
		sdata += (clip_top-y)*width;
		mdata += (clip_top-y)*width;
		y = clip_top;
	}
	if ( ! LockVideo() ) return;
	for ( row=y; row<maxy; ++row ) {
		for ( col=(x+xoff); col<maxx; ++col ) {
			doffset = (col-x);
			if ( mdata[doffset] ) {
				doffset *= video_bpp;
				LinePoke(shared_mem, row*pitch+col*video_bpp,
					pitch, &sdata[doffset], video_bpp);
			}
		}
		sdata += width*video_bpp;
		mdata += width;
	}
	UnlockVideo();
	RefreshArea(x, y, width, height);
}

void
FrameBuf:: UnClipBlit_Sprite(int x, int y, int width, int height,
						unsigned char *mdata)
{
	int row, col;
	int doffset, soffset, boffset;
	int xoff, maxx, maxy;

	/* Inefficient, for now. :) */
	maxx = (x+width);
	if ( maxx > clip_right )
		maxx = clip_right;
	xoff = 0;
	if ( x < clip_left )
		xoff = clip_left-x;
	maxy = (y+height);
	if ( maxy > clip_bottom )
		maxy = clip_bottom;
	if ( y < clip_top ) {
		mdata += (clip_top-y)*width;
		y = clip_top;
	}
	if ( ! LockVideo() ) return;
	for ( row=y; row<maxy; ++row ) {
		for ( col=(x+xoff); col<maxx; ++col ) {
			doffset = (col-x);
			if ( mdata[doffset] ) {
#ifdef PIXEL_DOUBLING
				boffset = ((row*VWIDTH)*4+col*video_bpp*2);
#else
				boffset = ((row*VWIDTH)+col*video_bpp);
#endif
				LinePoke(shared_mem, row*pitch+col*video_bpp,
					pitch, &backbuf[boffset], video_bpp);
			}
		}
		mdata += width;
	}
	UnlockVideo();
	RefreshArea(x, y, width, height);
}

/* The sprite data we are passed should already have been mapped to our
   video depth by ReColor()
*/
CSprite *
FrameBuf:: Compile_Sprite(int width, int height, 
			unsigned char *sdata, unsigned char *mdata)
{
	struct FrameBuf::artwork colored;
	int row, col, offset;
	int drun=0, srun=0;
	const int runlimit = ((255/video_bpp)-video_bpp);

	CSprite *sprite = new CSprite;
	unsigned char *ops = new unsigned char[(width*2)*(height+1)+1];
	int ops_i=0;
	unsigned char *pixels = new unsigned char[width*height*video_bpp];
	int pixels_i=0;

	for ( row=0; row<height; ++row ) {
		for ( col=0; col<width; ++col ) {
			offset = ((row*width)+col);
			if ((mdata[offset/8]>>(7-(offset%8))) & 0x01) {
				if ( srun ) {
					ops[ops_i++] = OP_SKIP;
					ops[ops_i++] = (unsigned char)srun;
					sdata += srun;
					srun = 0;
				}
				/* this value can increase in ReColorArt() */
				if ( drun > runlimit ) {
					ops[ops_i++] = OP_DRAW;
					ops[ops_i++] = drun;
					memcpy(&pixels[pixels_i], sdata, drun);
					pixels_i += drun;
					sdata += drun;
					drun = video_bpp;
				} else
					drun += video_bpp;
			} else {
				if ( drun ) {
					ops[ops_i++] = OP_DRAW;
					ops[ops_i++] = drun;
					memcpy(&pixels[pixels_i], sdata, drun);
					pixels_i += drun;
					sdata += drun;
					drun = 0;
				}
				if ( srun > runlimit ) {
					ops[ops_i++] = OP_SKIP;
					ops[ops_i++] = srun;
					sdata += srun;
					srun = video_bpp;
				} else
					srun += video_bpp;
			}
		}
		if ( drun ) {
			ops[ops_i++] = OP_DRAW;
			ops[ops_i++] = drun;
			memcpy(&pixels[pixels_i], sdata, drun);
			pixels_i +=  drun;
			sdata +=  drun;
			drun = 0;
		}
		if ( srun ) {
			sdata += srun;
			srun=0;
		}
		ops[ops_i++] = OP_NEXT_LINE;
	}
	ops[ops_i++] = OP_TERMINATE;

	/* Fill out the sprite */
	sprite->numops = ops_i;
	sprite->ops = new unsigned char[ops_i];
	memcpy(sprite->ops, ops, ops_i);
	delete[] ops;
	sprite->numpixels = (pixels_i/video_bpp);
	if ( ! pixels_i ) ++pixels_i;		/* Prevent 0 length alloc */
	sprite->pixels = new unsigned char[pixels_i];
	memcpy(sprite->pixels, pixels, pixels_i);
	sprite->width = width;
	sprite->height = height;

#ifdef VARIABLE_DEPTH
	/* Remap the pixels back to the original colormap */
	if ( video_bpp == 1 ) {
		int i;
		unsigned char *pixelptr, RevPixels[NUM_COLORS];

		/* Reverse map the pixels */
		for ( i=0; i<NUM_COLORS; ++i )
			RevPixels[Pixel_colors[i]] = i;

		for ( i=0, pixelptr=pixels; i<sprite->numpixels;
							++i, ++pixelptr )
			*pixelptr = RevPixels[*pixelptr];
	}
	/* Save the state of the artwork */
	colored.original = pixels;
	colored.ops = sprite->ops;
	colored.ops_bpp = video_bpp;
	colored.bpp = video_bpp;
	colored.len = sprite->numpixels;
	colored.newptr = &sprite->pixels;
	ourart->Add(colored);
#endif
	/* Return it! */
	return(sprite);
}

void
FrameBuf:: Blit_CSprite(int x, int y, CSprite *sprite)
{
	int ops_i=0, pixels_i=0;
	int offset, len, Y=y;
	int clipped=0;
	unsigned char *pixels;

	/* Do we need to worry about clipping? */
	if ( (x < clip_left) || ((x+sprite->width) > clip_right) ||
	     (y < clip_top) || ((y+sprite->height) > clip_bottom) ) {
		sprite = ClipSprite(x, y, sprite);
		clipped = 1;
	}

	pixels = sprite->pixels;
	offset = ((Y*pitch)+x*video_bpp);
	if ( ! LockVideo() ) return;
	for ( ; ; ) {
		switch (sprite->ops[ops_i++]) {
			case OP_SKIP:
				offset += sprite->ops[ops_i++];
				break;
			case OP_DRAW:
				len = sprite->ops[ops_i++];
				LinePoke(shared_mem, offset, pitch,
						&pixels[pixels_i], len);
				offset += len;
				pixels_i += len;
				break;
			case OP_NEXT_LINE:
				offset = ((++Y*pitch)+x*video_bpp);
				break;
			case OP_TERMINATE:
				goto finished;
			default:
				UnlockVideo();
				error( 
			"FrameBuf: Unknown sprite opcode! (0x%.2x)\n",
						sprite->ops[ops_i-1]);
				exit(255);
		}
	}
finished:
	UnlockVideo();
	RefreshArea(x, y, sprite->width, sprite->height);
	if ( clipped ) {
		delete[] sprite->ops;
		delete[] sprite->pixels;
		delete   sprite;
	}
	return;
}

void
FrameBuf:: UnBlit_CSprite(int x, int y, CSprite *sprite)
{
	int ops_i=0;
	int offset, soffset, len, Y=y;
	int clipped=0;
	unsigned char *pixels;

	/* Do we need to worry about clipping? */
	if ( (x < clip_left) || ((x+sprite->width) > clip_right) ||
	     (y < clip_top) || ((y+sprite->height) > clip_bottom) ) {
		sprite = ClipSprite(x, y, sprite);
		clipped = 1;
	}

	pixels = sprite->pixels;
	if ( ! LockVideo() ) return;
	if ( pitch == VWIDTH ) {
		offset = (Y*VWIDTH)+(x*video_bpp);
		for ( ; ; ) {
			switch (sprite->ops[ops_i++]) {
				case OP_SKIP:
					offset += sprite->ops[ops_i++];
					break;
				case OP_DRAW:
					len = sprite->ops[ops_i++];
					LinePoke(shared_mem, offset, pitch,
#ifdef PIXEL_DOUBLING
		&backbuf[(offset/VWIDTH)*VWIDTH*4+(offset%VWIDTH)*2], len);
#else
							&backbuf[offset], len);
#endif
					offset += len;
					break;
				case OP_NEXT_LINE:
					offset = ((++Y)*VWIDTH)+(x*video_bpp);
					break;
				case OP_TERMINATE:
					goto finished;
				default:
					UnlockVideo();
					error( 
				"FrameBuf: Unknown sprite opcode! (0x%.2x)\n",
							sprite->ops[ops_i-1]);
					exit(255);
			}
		}
	} else {
		offset = (Y*VWIDTH)+(x*video_bpp);
		soffset = (Y*pitch)+(x*video_bpp);
		for ( ; ; ) {
			switch (sprite->ops[ops_i++]) {
				case OP_SKIP:
					offset += sprite->ops[ops_i];
					soffset += sprite->ops[ops_i];
					++ops_i;
					break;
				case OP_DRAW:
					len = sprite->ops[ops_i++];
					LinePoke(shared_mem, soffset, pitch,
#ifdef PIXEL_DOUBLING
		&backbuf[(offset/VWIDTH)*VWIDTH*4+(offset%VWIDTH)*2], len);
#else
							&backbuf[offset], len);
#endif
					offset += len;
					soffset += len;
					break;
				case OP_NEXT_LINE:
					++Y;
					offset = (Y*VWIDTH)+(x*video_bpp);
					soffset = (Y*pitch)+(x*video_bpp);
					break;
				case OP_TERMINATE:
					goto finished;
				default:
					UnlockVideo();
					error( 
				"FrameBuf: Unknown sprite opcode! (0x%.2x)\n",
							sprite->ops[ops_i-1]);
					exit(255);
			}
		}
	}
finished:
	UnlockVideo();
	RefreshArea(x, y, sprite->width, sprite->height);
	if ( clipped ) {
		delete[] sprite->ops;
		delete[] sprite->pixels;
		delete   sprite;
	}
	return;
}

void
FrameBuf:: Free_CSprite(CSprite *sprite)
{
	delete[] sprite->ops;
	FreeArt(sprite->pixels);
	delete   sprite;
}

/* Recompile a CSprite, taking clipping into account */
CSprite *
FrameBuf:: ClipSprite(int x, int y, CSprite *sprite)
{
	CSprite *newsprite = new CSprite;
	int ops_i=0, numops=0;
	char newops[MAX_NOPS];
	int pixels_i=0, numpixels=0;
	char newpixels[MAX_NPIXELS];
	int col;
	int len, skip;

	/* Check top clipping */
	for ( ; y < clip_top; ++y ) {
		while ( sprite->ops[ops_i] != OP_NEXT_LINE ) {
			switch (sprite->ops[ops_i++]) {
				case OP_SKIP:
					ops_i++;
					break;
				case OP_DRAW:
					pixels_i += sprite->ops[ops_i++];
					break;
				case OP_TERMINATE:
					goto endsprite;
			}
		}
		newops[numops++] = OP_NEXT_LINE;
		++ops_i;
	}
	for ( ; y <= clip_bottom; ++y ) {
		if ( x < clip_left ) {
			newops[numops++] = OP_SKIP;
			newops[numops++] = (clip_left-x)*video_bpp;
		}
		for ( col=x; col<clip_left; ) {
			switch (sprite->ops[ops_i++]) {
				case OP_SKIP:
					len = sprite->ops[ops_i++];
					col += (len/video_bpp);
					if ( col >= clip_left ) {
						newops[numops++] = OP_SKIP;
						newops[numops++] =
						(col-clip_left+1)*video_bpp;
					}
					break;
				case OP_DRAW:
					len = sprite->ops[ops_i++];
					skip = (clip_left-col)*video_bpp;
					col += len/video_bpp;
					if ( len > skip ) {
						/* draw after the edge */
						pixels_i += skip;
						len -= skip;
						newops[numops++] = OP_DRAW;
						newops[numops++] = len;
						memcpy(&newpixels[numpixels], &sprite->pixels[pixels_i], len);
						numpixels += len;
					}
					pixels_i += len;
					break;
				case OP_NEXT_LINE:
					--ops_i;
					goto nextline;
				case OP_TERMINATE:
					goto endsprite;
			}
		}
		for ( ; col <= clip_right; ) {
			switch (sprite->ops[ops_i++]) {
				case OP_SKIP:
					len = sprite->ops[ops_i++];
					col += (len/video_bpp);
					if ( col > clip_right ) {
						skip = (col-clip_right)*
								video_bpp;
						len -= skip;
						newops[numops++] = OP_SKIP;
						newops[numops++] = len;
						goto nextline;
					} else {
						newops[numops++] = OP_SKIP;
						newops[numops++] = len;
					}
					break;
				case OP_DRAW:
					len = sprite->ops[ops_i++];
					col += (len/video_bpp);
					if ( col > clip_right ) {
						skip = (col-clip_right)*
								video_bpp;
						len -= skip;
						newops[numops++] = OP_DRAW;
						newops[numops++] = len;
						memcpy(&newpixels[numpixels], &sprite->pixels[pixels_i], len);
						numpixels += len;
						pixels_i += (len+skip);
						goto nextline;
					} else {
						newops[numops++] = OP_DRAW;
						newops[numops++] = len;
						memcpy(&newpixels[numpixels], &sprite->pixels[pixels_i], len);
						numpixels += len;
						pixels_i += len;
					}
					break;
				case OP_NEXT_LINE:
					--ops_i;
					goto nextline;
				case OP_TERMINATE:
					goto endsprite;
			}
		}
	nextline:
		while ( sprite->ops[ops_i] != OP_NEXT_LINE ) {
			switch (sprite->ops[ops_i++]) {
				case OP_SKIP:
					ops_i++;
					break;
				case OP_DRAW:
					pixels_i += sprite->ops[ops_i++];
					break;
				case OP_TERMINATE:
					goto endsprite;
			}
		}
		newops[numops++] = OP_NEXT_LINE;
		++ops_i;
	}
endsprite:
	newsprite->width = sprite->width;
	newsprite->height = sprite->height;
	newops[numops++] = OP_TERMINATE;
	newsprite->numops = numops;
	newsprite->ops = new unsigned char[numops];
	memcpy(newsprite->ops, newops, numops);
	if ( ! numpixels ) ++numpixels;		/* Prevent 0 length alloc */
	newsprite->pixels = new unsigned char[numpixels];
	memcpy(newsprite->pixels, newpixels, numpixels);
	return(newsprite);
}


/* A speckling pixel fade algorithm, interface independent */
/*
   This should probably be split into two separate functions
   sharing the saved_mem buffer, eliminating the black buffer.
   That would save us a full screen buffer worth of memory.
*/
void
FrameBuf:: Pixel_Fade(int state)
{
	unsigned char *black_mem;
	unsigned char *saved_mem;
	int x, y;

	if ( state == XFADE_OUT ) {
		Grab_Area(0, 0, WIDTH, HEIGHT, &saved_mem);
		black_mem = new unsigned char[VWIDTH*HEIGHT*video_bpp];
		for ( y=(VWIDTH*HEIGHT-1)*video_bpp; y>=0; y -= video_bpp ) {
			switch (video_bpp) {
				case 1:
					POKE_1(&black_mem[y], Black);
					break;
				case 2:
					POKE_2(&black_mem[y], Black);
					break;
				case 3:
				case 4:
					POKE_4(&black_mem[y], Black);
					break;
			}
		}
	} else {
		faded = 0;
		Grab_Area(0, 0, WIDTH, HEIGHT, &black_mem);
		Clear();
	}

	if ( ! LockVideo() ) return;
        for ( y=0; y<HEIGHT; y += 4 ) {
                for ( x=0; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=0; y<HEIGHT; y += 4 ) {
                for ( x=3; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=3; y<HEIGHT; y += 4 ) {
                for ( x=3; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=3; y<HEIGHT; y += 4 ) {
                for ( x=0; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();

	if ( ! LockVideo() ) return;
        for ( y=1; y<HEIGHT; y += 4 ) {
                for ( x=1; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=1; y<HEIGHT; y += 4 ) {
                for ( x=2; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=2; y<HEIGHT; y += 4 ) {
                for ( x=2; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=2; y<HEIGHT; y += 4 ) {
                for ( x=1; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();

	if ( ! LockVideo() ) return;
        for ( y=0; y<HEIGHT; y += 4 ) {
                for ( x=1; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=0; y<HEIGHT; y += 4 ) {
                for ( x=2; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=1; y<HEIGHT; y += 4 ) {
                for ( x=3; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=2; y<HEIGHT; y += 4 ) {
                for ( x=3; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();

	if ( ! LockVideo() ) return;
        for ( y=3; y<HEIGHT; y += 4 ) {
                for ( x=2; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=3; y<HEIGHT; y += 4 ) {
                for ( x=1; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=2; y<HEIGHT; y += 4 ) {
                for ( x=0; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();
	if ( ! LockVideo() ) return;
        for ( y=1; y<HEIGHT; y += 4 ) {
                for ( x=0; x<WIDTH; x += 4 ) {
			LinePoke(shared_mem, y*pitch+x*video_bpp, pitch,
				&black_mem[y*VWIDTH+x*video_bpp], video_bpp);
                }
        } 
	UnlockVideo();
	Refresh();

	if ( state == XFADE_OUT ) {
		Set_Area(0, 0, WIDTH, HEIGHT, saved_mem);
		Free_Area(saved_mem);
		delete[] black_mem;
		faded = 1;
	} else
		Free_Area(black_mem);
}
