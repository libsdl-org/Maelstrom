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

#ifndef _UIDrawEngine_h
#define _UIDrawEngine_h

#include "../utils/rapidxml.h"

// This class is constructed with a UIElement and is responsible for showing
// it on the screen.

class FrameBuf;
class UIManager;
class UIElement;
class UITexture;
class UITemplates;

class UIDrawEngine
{
public:
	UIDrawEngine();
	virtual ~UIDrawEngine();

	// This gets called when this instance is attached to a UIElement
	virtual void Init(UIElement *element);

	// This gets called when the element loads configuration data
	virtual bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);

	// This gets called when the element has finished loading
	virtual void OnLoad() { }

	// This gets called when it's time to draw the element
	virtual void OnDraw();

	// This gets called when the element border changes
	virtual void OnBorderChanged() { }

	// This gets called when the element fill changes
	virtual void OnFillChanged() { }

	// This gets called when the element fill color changes
	virtual void OnFillColorChanged() { }

	// This gets called when the element color changes
	virtual void OnColorChanged();

	// This gets called when the element font changes
	virtual void OnFontChanged();

	// This gets called when the element text changes
	virtual void OnTextChanged();

	// This gets called when the element image changes
	virtual void OnImageChanged();

	// This gets called when the mouse enters the element
	virtual void OnMouseEnter() { }

	// This gets called when the mouse leaves the element
	virtual void OnMouseLeave() { }

	// This gets called when a mouse button is pressed in the element
	virtual void OnMouseDown() { }

	// This gets called when a mouse button is released in the element
	virtual void OnMouseUp() { }

protected:
	FrameBuf *m_screen;
	UIManager *m_ui;
	UIElement *m_element;
	UITexture *m_textImage;

protected:
	bool LoadBool(rapidxml::xml_node<> *node, const char *name, bool &value);
	bool LoadNumber(rapidxml::xml_node<> *node, const char *name, int &value);
	bool LoadString(rapidxml::xml_node<> *node, const char *name, char *&value);
};

class UIDrawEngineLine : public UIDrawEngine
{
public:
	UIDrawEngineLine() : UIDrawEngine() { }

	virtual void OnDraw();
};

class UIDrawEngineRect : public UIDrawEngine
{
public:
	UIDrawEngineRect() : UIDrawEngine() { }

	virtual void OnDraw();
};

#endif // _UIDrawEngine_h
