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

#include <stdio.h>

#include "UIContainer.h"

UIElementType UIContainer::s_elementType;


UIContainer::UIContainer(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIElement(parent, name, drawEngine)
{
	m_layoutType = LAYOUT_VERTICAL;
	m_spacing = 0;
	m_borderSpacing = 0;
	m_layoutInProgress = false;
}


bool
UIContainer::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	if (!UIElement::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("layout", 0, false);
	if (attr) {
		if (!ParseLayoutType(attr->value())) {
			SDL_Log("Warning: Unknown layout type '%s'", attr->value());
			return false;
		}
	}

	LoadNumber(node, "spacing", m_spacing);
	LoadNumber(node, "borderSpacing", m_borderSpacing);

	return true;
}

void
UIContainer::LayoutChildren()
{
	UIBaseElement *anchor;
	AnchorLocation anchorLocation;
	int offsetX = 0;
	int offsetY = 0;
	unsigned int i;

	if (m_layoutInProgress) {
		return;
	}
	m_layoutInProgress = true;

	// Anchor the first visible element
	anchor = NULL;
	for (i = 0; i < m_elements.length(); ++i) {
		if (m_elements[i]->IsShown()) {
			int anchorOffset = m_borderSpacing;
			if (HasBorder()) {
				anchorOffset += 1;
			}
			m_elements[i]->SetAnchor(TOPLEFT, TOPLEFT, this,
						anchorOffset, anchorOffset);
			anchor = m_elements[i];
			break;
		}
	}

	// Anchor the rest of the elements
	if (m_layoutType == LAYOUT_HORIZONTAL) {
		anchorLocation = TOPRIGHT;
		offsetX = m_spacing;
	} else {
		anchorLocation = BOTTOMLEFT;
		offsetY = m_spacing;
	}
	for (++i; i < m_elements.length(); ++i) {
		if (m_elements[i]->IsShown()) {
			m_elements[i]->SetAnchor(TOPLEFT, anchorLocation, anchor, offsetX, offsetY);
			anchor = m_elements[i];
		}
	}

	// Adjust our width and height to match
	int w = 0, h = 0;
	if (m_layoutType == LAYOUT_HORIZONTAL) {
		// Width is the sum of children, height is the max of children
		for (i = 0; i < m_elements.length(); ++i) {
			if (m_elements[i]->IsShown()) {
				if (m_elements[i]->Height() > h) {
					h = m_elements[i]->Height();
				}
			}
		}
		if (HasBorder()) {
			h += 2;
		}
		if (anchor) {
			w = (anchor->X() - X()) + anchor->Width();
			if (HasBorder()) {
				w += 1;
			}
		}
		w += 1*m_borderSpacing;
		h += 2*m_borderSpacing;
	} else {
		// Width is the max of children, height is the sum of children
		for (i = 0; i < m_elements.length(); ++i) {
			if (m_elements[i]->IsShown()) {
				if (m_elements[i]->Width() > w) {
					w = m_elements[i]->Width();
				}
			}
		}
		if (HasBorder()) {
			w += 2;
		}
		if (anchor) {
			h = (anchor->Y() - Y()) + anchor->Height();
			if (HasBorder()) {
				h += 1;
			}
		}
		w += 2*m_borderSpacing;
		h += 1*m_borderSpacing;
	}
	AutoSize(w, h);

	m_layoutInProgress = false;
}

bool
UIContainer::ParseLayoutType(const char *text)
{
	if (SDL_strcasecmp(text, "HORIZONTAL") == 0) {
		m_layoutType = LAYOUT_HORIZONTAL;
		return true;
	}
	if (SDL_strcasecmp(text, "VERTICAL") == 0) {
		m_layoutType = LAYOUT_VERTICAL;
		return true;
	}
	return false;
}
