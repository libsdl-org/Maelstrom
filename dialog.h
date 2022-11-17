
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef __WIN95__
#include <windows.h>
#endif

/* From shared.cc */
extern void select_usleep(unsigned long usec);

#include "Sprite.h"
#include "fontserv.h"
#include "framebuf.h"

/*  This is a class set for Macintosh-like dialogue boxes. :) */
/*  Sorta complex... */

/* Defaults for various dialog classes */

#define BUTTON_WIDTH	75
#define BUTTON_HEIGHT	19

#define BOX_WIDTH	170
#define BOX_HEIGHT	20

class Dialog {

public:
	Dialog(FrameBuf *win, int x, int y);

	virtual void SetButtonPress(void (*new_button_callback)(int x, int y,
						int button, int *doneflag)) {
		button_callback = new_button_callback;
	}
	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		(*button_callback)(x, y, button, doneflag);
	}
	virtual void SetKeyPress(void (*new_key_callback)(KeySym key,
							int *doneflag)) {
		key_callback = new_key_callback;
	}
	virtual void HandleKeyPress(KeySym key, int *doneflag) {
		(*key_callback)(key, doneflag);
	}

protected:
	FrameBuf *Win;
	int  X, Y;
	void (*button_callback)(int x, int y, int button, int *doneflag);
	void (*key_callback)(KeySym key, int *doneflag);

	// Utility routines for dialogs
	int IsSensitive(Rect *RecT, int x, int y) {
		if ( (y > RecT->top) && (y < RecT->bottom) &&
	    	     (x > RecT->left) && (x < RecT->right) )
			return(1);
		return(0);
	}

};


/* Macro to set a bit in a bytely scanline */
#ifdef DEBUG
#define SETBIT(scanline, i, bit) \
		{ \
			if ( bit << (7 - (i)%8) ) \
				printf("X"); \
			else \
				printf(" "); \
			(scanline[(i)/8] |= bit << (7 - (i)%8)); \
		} 
#else
#define SETBIT(scanline, i, bit) \
		(scanline[(i)/8] |= bit << (7 - (i)%8))
#endif /* DEBUG */

/* The button callbacks should return 1 if they finish the dialog,
   or 0 if they do not.
*/

class Mac_Button : public Dialog {

public:
	Mac_Button(FrameBuf *win, int x, int y, int width, int height,
				char *text, MFont *font, FontServ *fontserv, 
				unsigned long fg, unsigned long bg, 
				int (*callback)(void)) : Dialog(win, x, y) {
		int            i;
		unsigned char *byte_map;

		/* Build sensitivity area */
		Width = width;
		Height = height;
		sensitive.left = x;
		sensitive.top  = y;
		sensitive.right = x+Width;
		sensitive.bottom = y+Height;

		/* Build bitmap of the button */
		byte_map = new unsigned char[Width*Height];
		memset(byte_map, 0, Width*Height);
		Bevel_ByteMap(byte_map, Width, Height);
		button_map = ByteMap_to_BitMap(byte_map, Width, Height);
		for ( i=0; i<(Width*Height); ++i )
			byte_map[i] = (byte_map[i] ? 0x00 : 0x01);
		Bevel_ByteMap(byte_map, Width, Height);
		invert_map = ByteMap_to_BitMap(byte_map, Width, Height);
		/* Display the Button */
		Fontserv = fontserv;
		label = Fontserv->Text_to_BitMap(text, font, STYLE_NORM);
		Fg = fg;
		Bg = bg;
		Win->Blit_BitMap(X, Y, Width, Height, invert_map, Bg);
		Win->Blit_BitMap(X, Y, Width, Height, button_map, Fg);
		Win->Blit_BitMap(X+(Width-label->width)/2, Y+2, 
				label->width, label->height, label->bits, Fg);
		/* Set the callback */
		Callback = callback;

		/* Clean up and return */
		delete[] byte_map;
	}

	virtual ~Mac_Button() {
		delete[] button_map;
		delete[] invert_map;
		Fontserv->Free_Text(label);
	}
		
	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		Unused(button);		/* Default callback ignores button */

		if ( IsSensitive(&sensitive, x, y) )
			ActivateButton(doneflag);
	}

