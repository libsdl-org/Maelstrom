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

#include "SDL.h"
#include "UIBaseElement.h"
#include "UIManager.h"
#include "UIElement.h"
#include "UITemplates.h"


UIElementType UIBaseElement::s_elementTypeIndex;
UIElementType UIBaseElement::s_elementType;


UIBaseElement::UIBaseElement(UIManager *ui, const char *name) :
	UIArea(ui, ui->Width(), ui->Height())
{
	m_screen = ui->GetScreen();
	m_ui = ui;
	m_parent = NULL;
	m_name = SDL_strdup(name);
	m_shown = true;
	m_disabled = false;
	m_parentDisabled = false;
	m_drawLevel = DRAWLEVEL_NORMAL;
}

UIBaseElement::UIBaseElement(UIBaseElement *parent, const char *name) :
	UIArea(parent, parent->Width(), parent->Height())
{
	m_screen = parent->GetScreen();
	m_ui = parent->GetUI();
	m_parent = parent;
	m_name = SDL_strdup(name);
	m_shown = true;
	m_disabled = false;
	m_parentDisabled = parent->IsDisabled();
	m_drawLevel = DRAWLEVEL_NORMAL;
}

UIBaseElement::~UIBaseElement()
{
	SDL_free(m_name);

	for (unsigned int i = 0; i < m_elements.length(); ++i) {
		delete m_elements[i];
	}
}

bool
UIBaseElement::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_node<> *child;

	child = templates->GetTemplateFor(node);
	if (child) {
		if (!Load(child, templates)) {
			return false;
		}
	}

	if (!UIArea::Load(node)) {
		return false;
	}

	LoadBool(node, "show", m_shown);

	bool disabled;
	if (LoadBool(node, "disabled", disabled)) {
		SetDisabled(disabled);
	}

	child = node->first_node("elements", 0, false);
	if (child) {
		if (!LoadElements(child, templates)) {
			return false;
		}
	}

	DRAWLEVEL drawLevel;
	if (LoadDrawLevel(node, "drawLevel", drawLevel)) {
		SetDrawLevel(drawLevel);
	}

	return true;
}

void
UIBaseElement::LoadData(Prefs *prefs)
{
	for (unsigned int i = 0; i < m_elements.length(); ++i) {
		m_elements[i]->LoadData(prefs);
	}
}

void
UIBaseElement::SaveData(Prefs *prefs)
{
	for (unsigned int i = 0; i < m_elements.length(); ++i) {
		m_elements[i]->SaveData(prefs);
	}
}

UIArea *
UIBaseElement::GetAnchorElement(const char *name)
{
	if (name) {
		if (m_parent) {
			UIArea *element = m_parent->GetElement<UIBaseElement>(name);
			if (!element) {
				element = m_parent->GetAnchorElement(name);
			}
			return element;
		} else {
			return NULL;
		}
	} else {
		if (m_parent) {
			return m_parent;
		} else {
			return m_ui;
		}
	}
}

void
UIBaseElement::SetDisabled(bool disabled)
{
	if (disabled == m_disabled) {
		return;
	}

	bool wasDisabled = IsDisabled();
	m_disabled = disabled;
	if (wasDisabled != IsDisabled()) {
		UpdateDisabledState();
	}
}

void
UIBaseElement::SetParentDisabled(bool disabled)
{
	if (disabled == m_parentDisabled) {
		return;
	}

	bool wasDisabled = IsDisabled();
	m_parentDisabled = disabled;
	if (wasDisabled != IsDisabled()) {
		UpdateDisabledState();
	}
}

void
UIBaseElement::UpdateDisabledState()
{
	for (unsigned int i = 0; i < m_elements.length(); ++i) {
		m_elements[i]->SetParentDisabled(IsDisabled());
	}
}

void
UIBaseElement::SetDrawLevel(DRAWLEVEL drawLevel)
{
	m_drawLevel = drawLevel;

	for (unsigned int i = 0; i < m_elements.length(); ++i) {
		m_elements[i]->SetDrawLevel(drawLevel);
	}
}

void
UIBaseElement::Draw(DRAWLEVEL drawLevel)
{
	for (unsigned int i = 0; i < m_elements.length(); ++i) {
		if (m_elements[i]->IsShown()) {
			m_elements[i]->Draw(drawLevel);
		}
	}
}

bool
UIBaseElement::DispatchEvent(const SDL_Event &event, DRAWLEVEL drawLevel)
{
	for (int i = m_elements.length(); i--; ) {
		UIBaseElement *element = m_elements[i];

		if (!element->IsShown()) {
			continue;
		}
		if (element->IsDisabled()) {
			continue;
		}
		if (element->DispatchEvent(event, drawLevel)) {
			return true;
		}
	}
	if (drawLevel == GetDrawLevel()) {
		return HandleEvent(event);
	}
	return false;
}

bool
UIBaseElement::HandleEvent(const SDL_Event &event)
{
	return false;
}

void
UIBaseElement::Action(UIBaseElement *sender, const char *action)
{
	if (m_parent) {
		m_parent->Action(sender, action);
	}
}

UIBaseElement *
UIBaseElement::CreateElement(const char *name)
{
	return GetUI()->CreateElement(this, name);
}


bool
UIBaseElement::LoadElements(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	for (node = node->first_node(); node; node = node->next_sibling()) {
		/* Check conditions before creating elements */
		attr = node->first_attribute("condition", 0, false);
		if (attr && !GetUI()->CheckCondition(attr->value())) {
			continue;
		}

		UIBaseElement *element = CreateElement(node->name());
		if (!element) {
			SDL_Log("Warning: Couldn't find handler for element %s", node->name());
			continue;
		}

		if (!element->Load(node, templates) ||
		    !element->FinishLoading()) {
			SDL_Log("Warning: Couldn't load element %s: %s", node->name(), element->Error());
			delete element;
		} else {
			AddElement(element);
		}
	}
	return true;
}

bool
UIBaseElement::LoadDrawLevel(rapidxml::xml_node<> *node, const char *name, DRAWLEVEL &value)
{
	rapidxml::xml_attribute<> *attr;

	attr = node->first_attribute(name, 0, false);
	if (attr) {
		static const char *s_drawLevels[] = {
			"background",
			"normal",
			"popup"
		};
		SDL_COMPILE_TIME_ASSERT(drawLevels, SDL_arraysize(s_drawLevels) == NUM_DRAWLEVELS);

		for (int i = 0; i < NUM_DRAWLEVELS; ++i) {
			if (SDL_strcasecmp(attr->value(), s_drawLevels[i]) == 0) {
				value = (DRAWLEVEL)i;
				return true;
			}
		}
		SDL_Log("Warning: Unknown draw level: %s", attr->value());
		return false;
	}
	return false;
}
