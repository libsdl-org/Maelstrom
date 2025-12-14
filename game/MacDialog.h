/*
    Maelstrom: Open Source version of the classic game by Ambrosia Software
    Copyright (C) 1997-2011  Sam Lantinga

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

class MacDialogButton : public MacDialogDrawEngine
{
public:
	MacDialogButton() : MacDialogDrawEngine() { }

	virtual void Init(UIElement *element) override;
	virtual void OnLoad() override;
	virtual void OnDraw() override;
	virtual void OnMouseDown() override;
	virtual void OnMouseUp() override;

protected:
	Uint32 m_colors[2];
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
