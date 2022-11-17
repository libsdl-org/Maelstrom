
#include <unistd.h>

#include "dialog.h"


static void def_button_callback(int x, int y, int button, int *doneflag)
{
	Unused(x); Unused(y); Unused(button); Unused(doneflag);
	//error("Button press <%d> at (%d,%d)\n", button, x, y);
}
static void def_key_callback(KeySym key, int *doneflag)
{
	Unused(key); Unused(doneflag);
	//error("Got keysym %.4x\n", key);
}

Dialog:: Dialog(FrameBuf *win, int x, int y)
{
	Win = win;
	X = x;
	Y = y;
	button_callback = def_button_callback;
	key_callback = def_key_callback;
}

Radio_List:: Radio_List(FrameBuf *win, int x, int y, int nradios) :
							Dialog(win, x, y)
{
	radio_list = new struct radio[nradios];
	radiovar = &curradio;
	numradios = 0;
	black = Win->Map_Color(0x0000, 0x0000, 0x0000);
	white = Win->Map_Color(0xFFFF, 0xFFFF, 0xFFFF);
}

#define EXPAND_STEPS	50

Maclike_Dialog:: Maclike_Dialog(FrameBuf *win, int x, int y, 
				int width, int height, unsigned long white)
{
	unsigned long black, dark, medium, light;
	int           i;
	double        XX, YY, H, Hstep, V, Vstep;

	Win = win;
	X   = x;
	Y   = y;
	Width = width;
	Height = height;
	dialog_list.dialog = NULL;
	dialog_list.next = NULL;

	if ( ! Win->Grab_Area(x, y, width+1, height+1, &SavedArea) ) {
		error("Maclike_Dialog: Couldn't grab screen area!\n");
		exit(255);
	}

	/* Draw the nice Mac border area */
	black = Win->Map_Color(0x0000, 0x0000, 0x0000);
	dark = Win->Map_Color(0x6666, 0x6666, 0x9999);
	medium = Win->Map_Color(0xBBBB, 0xBBBB, 0xBBBB);
	light = Win->Map_Color(0xCCCC, 0xCCCC, 0xFFFF);
	Win->DrawLine(x, y, x+width, y, light);
	Win->DrawLine(x, y, x, y+height, light);
	Win->DrawLine(x, y+height, x+width, y+height, dark);
	Win->DrawLine(x+width, y, x+width, y+height+1, dark);
	Win->DrawRectangle(x+1, y+1, width-2, height-2, medium);
	Win->DrawLine(x+2, y+2, x+width-1, y+2, dark);
	Win->DrawLine(x+2, y+2, x+2, y+height-2, dark);
	Win->DrawLine(x+3, y+height-2, x+width-1, y+height-2, light);
	Win->DrawLine(x+width-2, y+3, x+width-2, y+height-1, light);
	Win->DrawRectangle(x+3, y+3, width-6, height-6, black);
	Win->FillRectangle(x+4, y+4, width-8, height-8, white);
	
	XX = (double)(X+Width/2);
	YY = (double)(Y+Height/2);
	Hstep = (double)(Width/EXPAND_STEPS);
	Vstep = (double)(Height/EXPAND_STEPS);
	for ( H=0, V=0, i=0; i<EXPAND_STEPS; ++i ) {
		H += Hstep;
		XX -= Hstep/2;
		V += Vstep;
		YY -= Vstep/2;
		Win->RefreshArea((int)XX, (int)YY, (int)H, (int)V);
		Win->Flush(1);
	}
	Win->Refresh();
	Win->Flush(1);
}

Maclike_Dialog:: ~Maclike_Dialog()
{
	struct dialog_elem *delem, *dtemp;

	/* Clean out the dialog element lists */
	for ( delem = dialog_list.next; delem; ) {
		dtemp = delem;
		delem = dtemp->next;
		delete dtemp;
	}

	/* Replace the old section of screen */
	Win->Set_Area(X, Y, Width+1, Height+1, SavedArea);
	Win->Free_Area(SavedArea);
	Win->Refresh();
	Win->Flush(1);
}

void
Maclike_Dialog:: Add_Dialog(Dialog *dialog)
{
	struct dialog_elem *delem;
	
	for ( delem = &dialog_list; delem->next; delem = delem->next );
	delem->next = new dialog_elem;
	delem = delem->next;
	delem->dialog = dialog;
	delem->next = NULL;
}

void
Maclike_Dialog:: Add_Title(struct Title *title, int x, int y)
{
	Win->Blit_Title(x, y, title->width, title->height, title->data);
}

void
Maclike_Dialog:: Add_BitMap(BitMap *title, int x, int y, unsigned long color)
{
	Win->Blit_BitMap(x, y, title->width, title->height, title->bits, color);
}

void
Maclike_Dialog:: Add_CIcon(CIcon *icon, int x, int y)
{
	Win->Blit_Sprite(x, y, icon->width, icon->height,
                                        	icon->pixels, icon->mask);
}

/* The big Kahones */
void
Maclike_Dialog:: Run(void)
{
	struct dialog_elem *delem;
	XEvent              event;
	KeySym              key;
	char                buf[128];

	for ( done = 0; !done; ) {
		Win->GetEvent(&event);

		switch (event.type) {
			/* -- Handle mouse clicks */
			case ButtonPress:
	for ( delem = dialog_list.next; delem; delem = delem->next )
		(delem->dialog)->HandleButtonPress(event.xbutton.x, 
				event.xbutton.y, event.xbutton.button, &done);
				break;
			/* -- Handle key presses/releases */
			case KeyPress:
				// Clear the status bits
				event.xkey.state = 0;
				Win->KeyToAscii(&event, buf, 127, &key);
	for ( delem = dialog_list.next; delem; delem = delem->next )
		(delem->dialog)->HandleKeyPress(key, &done);
				break;
		}
	}
}
