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

#ifndef _UIElementRadio_h
#define _UIElementRadio_h

#include "UIElement.h"
#include "UIElementCheckbox.h"

class UIRadioCallback
{
public:
    virtual ~UIRadioCallback() { }

	virtual void operator ()(int value) = 0;
};

template <class C>
class UIObjectRadioCallback : public UIRadioCallback
{
public:
	UIObjectRadioCallback(C *obj, void (C::*callback)(void*, int), void *param) : UIRadioCallback() {
		m_obj = obj;
		m_callback = callback;
		m_param = param;
	}

	virtual void operator()(int value) {
		(m_obj->*m_callback)(m_param, value);
	}

protected:
	C *m_obj;
	void (C::*m_callback)(void*, int);
	void *m_param;
};

class UIFunctionRadioCallback : public UIRadioCallback
{
public:
	UIFunctionRadioCallback(void (*callback)(void*, int), void *param) : UIRadioCallback() {
		m_callback = callback;
		m_param = param;
	}

	virtual void operator()(int value) {
		(*m_callback)(m_param, value);
	}

protected:
	void (*m_callback)(void*, int);
	void *m_param;
};

//
// This file has two classes:
//
// UIElementRadioGroup maintains the state of radio buttons
//
// UIElementRadioButton is a member of the radio group and notifies the group
// when it is clicked.

class UIElementRadioButton;

class UIElementRadioGroup : public UIElement
{
DECLARE_TYPESAFE_CLASS(UIElement)
public:
	UIElementRadioGroup(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine);
	~UIElementRadioGroup();

	override bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);

	override void LoadData(Prefs *prefs);
	override void SaveData(Prefs *prefs);

	UIElementRadioButton *GetRadioButton(int id);

	void SetValue(int value);

	int GetValue() const {
		return m_value;
	}

	// Once set, the element owns the callback and will free it.
	template <class C>
	void SetValueCallback(C *obj, void (C::*callback)(void*, int), void *param = 0) {
		SetValueCallback(new UIObjectRadioCallback<C>(obj, callback, param));
	}
	void SetValueCallback(void (*callback)(void*, int), void *param = 0) {
		SetValueCallback(new UIFunctionRadioCallback(callback, param));
	}
	void SetValueCallback(UIRadioCallback *callback);

protected:
	int m_value;
	char *m_valueBinding;
	UIRadioCallback *m_callback;
};

class UIElementRadioButton : public UIElementCheckbox
{
DECLARE_TYPESAFE_CLASS(UIElementCheckbox)
public:
	UIElementRadioButton(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine);

	int GetID() const {
		return m_id;
	}

	override bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);

	override void OnClick();
	override void OnChecked(bool checked);

protected:
	int m_id;
};

#endif // _UIElementRadio_h
