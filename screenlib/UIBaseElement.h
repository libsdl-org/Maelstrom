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

#ifndef _UIBaseElement_h
#define _UIBaseElement_h

#include "../utils/array.h"
#include "../utils/rapidxml.h"
#include "../utils/prefs.h"

#include "SDL.h"
#include "UIArea.h"

class FrameBuf;
class UIManager;
class UITemplates;

typedef int UIElementType;

enum DRAWLEVEL
{
	DRAWLEVEL_BACKGROUND,
	DRAWLEVEL_NORMAL,
	DRAWLEVEL_POPUP,
	NUM_DRAWLEVELS
};

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
	template <typename T>
	T *Cast() {
		if (IsA(T::GetType())) {
			return (T*)this;
		}
		return NULL;
	}


	FrameBuf *GetScreen() const {
		return m_screen;
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

	// Bind any preferences variables to the preferences manager
	virtual void LoadData(Prefs *prefs);
	virtual void SaveData(Prefs *prefs);

	virtual UIArea *GetAnchorElement(const char *name);

	void AddElement(UIBaseElement *element) {
		m_elements.add(element);
	}
	void GetElements(array<UIBaseElement*> &elements) {
		for (unsigned int i = 0; i < m_elements.length(); ++i) {
			elements.add(m_elements[i]);
		}
	}
	template <typename T>
	T *GetElement(const char *name) {
		// Do a breadth first search
		array<UIBaseElement*> lists[2];
		unsigned int i, current = 0;

		GetElements(lists[current]);
		while (lists[current].length() > 0) {
			array<UIBaseElement*> &list = lists[current];
			
			for (i = 0; i < list.length(); ++i) {
				UIBaseElement *element = list[i];
				if (strcmp(name, element->GetName()) == 0) {
					if (element->IsA(T::GetType())) {
						return (T*)element;
					}
				}
			}

			// Didn't find it, grab the children
			array<UIBaseElement*> &children = lists[!current];
			for (i = 0; i < list.length(); ++i) {
				list[i]->GetElements(children);
			}
			list.clear();

			current = !current;
		}
		return NULL;
	}
	template <typename T>
	T *FindElement(UIBaseElement *start = NULL) {
		unsigned int i, j;
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
		for (unsigned int i = 0; i < m_elements.length(); ++i) {
			UIBaseElement *element = m_elements[i];
			if (element->IsA(T::GetType())) {
				elements.add((T*)element);
			}
		}
	}
	void RemoveElement(UIBaseElement *element) {
		m_elements.remove(element);
	}

	virtual void Show() {
		m_shown = true;
		if (m_parent) {
			m_parent->OnChildShown(this);
		}
	}
	virtual void Hide() {
		m_shown = false;
		if (m_parent) {
			m_parent->OnChildHidden(this);
		}
	}
	bool IsShown() const {
		return m_shown;
	}
	virtual void OnRectChanged() {
		UIArea::OnRectChanged();

		for (unsigned int i = 0; i < m_elements.length(); ++i) {
			UIBaseElement *element = m_elements[i];
			element->AutoSize(Width(), Height(), true);
		}
		if (m_parent) {
			m_parent->OnChildRectChanged(this);
		}
	}

	void SetDisabled(bool disabled);
	void SetParentDisabled(bool disabled);
	bool IsDisabled() const {
		return m_disabled || m_parentDisabled;
	}

	void SetDrawLevel(DRAWLEVEL drawLevel);
	DRAWLEVEL GetDrawLevel() const { return m_drawLevel; }

	virtual void Draw(DRAWLEVEL drawLevel);
	bool DispatchEvent(const SDL_Event &event, DRAWLEVEL drawLevel);
	virtual bool HandleEvent(const SDL_Event &event);
	virtual void Action(UIBaseElement *sender, const char *action);

	virtual void OnChildShown(UIBaseElement *child) { }
	virtual void OnChildHidden(UIBaseElement *child) { }
	virtual void OnChildRectChanged(UIBaseElement *child) { }

protected:
	FrameBuf *m_screen;
	UIManager *m_ui;
	UIBaseElement *m_parent;
	char *m_name;
	bool m_shown;
	bool m_disabled;
	bool m_parentDisabled;
	DRAWLEVEL m_drawLevel;
	array<UIBaseElement *> m_elements;

protected:
	UIBaseElement *CreateElement(const char *type);

	bool LoadElements(rapidxml::xml_node<> *node, const UITemplates *templates);
	bool LoadDrawLevel(rapidxml::xml_node<> *node, const char *name, DRAWLEVEL &value);

	virtual void UpdateDisabledState();

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
