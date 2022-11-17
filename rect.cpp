
#include "rect.h"

void SetRect(Rect *R, int left, int top, int right, int bottom)
{
	R->left = left;
	R->top = top;
	R->right = right;
	R->bottom = bottom;
}

void OffsetRect(Rect *R, int x, int y)
{
	R->left += x;
	R->top += y;
	R->right += x;
	R->bottom += y;
}

void InsetRect(Rect *R, int x, int y)
{
	R->left += x;
	R->top += y;
	R->right -= x;
	R->bottom -= y;
}
