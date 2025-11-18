/*
    Maelstrom: Open Source version of the classic game by Ambrosia Software
    Copyright (C) 1997-2011  Sam Lantinga

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifndef _about_h
#define _about_h

#include "Maelstrom_Globals.h"

class Object;

class AboutPanelDelegate : public UIPanelDelegate
{
public:
	AboutPanelDelegate(UIPanel *panel) : UIPanelDelegate(panel) {
		numsprites = 0;
	}
	virtual ~AboutPanelDelegate() {
		assert(numsprites == 0);
	}

	virtual void OnShow();
	virtual void OnHide();
	virtual void OnDraw(DRAWLEVEL drawLevel);

protected:
	int numsprites;
	Object *objects[MAX_SPRITES];
};

#endif // _about_h
