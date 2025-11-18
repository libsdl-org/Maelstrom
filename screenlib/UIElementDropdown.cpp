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

#include "UIManager.h"
#include "UIElementDropdown.h"

UIElementType UIElementDropdown::s_elementType;


UIElementDropdown::UIElementDropdown(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIElementButton(parent, name, drawEngine)
{
	m_showingElements = true;
}

UIElementDropdown::~UIElementDropdown()
{
	GetUI()->ReleaseEvents(this);
}

bool
UIElementDropdown::FinishLoading()
{
	if (!UIElementButton::FinishLoading()) {
		return false;
	}

	HideElements();

	return true;
}

bool
UIElementDropdown::HandleEvent(const SDL_Event &event)
{
	if (UIElementButton::HandleEvent(event)) {
		return true;
	}

	// Hide the dropdown when we get a mouse down event anywhere else
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		HideElements();
	}
	return false;
}

void
UIElementDropdown::Action(UIBaseElement *sender, const char *action)
{
	// Hide the dropdown when we execute an action
	HideElements();

	UIElementButton::Action(sender, action);
}

void
UIElementDropdown::UpdateDisabledState()
{
	// Hide the dropdown when we are disabled
	HideElements();

	UIElementButton::UpdateDisabledState();
}

void
UIElementDropdown::OnClick()
{
	ToggleElements();

	UIElementButton::OnClick();
}

void
UIElementDropdown::ShowElements()
{
	GetUI()->CaptureEvents(this);

	for (unsigned int i = 0; i < m_elements.length(); ++i) {
		m_elements[i]->Show();
	}
	m_showingElements = true;
}

void
UIElementDropdown::HideElements()
{
	GetUI()->ReleaseEvents(this);

	for (unsigned int i = 0; i < m_elements.length(); ++i) {
		m_elements[i]->Hide();
	}
	m_showingElements = false;
}

void
UIElementDropdown::ToggleElements()
{
	if (m_showingElements) {
		HideElements();
	} else {
		ShowElements();
	}
}
