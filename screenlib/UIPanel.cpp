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
#include "UIPanel.h"
#include "UIManager.h"
#include "UIElement.h"
#include "UIElementEditbox.h"
#include "UITemplates.h"

UIElementType UIPanel::s_elementType;


UIPanel::UIPanel(UIManager *ui, const char *name) :
	UIBaseElement(ui, name)
{
	m_shown = false;
	m_fullscreen = true;
	m_alwaysOnTop = false;
	m_cursorVisible = true;
	m_enterSound = NULL;
	m_leaveSound = NULL;
	m_delegate = NULL;
	m_deleteDelegate = false;

	m_ui->AddPanel(this);
}

UIPanel::~UIPanel()
{
	m_ui->RemovePanel(this);

	SetPanelDelegate(NULL);

	if (m_enterSound) {
		SDL_free(m_enterSound);
	}
	if (m_leaveSound) {
		SDL_free(m_leaveSound);
	}
}

bool
UIPanel::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	if (!UIBaseElement::Load(node, templates)) {
		return false;
	}

	LoadBool(node, "fullscreen", m_fullscreen);
	LoadBool(node, "alwaysOnTop", m_alwaysOnTop);
	LoadBool(node, "cursor", m_cursorVisible);
	LoadString(node, "enterSound", m_enterSound);
	LoadString(node, "leaveSound", m_leaveSound);

	return true;
}

bool
UIPanel::FinishLoading()
{
	if (m_delegate) {
		if (!m_delegate->OnLoad()) {
			return false;
		}
	}
	return UIBaseElement::FinishLoading();
}

void
UIPanel::SetPanelDelegate(UIPanelDelegate *delegate, bool autodelete)
{
	if (m_delegate && m_deleteDelegate) {
		delete m_delegate;
	}
	m_delegate = delegate;
	m_deleteDelegate = autodelete;
}

void
UIPanel::Show()
{
	if (m_enterSound) {
		m_ui->PlaySound(m_enterSound);
	}

	// Load data from preferences
	LoadData(GetUI()->GetPrefs());

	UIBaseElement::Show();

	// Give the first editbox focus
	UIElementEditbox *editbox = FindElement<UIElementEditbox>();
	if (editbox) {
		editbox->SetFocus(true);
	}

	if (m_delegate) {
		m_delegate->OnShow();
	}
}

void
UIPanel::Hide()
{
	if (m_leaveSound) {
		m_ui->PlaySound(m_leaveSound);
	}

	UIBaseElement::Hide();

	// Clear focus on editboxes
	array<UIElementEditbox*> editboxes;
	FindElements<UIElementEditbox>(editboxes);
	for (unsigned int i = 0; i < editboxes.length(); ++i) {
		editboxes[i]->SetFocus(false);
	}

	// Save data to preferences
	if (ShouldSaveData()) {
		Prefs *prefs = GetUI()->GetPrefs();

		// Save any data bindings to the preferences
		SaveData(prefs);

		// Save the preferences to disk (comment to just save at exit)
		prefs->Save();
	}

	if (m_delegate) {
		m_delegate->OnHide();
	}
}

void
UIPanel::HideAll()
{
	for (unsigned int i = 0; i < m_elements.length(); ++i) {
		m_elements[i]->Hide();
	}
}

void
UIPanel::Poll()
{
	if (m_delegate) {
		m_delegate->OnPoll();
	}
}

void
UIPanel::Tick()
{
	if (m_delegate) {
		m_delegate->OnTick();
	}
}

void
UIPanel::Draw(DRAWLEVEL drawLevel)
{
	UIBaseElement::Draw(drawLevel);

	if (m_delegate) {
		m_delegate->OnDraw(drawLevel);
	}
}

bool
UIPanel::HandleEvent(const SDL_Event &event)
{
	if (UIBaseElement::HandleEvent(event)) {
		return true;
	}

	if (m_delegate) {
		return m_delegate->HandleEvent(event);
	}
	return false;
}

void
UIPanel::Action(UIBaseElement *sender, const char *action)
{
	if (m_delegate) {
		if (m_delegate->OnAction(sender, action)) {
			return;
		}
	}

	// Dialogs pass actions to their parents
	UIPanel *panel;
	if (m_ui->IsShown(this)) {
		panel = m_ui->GetPrevPanel(this);
	} else {
		panel = m_ui->GetCurrentPanel();
	}
	if (panel) {
		panel->Action(sender, action);
	} else {
		if (SDL_strncmp(action, "show_", 5) == 0) {
			GetUI()->ShowPanel(action+5);
		}
	}
}
