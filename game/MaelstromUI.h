/*
  Maelstrom: Open Source version of the classic game by Ambrosia Software
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

#include "Maelstrom_Globals.h"
#include "Localization.h"

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
	virtual UITexture *CreateImage(SDL_Surface *surface);
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

	virtual bool Load(rapidxml::xml_node<> *node, const UITemplates *templates) override;
};

//////////////////////////////////////////////////////////////////////////////
class UIDrawEngineSprite : public UIDrawEngine
{
public:
	UIDrawEngineSprite() : UIDrawEngine() { }

	virtual bool Load(rapidxml::xml_node<> *node, const UITemplates *templates) override;
};

//////////////////////////////////////////////////////////////////////////////
class UIDrawEngineTitle : public UIDrawEngine
{
public:
	UIDrawEngineTitle() : UIDrawEngine() { }

	virtual bool Load(rapidxml::xml_node<> *node, const UITemplates *templates) override;
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

