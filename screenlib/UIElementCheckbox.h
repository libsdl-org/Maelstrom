/*
  screenlib:  A simple window and UI library based on the SDL library
  Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

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

#ifndef _UIElementCheckbox_h
#define _UIElementCheckbox_h

#include "UIElementButton.h"


class UIElementCheckbox : public UIElementButton
{
DECLARE_TYPESAFE_CLASS(UIElementButton)
public:
	UIElementCheckbox(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine);
	virtual ~UIElementCheckbox();

	virtual bool Load(rapidxml::xml_node<> *node, const UITemplates *templates) override;
	virtual bool FinishLoading() override;

	// Bind any preferences variables to the preferences manager
	virtual void LoadData(Prefs *prefs) override;
	virtual void SaveData(Prefs *prefs) override;

	void SetChecked(bool checked) {
		if (checked != m_checked) {
			m_checked = checked;
			if (m_images[m_checked]) {
				SetImage(m_images[m_checked]);
			}
			OnChecked(checked);
		}
	}
	bool IsChecked() const {
		return m_checked;
	}

	virtual void OnClick() override;

protected:
	// This can be overridden by inheriting classes
	virtual void OnChecked(bool checked) { }

protected:
	bool m_checked;
	char *m_valueBinding;
	char *m_images[2];
};

#endif // _UIElementCheckbox_h
