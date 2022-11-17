
#include <stdio.h>
#include <stdarg.h>

#include "SDL_FrameBuf.h"
#include "Mac_FontServ.h"

/*  This is a class set for Macintosh-like dialogue boxes. :) */
/*  Sorta complex... */

/* Defaults for various dialog classes */

#define BUTTON_WIDTH	75
#define BUTTON_HEIGHT	19

#define BOX_WIDTH	170
#define BOX_HEIGHT	20

#define EXPAND_STEPS	50

class Mac_Dialog {

public:
	Mac_Dialog(int x, int y);
	virtual ~Mac_Dialog() { }

	/* Input handling */
	virtual void SetButtonPress(void (*new_button_callback)
				(int x, int y, int button, int *doneflag)) {
		button_callback = new_button_callback;
	}
	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		if ( button_callback ) {
			(*button_callback)(x, y, button, doneflag);
		}
	}
	virtual void SetKeyPress(void (*new_key_callback)
				(SDL_keysym key, int *doneflag)) {
		key_callback = new_key_callback;
	}
	virtual void HandleKeyPress(SDL_keysym key, int *doneflag) {
		if ( key_callback ) {
			(*key_callback)(key, doneflag);
		}
	}

	/* Display handling */
	virtual void Map(int Xoff, int Yoff, FrameBuf *screen,
				Uint8 R_bg, Uint8 G_bg, Uint8 B_bg,
				Uint8 R_fg, Uint8 G_fg, Uint8 B_fg) {
		X += Xoff;
		Y += Yoff;
		Screen = screen;
	}
	virtual void Show(void) {
	}

	static void EnableText(void) {
		if ( text_enabled++ == 0 ) {
			SDL_EnableUNICODE(1);
		}
	}
	static void DisableText(void) {
		if ( --text_enabled == 0 ) {
			SDL_EnableUNICODE(0);
		}
	}

	/* Error message routine */
	virtual char *Error(void) {
		return(errstr);
	}

protected:
	static int text_enabled;
	FrameBuf *Screen;
	int  X, Y;
	void (*button_callback)(int x, int y, int button, int *doneflag);
	void (*key_callback)(SDL_keysym key, int *doneflag);

	/* Utility routines for dialogs */
	int IsSensitive(SDL_Rect *area, int x, int y) {
		if ( (y > area->y) && (y < (area->y+area->h)) &&
	    	     (x > area->x) && (x < (area->x+area->w)) )
			return(1);
		return(0);
	}

	/* Error message */
	virtual void SetError(char *fmt, ...) {
		va_list ap;

		va_start(ap, fmt);
		vsprintf(errbuf, fmt, ap);
		va_end(ap);
		errstr = errbuf;
        }
	char *errstr;
	char  errbuf[1024];
};


/* The button callbacks should return 1 if they finish the dialog,
   or 0 if they do not.
*/

class Mac_Button : public Mac_Dialog {

public:
	Mac_Button(int x, int y, int width, int height,
		char *text, MFont *font, FontServ *fontserv, 
				int (*callback)(void));
	virtual ~Mac_Button() {
		SDL_FreeSurface(button);
	}

	virtual void Map(int Xoff, int Yoff, FrameBuf *screen,
				Uint8 R_bg, Uint8 G_bg, Uint8 B_bg,
				Uint8 R_fg, Uint8 G_fg, Uint8 B_fg) {
		/* Do the normal dialog mapping */
		Mac_Dialog::Map(Xoff, Yoff, screen,
				R_bg, G_bg, B_bg, R_fg, G_fg, B_fg);

		/* Set up the button sensitivity */
		sensitive.x = X;
		sensitive.y  = Y;
		sensitive.w = Width;
		sensitive.h = Height;

		/* Map the bitmap image */
		button->format->palette->colors[0].r = R_bg;
		button->format->palette->colors[0].g = G_bg;
		button->format->palette->colors[0].b = B_bg;
		button->format->palette->colors[1].r = R_fg;
		button->format->palette->colors[1].g = G_fg;
		button->format->palette->colors[1].b = B_fg;
	}
	virtual void Show(void) {
		Screen->QueueBlit(X, Y, button, NOCLIP);
	}

	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		if ( IsSensitive(&sensitive, x, y) )
			ActivateButton(doneflag);
	}

