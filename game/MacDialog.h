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

	override bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);

	override void Show();
	override void Draw(DRAWLEVEL drawLevel);

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
	override void Init(UIElement *element);
};

//////////////////////////////////////////////////////////////////////////////

class MacDialogButton : public MacDialogDrawEngine
{
public:
	MacDialogButton() : MacDialogDrawEngine() { }

	override void Init(UIElement *element);
	override void OnLoad();
	override void OnDraw();
	override void OnMouseDown();
	override void OnMouseUp();

protected:
	Uint32 m_colors[2];
};

//////////////////////////////////////////////////////////////////////////////

class MacDialogCheckbox : public MacDialogDrawEngine
{
public:
	MacDialogCheckbox() : MacDialogDrawEngine() { }

	override void Init(UIElement *element);
	override void OnDraw();
};

//////////////////////////////////////////////////////////////////////////////

class MacDialogRadioButton : public MacDialogDrawEngine
{
public:
	MacDialogRadioButton() : MacDialogDrawEngine() { }

	override void Init(UIElement *element);
	override void OnDraw();
};

//////////////////////////////////////////////////////////////////////////////

class MacDialogEditbox : public MacDialogDrawEngine
{
public:
	MacDialogEditbox() : MacDialogDrawEngine() { }

	override void Init(UIElement *element);
	override void OnLoad();
	override void OnDraw();

protected:
	Uint32 m_colors[2];
};

//////////////////////////////////////////////////////////////////////////////

#endif // _MacDialog_h
