/*
    SCREENLIB:  A framebuffer library based on the SDL library
    Copyright (C) 1997  Sam Lantinga

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

#ifndef _UIPanel_h
#define _UIPanel_h

#include "SDL.h"

#include "../utils/rapidxml.h"
#include "../utils/array.h"

#include "UIArea.h"
#include "UIElement.h"

class FrameBuf;
class UIManager;
class UIPanel;

class UIPanelDelegate
{
public:
	UIPanelDelegate(UIPanel *panel) { m_panel = panel; }

	virtual bool OnLoad() { return true; }
	virtual void OnShow() { }
	virtual void OnHide() { }
	virtual void OnTick() { }
	virtual void OnDraw() { }

protected:
	UIPanel *m_panel;
};

class UIPanel : public UIArea
{
public:
	UIPanel(UIManager *ui, const char *name);
	virtual ~UIPanel();

	UIManager *GetUI() const {
		return m_ui;
	}
	const char *GetName() const {
		return m_name;
	}
	bool IsFullscreen() const {
		return m_fullscreen;
	}
	bool IsCursorVisible() const {
		return m_cursorVisible;
	}

	bool Load(rapidxml::xml_node<> *node);

	virtual UIArea *GetAnchorElement(const char *name);

	void AddElement(UIElement *element) {
		m_elements.add(element);
	}
	template <typename T>
	T *GetElement(const char *name) {
		UIElement *element = GetElement(name);
		if (element && element->IsA(T::GetType())) {
			return (T*)element;
		}
		return NULL;
	}
	void RemoveElement(UIElement *element) {
		m_elements.remove(element);
	}

	void SetPanelDelegate(UIPanelDelegate *delegate, bool autodelete = true);

	virtual void Show();
	virtual void Hide();

	void Draw();
	bool HandleEvent(const SDL_Event &event);

protected:
	UIManager *m_ui;
	char *m_name;
	bool m_fullscreen;
	bool m_cursorVisible;
	int m_enterSound;
	int m_leaveSound;
	UIPanelDelegate *m_delegate;
	bool m_deleteDelegate;
	array<UIElement *> m_elements;

protected:
	UIElement *GetElement(const char *name);

	bool LoadElements(rapidxml::xml_node<> *node);
};

#endif // _UIPanel_h