protected:
	int Width, Height;
	SDL_Surface *button;
	SDL_Rect sensitive;
	int (*Callback)(void);

	virtual void Bevel_Button(SDL_Surface *image) {
		Uint16 h;
		Uint8 *image_bits;

		image_bits = (Uint8 *)image->pixels;

		/* Bevel upper corners */
		memset(image_bits+3, 0x01, image->w-6);
		image_bits += image->pitch;
		memset(image_bits+1, 0x01, 2);
		memset(image_bits+image->w-3, 0x01, 2);
		image_bits += image->pitch;
		memset(image_bits+1, 0x01, 1);
		memset(image_bits+image->w-2, 0x01, 1);
		image_bits += image->pitch;

		/* Draw sides */
		for ( h=3; h<(image->h-3); ++h ) {
			image_bits[0] = 0x01;
			image_bits[image->w-1] = 0x01;
			image_bits += image->pitch;
		}

		/* Bevel bottom corners */
		memset(image_bits+1, 0x01, 1);
		memset(image_bits+image->w-2, 0x01, 1);
		image_bits += image->pitch;
		memset(image_bits+1, 0x01, 2);
		memset(image_bits+image->w-3, 0x01, 2);
		image_bits += image->pitch;
		memset(image_bits+3, 0x01, image->w-6);
	}
	virtual void InvertImage(void) {
		int i;
		Uint8 *buf;

		for ( i=button->h*button->pitch, buf=(Uint8 *)button->pixels;
							i > 0; --i, ++buf ) {
			*buf = !*buf;
		}
	}
	virtual void ActivateButton(int *doneflag) {
		/* Flash the button */
		InvertImage();
		Show();
		Screen->Update();
		SDL_Delay(50);
		InvertImage();
		Show();
		Screen->Update();
		/* Run the callback */
		if ( Callback )
			*doneflag = (*Callback)();
		else
			*doneflag = 1;
	}
};

/* The only difference between this button and the Mac_Button is that
   if <Return> is pressed, this button is activated.
*/
class Mac_DefaultButton : public Mac_Button {

public:
	Mac_DefaultButton(int x, int y, int width, int height,
				char *text, MFont *font, FontServ *fontserv, 
						int (*callback)(void));
	virtual ~Mac_DefaultButton() { }

	virtual void HandleKeyPress(SDL_keysym key, int *doneflag) {
		if ( key.sym == SDLK_RETURN )
			ActivateButton(doneflag);
	}

	virtual void Map(int Xoff, int Yoff, FrameBuf *screen,
				Uint8 R_bg, Uint8 G_bg, Uint8 B_bg,
				Uint8 R_fg, Uint8 G_fg, Uint8 B_fg) {
		Mac_Button::Map(Xoff, Yoff, screen,
				R_bg, G_bg, B_bg, R_fg, G_fg, B_fg);
		Fg = Screen->MapRGB(R_fg, G_fg, B_fg);
	}
	virtual void Show(void) {
		int x, y, maxx, maxy;

		/* Show the normal button */
		Mac_Button::Show();

		/* Show the thick edge */
		x = X-4;
		maxx = x+4+Width+4-1;
		y = Y-4;
		maxy = y+4+Height+4-1;

		Screen->DrawLine(x+5, y, maxx-5, y, Fg);
		Screen->DrawLine(x+3, y+1, maxx-3, y+1, Fg);
		Screen->DrawLine(x+2, y+2, maxx-2, y+2, Fg);
		Screen->DrawLine(x+1, y+3, x+5, y+3, Fg);
		Screen->DrawLine(maxx-5, y+3, maxx-1, y+3, Fg);
		Screen->DrawLine(x+1, y+4, x+3, y+4, Fg);
		Screen->DrawLine(maxx-3, y+4, maxx-1, y+4, Fg);
		Screen->DrawLine(x, y+5, x+3, y+5, Fg);
		Screen->DrawLine(maxx-3, y+5, maxx, y+5, Fg);

		Screen->DrawLine(x, y+6, x, maxy-6, Fg);
		Screen->DrawLine(maxx, y+6, maxx, maxy-6, Fg);
		Screen->DrawLine(x+1, y+6, x+1, maxy-6, Fg);
		Screen->DrawLine(maxx-1, y+6, maxx-1, maxy-6, Fg);
		Screen->DrawLine(x+2, y+6, x+2, maxy-6, Fg);
		Screen->DrawLine(maxx-2, y+6, maxx-2, maxy-6, Fg);

		Screen->DrawLine(x, maxy-5, x+3, maxy-5, Fg);
		Screen->DrawLine(maxx-3, maxy-5, maxx, maxy-5, Fg);
		Screen->DrawLine(x+1, maxy-4, x+3, maxy-4, Fg);
		Screen->DrawLine(maxx-3, maxy-4, maxx-1, maxy-4, Fg);
		Screen->DrawLine(x+1, maxy-3, x+5, maxy-3, Fg);
		Screen->DrawLine(maxx-5, maxy-3, maxx-1, maxy-3, Fg);
		Screen->DrawLine(x+2, maxy-2, maxx-2, maxy-2, Fg);
		Screen->DrawLine(x+3, maxy-1, maxx-3, maxy-1, Fg);
		Screen->DrawLine(x+5, maxy, maxx-5, maxy, Fg);
	}

protected:
	Uint32 Fg;		/* The foreground color of the dialog */

};

