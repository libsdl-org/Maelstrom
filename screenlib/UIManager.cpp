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

#include "UIManager.h"
#include "UIPanel.h"


UIManager::UIManager(FrameBuf *screen, UIElementFactory factory) : ErrorBase()
{
	m_screen = screen;
	m_elementFactory = factory;
}

UIManager::~UIManager()
{
	/* Deleting the panels will remove them from the manager */
	while (m_panels.length() > 0) {
		delete m_panels[m_panels.length()-1];
	}
}

UIPanel *
UIManager::LoadPanel(const char *name)
{
	UIPanel *panel;
	char file[1024];

	sprintf(file, "%s.xml", name);
	panel = new UIPanel(this, name);
	if (!panel->Load(file)) {
		SetError("%s", panel->Error());
		delete panel;
		return false;
	}
	return panel;
}

UIPanel *
UIManager::GetPanel(const char *name)
{
	for (unsigned i = 0; i < m_panels.length(); ++i) {
		if (strcmp(name, m_panels[i]->GetName()) == 0) {
			return m_panels[i];
		}
	}
	return NULL;
}

void
UIManager::ShowPanel(UIPanel *panel)
{
	if (panel && !m_visible.find(panel)) {
		m_visible.add(panel);
		panel->Show();
	}
}

void
UIManager::HidePanel(UIPanel *panel)
{
	if (panel && m_visible.remove(panel)) {
		panel->Hide();
	}
}

void
UIManager::Draw()
{
	for (unsigned i = 0; i < m_visible.length(); ++i) {
		UIPanel *panel = m_visible[i];

		panel->Draw();

		if (panel->IsFullscreen()) {
			break;
		}
	}
}

bool
UIManager::HandleEvent(const SDL_Event &event)
{
	for (unsigned i = m_visible.length(); i--; ) {
		UIPanel *panel = m_visible[i];

		if (panel->HandleEvent(event)) {
			return true;
		}
		if (panel->IsFullscreen()) {
			break;
		}
	}
	return false;
}