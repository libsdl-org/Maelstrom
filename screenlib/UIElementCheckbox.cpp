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

#include "UIElementCheckbox.h"

UIElementType UIElementCheckbox::s_elementType;


UIElementCheckbox::UIElementCheckbox(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIElementButton(parent, name, drawEngine)
{
	m_checked = false;
	m_valueBinding = NULL;
	for (int i = 0; i < SDL_arraysize(m_images); ++i) {
		m_images[i] = NULL;
	}
}

UIElementCheckbox::~UIElementCheckbox()
{
	if (m_valueBinding) {
		SDL_free(m_valueBinding);
	}
	for (int i = 0; i < SDL_arraysize(m_images); ++i) {
		if (m_images[i]) {
			SDL_free(m_images[i]);
		}
	}
}

bool
UIElementCheckbox::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	if (!UIElementButton::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("checkedImage", 0, false);
	if (attr) {
		m_images[1] = SDL_strdup(attr->value());
		attr = node->first_attribute("image", 0, false);
		if (attr) {
			m_images[0] = SDL_strdup(attr->value());
		}
	}

	/* Call SetChecked() to trigger derived classes' behaviors */
	bool checked;
	if (LoadBool(node, "checked", checked)) {
		SetChecked(checked);
	}

	LoadString(node, "bindValue", m_valueBinding);

	return true;
}

bool
UIElementCheckbox::FinishLoading()
{
	// Extend the sensitive area to encompass the label
	if (!m_textArea.IsEmpty()) {
		if (m_textArea.X() >= X()) {
			int width = ((m_textArea.X()+m_textArea.Width()) - X());
			if (width > Width()) {
				SetWidth(width);
			}
		} else {
			assert(!"Need code for labels on the left");
		}
	}
	if (!m_imageArea.IsEmpty()) {
		if (m_imageArea.X() >= X()) {
			int width = ((m_imageArea.X()+m_imageArea.Width()) - X());
			if (width > Width()) {
				SetWidth(width);
			}
		} else {
			assert(!"Need code for images on the left");
		}
		if (m_imageArea.Height() > Height()) {
			SetHeight(m_imageArea.Height());
		}
	}
	return UIElementButton::FinishLoading();
}

void
UIElementCheckbox::LoadData(Prefs *prefs)
{
	if (m_valueBinding) {
		SetChecked(prefs->GetBool(m_valueBinding, IsChecked()));
	}
	UIElementButton::LoadData(prefs);
}

void
UIElementCheckbox::SaveData(Prefs *prefs)
{
	if (m_valueBinding) {
		prefs->SetBool(m_valueBinding, IsChecked());
	}
	UIElementButton::SaveData(prefs);
}

void
UIElementCheckbox::OnClick()
{
	SetChecked(!m_checked);

	UIElementButton::OnClick();
}