/* Class of checkboxes */

#define CHECKBOX_SIZE	12

class Mac_CheckBox : public Mac_Dialog {

public:
	Mac_CheckBox(int *toggle, int x, int y, char *text,
				MFont *font, FontServ *fontserv);
	virtual ~Mac_CheckBox() {
		if ( label ) {
			Fontserv->FreeText(label);
		}
	}

	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		if ( IsSensitive(&sensitive, x, y) ) {
			*checkval = !*checkval;
			Check_Box(*checkval);
			Screen->Update();
		}
	}

	virtual void Map(int Xoff, int Yoff, FrameBuf *screen,
				Uint8 R_bg, Uint8 G_bg, Uint8 B_bg,
				Uint8 R_fg, Uint8 G_fg, Uint8 B_fg) {
		/* Do the normal dialog mapping */
		Mac_Dialog::Map(Xoff, Yoff, screen,
				R_bg, G_bg, B_bg, R_fg, G_fg, B_fg);

		/* Set up the checkbox sensitivity */
		sensitive.x = X;
		sensitive.y = Y;
		sensitive.w = CHECKBOX_SIZE;
		sensitive.h = CHECKBOX_SIZE;

		/* Get the screen colors */
		Fg = Screen->MapRGB(R_fg, G_fg, B_fg);
		Bg = Screen->MapRGB(R_bg, G_bg, B_bg);

		/* Map the checkbox text */
		label->format->palette->colors[1].r = R_fg;
		label->format->palette->colors[1].g = G_fg;
		label->format->palette->colors[1].b = B_fg;
	}
	virtual void Show(void) {
		Screen->DrawRect(X, Y, CHECKBOX_SIZE, CHECKBOX_SIZE, Fg);
		if ( label ) {
			Screen->QueueBlit(X+CHECKBOX_SIZE+4, Y-2, label,NOCLIP);
		}
		Check_Box(*checkval);
	}

private:
	FontServ *Fontserv;
	SDL_Surface *label;
	Uint32 Fg, Bg;
	int *checkval;
	SDL_Rect sensitive;

	void Check_Box(int checked) {
		Uint32 color;

		if ( checked )
			color = Fg;
		else
			color = Bg;

		Screen->DrawLine(X, Y,
				X+CHECKBOX_SIZE-1, Y+CHECKBOX_SIZE-1, color);
		Screen->DrawLine(X, Y+CHECKBOX_SIZE-1,
					X+CHECKBOX_SIZE-1, Y, color);
	}
};

/* Class of radio buttons */

class Mac_RadioList : public Mac_Dialog {

public:
	Mac_RadioList(int *variable, int x, int y,
			MFont *font, FontServ *fontserv);
	virtual ~Mac_RadioList() {
		struct radio *radio, *old;

		for ( radio=radio_list.next; radio; ) {
			old = radio;
			radio = radio->next;
			if ( old->label )
				Fontserv->FreeText(old->label);
			delete old;
		}
	}

	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		int n;
		struct radio *radio, *oldradio;

