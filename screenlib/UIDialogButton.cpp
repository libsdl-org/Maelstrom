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
#include "UIManager.h"
#include "UIDialog.h"
#include "UIDialogButton.h"

UIElementType UIDialogButton::s_elementType;


UIDialogButton::UIDialogButton(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIElementButton(parent, name, drawEngine)
{
	m_statusID = 0;
	m_default = false;
	m_closeDialog = true;
}

bool
UIDialogButton::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	if (!UIElementButton::Load(node, templates)) {
		return false;
	}

	LoadNumber(node, "id", m_statusID);

	LoadBool(node, "default", m_default);
	if (m_default) {
		m_hotkey = SDLK_RETURN;
	}

	LoadBool(node, "closeDialog", m_closeDialog);

	return true;
}

void
UIDialogButton::OnClick()
{
	UIBaseElement *parent;
	UIPanel *panel = NULL;

	parent = this;
	while (!panel && (parent = parent->GetParent()) != NULL) {
		panel = parent->Cast<UIPanel>();
	}

	if (m_statusID && panel && panel->IsA(UIDialog::GetType())) {
		static_cast<UIDialog*>(panel)->SetDialogStatus(m_statusID);
	}

	// Hide before doing the action (which may change the current panel)
	if (m_closeDialog && panel) {
		GetUI()->HidePanel(panel);
	}

	UIElementButton::OnClick();
}
