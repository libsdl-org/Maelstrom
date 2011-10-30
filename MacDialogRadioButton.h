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

#ifndef _MacDialogRadioButton_h
#define _MacDialogRadioButton_h

#include "screenlib/UIElementRadio.h"


class MacDialogRadioButton : public UIElementRadioButton
{
DECLARE_TYPESAFE_CLASS(UIElementRadioButton)
public:
	MacDialogRadioButton(UIBaseElement *parent, const char *name = "");

	override void Draw();

protected:
	override UIElementLabel *CreateLabel();

protected:
	Uint32 m_color;
};

#endif // _MacDialogRadioButton_h