		oldradio = radio_list.next;
		for (n=0, radio=radio_list.next; radio; radio=radio->next, ++n){
			if ( n == *radiovar ) {
				oldradio = radio;
				break;
			}
		}
		for (n=0, radio=radio_list.next; radio; radio=radio->next, ++n){
			if ( IsSensitive(&radio->sensitive, x, y) ) {
				Spot(oldradio->x, oldradio->y, Bg);
				*radiovar = n;
				Spot(radio->x, radio->y, Fg);
				Screen->Update();
			}
		}
	}

	virtual void Add_Radio(int x, int y, char *text) {
		struct radio *radio;

		for ( radio=&radio_list; radio->next; radio=radio->next )
			/* Loop to end of radio box list */;
/* Which is ANSI C++? */
#ifdef linux
		radio->next = new struct Mac_RadioList::radio;
#else
		radio->next = new struct radio;
#endif
		radio = radio->next;
		radio->label = Fontserv->TextImage(text, Font,
							STYLE_NORM, 0, 0, 0);
		radio->x = x;
		radio->y = y;
		radio->sensitive.x = x;
		radio->sensitive.y = y;
		radio->sensitive.w = 20+radio->label->w;
		radio->sensitive.h = BOX_HEIGHT;
		radio->next = NULL;
	}

	virtual void Map(int Xoff, int Yoff, FrameBuf *screen,
				Uint8 R_bg, Uint8 G_bg, Uint8 B_bg,
				Uint8 R_fg, Uint8 G_fg, Uint8 B_fg) {
		struct radio *radio;

		/* Do the normal dialog mapping */
		Mac_Dialog::Map(Xoff, Yoff, screen,
				R_bg, G_bg, B_bg, R_fg, G_fg, B_fg);

		/* Get the screen colors */
		Fg = Screen->MapRGB(R_fg, G_fg, B_fg);
		Bg = Screen->MapRGB(R_bg, G_bg, B_bg);

		/* Adjust sensitivity and map the radiobox text */
		for ( radio=radio_list.next; radio; radio=radio->next ) {
			radio->x += Xoff;
			radio->y += Yoff;
			radio->sensitive.x += Xoff;
			radio->sensitive.y += Yoff;
			radio->label->format->palette->colors[1].r = R_fg;
			radio->label->format->palette->colors[1].g = G_fg;
			radio->label->format->palette->colors[1].b = B_fg;
		}
	}
	virtual void Show(void) {
		int n;
		struct radio *radio;

		for (n=0, radio=radio_list.next; radio; radio=radio->next, ++n){
			Circle(radio->x, radio->y);
			if ( n == *radiovar ) {
				Spot(radio->x, radio->y, Fg);
			}
			if ( radio->label ) {
				Screen->QueueBlit(radio->x+21, radio->y+3,
							radio->label, NOCLIP);
			}
		}
	}

private:
	FontServ *Fontserv;
	MFont *Font;
	Uint32 Fg, Bg;
	int *radiovar;
	struct radio {
		SDL_Surface *label;
		int x, y;
		SDL_Rect sensitive;
		struct radio *next;
	} radio_list;

	void Circle(int x, int y) {
		x += 5;
		y += 5;
		Screen->DrawLine(x+4, y, x+7, y, Fg);
		Screen->DrawLine(x+2, y+1, x+3, y+1, Fg);
		Screen->DrawLine(x+8, y+1, x+9, y+1, Fg);
		Screen->DrawLine(x+1, y+2, x+1, y+3, Fg);
		Screen->DrawLine(x+10, y+2, x+10, y+3, Fg);
		Screen->DrawLine(x, y+4, x, y+7, Fg);
		Screen->DrawLine(x+11, y+4, x+11, y+7, Fg);
		Screen->DrawLine(x+1, y+8, x+1, y+9, Fg);
		Screen->DrawLine(x+10, y+8, x+10, y+9, Fg);
		Screen->DrawLine(x+2, y+10, x+3, y+10, Fg);
		Screen->DrawLine(x+8, y+10, x+9, y+10, Fg);
		Screen->DrawLine(x+4, y+11, x+7, y+11, Fg);
	}
	void Spot(int x, int y, Uint32 color)
	{
		x += 8;
		y += 8;
		Screen->DrawLine(x+1, y, x+4, y, color);
		++y;
		Screen->DrawLine(x, y, x+5, y, color);
		++y;
		Screen->DrawLine(x, y, x+5, y, color);
		++y;
		Screen->DrawLine(x, y, x+5, y, color);
		++y;
		Screen->DrawLine(x, y, x+5, y, color);
		++y;
		Screen->DrawLine(x+1, y, x+4, y, color);
	}
};
		

