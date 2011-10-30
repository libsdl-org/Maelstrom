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

#ifndef _UIBaseElement_h
#define _UIBaseElement_h

#include "../utils/array.h"
#include "../utils/rapidxml.h"

#include "SDL.h"
#include "UIArea.h"

class UIManager;
class UITemplates;

typedef int UIElementType;

class UIBaseElement : public UIArea
{
public:
	UIBaseElement(UIManager *ui, const char *name = "");
	UIBaseElement(UIBaseElement *parent, const char *name = "");
	virtual ~UIBaseElement();

	/* This is used for type-safe casting */
	virtual bool IsA(UIElementType type) {
		return type == GetType();
	}

	UIManager *GetUI() {
		return m_ui;
	}
	UIBaseElement *GetParent() {
		return m_parent;
	}
	const char *GetName() const {
		return m_name;
	}

	virtual bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);
	virtual bool FinishLoading() {
		return true;
	}

	virtual UIArea *GetAnchorElement(const char *name);

	void AddElement(UIBaseElement *element) {
		m_elements.add(element);
	}
	template <typename T>
	T *GetElement(const char *name) {
		UIBaseElement *element = GetElement(name);
		if (element && element->IsA(T::GetType())) {
			return (T*)element;
		}
		return NULL;
	}
	template <typename T>
	T *FindElement(UIBaseElement *start = NULL) {
		unsigned i, j;
		if (start) {
			// Find the starting element
			for (i = 0; i < m_elements.length(); ++i) {
				if (m_elements[i] == start) {
					break;
				}
			}
			if (i == m_elements.length()) {
				return NULL;
			}
			// Find the next element of that type
			j = (i+1)%m_elements.length();
			for ( ; j != i; j = (j+1)%m_elements.length()) {
				UIBaseElement *element = m_elements[j];
				if (element->IsA(T::GetType())) {
					return (T*)element;
				}
			}
		} else {
			for (i = 0; i < m_elements.length(); ++i) {
				UIBaseElement *element = m_elements[i];
				if (element->IsA(T::GetType())) {
					return (T*)element;
				}
			}
		}
		return NULL;
	}
	template <typename T>
	void FindElements(array<T*> &elements) {
		for (unsigned i = 0; i < m_elements.length(); ++i) {
			UIBaseElement *element = m_elements[i];
			if (element->IsA(T::GetType())) {
				elements.add((T*)element);
			}
		}
	}
	void RemoveElement(UIBaseElement *element) {
		m_elements.remove(element);
	}

	virtual void Draw();
	virtual bool HandleEvent(const SDL_Event &event);

protected:
	UIManager *m_ui;
	UIBaseElement *m_parent;
	char *m_name;
	array<UIBaseElement *> m_elements;

protected:
	UIBaseElement *GetElement(const char *name);
	UIBaseElement *CreateElement(const char *type);

	bool LoadElements(rapidxml::xml_node<> *node, const UITemplates *templates);

protected:
	static UIElementType s_elementTypeIndex;
	static UIElementType s_elementType;

	static UIElementType GenerateType() {
		return ++s_elementTypeIndex;
	}
public:
	static UIElementType GetType() {
		if (!s_elementType) {
			s_elementType = GenerateType();
		}
		return s_elementType;
	}
};

/////////////////////////////////////////////////////////////////////////
#define DECLARE_TYPESAFE_CLASS(BASECLASS)				\
protected:								\
	static UIElementType s_elementType;				\
									\
public:									\
	static UIElementType GetType() {				\
		if (!s_elementType) {					\
			s_elementType = GenerateType();			\
		}							\
		return s_elementType;					\
	}								\
	virtual bool IsA(UIElementType type) {				\
		return BASECLASS::IsA(type) || type == GetType();	\
	}
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// So we can be explicit about what methods are being newly exposed and
// which methods are overriding a base class method, I'm going to introduce
// the "override" keyword in this inheritance hierarchy.
#define override virtual
/////////////////////////////////////////////////////////////////////////

#endif // _UIBaseElement_h
