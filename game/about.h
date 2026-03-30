/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
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
