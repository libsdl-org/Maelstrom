/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

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

#ifndef _MacDialog_h
#define _MacDialog_h

#include "../screenlib/UIDialog.h"
#include "../screenlib/UIDrawEngine.h"

class MacDialog : public UIDialog
{
DECLARE_TYPESAFE_CLASS(UIDialog)
public:
	MacDialog(UIManager *ui, const char *name);

	virtual bool Load(rapidxml::xml_node<> *node, const UITemplates *templates) override;

	virtual void Show() override;
	virtual void Draw(DRAWLEVEL drawLevel) override;

protected:
	enum {
		COLOR_BLACK,
		COLOR_DARK,
		COLOR_MEDIUM,
		COLOR_LIGHT,
		COLOR_WHITE,
		NUM_COLORS
	};
	Uint32 m_colors[NUM_COLORS];
	bool m_expand;
	int m_step;
};

//////////////////////////////////////////////////////////////////////////////

class MacDialogDrawEngine : public UIDrawEngine
{
public:
	virtual void Init(UIElement *element) override;
};

//////////////////////////////////////////////////////////////////////////////

class MacRoundedButton : public MacDialogDrawEngine
{
public:
	MacRoundedButton() : MacDialogDrawEngine() { }

	virtual void Init(UIElement *element) override;
	virtual void OnLoad() override;
	virtual void OnDraw() override;
	virtual void OnMouseDown() override;
	virtual void OnMouseUp() override;

protected:
	Uint32 m_colors[2];
};

//////////////////////////////////////////////////////////////////////////////

class MacDialogButton : public MacRoundedButton
{
public:
	MacDialogButton() : MacRoundedButton() { }

	virtual void Init(UIElement *element) override;
};

//////////////////////////////////////////////////////////////////////////////

class MacDialogCheckbox : public MacDialogDrawEngine
{
public:
	MacDialogCheckbox() : MacDialogDrawEngine() { }

	virtual void Init(UIElement *element) override;
	virtual void OnDraw() override;
};

//////////////////////////////////////////////////////////////////////////////

class MacDialogRadioButton : public MacDialogDrawEngine
{
public:
	MacDialogRadioButton() : MacDialogDrawEngine() { }

	virtual void Init(UIElement *element) override;
	virtual void OnDraw() override;
};

//////////////////////////////////////////////////////////////////////////////

class MacDialogEditbox : public MacDialogDrawEngine
{
public:
	MacDialogEditbox() : MacDialogDrawEngine() { }

	virtual void Init(UIElement *element) override;
	virtual void OnLoad() override;
	virtual void OnDraw() override;

protected:
	Uint32 m_colors[2];
};

//////////////////////////////////////////////////////////////////////////////

#endif // _MacDialog_h
