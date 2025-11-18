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

#ifndef _UIDialog_h
#define _UIDialog_h

#include "UIPanel.h"
#include "UIElement.h"

class UIDialog;

class UIDialogDelegate : public UIPanelDelegate
{
public:
	UIDialogDelegate(UIPanel *panel);

protected:
	UIDialog *m_dialog;
};

/* This function gets called when the dialog is shown.
*/
typedef void (*UIDialogInitHandler)(void *data, UIDialog *dialog);

/* This function gets called when the dialog is hidden.
   The status defaults to 0, but can be changed by dialog buttons.
 */
typedef void (*UIDialogDoneHandler)(void *data, UIDialog *dialog, int status);

class UIDialog : public UIPanel
{
DECLARE_TYPESAFE_CLASS(UIPanel)
public:
	UIDialog(UIManager *ui, const char *name);

	void SetDialogInitHandler(UIDialogInitHandler handleInit, void *data = 0) {
		m_handleInit = handleInit;
		m_handleInitData = data;
	}
	void SetDialogDoneHandler(UIDialogDoneHandler handleDone, void *data = 0) {
		m_handleDone = handleDone;
		m_handleDoneData = data;
	}
	void SetDialogStatus(int status) {
		m_status = status;
	}
	int GetDialogStatus() const {
		return m_status;
	}

	override void Show();
	override void Hide();
	override bool HandleEvent(const SDL_Event &event);

protected:
	int m_status;
	UIDialogInitHandler m_handleInit;
	void *m_handleInitData;
	UIDialogDoneHandler m_handleDone;
	void *m_handleDoneData;
};

#endif // _UIDialog_h