protected:
	int            Width, Height;
	BitMap        *label;
	FontServ      *Fontserv;
	unsigned long  Fg, Bg;
	unsigned char *button_map;
	unsigned char *invert_map;
	Rect           sensitive;
	int          (*Callback)(void);

	void ActivateButton(int *doneflag) {
		Win->Blit_BitMap(X, Y, Width, Height, invert_map, Fg);
		Win->Blit_BitMap(X+(Width-label->width)/2, Y+2, 
				label->width, label->height, label->bits, Bg);
		Win->Flush(1);
		select_usleep(50000);
		Win->Blit_BitMap(X, Y, Width, Height, invert_map, Bg);
		Win->Blit_BitMap(X, Y, Width, Height, button_map, Fg);
		Win->Blit_BitMap(X+(Width-label->width)/2, Y+2, 
				label->width, label->height, label->bits, Fg);
		Win->Flush(1);
		if ( Callback )
			*doneflag = Callback();
		else
			*doneflag = 1;
	}

	// Utility Functions:
	void Bevel_ByteMap(unsigned char *byte_map, int width, int height) {
		/* Blast the crude outline */
		for ( int i=0; i<height; ++i ) {
			if ( (i < 3) || ((height-i-1) < 3) ) {
				int row;
				if ( (height-i-1) < 3 )
					row = height-i-1;
				else
					row = i;
				switch (row) {
					case 0:
				memset(&byte_map[i*width], 0x01, width);
						break;
					case 1:
				memset(&byte_map[i*width], 0x01, 3);
				memset(&byte_map[i*width+width-3], 0x01, 3);
						break;
					case 2:
				memset(&byte_map[i*width], 0x01, 2);
				memset(&byte_map[i*width+width-2], 0x01, 2);
						break;
				}
			} else {
				byte_map[i*width] = 0x01;
				byte_map[i*width+width-1] = 0x01;
			}
		}

		/* Now mask the corners */

		/* Left upper corner */
		byte_map[0] = 0x00;
		byte_map[1] = 0x00;
		byte_map[2] = 0x00;
		byte_map[width] = 0x00;
		byte_map[2*width] = 0x00;
		/* Right upper corner */
		byte_map[width-3] = 0x00;
		byte_map[width-2] = 0x00;
		byte_map[width-1] = 0x00;
		byte_map[2*width-1] = 0x00;
		byte_map[3*width-1] = 0x00;
		/* Left lower corner */
		byte_map[(height-3)*width] = 0x00;
		byte_map[(height-2)*width] = 0x00;
		byte_map[(height-1)*width] = 0x00;
		byte_map[(height-1)*width+1] = 0x00;
		byte_map[(height-1)*width+2] = 0x00;
		/* Right lower corner */
		byte_map[(height-2)*width-1] = 0x00;
		byte_map[(height-1)*width-1] = 0x00;
		byte_map[height*width-1] = 0x00;
		byte_map[height*width-2] = 0x00;
		byte_map[height*width-3] = 0x00;
	}
	unsigned char *ByteMap_to_BitMap(unsigned char *byte_map, 
						int width, int height) {
		unsigned char *bitmap=new unsigned char[((width/8)+1)*height];

		memset(bitmap, 0, ((width/8)+1)*height);
		for ( int row=0; row<height; ++row ) {
			for ( int col=0; col<width; ++col ) {
				int offset = row*width+col;
				SETBIT(bitmap, offset, byte_map[offset]);
			}
#ifdef DEBUG
			printf("\n");
#endif
		}
		return(bitmap);
	}
};

/* The only difference between this button and the Mac_Button is that
   if <Return> is pressed, this button is activated.
*/
class Mac_DefaultButton : public Mac_Button {

public:
	Mac_DefaultButton(FrameBuf *win, int x, int y, int width, int height,
				char *text, MFont *font, FontServ *fontserv, 
				unsigned long fg, unsigned long bg, 
				int (*callback)(void)) : 
Mac_Button(win, x, y, width, height, text, font, fontserv, fg, bg, callback) {
		ThickBevel(Win, X, Y, Width, Height, Fg);
	}

