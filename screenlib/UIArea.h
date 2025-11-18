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

#ifndef _UIArea_h
#define _UIArea_h

#include "SDL_rect.h"
#include "../utils/ErrorBase.h"
#include "../utils/array.h"
#include "../utils/rapidxml.h"

enum {
	X_LEFT = 0x01,
	X_CENTER = 0x02,
	X_RIGHT = 0x4,
	X_MASK = (X_LEFT|X_CENTER|X_RIGHT),
	Y_TOP = 0x10,
	Y_CENTER = 0x20,
	Y_BOTTOM = 0x40,
	Y_MASK = (Y_TOP|Y_CENTER|Y_BOTTOM)
};
enum AnchorLocation {
	TOPLEFT = (Y_TOP|X_LEFT),
	TOP = (Y_TOP|X_CENTER),
	TOPRIGHT = (Y_TOP|X_RIGHT),
	LEFT = (Y_CENTER|X_LEFT),
	CENTER = (Y_CENTER|X_CENTER),
	RIGHT = (Y_CENTER|X_RIGHT),
	BOTTOMLEFT = (Y_BOTTOM|X_LEFT),
	BOTTOM = (Y_BOTTOM|X_CENTER),
	BOTTOMRIGHT = (Y_BOTTOM|X_RIGHT)
};

class UIArea;

struct UIAnchorInfo {
	UIArea *element;
	AnchorLocation anchorFrom;
	AnchorLocation anchorTo;
	int offsetX, offsetY;
};

class UIArea : public ErrorBase
{
public:
	UIArea(UIArea *anchor = NULL, int w = 0, int h = 0);
	virtual ~UIArea();

	bool Load(rapidxml::xml_node<> *node);

	// This function returns anchor areas by name
	virtual UIArea *GetAnchorElement(const char *name);

	void SetPosition(int x, int y);
	void SetSize(int w, int h, bool autosize = false, bool parent = false);
	void SetWidth(int w, bool autosize = false, bool parent = false);
	void SetHeight(int h, bool autosize = false, bool parent = false);
	void AutoSize(int w, int h, bool parent = false);
	bool IsAutoSizing() const {
		return m_autosizeWidth && m_autosizeHeight;
	}
	bool IsAutoSizingWidth() const {
		return m_autosizeWidth;
	}
	bool IsAutoSizingHeight() const {
		return m_autosizeHeight;
	}
	void SetAnchor(AnchorLocation from, AnchorLocation to, UIArea *anchor,
					int offsetX = 0, int offsetY = 0);
	void GetAnchor(UIAnchorInfo &anchor) {
		anchor = m_anchor;
	}

	bool ContainsPoint(int x, int y) const {
		return (x >= m_rect.x && x < m_rect.x+m_rect.w &&
		        y >= m_rect.y && y < m_rect.y+m_rect.h);
	}

	const SDL_Rect &GetRect() const {
		return m_rect;
	}
	int X() const {
		return m_rect.x;
	}
	int Y() const {
		return m_rect.y;
	}
	int CenterX() const {
		return m_rect.x + m_rect.w/2;
	}
	int CenterY() const {
		return m_rect.y + m_rect.h/2;
	}
	int Width() const {
		return m_rect.w;
	}
	int Height() const {
		return m_rect.h;
	}

	bool IsEmpty() const {
		return !m_rect.w || !m_rect.h;
	}

	void AddAnchoredArea(UIArea *area) {
		m_anchoredAreas.add(area);
	}
	void DelAnchoredArea(UIArea *area) {
		m_anchoredAreas.remove(area);
	}

protected:
	bool LoadBool(rapidxml::xml_node<> *node, const char *name, bool &value);
	bool LoadNumber(rapidxml::xml_node<> *node, const char *name, int &value);
	bool LoadNumber(rapidxml::xml_node<> *node, const char *name, float &value);
	bool LoadString(rapidxml::xml_node<> *node, const char *name, char *&value);
	bool LoadAnchorLocation(rapidxml::xml_node<> *node, const char *name, AnchorLocation &value);

	void SetAnchorElement(UIArea *anchor);
	void GetAnchorLocation(AnchorLocation spot, int *x, int *y) const;
	void CalculateAnchor(bool triggerRectChanged = true);
	virtual void OnRectChanged();

private:
	/* This is private so updates can trigger OnRectChanged() */
	bool m_autosizeParentWidth;
	bool m_autosizeParentHeight;
	bool m_autosizeWidth;
	bool m_autosizeHeight;
	SDL_Rect m_rect;
	UIAnchorInfo m_anchor;

	array<UIArea *> m_anchoredAreas;
};

#endif // _UIArea_h
