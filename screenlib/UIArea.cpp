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

#include "SDL_FrameBuf.h"
#include "UIArea.h"


UIArea::UIArea(UIArea *anchor, int w, int h) : ErrorBase()
{
	m_autosizeParentWidth = true;
	m_autosizeParentHeight = true;
	m_autosizeWidth = true;
	m_autosizeHeight = true;
	m_rect.x = 0;
	m_rect.y = 0;
	m_rect.w = w;
	m_rect.h = h;
	m_anchor.element = NULL;
	SetAnchorElement(anchor);
	m_anchor.anchorFrom = CENTER;
	m_anchor.anchorTo = CENTER;
	m_anchor.offsetX = 0;
	m_anchor.offsetY = 0;
}

UIArea::~UIArea()
{
	SetAnchorElement(NULL);
	for (int i = m_anchoredAreas.length(); i--; ) {
		m_anchoredAreas[i]->SetAnchor(CENTER, CENTER, NULL);
	}
}

bool
UIArea::Load(rapidxml::xml_node<> *node)
{
	rapidxml::xml_node<> *child;
	rapidxml::xml_attribute<> *attr;
	SDL_Rect rect = m_rect;

	child = node->first_node("size", 0, false);
	if (child) {
		int w, h;
		if (LoadNumber(child, "w", w)) {
			SetWidth(w);
		}
		if (LoadNumber(child, "h", h)) {
			SetHeight(h);
		}
	}

	child = node->first_node("anchor", 0, false);
	if (child) {
		attr = child->first_attribute("anchor", 0, false);
		SetAnchorElement(GetAnchorElement(attr ? attr->value() : NULL));
		if (!m_anchor.element) {
			SetError("Element 'anchor' couldn't find anchor element %s",
				attr ? attr->value() : "NULL");
			return false;
		}

		LoadAnchorLocation(child, "anchorFrom", m_anchor.anchorFrom);
		LoadAnchorLocation(child, "anchorTo", m_anchor.anchorTo);

		LoadNumber(child, "x", m_anchor.offsetX);
		LoadNumber(child, "y", m_anchor.offsetY);
	}

	CalculateAnchor(false);
	if (m_rect.x != rect.x || m_rect.y != rect.y ||
	    m_rect.w != rect.w || m_rect.h != rect.h) {
		OnRectChanged();
	}

	return true;
}

UIArea *
UIArea::GetAnchorElement(const char *name)
{
	if (m_anchor.element) {
		if (name) {
			return m_anchor.element->GetAnchorElement(name);
		} else {
			return m_anchor.element;
		}
	}
	return NULL;
}

void
UIArea::SetPosition(int x, int y) {
	/* Setting the position breaks the anchoring */
	SetAnchorElement(NULL);

	if (x != m_rect.x || y != m_rect.y) {
		m_rect.x = x;
		m_rect.y = y;
		OnRectChanged();
	}
}

void
UIArea::SetSize(int w, int h, bool autosize, bool parent)
{
	if (w != m_rect.w || h != m_rect.h) {
		m_rect.w = w;
		m_rect.h = h;
		CalculateAnchor(false);
		OnRectChanged();
	}
	if (!autosize) {
		m_autosizeWidth = false;
		m_autosizeHeight = false;
	}
	if (!parent) {
		m_autosizeParentWidth = false;
		m_autosizeParentHeight = false;
	}
}

void
UIArea::SetWidth(int w, bool autosize, bool parent)
{
	if (w != m_rect.w) {
		m_rect.w = w;
		CalculateAnchor(false);
		OnRectChanged();
	}
	if (!autosize) {
		m_autosizeWidth = false;
	}
	if (!parent) {
		m_autosizeParentWidth = false;
	}
}

void
UIArea::SetHeight(int h, bool autosize, bool parent)
{
	if (h != m_rect.h) {
		m_rect.h = h;
		CalculateAnchor(false);
		OnRectChanged();
	}
	if (!autosize) {
		m_autosizeHeight = false;
	}
	if (!parent) {
		m_autosizeParentHeight = false;
	}
}

void
UIArea::AutoSize(int w, int h, bool parent)
{
	if (m_autosizeWidth && m_autosizeHeight) {
		if (parent && !m_autosizeParentWidth && !m_autosizeParentHeight) {
			return;
		}
		if (parent && !m_autosizeParentWidth) {
			SetHeight(h, true, parent);
		} else if (parent && !m_autosizeParentHeight) {
			SetWidth(w, true, parent);
		} else {
			SetSize(w, h, true, parent);
		}
	} else if (m_autosizeWidth) {
		if (parent && !m_autosizeParentWidth) {
			return;
		}
		SetWidth(w, true, parent);
	} else if (m_autosizeHeight)  {
		if (parent && !m_autosizeParentHeight) {
			return;
		}
		SetHeight(h, true, parent);
	}
}

void
UIArea::SetAnchor(AnchorLocation from, AnchorLocation to, UIArea *anchor,
						int offsetX, int offsetY)
{
	SetAnchorElement(anchor);
	m_anchor.anchorFrom = from;
	m_anchor.anchorTo = to;
	m_anchor.offsetX = offsetX;
	m_anchor.offsetY = offsetY;
	CalculateAnchor();
}