	virtual void HandleKeyPress(KeySym key, int *doneflag) {
		if ( key == XK_Return )
			ActivateButton(doneflag);
	}

protected:
	// More Utility routines...
	void ThickBevel(FrameBuf *win, int x, int y, int width, int height,
							unsigned long color) {
		/* Expand outward.. */
		x -= 4;
		width += 8;
		y -= 4;
		height += 8;

		/* Start doin' the blimey thing! :) */
		win->DrawLine(x+5, y, x+width-5, y, color);
		win->DrawLine(x+3, y+1, x+width-3, y+1, color);
		win->DrawLine(x+2, y+2, x+width-2, y+2, color);
		win->DrawLine(x+1, y+3, x+6, y+3, color);
		win->DrawLine(x+width-1-5, y+3, x+width-1, y+3, color);
		win->DrawLine(x+1, y+4, x+4, y+4, color);
		win->DrawLine(x+width-1-3, y+4, x+width-1, y+4, color);
		win->DrawLine(x, y+5, x+4, y+5, color);
		win->DrawLine(x+width-1-3, y+5, x+width, y+5, color);

		win->DrawLine(x, y+6, x, y+height-6, color);
		win->DrawLine(x+width-1, y+6, x+width-1, y+height-6, color);
		win->DrawLine(x+1, y+6, x+1, y+height-6, color);
		win->DrawLine(x+width-1-1, y+6, x+width-1-1, y+height-6,color);
		win->DrawLine(x+2, y+6, x+2, y+height-6, color);
		win->DrawLine(x+width-1-2, y+6, x+width-1-2, y+height-6,color);

		win->DrawLine(x, y+height-1-5, x+4, y+height-1-5, color);
		win->DrawLine(x+width-1-3, y+height-1-5, x+width, 
							y+height-1-5, color);
		win->DrawLine(x+1, y+height-1-4, x+4, y+height-1-4, color);
		win->DrawLine(x+width-1-3, y+height-1-4, x+width-1, 
							y+height-1-4, color);
		win->DrawLine(x+1, y+height-1-3, x+6, y+height-1-3, color);
		win->DrawLine(x+width-1-5, y+height-1-3, x+width-1, 
							y+height-1-3, color);
		win->DrawLine(x+2, y+height-1-2,x+width-2, y+height-1-2,color);
		win->DrawLine(x+3, y+height-1-1,x+width-3, y+height-1-1,color);
		win->DrawLine(x+5, y+height-1, x+width-5, y+height-1, color);

		/* Ahh, a refreshing return. */
		win->RefreshArea(x, y, width, height);
	}
};

/* Class of checkboxes */

#define CHECKBOX_SIZE	12

class CheckBox : public Dialog {

public:
	CheckBox(FrameBuf *win, int x, int y, char *text, MFont *font, 
		FontServ *fontserv, unsigned long fg, unsigned long bg, 
					int *toggle) : Dialog(win, x, y) {
		BitMap *textBM;

		sensitive.left = X;
		sensitive.top = Y;
		sensitive.right = X+CHECKBOX_SIZE;
		sensitive.bottom = Y+CHECKBOX_SIZE;
		Fg = fg;
		Bg = bg;
		Win->DrawRectangle(X, Y, CHECKBOX_SIZE, CHECKBOX_SIZE, Fg);
		textBM = fontserv->Text_to_BitMap(text, font, STYLE_NORM);
		Win->Blit_BitMap(X+CHECKBOX_SIZE+4, Y-2, textBM->width,
					textBM->height, textBM->bits, Fg);
		togglevar = toggle;
		Update_Toggle();
		fontserv->Free_Text(textBM);
	}

	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		Unused(button);		/* Default callback ignores button */
		Unused(doneflag);	/* Callback doesn't set doneflag */

		if ( IsSensitive(&sensitive, x, y) ) {
			if ( *togglevar )
				*togglevar = 0;
			else
				*togglevar = 1;
			Update_Toggle();
			Win->RefreshArea(X, Y, CHECKBOX_SIZE, CHECKBOX_SIZE);
		}
	}

private:
	unsigned long Fg, Bg;
	int          *togglevar;
	Rect          sensitive;

	void Update_Toggle(void) {
		unsigned long color;

		if ( *togglevar )
			color = Fg;
		else
			color = Bg;

		Win->DrawLine(X+1, Y+1, X+CHECKBOX_SIZE,
						Y+CHECKBOX_SIZE, color);
		Win->DrawLine(X+CHECKBOX_SIZE-1, Y+1, X, 
						Y+CHECKBOX_SIZE, color);
	}
};

/* Class of radio buttons */

class Radio_List : public Dialog {

public:
	Radio_List(FrameBuf *win, int x, int y, int nradios);
	virtual ~Radio_List() { delete[] radio_list; }

	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		int n;

		Unused(button);		/* Default callback ignores button */
		Unused(doneflag);	/* Callback doesn't set doneflag */