/* Class of text entry boxes */

class Mac_TextEntry : public Mac_Dialog {

public:
	Mac_TextEntry(int x, int y, MFont *font, FontServ *fontserv);
	virtual ~Mac_TextEntry() { 
		struct text_entry *entry, *old;

		for ( entry=entry_list.next; entry; ) {
			old = entry;
			entry = entry->next;
			if ( old->text )
				Fontserv->FreeText(old->text);
			delete old;
		}
		DisableText();
	}

	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		struct text_entry *entry;

		for ( entry=entry_list.next; entry; entry=entry->next ) {
			if ( IsSensitive(&entry->sensitive, x, y) ) {
				current->hilite = 0;
				Update_Entry(current);
				current = entry;
				DrawCursor(current);
				Screen->Update();
			}
		}
	}
	virtual void HandleKeyPress(SDL_keysym key, int *doneflag) {
		int n;

		switch (key.sym) {
			case SDLK_TAB:
				current->hilite = 0;
				Update_Entry(current);
				if ( current->next )
					current=current->next;
				else
					current=entry_list.next;
				current->hilite = 1;
				Update_Entry(current);
				break;

			case SDLK_DELETE:
			case SDLK_BACKSPACE:
				if ( current->hilite ) {
					*current->variable = '\0';
					current->hilite = 0;
				} else if ( *current->variable ) {
					n = strlen(current->variable);
					current->variable[n-1] = '\0';
				}
				Update_Entry(current);
				DrawCursor(current);
				break;

			default:
				if ( (current->end+Cwidth) > current->width )
					return;
				if ( key.unicode ) {
					current->hilite = 0;
					n = strlen(current->variable);
					current->variable[n] = (char)key.unicode;
					current->variable[n+1] = '\0';
					Update_Entry(current);
					DrawCursor(current);
				}
				break;
		}
		Screen->Update();
	}

	virtual void Add_Entry(int x, int y, int width, int is_default, 
							char *variable) {
		struct text_entry *entry;

		for ( entry=&entry_list; entry->next; entry=entry->next )
			/* Loop to end of entry list */;
		entry->next = new struct text_entry;
		entry = entry->next;

		entry->variable = variable;
		if ( is_default ) {
			current = entry;
			entry->hilite = 1;
		} else
			entry->hilite = 0;
		entry->x = x+3;
		entry->y = y+3;
		entry->width = width*Cwidth;
		entry->height = Cheight;
		entry->sensitive.x = x;
		entry->sensitive.y = y;
		entry->sensitive.w = 3+(width*Cwidth)+3;
		entry->sensitive.h = 3+Cheight+3;
		entry->text = NULL;
		entry->next = NULL;
	}

	virtual void Map(int Xoff, int Yoff, FrameBuf *screen,
				Uint8 R_bg, Uint8 G_bg, Uint8 B_bg,
				Uint8 R_fg, Uint8 G_fg, Uint8 B_fg) {
		struct text_entry *entry;

		/* Do the normal dialog mapping */
		Mac_Dialog::Map(Xoff, Yoff, screen,
				R_bg, G_bg, B_bg, R_fg, G_fg, B_fg);

		/* Get the screen colors */
		foreground.r = R_fg;
		foreground.g = G_fg;
		foreground.b = B_fg;
		background.r = R_bg;
		background.g = G_bg;
		background.b = B_bg;
		Fg = Screen->MapRGB(R_fg, G_fg, B_fg);
		Bg = Screen->MapRGB(R_bg, G_bg, B_bg);

		/* Adjust sensitivity and map the radiobox text */
		for ( entry=entry_list.next; entry; entry=entry->next ) {
			entry->x += Xoff;
			entry->y += Yoff;
			entry->sensitive.x += Xoff;
			entry->sensitive.y += Yoff;
		}
	}
	virtual void Show(void) {
		struct text_entry *entry;

		for ( entry=entry_list.next; entry; entry=entry->next ) {
			Screen->DrawRect(entry->x-3, entry->y-3,
					3+entry->width+3, 3+Cheight+3, Fg);
			Update_Entry(entry);
		}
	}

