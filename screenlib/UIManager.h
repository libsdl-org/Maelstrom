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

#ifndef _UIManager_h
#define _UIManager_h

#include "SDL.h"
#include "../utils/array.h"
#include "UIArea.h"
#include "UIPanel.h"
#include "UIFontInterface.h"
#include "UIImageInterface.h"
#include "UISoundInterface.h"
#include "UITemplates.h"

class FrameBuf;
class UIBaseElement;
class UIElement;
class Prefs;
struct HashTable;

enum PANEL_TRANSITION_TYPE
{
	PANEL_TRANSITION_NONE,
	PANEL_TRANSITION_FADE,
};

class UIManager : public UIArea, public UIFontInterface, public UIImageInterface, public UISoundInterface
{
public:
	UIManager(FrameBuf *screen, Prefs *prefs);
	virtual ~UIManager();

	FrameBuf *GetScreen() const {
		return m_screen;
	}
	Prefs *GetPrefs() const {
		return m_prefs;
	}
	const UITemplates *GetTemplates() const {
		return &m_templates;
	}

	void Shutdown();
	void ClearLoadPath();
	void AddLoadPath(const char *path);
	bool LoadTemplates(const char *file);
	UIPanel *LoadPanel(const char *name);
	UIPanel *GetPanel(const char *name, bool allowLoad = true);
	template <typename T>
	T *GetPanel(const char *name) {
		UIPanel *panel = GetPanel(name);
		if (panel && panel->IsA(T::GetType())) {
			return (T*)panel;
		}
		return NULL;
	}
	UIPanel *GetFullscreenPanel();
	UIPanel *GetNextPanel(UIPanel *panel);
	UIPanel *GetPrevPanel(UIPanel *panel);
	UIPanel *GetCurrentPanel();

	/* These are called by the UIPanel class */
	void AddPanel(UIPanel *panel) {
		if (!m_panels.find(panel)) {
			m_panels.add(panel);
		}
	}
	void RemovePanel(UIPanel *panel) {
		m_visible.remove(panel);
		m_panels.remove(panel);
	}

	bool IsShown(const char *name) const;
	bool IsShown(UIPanel *panel) const;
	void ShowPanel(UIPanel *panel);
	void ShowPanel(const char *name) {
		ShowPanel(GetPanel(name));
	}
	void HidePanel(UIPanel *panel);
	void HidePanel(const char *name) {
		HidePanel(GetPanel(name));
	}
	void DeletePanel(UIPanel *panel);
	void DeletePanel(const char *name) {
		DeletePanel(GetPanel(name, false));
	}

	void SetPanelTransition(PANEL_TRANSITION_TYPE transition) {
		m_panelTransition = transition;
	}
	PANEL_TRANSITION_TYPE GetPanelTransition() const {
		return m_panelTransition;
	}

	void CaptureEvents(UIBaseElement *element);
	void ReleaseEvents(UIBaseElement *element);
	bool IsCapturingEvents(UIBaseElement *element);

	void HideDialogs();

	void SetCondition(const char *token, bool isTrue = true);
	bool CheckCondition(const char *condition);

	void Poll();
	void Draw(bool fullUpdate = true);
	bool HandleEvent(const SDL_Event &event);

	virtual void OnRectChanged() {
		UIArea::OnRectChanged();

		for (unsigned int i = 0; i < m_panels.length(); ++i) {
			UIPanel *panel = m_panels[i];
			panel->AutoSize(Width(), Height(), true);
		}
	}

public:
	/* These should be implemented to load UI from XMl */
	virtual UIPanel *CreatePanel(const char *type, const char *name) {
		return NULL;
	}
	virtual UIPanelDelegate *CreatePanelDelegate(UIPanel *panel, const char *delegate) {
		return NULL;
	}
	virtual UIElement *CreateElement(UIBaseElement *parent, const char *type, const char *name = "") {
		return NULL;
	}

protected:
	FrameBuf *m_screen;
	Prefs *m_prefs;
	array<char *> m_loadPath;
	UITemplates m_templates;
	array<UIPanel *> m_panels;
	array<UIPanel *> m_visible;
	array<UIPanel *> m_delete;
	array<UIBaseElement *> m_eventCapture;
	PANEL_TRANSITION_TYPE m_panelTransition;
	HashTable *m_conditions;
};

#endif // _UIManager_h