		for ( n=0; n<numradios; ++n ) {
			if ( IsSensitive(&radio_list[n].sensitive, x, y) ) {
				UnSpot(radio_list[curradio].x, 
							radio_list[curradio].y);
				curradio = n;
				*radiovar = n;
				Spot(radio_list[curradio].x, 
							radio_list[curradio].y);
				Win->Refresh();
			}
		}
	}

	virtual void Set_RadioVar(int *var) {
		radiovar = var;
	}
	virtual void Add_Radio(int x, int y, int is_default, BitMap *label) {
		Circle(x, y);
		if ( is_default ) {
			Spot(x, y);
			curradio = numradios;
		}
		Win->Blit_BitMap(x+21, y+3, label->width, label->height,
							label->bits, black);
		radio_list[numradios].sensitive.top = y;
		radio_list[numradios].sensitive.left = x;
		radio_list[numradios].sensitive.bottom = y+BOX_HEIGHT;
		radio_list[numradios].sensitive.right = x+20+label->width;
		radio_list[numradios].x = x;
		radio_list[numradios].y = y;
		++numradios;
	}

private:
	unsigned long black, white;
	int  numradios;
	int  curradio;
	int *radiovar;
	struct radio {
		Rect sensitive;
		int  x, y;
		} *radio_list;

	void Circle(int x, int y) {
		x += 5;
		y += 5;
		Win->DrawLine(x+4, y, x+8, y, black);
		Win->DrawLine(x+2, y+1, x+4, y+1, black);
		Win->DrawLine(x+8, y+1, x+10, y+1, black);
		Win->DrawLine(x+1, y+2, x+1, y+4, black);
		Win->DrawLine(x+10, y+2, x+10, y+4, black);
		Win->DrawLine(x, y+4, x, y+8, black);
		Win->DrawLine(x+11, y+4, x+11, y+8, black);
		Win->DrawLine(x+1, y+8, x+1, y+10, black);
		Win->DrawLine(x+10, y+8, x+10, y+10, black);
		Win->DrawLine(x+2, y+10, x+4, y+10, black);
		Win->DrawLine(x+8, y+10, x+10, y+10, black);
		Win->DrawLine(x+4, y+11, x+8, y+11, black);
	}
	void PutSpot(int x, int y, unsigned long color)
	{
		x += 8;
		y += 8;
		Win->DrawLine(x+1, y, x+5, y, color);
		++y;
		Win->DrawLine(x, y, x+6, y, color);
		++y;
		Win->DrawLine(x, y, x+6, y, color);
		++y;
		Win->DrawLine(x, y, x+6, y, color);
		++y;
		Win->DrawLine(x, y, x+6, y, color);
		++y;
		Win->DrawLine(x+1, y, x+5, y, color);
	}
	void Spot(int x, int y) {
		PutSpot(x, y, black);
	}
	void UnSpot(int x, int y) {
		PutSpot(x, y, white);
	}
};
		

/* Numeric entry class */

class Numeric_Entry : public Dialog {

public:
	Numeric_Entry(FrameBuf *win, int x, int y, MFont *font, 
		FontServ *fontserv, unsigned long fg, unsigned long bg) :
							Dialog(win, x, y) {
		BitMap *space;

		Win = win;
		Fontserv = fontserv;
		Font = font;
		Fg = fg;
		Bg = bg;
		entry_list.next = NULL;
		current = &entry_list;
		space = Fontserv->Text_to_BitMap("0", Font, STYLE_NORM);
		Cwidth = space->width;
		Cheight = space->height;
		Fontserv->Free_Text(space);
	}
 
	virtual ~Numeric_Entry() { 
		struct numeric_entry *nent, *ntmp;

		for ( nent=entry_list.next; nent; ) {
			ntmp = nent;
			nent = nent->next;
			delete ntmp;
		}
	}

	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		struct numeric_entry *nent;

		Unused(button);		/* Default callback ignores button */
		Unused(doneflag);	/* Callback doesn't set doneflag */