private:
	FontServ *Fontserv;
	MFont *Font;
	Uint32 Fg, Bg;
	int Cwidth, Cheight;
	SDL_Color foreground;
	SDL_Color background;

	struct text_entry {
		SDL_Surface *text;
		char *variable;
		SDL_Rect sensitive;
		int  x, y;
		int  width, height;
		int  end;
		int  hilite;
		struct text_entry *next;
	} entry_list, *current;


	void Update_Entry(struct text_entry *entry) {
		Uint32 clear;

		/* Create the new entry text */
		if ( entry->text ) {
			Fontserv->FreeText(entry->text);
		}
		if ( entry->hilite ) {
			clear = Fg;
			entry->text = Fontserv->TextImage(entry->variable,
				Font, STYLE_NORM, background, foreground);
		} else {
			clear = Bg;
			entry->text = Fontserv->TextImage(entry->variable,
				Font, STYLE_NORM, foreground, background);
		}
		Screen->FillRect(entry->x, entry->y,
					entry->width, entry->height, clear);
		if ( entry->text ) {
			entry->end = entry->text->w;
			Screen->QueueBlit(entry->x, entry->y, entry->text, NOCLIP);
		} else {
			entry->end = 0;
		}
	}
	void DrawCursor(struct text_entry *entry) {
		Screen->DrawLine(entry->x+entry->end, entry->y,
			entry->x+entry->end, entry->y+entry->height-1, Fg);
	}
};


/* Class of numeric entry boxes */

class Mac_NumericEntry : public Mac_Dialog {

public:
	Mac_NumericEntry(int x, int y, MFont *font, FontServ *fontserv);
	virtual ~Mac_NumericEntry() { 
		struct numeric_entry *entry, *old;

		for ( entry=entry_list.next; entry; ) {
			old = entry;
			entry = entry->next;
			if ( old->text )
				Fontserv->FreeText(old->text);
			delete old;
		}
	}

	virtual void HandleButtonPress(int x, int y, int button, 
							int *doneflag) {
		struct numeric_entry *entry;

		for ( entry=entry_list.next; entry; entry=entry->next ) {
			if ( IsSensitive(&entry->sensitive, x, y) ) {
				current->hilite = 0;
				Update_Entry(current);
				current = entry;
				DrawCursor(current);
				Screen->Update();
			}
		}
	}
	virtual void HandleKeyPress(SDL_keysym key, int *doneflag) {
		int n;

		switch (key.sym) {
			case SDLK_TAB:
				current->hilite = 0;
				Update_Entry(current);
				if ( current->next )
					current=current->next;
				else
					current=entry_list.next;
				current->hilite = 1;
				Update_Entry(current);
				break;

			case SDLK_DELETE:
			case SDLK_BACKSPACE:
				if ( current->hilite ) {
					*current->variable = 0;
					current->hilite = 0;
				} else
					*current->variable /= 10;
				Update_Entry(current);
				DrawCursor(current);
				break;

			case SDLK_0:
			case SDLK_1:
			case SDLK_2:
			case SDLK_3:
			case SDLK_4:
			case SDLK_5:
			case SDLK_6:
			case SDLK_7:
			case SDLK_8:
			case SDLK_9:
				n = (key.sym-SDLK_0);
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
				break;

			default:
				break;
		}
		Screen->Update();
	}

	virtual void Add_Entry(int x, int y, int width, int is_default, 
							int *variable) {
		struct numeric_entry *entry;

		for ( entry=&entry_list; entry->next; entry=entry->next )
			/* Loop to end of numeric entry list */;
		entry->next = new struct numeric_entry;
		entry = entry->next;

		entry->variable = variable;
		if ( is_default ) {
			current = entry;
			entry->hilite = 1;
		} else
			entry->hilite = 0;
		entry->x = x+3;
		entry->y = y+3;
		entry->width = width*Cwidth;
		entry->height = Cheight;
		entry->sensitive.x = x;
		entry->sensitive.y = y;
		entry->sensitive.w = 3+(width*Cwidth)+3;
		entry->sensitive.h = 3+Cheight+3;
		entry->text = NULL;
		entry->next = NULL;
	}

