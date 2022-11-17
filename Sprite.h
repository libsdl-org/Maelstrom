
#ifndef _Sprite_h
#define _Sprite_h

/* The BitMap data structure */

typedef struct {
	unsigned short width;
	unsigned short height;
	unsigned char *bits;
	} BitMap;

/* The Title data structure */

struct Title {
	unsigned short width;
	unsigned short height;
	unsigned char *data;
	};

/* Simple Rectangle structure */

typedef struct {
	short top;
	short left;
	short bottom;
	short right;
	} Rect;

/* Color Icon data structure */

typedef struct {
	unsigned short width;
	unsigned short height;
	unsigned char *pixels;
	unsigned char *mask;
	} CIcon;
typedef CIcon *CIconPtr;

#endif /* _Sprite_h */