		for ( nent=entry_list.next; nent; nent=nent->next ) {
			if ( IsSensitive(&nent->sensitive, x, y) ) {
				current->hilite = 0;
				Update_Entry(current);
				current = nent;
				DrawCursor(current);
				Win->Refresh();
			}
		}
	}
	virtual void HandleKeyPress(KeySym key, int *doneflag) {
		int n;

		Unused(doneflag);	/* Callback doesn't set doneflag */

		switch (key) {
			case XK_Tab:
				current->hilite = 0;
				Update_Entry(current);
				if ( current->next )
					current=current->next;
				else
					current=entry_list.next;
				current->hilite = 1;
				Update_Entry(current);
				break;

			case XK_Delete:
#if XK_Delete != XK_BackSpace
			case XK_BackSpace:
#endif
				if ( current->hilite ) {
					*current->variable = 0;
					current->hilite = 0;
				} else
					*current->variable /= 10;
				Update_Entry(current);
				DrawCursor(current);
				break;

			case XK_0:
				Press_Num(0);
				break;
			case XK_1:
				Press_Num(1);
				break;
			case XK_2:
				Press_Num(2);
				break;
			case XK_3:
				Press_Num(3);
				break;
			case XK_4:
				Press_Num(4);
				break;
			case XK_5:
				Press_Num(5);
				break;
			case XK_6:
				Press_Num(6);
				break;
			case XK_7:
				Press_Num(7);
				break;
			case XK_8:
				Press_Num(8);
				break;
			case XK_9:
				Press_Num(9);
				break;

			default:
				break;
		}
		Win->Refresh();
	}

	virtual void Add_Entry(int x, int y, int width, int is_default, 
								int *var) {
		struct numeric_entry *nent;

		for ( nent=&entry_list; nent->next; nent=nent->next );
		nent->next = new struct Numeric_Entry::numeric_entry;
		nent = nent->next;

		nent->variable = var;
		if ( is_default ) {
			current = nent;
			nent->hilite = 1;
		} else
			nent->hilite = 0;
		nent->x = x+3;
		nent->y = y+3;
		nent->width = width*Cwidth;
		nent->height = Cheight;
		nent->sensitive.top = y;
		nent->sensitive.left = x;
		nent->sensitive.bottom = y+3+Cheight+3;
		nent->sensitive.right = x+3+(width*Cwidth)+3;
		nent->next = NULL;
		Win->DrawRectangle(x, y, 3+(width*Cwidth)+3, 3+Cheight+3, Fg);
		Update_Entry(nent);
	}

private:
	FontServ     *Fontserv;
	MFont        *Font;
	int           Cwidth, Cheight;
	unsigned long Fg, Bg;

	struct numeric_entry {
		int *variable;
		Rect sensitive;
		int  x, y;
		int  width, height;
		int  end;
		int  hilite;
		struct numeric_entry *next;
		} entry_list, *current;

	void Press_Num(int n) {
		if ( (current->end+Cwidth) > current->width )
			return;
		if ( current->hilite ) {
			*current->variable = n;
			current->hilite = 0;
		} else {
			*current->variable *= 10;
			*current->variable += n;
		}
		Update_Entry(current);
		DrawCursor(current);
	}

	void Update_Entry(struct numeric_entry *entry) {
		BitMap       *text;
		char          buf[128];
		unsigned long color;

		/* Clear the entry */
		Win->FillRectangle(entry->x, entry->y, entry->width,
							entry->height, Bg);
		/* Create the new entry */
		sprintf(buf, "%d", *entry->variable);
		text = Fontserv->Text_to_BitMap(buf, Font, STYLE_NORM);
		entry->end = text->width;

		/* Print it, with appropriate highlighting */
		if ( entry->hilite ) {
			Win->FillRectangle(entry->x, entry->y, entry->width,
							entry->height, Fg);
			color = Bg;
		} else
			color = Fg;

		Win->Blit_BitMap(entry->x, entry->y, text->width,
					text->height, text->bits, color);
		Fontserv->Free_Text(text);
	}

	void DrawCursor(struct numeric_entry *entry) {
		Win->DrawLine(entry->x+entry->end, entry->y,
			entry->x+entry->end, entry->y+entry->height, Fg);
	}
	void UnDrawCursor(struct numeric_entry *entry) {
		Win->DrawLine(entry->x+entry->end, entry->y,
			entry->x+entry->end, entry->y+entry->height, Bg);
	}
};
		
/* Miscellaneous list elements */

/* Finally, the macintosh-like dialog class */

class Maclike_Dialog {

public:
	Maclike_Dialog(FrameBuf *win, int x, int y, int width, int height,
							unsigned long white);
	~Maclike_Dialog();

	void Add_Dialog(Dialog *dialog);
	void Add_Title(struct Title *title, int x, int y);
	void Add_BitMap(BitMap *bitmap, int x, int y, unsigned long color);
	void Add_CIcon(CIcon *icon, int x, int y);

	void Run(void);

private:
	struct dialog_elem {
		Dialog *dialog;
		struct dialog_elem *next;
	} dialog_list;

	FrameBuf *Win;
	unsigned char *SavedArea;

	int X, Y;
	int Width, Height;
	int done;
};
