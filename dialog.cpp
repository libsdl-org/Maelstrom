
#include "dialog.h"


int Mac_Dialog::text_enabled = 0;

Mac_Dialog::Mac_Dialog(int x, int y)
{
	Screen = NULL;
	X = x;
	Y = y;
	button_callback = NULL;
	key_callback = NULL;
	errstr = NULL;
}

Mac_Button::Mac_Button(int x, int y, int width, int height,
				char *text, MFont *font, FontServ *fontserv, 
				int (*callback)(void)) : Mac_Dialog(x, y)
{
	SDL_Surface *textb;
	SDL_Rect dstrect;

	/* Set private variables */
	Width = width;
	Height = height;

	/* Build image of the button */
	button = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height,
						8, 0, 0, 0, 0);
	if ( button == NULL ) {
		SetError("%s", SDL_GetError());
		return;
	}
	button->format->palette->colors[0].r = 0xFF;
	button->format->palette->colors[0].g = 0xFF;
	button->format->palette->colors[0].b = 0xFF;
	button->format->palette->colors[1].r = 0x00;
	button->format->palette->colors[1].g = 0x00;
	button->format->palette->colors[1].b = 0x00;
	textb = fontserv->TextImage(text, font, STYLE_NORM,
					0x00, 0x00, 0x00);
	if ( textb != NULL ) {
		if ( (textb->w <= button->w) && 
					(textb->h <= button->h) ) {
			dstrect.x = (button->w-textb->w)/2;
			dstrect.y = (button->h-textb->h)/2;
			dstrect.w = textb->w;
			dstrect.h = textb->h;
			SDL_BlitSurface(textb, NULL, button, &dstrect);
		}
		fontserv->FreeText(textb);
	}
	Bevel_Button(button);

	/* Set the callback */
	Callback = callback;
}

Mac_DefaultButton::Mac_DefaultButton(int x, int y, int width, int height,
				char *text, MFont *font, FontServ *fontserv, 
						int (*callback)(void)) : 
	Mac_Button(x, y, width, height, text, font, fontserv, callback)
{
	return;
}

Mac_CheckBox::Mac_CheckBox(int *toggle, int x, int y, char *text,
			MFont *font, FontServ *fontserv) : Mac_Dialog(x, y)
{
	/* Create the text label */
	Fontserv = fontserv;
	label = Fontserv->TextImage(text, font, STYLE_NORM, 0, 0, 0);

	/* Set the checkbox variable */
	checkval = toggle;
}

Mac_RadioList::Mac_RadioList(int *variable, int x, int y,
			MFont *font, FontServ *fontserv) : Mac_Dialog(x, y)
{
	Fontserv = fontserv;
	Font = font;
	radiovar = variable;
	*radiovar = 0;
	radio_list.next = NULL;
}

Mac_TextEntry::Mac_TextEntry(int x, int y,
			MFont *font, FontServ *fontserv) : Mac_Dialog(x, y)
{
	Fontserv = fontserv;
	Font = font;
	Cwidth = Fontserv->TextWidth("0", Font, STYLE_NORM);
	Cheight = Fontserv->TextHeight(font);
	entry_list.next = NULL;
	current = &entry_list;
	EnableText();
}

Mac_NumericEntry::Mac_NumericEntry(int x, int y,
			MFont *font, FontServ *fontserv) : Mac_Dialog(x, y)
{
	Fontserv = fontserv;
	Font = font;
	Cwidth = Fontserv->TextWidth("0", Font, STYLE_NORM);
	Cheight = Fontserv->TextHeight(font);
	entry_list.next = NULL;
	current = &entry_list;
}

Maclike_Dialog:: Maclike_Dialog(int x, int y, int width, int height,
							FrameBuf *screen)
{
	Screen = screen;
	X = x;
	Y = y;
	Width = width;
	Height = height;
	rect_list.next = NULL;
	image_list.next = NULL;
	dialog_list.next = NULL;
}

Maclike_Dialog:: ~Maclike_Dialog()
{
	struct rect_elem *rect, *rtemp;
	struct image_elem *image, *itemp;
	struct dialog_elem *dialog, *dtemp;

	/* Clean out the lists */
	for ( rect = rect_list.next; rect; ) {
		rtemp = rect;
		rect = rect->next;
		delete rtemp;
	}
	for ( image = image_list.next; image; ) {
		itemp = image;
		image = image->next;
		delete itemp;
	}
	for ( dialog = dialog_list.next; dialog; ) {
		dtemp = dialog;
		dialog = dialog->next;
		delete dtemp->dialog;
		delete dtemp;
	}
}

void
Maclike_Dialog:: Add_Rectangle(int x, int y, int w, int h, Uint32 color)
{
	struct rect_elem *relem;
	
	for ( relem = &rect_list; relem->next; relem = relem->next );
	relem->next = new rect_elem;
	relem = relem->next;
	relem->x = x;
	relem->y = y;
	relem->w = w;
	relem->h = h;
	relem->color = color;
	relem->next = NULL;
}

void
Maclike_Dialog:: Add_Image(SDL_Surface *image, int x, int y)
{
	struct image_elem *ielem;
	
	for ( ielem = &image_list; ielem->next; ielem = ielem->next );
	ielem->next = new image_elem;
	ielem = ielem->next;
	ielem->image = image;
	ielem->x = x;
	ielem->y = y;
	ielem->next = NULL;
}

