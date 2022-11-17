
#ifndef _rect_h
#define _rect_h

typedef struct {
	short top;
	short left;
	short bottom;
	short right;
} Rect;

/* Functions exported from rect.cc */
extern void SetRect(Rect *R, int left, int top, int right, int bottom);

#endif /* _load_h */
