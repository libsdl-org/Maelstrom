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

#include "UIElementRadio.h"

UIElementType UIElementRadioGroup::s_elementType;


UIElementRadioGroup::UIElementRadioGroup(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIElement(parent, name, drawEngine)
{
	m_value = -1;
	m_valueBinding = NULL;
	m_callback = NULL;
}

UIElementRadioGroup::~UIElementRadioGroup()
{
	if (m_valueBinding) {
		SDL_free(m_valueBinding);
	}
	if (m_callback) {
		delete m_callback;
	}
}

bool
UIElementRadioGroup::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	if (!UIElement::Load(node, templates)) {
		return false;
	}

	LoadString(node, "bindValue", m_valueBinding);

	return true;
}

void
UIElementRadioGroup::LoadData(Prefs *prefs)
{
	if (m_valueBinding) {
		SetValue(prefs->GetNumber(m_valueBinding, GetValue()));
	}
	UIElement::LoadData(prefs);
}

void
UIElementRadioGroup::SaveData(Prefs *prefs)
{
	if (m_valueBinding) {
		prefs->SetNumber(m_valueBinding, GetValue());
	}
	UIElement::SaveData(prefs);
}

UIElementRadioButton *
UIElementRadioGroup::GetRadioButton(int id)
{
	UIElementRadioButton *button;

	for (unsigned int i = 0; i < m_elements.length(); ++i) {
		if (!m_elements[i]->IsA(UIElementRadioButton::GetType())) {
			continue;
		}

		button = static_cast<UIElementRadioButton*>(m_elements[i]);
		if (button->GetID() == id) {
			return button;
		}
	}
	return NULL;
}

void
UIElementRadioGroup::SetValue(int value)
{
	int oldValue;
	array<UIElementRadioButton*> buttons;

	if (value == m_value) {
		return;
	}

	oldValue = m_value;
	m_value = value;

	FindElements<UIElementRadioButton>(buttons);
	for (unsigned int i = 0; i < buttons.length(); ++i) {
		if (buttons[i]->GetID() == value) {
			buttons[i]->SetChecked(true);
		} else {
			buttons[i]->SetChecked(false);
		}
	}

	if (m_callback) {
		(*m_callback)(m_value);
	}
}

void
UIElementRadioGroup::SetValueCallback(UIRadioCallback *callback)
{
	if (m_callback) {
		delete m_callback;
	}
	m_callback = callback;
}


UIElementType UIElementRadioButton::s_elementType;


UIElementRadioButton::UIElementRadioButton(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIElementCheckbox(parent, name, drawEngine)
{
	m_id = -1;
}

bool
UIElementRadioButton::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	// Load the ID first so OnChecked() will do the right thing
	LoadNumber(node, "id", m_id);

	if (!UIElementCheckbox::Load(node, templates)) {
		return false;
	}

	return true;
}

void
UIElementRadioButton::OnClick()
{
	SetChecked(true);

	UIElementButton::OnClick();
}

void
UIElementRadioButton::OnChecked(bool checked)
{
	if (checked) {
		if (m_parent->IsA(UIElementRadioGroup::GetType())) {
			static_cast<UIElementRadioGroup*>(m_parent)->SetValue(GetID());
		}
	}
}