bool
UIArea::LoadBool(rapidxml::xml_node<> *node, const char *name, bool &value)
{
	rapidxml::xml_attribute<> *attr;

	attr = node->first_attribute(name, 0, false);
	if (attr) {
		const char *text = attr->value();
		if (*text == '\0' || *text == '0' ||
		    *text == 'f' || *text == 'F') {
			value = false;
		} else {
			value = true;
		}
		return true;
	}
	return false;
}

bool
UIArea::LoadNumber(rapidxml::xml_node<> *node, const char *name, int &value)
{
	rapidxml::xml_attribute<> *attr;

	attr = node->first_attribute(name, 0, false);
	if (attr) {
		value = (int)SDL_strtol(attr->value(), NULL, 0);
		return true;
	}
	return false;
}

bool
UIArea::LoadNumber(rapidxml::xml_node<> *node, const char *name, float &value)
{
	rapidxml::xml_attribute<> *attr;

	attr = node->first_attribute(name, 0, false);
	if (attr) {
		value = (float)SDL_atof(attr->value());
		return true;
	}
	return false;
}

bool
UIArea::LoadString(rapidxml::xml_node<> *node, const char *name, char *&value)
{
	rapidxml::xml_attribute<> *attr;

	attr = node->first_attribute(name, 0, false);
	if (attr) {
		if (value) {
			SDL_free(value);
		}
		if (*attr->value()) {
			value = SDL_strdup(attr->value());
		} else {
			value = NULL;
		}
		return true;
	}
	return false;
}

bool
UIArea::LoadAnchorLocation(rapidxml::xml_node<> *node, const char *name, AnchorLocation &value)
{
	rapidxml::xml_attribute<> *attr;

	attr = node->first_attribute(name, 0, false);
	if (attr) {
		const char *text = attr->value();

		if (SDL_strcasecmp(text, "TOPLEFT") == 0) {
			value = TOPLEFT;
		} else if (SDL_strcasecmp(text, "TOP") == 0) {
			value = TOP;
		} else if (SDL_strcasecmp(text, "TOPRIGHT") == 0) {
			value = TOPRIGHT;
		} else if (SDL_strcasecmp(text, "LEFT") == 0) {
			value = LEFT;
		} else if (SDL_strcasecmp(text, "CENTER") == 0) {
			value = CENTER;
		} else if (SDL_strcasecmp(text, "RIGHT") == 0) {
			value = RIGHT;
		} else if (SDL_strcasecmp(text, "BOTTOMLEFT") == 0) {
			value = BOTTOMLEFT;
		} else if (SDL_strcasecmp(text, "BOTTOM") == 0) {
			value = BOTTOM;
		} else if (SDL_strcasecmp(text, "BOTTOMRIGHT") == 0) {
			value = BOTTOMRIGHT;
		} else {
			/* Failed to parse */
			return false;
		}
		return true;
	}
	return false;
}

void
UIArea::SetAnchorElement(UIArea *anchor)
{
	if (anchor == m_anchor.element) {
		return;
	}

	if (m_anchor.element) {
		m_anchor.element->DelAnchoredArea(this);
	}

	m_anchor.element = anchor;

	if (m_anchor.element) {
		m_anchor.element->AddAnchoredArea(this);
	}
}

void
UIArea::GetAnchorLocation(AnchorLocation spot, int *x, int *y) const
{
	switch (spot & X_MASK) {
		case X_LEFT:
			*x = m_rect.x;
			break;
		case X_CENTER:
			*x = m_rect.x + m_rect.w/2;
			break;
		case X_RIGHT:
			*x = m_rect.x + m_rect.w;
			break;
		default:
			assert(0);
	}
	switch (spot & Y_MASK) {
		case Y_TOP:
			*y = m_rect.y;
			break;
		case Y_CENTER:
			*y = m_rect.y + m_rect.h/2;
			break;
		case Y_BOTTOM:
			*y = m_rect.y + m_rect.h;
			break;
		default:
			assert(0);
	}
}

void
UIArea::CalculateAnchor(bool triggerRectChanged)
{
	int x, y;

	if (!m_anchor.element) {
		return;
	}
	m_anchor.element->GetAnchorLocation(m_anchor.anchorTo, &x, &y);

	switch (m_anchor.anchorFrom & X_MASK) {
		case X_CENTER:
			x -= Width() / 2;
			break;
		case X_RIGHT:
			x -= Width();
			break;
		default:
			break;
	}
	switch (m_anchor.anchorFrom & Y_MASK) {
		case Y_CENTER:
			y -= Height() / 2;
			break;
		case Y_BOTTOM:
			y -= Height();
			break;
		default:
			break;
	}

	x += m_anchor.offsetX;
	y += m_anchor.offsetY;
	if (x != m_rect.x || y != m_rect.y) {
		m_rect.x = x;
		m_rect.y = y;

		if (triggerRectChanged) {
			OnRectChanged();
		}
	}
}

void
UIArea::OnRectChanged()
{
	for (unsigned int i = 0; i < m_anchoredAreas.length(); ++i) {
		m_anchoredAreas[i]->CalculateAnchor(true);
	}
}
