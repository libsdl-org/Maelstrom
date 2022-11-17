
#ifndef _Sprite_h
#define _Sprite_h

/* The Sprite Data structure */

typedef struct {
	unsigned short width;
	unsigned short height;
	unsigned char *pixels;
	unsigned char *mask;
	} Sprite;

#endif /* _Sprite_h */
