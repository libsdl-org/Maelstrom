/*
  screenlib:  A simple window and UI library based on the SDL library
  Copyright (C) 1997-2011 Sam Lantinga <slouken@libsdl.org>

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

#ifndef _UIContainer_h
#define _UIContainer_h

#include "UIElement.h"


enum LAYOUT_TYPE {
	LAYOUT_HORIZONTAL,
	LAYOUT_VERTICAL,
};

class UIContainer : public UIElement
{
DECLARE_TYPESAFE_CLASS(UIElement)
public:
	UIContainer(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine);

	override bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);
	override bool FinishLoading() {
		LayoutChildren();
		return true;
	}

	override void OnChildShown(UIBaseElement *child) {
		LayoutChildren();
	}
	override void OnChildHidden(UIBaseElement *child) {
		LayoutChildren();
	}
	override void OnChildRectChanged(UIBaseElement *child) {
		LayoutChildren();
	}
    
    int Spacing() { return m_spacing; }
    int BorderSpacing() { return m_borderSpacing; }
    

protected:
	LAYOUT_TYPE m_layoutType;
	int m_spacing;
	int m_borderSpacing;
	bool m_layoutInProgress;

protected:
	bool ParseLayoutType(const char *text);

	virtual void LayoutChildren();
};

#endif // _UIContainer_h
