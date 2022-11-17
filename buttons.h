
/*  A simple menu button class -- only handles mouse input */

class Buttons {

public:
	Buttons() {
		button_list.next = NULL;
	}
	~Buttons() {
		Delete_Buttons();
	}

	void Add_Button(int x, int y, int width, int height, 
						void (*callback)(void)) {
		struct button *belem;
		
		for ( belem=&button_list; belem->next; belem=belem->next );
		belem->next = new struct Buttons::button;
		belem = belem->next;
		belem->sensitive.left = x;
		belem->sensitive.top = y;
		belem->sensitive.right = x+width;
		belem->sensitive.bottom = y+height;
		belem->callback = callback;
		belem->next = NULL;
	}

	void Activate_Button(int x, int y, int mouse_button) {
		struct button *belem;

		for ( belem=button_list.next; belem; belem=belem->next ) {
			if ( (x > belem->sensitive.left) &&
					(x < belem->sensitive.right) &&
			     (y > belem->sensitive.top) &&
					(y < belem->sensitive.bottom) ) {
				if ( belem->callback )
					(*belem->callback)();
			}
		}
		Unused(mouse_button);
	}

	void Delete_Buttons(void) {
		struct button *belem, *btemp;

		for ( belem=button_list.next; belem; ) {
			btemp = belem;
			belem = belem->next;
			delete btemp;
		};
		button_list.next = NULL;
	}
	
private:
	struct button {
		Rect sensitive;
		void (*callback)(void);
		struct button *next;
		} button_list;
};