void
Maclike_Dialog:: Add_Dialog(Mac_Dialog *dialog)
{
	struct dialog_elem *delem;
	
	for ( delem = &dialog_list; delem->next; delem = delem->next );
	delem->next = new dialog_elem;
	delem = delem->next;
	delem->dialog = dialog;
	delem->next = NULL;
}


/* The big Kahones */
void
Maclike_Dialog:: Run(int expand_steps)
{
	SDL_Surface *savedfg;
	SDL_Surface *savedbg;
	SDL_Event event;
	struct rect_elem *relem;
	struct image_elem *ielem;
	struct dialog_elem *delem;
	Uint32 black, dark, medium, light, white;
	int i, done;
	int maxX, maxY;
	double XX, YY, H, Hstep, V, Vstep;

	/* Save the area behind the dialog box */
	savedfg = Screen->GrabArea(X, Y, Width, Height);
	Screen->FocusBG();
	savedbg = Screen->GrabArea(X, Y, Width, Height);

	/* Show the dialog box with the nice Mac border */
	black = Screen->MapRGB(0x00, 0x00, 0x00);
	dark = Screen->MapRGB(0x66, 0x66, 0x99);
	medium = Screen->MapRGB(0xBB, 0xBB, 0xBB);
	light = Screen->MapRGB(0xCC, 0xCC, 0xFF);
	white = Screen->MapRGB(0xFF, 0xFF, 0xFF);
	maxX = X+Width-1;
	maxY = Y+Height-1;
	Screen->DrawLine(X, Y, maxX, Y, light);
	Screen->DrawLine(X, Y, X, maxY, light);
	Screen->DrawLine(X, maxY, maxX, maxY, dark);
	Screen->DrawLine(maxX, Y, maxX, maxY, dark);
	Screen->DrawRect(X+1, Y+1, Width-2, Height-2, medium);
	Screen->DrawLine(X+2, Y+2, maxX-2, Y+2, dark);
	Screen->DrawLine(X+2, Y+2, X+2, maxY-2, dark);
	Screen->DrawLine(X+3, maxY-2, maxX-2, maxY-2, light);
	Screen->DrawLine(maxX-2, Y+3, maxX-2, maxY-2, light);
	Screen->DrawRect(X+3, Y+3, Width-6, Height-6, black);
	Screen->FillRect(X+4, Y+4, Width-8, Height-8, white);
	Screen->FocusFG();

	/* Allow the dialog to expand slowly */
	XX = (double)(X+Width/2);
	YY = (double)(Y+Height/2);
	Hstep = (double)(Width/expand_steps);
	Vstep = (double)(Height/expand_steps);
	for ( H=0, V=0, i=0; i<expand_steps; ++i ) {
		H += Hstep;
		XX -= Hstep/2;
		V += Vstep;
		YY -= Vstep/2;
		if ( XX < X ) {
			XX = X;
		}
		if ( YY < Y ) {
			YY = Y;
		}
		if ( H > Width ) {
			H = Width;
		}
		if ( V > Height ) {
			V = Height;
		}
		Screen->Clear((Uint16)XX, (Uint16)YY, (Uint16)H, (Uint16)V);
		Screen->Update();
	}
	Screen->Clear((Uint16)X, (Uint16)Y, (Uint16)Width, (Uint16)Height);
	Screen->Update();

	/* Draw the dialog elements (after the slow expand) */
	for ( relem = rect_list.next; relem; relem = relem->next ) {
		Screen->DrawRect(X+4+relem->x, Y+4+relem->y, relem->w, relem->h,
								relem->color);
	}
	for ( ielem = image_list.next; ielem; ielem = ielem->next ) {
		Screen->QueueBlit(X+4+ielem->x, Y+4+ielem->y,
							ielem->image, NOCLIP);
	}
	for ( delem = dialog_list.next; delem; delem = delem->next ) {
		delem->dialog->Map(X+4, Y+4, Screen,
					0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00);
		delem->dialog->Show();
	}
	Screen->Update();

	/* Wait until the dialog box is done */
	for ( done = 0; !done; ) {
		Screen->WaitEvent(&event);

		switch (event.type) {
			/* -- Handle mouse clicks */
			case SDL_MOUSEBUTTONDOWN:
	for ( delem = dialog_list.next; delem; delem = delem->next )
		(delem->dialog)->HandleButtonPress(event.button.x, 
				event.button.y, event.button.button, &done);
				break;
			/* -- Handle key presses */
			case SDL_KEYDOWN:
	for ( delem = dialog_list.next; delem; delem = delem->next )
		(delem->dialog)->HandleKeyPress(event.key.keysym, &done);
				break;
		}
	}

	/* Replace the old section of screen */
	if ( savedbg ) {
		Screen->FocusBG();
		Screen->QueueBlit(X, Y, savedbg, NOCLIP);
		Screen->Update();
		Screen->FocusFG();
		Screen->FreeImage(savedbg);
	}
	if ( savedfg ) {
		Screen->QueueBlit(X, Y, savedfg, NOCLIP);
		Screen->Update();
		Screen->FreeImage(savedfg);
	}
}