	virtual void Map(int Xoff, int Yoff, FrameBuf *screen,
				Uint8 R_bg, Uint8 G_bg, Uint8 B_bg,
				Uint8 R_fg, Uint8 G_fg, Uint8 B_fg) {
		struct numeric_entry *entry;

		/* Do the normal dialog mapping */
		Mac_Dialog::Map(Xoff, Yoff, screen,
				R_bg, G_bg, B_bg, R_fg, G_fg, B_fg);

		/* Get the screen colors */
		foreground.r = R_fg;
		foreground.g = G_fg;
		foreground.b = B_fg;
		background.r = R_bg;
		background.g = G_bg;
		background.b = B_bg;
		Fg = Screen->MapRGB(R_fg, G_fg, B_fg);
		Bg = Screen->MapRGB(R_bg, G_bg, B_bg);

		/* Adjust sensitivity and map the radiobox text */
		for ( entry=entry_list.next; entry; entry=entry->next ) {
			entry->x += Xoff;
			entry->y += Yoff;
			entry->sensitive.x += Xoff;
			entry->sensitive.y += Yoff;
		}
	}
	virtual void Show(void) {
		struct numeric_entry *entry;

		for ( entry=entry_list.next; entry; entry=entry->next ) {
			Screen->DrawRect(entry->x-3, entry->y-3,
					3+entry->width+3, 3+Cheight+3, Fg);
			Update_Entry(entry);
		}
	}

private:
	FontServ *Fontserv;
	MFont *Font;
	Uint32 Fg, Bg;
	int Cwidth, Cheight;
	SDL_Color foreground;
	SDL_Color background;

	struct numeric_entry {
		SDL_Surface *text;
		int *variable;
		SDL_Rect sensitive;
		int  x, y;
		int  width, height;
		int  end;
		int  hilite;
		struct numeric_entry *next;
	} entry_list, *current;


	void Update_Entry(struct numeric_entry *entry) {
		char buf[128];
		Uint32 clear;

		/* Create the new entry text */
		if ( entry->text ) {
			Fontserv->FreeText(entry->text);
		}
		sprintf(buf, "%d", *entry->variable);

		if ( entry->hilite ) {
			clear = Fg;
			entry->text = Fontserv->TextImage(buf,
				Font, STYLE_NORM, background, foreground);
		} else {
			clear = Bg;
			entry->text = Fontserv->TextImage(buf,
				Font, STYLE_NORM, foreground, background);
		}
		entry->end = entry->text->w;
		Screen->FillRect(entry->x, entry->y,
					entry->width, entry->height, clear);
		Screen->QueueBlit(entry->x, entry->y, entry->text, NOCLIP);
	}
	void DrawCursor(struct numeric_entry *entry) {
		Screen->DrawLine(entry->x+entry->end, entry->y,
			entry->x+entry->end, entry->y+entry->height-1, Fg);
	}
};
		

/* Finally, the macintosh-like dialog class */

class Maclike_Dialog {

public:
	Maclike_Dialog(int x, int y, int width, int height, FrameBuf *screen);
	~Maclike_Dialog();

	void Add_Rectangle(int x, int y, int w, int h, Uint32 color);
	void Add_Image(SDL_Surface *image, int x, int y);
	void Add_Dialog(Mac_Dialog *dialog);

	void Run(int expand_steps = 1);

private:
	FrameBuf *Screen;
	int X, Y;
	int Width, Height;

	struct rect_elem {
		Sint16 x, y;
		Uint16 w, h;
		Uint32 color;
		struct rect_elem *next;
	} rect_list;
	struct image_elem {
		SDL_Surface *image;
		int x, y;
		struct image_elem *next;
	} image_list;
	struct dialog_elem {
		Mac_Dialog *dialog;
		struct dialog_elem *next;
	} dialog_list;
};
