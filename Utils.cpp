

#include <stdlib.h>
#include <stdio.h>
#include "bitesex.h"
#include "framebuf.h"

extern FrameBuf *win;				/* From init.cpp */
extern char *file2libpath(char *file);		/* From shared.cpp */


int Load_Title(struct Title *title, int title_id)
{
	FILE *title_fp;
	unsigned char *pixels;
	char filename[256];
	
	/* Open the title file.. */
	sprintf(filename, "Images/Maelstrom_Titles#%d.icon", 
							title_id);
	if ( (title_fp=fopen(file2libpath(filename), "r")) == NULL )
		return(-1);
	fread(&title->width, 2, 1, title_fp);
	bytesexs(title->width);
	fread(&title->height, 2, 1, title_fp);
	bytesexs(title->height);
	pixels = new unsigned char[title->width*title->height];
	if ( ! fread(pixels, title->width*title->height, 1, title_fp) )
		return(-1);
	(void) fclose(title_fp);
	win->ReColor(pixels, &title->data, title->width*title->height);
	delete[] pixels;
	return(0);
}
void Free_Title(struct Title *title)
{
	win->FreeArt(title->data);
}

CIcon *GetCIcon(short cicn_id)
{
	CIcon         *cicn;
	FILE          *cicn_fp;
	unsigned char *pixels;
	char           filename[256];
	
	/* Open the cicn sprite file.. */
	sprintf(filename, "Images/Maelstrom_Icon#%hd.cicn", 
							cicn_id);
	if ( (cicn_fp=fopen(file2libpath(filename), "r")) == NULL ) {
		error("GetCIcon(%hd): Can't open CICN %s: ",
					cicn_id, file2libpath(filename));
		perror("");
		return(NULL);
	}

	cicn = new CIcon;
        fread(&cicn->width, sizeof(cicn->width), 1, cicn_fp);
	bytesexs(cicn->width);
        fread(&cicn->height, sizeof(cicn->width), 1, cicn_fp);
	bytesexs(cicn->height);
        pixels = new unsigned char[cicn->width*cicn->height];
        if ( fread(pixels, 1, cicn->width*cicn->height, cicn_fp) != 
						cicn->width*cicn->height) {
		error("GetCIcon(%hd): Corrupt CICN!\n", cicn_id);
		delete[] pixels;
		delete   cicn;
		return(NULL);
	}
        cicn->mask=new unsigned char[(cicn->width/8)*cicn->height];
        if ( fread(cicn->mask, 1, (cicn->width/8)*cicn->height, cicn_fp) != 
					((cicn->width/8)*cicn->height) ) {
		error("GetCIcon(%hd): Corrupt CICN!\n", cicn_id);
		delete[] pixels;
		delete[] cicn->mask;
		delete   cicn;
		return(NULL);
	}
	(void) fclose(cicn_fp);

	win->ReColor(pixels, &cicn->pixels, cicn->width*cicn->height);
	delete[] pixels;
	return(cicn);
}

void BlitCIcon(int x, int y, CIcon *cicn)
{
	win->Blit_Icon(x, y, cicn->width, cicn->height, 
						cicn->pixels, cicn->mask);
}

void FreeCIcon(CIcon *cicn)
{
	win->FreeArt(cicn->pixels);
	delete[] cicn->mask;
	delete   cicn;
}

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
