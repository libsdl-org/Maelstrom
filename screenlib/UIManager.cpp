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

#include "../utils/loadxml.h"
#include "../utils/hashtable.h"

#include "SDL_FrameBuf.h"
#include "UIManager.h"
#include "UIPanel.h"


static void
hash_nuke_string(const void *key, const void *value, void *data)
{
	SDL_free((char*)key);
}

UIManager::UIManager(FrameBuf *screen, Prefs *prefs) :
	UIArea(NULL, screen->Width(), screen->Height())
{
	m_screen = screen;
	m_prefs = prefs;
	m_panelTransition = PANEL_TRANSITION_NONE;
	m_conditions = hash_create(screen, hash_hash_string, hash_keymatch_string, hash_nuke_string);

	AddLoadPath(".");
}

UIManager::~UIManager()
{
	/* Make sure Shutdown() has been called */
	SDL_assert(m_panels.length() == 0);
	ClearLoadPath();
	hash_destroy(m_conditions);
}

void
UIManager::Shutdown()
{
	/* Deleting the panels will remove them from the manager */
	while (m_panels.length() > 0) {
		delete m_panels[m_panels.length()-1];
	}
}

void
UIManager::ClearLoadPath()
{
	for (unsigned int i = 0; i < m_loadPath.length(); ++i) {
		SDL_free(m_loadPath[i]);
	}
	m_loadPath.clear();
}

void
UIManager::AddLoadPath(const char *path)
{
	m_loadPath.add(SDL_strdup(path));
}

bool
UIManager::LoadTemplates(const char *file)
{
	char path[1024];

	for (unsigned int i = 0; i < m_loadPath.length(); ++i) {
		SDL_snprintf(path, sizeof(path), "%s/%s", m_loadPath[i], file);
		if (m_templates.Load(path)) {
			return true;
		}
	}
	SDL_Log("Couldn't load template %s", file);
	return false;
}

UIPanel *
UIManager::LoadPanel(const char *name)
{
	UIPanel *panel;

	panel = GetPanel(name, false);
	if (!panel) {
		char file[1024];
		char *buffer;
		rapidxml::xml_document<> doc;

		bool loaded = false;
		for (unsigned int i = 0; i < m_loadPath.length(); ++i) {
			SDL_snprintf(file, sizeof(file), "%s/%s.xml", m_loadPath[i], name);
			if (LoadXML(file, buffer, doc)) {
				loaded = true;
				break;
			}
		}
		if (!loaded) {
			SDL_Log("Couldn't load panel %s", name);
			return NULL;
		}

		rapidxml::xml_node<> *node = doc.first_node();
		rapidxml::xml_attribute<> *attr;

		panel = CreatePanel(node->name(), name);
		if (!panel) {
			SDL_Log("Warning: Couldn't create panel %s in %s",
						node->name(), file);
			SDL_free(buffer);
			return NULL;
		}

		attr = node->first_attribute("delegate", 0, false);
		if (attr) {
			UIPanelDelegate *delegate;

			delegate = CreatePanelDelegate(panel, attr->value());
			if (!delegate) {
				SDL_Log("Warning: Couldn't find delegate '%s'", attr->value());
				SDL_free(buffer);
				delete panel;
				return NULL;
			}
			panel->SetPanelDelegate(delegate);
		}
		
		if (!panel->Load(node, GetTemplates()) ||
		    !panel->FinishLoading()) {
			SDL_Log("Warning: Couldn't load %s: %s",
						file, panel->Error());
			SDL_free(buffer);
			delete panel;
			return NULL;
		}
		SDL_free(buffer);
	}
	return panel;
}

UIPanel *
UIManager::GetPanel(const char *name, bool allowLoad)
{
	for (unsigned int i = 0; i < m_panels.length(); ++i) {
		if (strcmp(name, m_panels[i]->GetName()) == 0) {
			return m_panels[i];
		}
	}
	if (allowLoad) {
		return LoadPanel(name);
	}
	return NULL;
}

UIPanel *
UIManager::GetFullscreenPanel()
{
	if (m_visible.length() > 0) {
		UIPanel *panel = m_visible[0];
		if (panel->IsFullscreen()) {
			return panel;
		}
	}
	return NULL;
}

UIPanel *
UIManager::GetNextPanel(UIPanel *panel)
{
	for (unsigned int i = 0; i < m_visible.length(); ++i) {
		if (m_visible[i] == panel) {
			if (i+1 < m_visible.length()) {
				return m_visible[i+1];
			} else {
				return NULL;
			}
		}
	}
	return NULL;
}

UIPanel *
UIManager::GetPrevPanel(UIPanel *panel)
{
	for (unsigned int i = 0; i < m_visible.length(); ++i) {
		if (m_visible[i] == panel) {
			if (i > 0) {
				return m_visible[i-1];
			} else {
				return NULL;
			}
		}
	}
	return NULL;
}

UIPanel *
UIManager::GetCurrentPanel()
{
	if (m_visible.length() > 0) {
		return m_visible[m_visible.length()-1];
	}
	return NULL;
}

bool
UIManager::IsShown(const char *name) const
{
	for (unsigned int i = 0; i < m_panels.length(); ++i) {
		if (strcmp(name, m_panels[i]->GetName()) == 0) {
			return m_visible.find(m_panels[i]);
		}
	}
	return false;
}

bool
UIManager::IsShown(UIPanel *panel) const
{
	return (panel && m_visible.find(panel));
}

