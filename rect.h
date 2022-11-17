
#ifndef _rect_h
#define _rect_h

/* Avoid collisions with Mac data structures */
#define Rect MaelstromRect
#define SetRect SetMaelstromRect
#define OffsetRect OffsetMaelstromRect
#define InsetRect InsetMaelstromRect

typedef struct {
	short top;
	short left;
	short bottom;
	short right;
} Rect;

/* Functions exported from rect.cpp */
extern void SetRect(Rect *R, int left, int top, int right, int bottom);
extern void OffsetRect(Rect *R, int x, int y);
extern void InsetRect(Rect *R, int x, int y);

#endif /* _rect_h */
