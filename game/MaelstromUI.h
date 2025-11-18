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

#include "Maelstrom_Globals.h"

#include "../screenlib/UIManager.h"
#include "../screenlib/UIDrawEngine.h"

struct HashTable;

class MaelstromUI : public UIManager
{
public:
	MaelstromUI(FrameBuf *screen, Prefs *prefs);
	virtual ~MaelstromUI();

	//
	// UIFontInterface
	//
	virtual UITexture *CreateText(const char *text, const char *fontName, int fontSize, UIFontStyle fontStyle, Uint32 color);
	virtual void FreeText(UITexture *texture);

	//
	// UIImageInterface
	//
	virtual UITexture *CreateImage(const char *name);
	virtual void FreeImage(UITexture *texture);
	virtual UITexture *CreateBackground(const char *name);
	virtual void FreeBackground(UITexture *texture);

	//
	// UISoundInterface
	//
	virtual void PlaySound(const char *name);

	//
	// UIManager functions
	//
	virtual UIPanel *CreatePanel(const char *type, const char *name);
	virtual UIPanelDelegate *CreatePanelDelegate(UIPanel *panel, const char *delegate);
	virtual UIElement *CreateElement(UIBaseElement *parent, const char *type, const char *name);

protected:
	HashTable *m_fonts;
	HashTable *m_strings;

protected:
	MFont *GetFont(const char *fontName, int fontSize);
};

//////////////////////////////////////////////////////////////////////////////
class UIDrawEngineIcon : public UIDrawEngine
{
public:
	UIDrawEngineIcon() : UIDrawEngine() { }

	override bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);
};

//////////////////////////////////////////////////////////////////////////////
class UIDrawEngineSprite : public UIDrawEngine
{
public:
	UIDrawEngineSprite() : UIDrawEngine() { }

	override bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);
};

//////////////////////////////////////////////////////////////////////////////
class UIDrawEngineTitle : public UIDrawEngine
{
public:
	UIDrawEngineTitle() : UIDrawEngine() { }

	override bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);
};

// Generic dialog function
extern void ShowMessage(const char *text,
                        const char *button1Text = TEXT("Okay"),
                        const char *button1Action = NULL,
                        const char *button2Text = NULL,
                        const char *button2Action = NULL,
                        const char *button3Text = NULL,
                        const char *button3Action = NULL);
extern void HideMessage();