void
UIManager::ShowPanel(UIPanel *panel)
{
	if (panel && !m_visible.find(panel)) {
		/* If this is fullscreen, then hide any previous fullscreen panel */
		if (panel->IsFullscreen()) {
			for (unsigned int i = m_visible.length(); i--; ) {
				if (m_visible[i]->IsFullscreen()) {
					HidePanel(m_visible[i]);
					break;
				}
			}
		}

		// Make sure that fullscreen panels go behind dialogs
		bool added = false;
		for (unsigned int i = 0; i < m_visible.length(); ++i) {
			if (panel->GetPriority() < m_visible[i]->GetPriority()) {
				m_visible.insert(panel, i);
				added = true;
				break;
			}
		}
		if (!added) {
			m_visible.add(panel);
		}

		panel->Show();
		if (panel->IsFullscreen()) {
			Draw();
		}
		if (!panel->IsCursorVisible()) {
			m_screen->HideCursor();
		}
	}
}

void
UIManager::HidePanel(UIPanel *panel)
{
	if (panel && m_visible.remove(panel)) {
		panel->Hide();

		if (m_panelTransition == PANEL_TRANSITION_FADE &&
		    panel->IsFullscreen()) {
			m_screen->FadeOut();
		}
		if (!panel->IsCursorVisible()) {
			m_screen->ShowCursor();
		}

#ifdef FAST_ITERATION
		// This is useful for iteration, panels are reloaded 
		// each time they are shown.
		DeletePanel(panel);
#endif
	}
}

void
UIManager::DeletePanel(UIPanel *panel)
{
	if (panel && m_panels.find(panel)) {
		// Remove us so we don't recurse in HidePanel() or callbacks
		m_panels.remove(panel);
		HidePanel(panel);
		m_delete.add(panel);
	}
}

void
UIManager::CaptureEvents(UIBaseElement *element)
{
	if (!IsCapturingEvents(element)) {
		m_eventCapture.add(element);
	}
}

void
UIManager::ReleaseEvents(UIBaseElement *element)
{
	m_eventCapture.remove(element);
}

bool
UIManager::IsCapturingEvents(UIBaseElement *element)
{
	return m_eventCapture.find(element);
}

void
UIManager::HideDialogs()
{
	for (int i = m_visible.length()-1; i >= 0; --i) {
		UIPanel *panel = m_visible[i];
		if (!panel->IsFullscreen()) {
			HidePanel(panel);
		}
	}
}

void
UIManager::SetCondition(const char *token, bool isTrue)
{
	if (isTrue) {
		hash_insert(m_conditions, SDL_strdup(token), NULL);
	} else {
		hash_remove(m_conditions, token);
	}
}

bool
UIManager::CheckCondition(const char *condition)
{
	if (!condition || !*condition) {
		return false;
	}

	if (*condition == '!') {
		return !CheckCondition(condition+1);
	}

	return hash_find(m_conditions, condition, NULL) != 0;
}

void
UIManager::Poll()
{
	unsigned int i;

	for (i = 0; i < m_visible.length(); ++i) {
		UIPanel *panel = m_visible[i];

		panel->Poll();
	}

	// Clean up any deleted panels when we're done...
	if (!m_delete.empty()) {
		for (i = 0; i < m_delete.length(); ++i) {
			delete m_delete[i];
		}
		m_delete.clear();
	}
}

void
UIManager::Draw(bool fullUpdate)
{
	unsigned int i;

	// Run the tick before we draw in case it changes drawing state
	for (i = 0; i < m_visible.length(); ++i) {
		UIPanel *panel = m_visible[i];

		panel->Poll();
		panel->Tick();
	}

	if (fullUpdate) {
		m_screen->Clear();
	}
	for (i = 0; i < m_visible.length(); ++i) {
		UIPanel *panel = m_visible[i];

		for (int drawLevel = 0; drawLevel < NUM_DRAWLEVELS; ++drawLevel) {
			panel->Draw((DRAWLEVEL)drawLevel);
		}
	}
	if (fullUpdate) {
		m_screen->Update();
		if (m_panelTransition == PANEL_TRANSITION_FADE) {
			m_screen->FadeIn();
		}
	}
}

bool
UIManager::HandleEvent(const SDL_Event &event)
{
	unsigned int i;

	if (event.type == SDL_WINDOWEVENT &&
	    event.window.event == SDL_WINDOWEVENT_RESIZED &&
	    m_screen->Resizable()) {
		SDL_Rect clip;

		// Reset the clip rectangle
		clip.x = 0;
		clip.y = 0;
		clip.w = m_screen->Width();
		clip.h = m_screen->Height();
		m_screen->ClipBlit(&clip);

		// Resize to match window size
		SetSize(m_screen->Width(), m_screen->Height());
	}

	// In case it's not called any other time...
	Poll();

	for (i = m_eventCapture.length(); i--; ) {
		UIBaseElement *element = m_eventCapture[i];

		for (int drawLevel = NUM_DRAWLEVELS; drawLevel--; ) {
			if (element->DispatchEvent(event, (DRAWLEVEL)drawLevel)) {
				return true;
			}
		}
	}

	for (i = m_visible.length(); i--; ) {
		UIPanel *panel = m_visible[i];

		for (int drawLevel = NUM_DRAWLEVELS; drawLevel--; ) {
			if (panel->DispatchEvent(event, (DRAWLEVEL)drawLevel)) {
				return true;
			}
		}
		if (panel->IsFullscreen()) {
			break;
		}
	}
	return false;
}
