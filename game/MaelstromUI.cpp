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

#include "MaelstromUI.h"
#include "Maelstrom_Globals.h"
#include "main.h"
#include "load.h"
#include "controls.h"
#include "about.h"
#include "game.h"
#include "continue.h"
#include "gameover.h"
#include "player.h"
#include "lobby.h"
#include "MacDialog.h"
#include "../screenlib/UIContainer.h"
#include "../screenlib/UIElementButton.h"
#include "../screenlib/UIElementCheckbox.h"
#include "../screenlib/UIElementEditbox.h"
#include "../screenlib/UIElementRadio.h"
#include "../screenlib/UIElementThumbstick.h"
#include "../screenlib/UIDialogButton.h"
#include "../utils/hashtable.h"

struct DialogInfo {
    DialogInfo(const char *text,
               const char *button1Text, const char *button1Action,
               const char *button2Text, const char *button2Action,
               const char *button3Text, const char *button3Action)
        : text(text ? SDL_strdup(text) : NULL),
          button1Text(button1Text ? SDL_strdup(button1Text) : NULL),
          button1Action(button1Action ? SDL_strdup(button1Action) : NULL),
          button2Text(button2Text ? SDL_strdup(button2Text) : NULL),
          button2Action(button2Action ? SDL_strdup(button2Action) : NULL),
          button3Text(button3Text ? SDL_strdup(button3Text) : NULL),
          button3Action(button3Action ? SDL_strdup(button3Action) : NULL)
    { }

    ~DialogInfo() {
        if (this->text) {
            SDL_free(this->text);
        }
        if (this->button1Text) {
            SDL_free(this->button1Text);
        }
        if (this->button1Action) {
            SDL_free(this->button1Action);
        }
        if (this->button2Text) {
            SDL_free(this->button2Text);
        }
        if (this->button2Action) {
            SDL_free(this->button2Action);
        }
        if (this->button3Text) {
            SDL_free(this->button3Text);
        }
        if (this->button3Action) {
            SDL_free(this->button3Action);
        }
    }

    char *text;
    char *button1Text;
    char *button1Action;
    char *button2Text;
    char *button2Action;
    char *button3Text;
    char *button3Action;
};

static void MessageDialogInit(void *param, UIDialog *dialog)
{
    DialogInfo *info = (DialogInfo *)param;

    UIElement *message = dialog->GetElement<UIElement>("message");
    if (message) {
        message->SetText(info->text);
    }

    UIElement *button1 = dialog->GetElement<UIElement>("button1");
    if (button1) {
        if (info->button1Text) {
            button1->SetText(info->button1Text);
            button1->Show();
        } else {
            button1->Hide();
        }
        if (info->button1Action) {
            button1->SetAction(info->button1Action);
        } else {
            button1->SetAction(NULL);
        }
    }

    UIElement *button2 = dialog->GetElement<UIElement>("button2");
    if (button2) {
        if (info->button2Text) {
            button2->SetText(info->button2Text);
            button2->Show();
        } else {
            button2->Hide();
        }
        if (info->button2Action) {
            button2->SetAction(info->button2Action);
        } else {
            button2->SetAction(NULL);
        }
    }

    UIElement *button3 = dialog->GetElement<UIElement>("button3");
    if (button3) {
        if (info->button3Text) {
            button3->SetText(info->button3Text);
            button3->Show();
        } else {
            button3->Hide();
        }
        if (info->button3Action) {
            button3->SetAction(info->button3Action);
        } else {
            button3->SetAction(NULL);
        }
    }
}

static void MessageDialogDone(void *param, UIDialog *dialog, int status)
{
    DialogInfo *info = (DialogInfo *)param;

    delete info;
}

void ShowMessage(const char *text,
                 const char *button1Text, const char *button1Action,
                 const char *button2Text, const char *button2Action,
                 const char *button3Text, const char *button3Action)
{
    if (!ui) {
        // Not initialized yet
        return;
    }

    UIDialog *dialog = ui->GetPanel<UIDialog>(DIALOG_MESSAGE);
    if (dialog) {
        // Free any existing message
        ui->HidePanel(dialog);

        DialogInfo *info = new DialogInfo(text, button1Text, button1Action,
                                          button2Text, button2Action,
                                          button3Text, button3Action);
        dialog->SetDialogInitHandler(MessageDialogInit, info);
        dialog->SetDialogDoneHandler(MessageDialogDone, info);
        ui->ShowPanel(dialog);
    }
}

void HideMessage()
{
    UIDialog *dialog = ui->GetPanel<UIDialog>(DIALOG_MESSAGE);
    if (dialog) {
        ui->HidePanel(dialog);
    }
}

static void
hash_nuke_string_font(const void *key, const void *value, void *data)
{
	SDL_free((char*)key);
	fontserv->FreeFont((MFont *)value);
}

static void
hash_nuke_string_text(const void *key, const void *value, void *data)
{
	SDL_free((char*)key);
	fontserv->FreeText((SDL_Texture *)value);
}

MaelstromUI::MaelstromUI(FrameBuf *screen, Prefs *prefs) : UIManager(screen, prefs)
{
	/* Create our font hashtables */
	m_fonts = hash_create(screen, hash_hash_string, hash_keymatch_string, hash_nuke_string_font);
	m_strings = hash_create(screen, hash_hash_string, hash_keymatch_string, hash_nuke_string_text);

	/* Set up some conditions useful for UI loading */
#ifdef USE_TOUCHCONTROL
	SetCondition("TOUCH");
#endif
#if __IPHONEOS__ || __ANDROID__
	SetCondition("MOBILE");
#endif
#if __IPHONEOS__
	SetCondition("IOS");
#endif
	if (gClassic) {
		SetCondition("CLASSIC");
	}

	/* Load up our UI templates */
	ClearLoadPath();
	for (int i = gResolutionIndex; i < gResolutions.length(); ++i) {
		char path[1024];

		SDL_snprintf(path, sizeof(path), "UI%s", gResolutions[i].path_suffix);
		AddLoadPath(path);
	}
	LoadTemplates("UITemplates.xml");
}

MaelstromUI::~MaelstromUI()
{
	Shutdown();
	hash_destroy(m_fonts);
	hash_destroy(m_strings);
}

MFont *
MaelstromUI::GetFont(const char *fontName, int fontSize)
{
	char *key;
	int keysize;
	MFont *font;

	keysize = strlen(fontName)+1+2+1;
	key = SDL_stack_alloc(char, keysize);
	SDL_snprintf(key, keysize, "%s:%d", fontName, fontSize);
	if (hash_find(m_fonts, key, (const void**)&font)) {
		SDL_stack_free(key);
		return font;
	}

	font = fontserv->NewFont(fontName, fontSize);
	if (font) {
		/* Add it to our cache */
		hash_insert(m_fonts, SDL_strdup(key), font);
	}
	SDL_stack_free(key);

	return font;
}

UITexture *
MaelstromUI::CreateText(const char *text, const char *fontName, int fontSize, UIFontStyle fontStyle, Uint32 color)
{
	MFont *font;
	Uint8 style;
	char *key;
	int keysize;
	SDL_Texture *texture;
	float logicalScale;

	/* Adjust the font size by our logical scale */
	//logicalScale = m_screen->GetLogicalScale();
	int displayWidth, displayHeight;
	int logicalWidth, logicalHeight;
	m_screen->GetDisplaySize(displayWidth, displayHeight);
	m_screen->GetLogicalSize(logicalWidth, logicalHeight);
	logicalScale = (float)displayWidth / logicalWidth;
	fontSize = (int)(fontSize * logicalScale);

	/* First see if we can find it in our cache */
	keysize = strlen(fontName)+1+2+1+1+1+8+1+strlen(text)+1;
	key = SDL_stack_alloc(char, keysize);
	SDL_snprintf(key, keysize, "%s:%d:%c:%8.8x:%s", fontName, fontSize, '0'+fontStyle, color, text);
	if (hash_find(m_strings, key, (const void**)&texture)) {
		SDL_stack_free(key);
		return new UITexture(texture, logicalScale);
	}

	font = GetFont(fontName, fontSize);
	if (!font) {
		error("Couldn't find font %s size %d\n", fontName, fontSize);
		return NULL;
	}

	switch (fontStyle) {
		case UIFONT_STYLE_NORMAL:
			style = STYLE_NORM;
			break;
		case UIFONT_STYLE_BOLD:
			style = STYLE_BOLD;
			break;
		case UIFONT_STYLE_UNDERLINE:
			style = STYLE_ULINE;
			break;
		case UIFONT_STYLE_ITALIC:
			style = STYLE_ITALIC;
			break;
	}

	texture = fontserv->TextImage(text, font, style, color);
	if (texture) {
		/* Add it to our cache */
		hash_insert(m_strings, SDL_strdup(key), texture);
	}
	SDL_stack_free(key);

	return new UITexture(texture, logicalScale);
}

void 
MaelstromUI::FreeText(UITexture *texture)
{
	/* Leave the SDL texture in the cache */
	delete texture;
}

UITexture *
MaelstromUI::CreateImage(const char *name)
{
	return Load_Image(screen, name);
}

void
MaelstromUI::FreeImage(UITexture *texture)
{
	Free_Texture(screen, texture);
}

UITexture *
MaelstromUI::CreateBackground(const char *name)
{
    UITexture *background;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    background = CreateImage(name);
    if (background) {
        int gridSize = background->Width() / 3;
        background->SetStretchGrid(gridSize);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

    return background;
}

void
MaelstromUI::FreeBackground(UITexture *texture)
{
    FreeImage(texture);
}

void
MaelstromUI::PlaySound(const char *name)
{
	sound->PlaySound(SDL_atoi(name), 5);
}

UIPanel *
MaelstromUI::CreatePanel(const char *type, const char *name)
{
	if (SDL_strcasecmp(type, "Panel") == 0) {
		return new UIPanel(this, name);
	} else if (SDL_strcasecmp(type, "Dialog") == 0) {
		return new MacDialog(ui, name);
	}
	return UIManager::CreatePanel(type, name);
}

UIPanelDelegate *
MaelstromUI::CreatePanelDelegate(UIPanel *panel, const char *delegate)
{
	if (SDL_strcasecmp(delegate, "MainPanel") == 0) {
		return new MainPanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "AboutPanel") == 0) {
		return new AboutPanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "LobbyDialog") == 0) {
		return new LobbyDialogDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "GamePanel") == 0) {
		return new GamePanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "ContinuePanel") == 0) {
		return new ContinuePanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "GameOverPanel") == 0) {
		return new GameOverPanelDelegate(panel);
	} else if (SDL_strcasecmp(delegate, "ControlsDialog") == 0) {
		return new ControlsDialogDelegate(panel);
	}
	return UIManager::CreatePanelDelegate(panel, delegate);
}

UIElement *
MaelstromUI::CreateElement(UIBaseElement *parent, const char *type, const char *name)
{
	UIElement *element;

	if (SDL_strcasecmp(type, "Area") == 0) {
		element = new UIElement(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Line") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineLine());
	} else if (SDL_strcasecmp(type, "Rectangle") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineRect());
	} else if (SDL_strcasecmp(type, "Label") == 0) {
		element = new UIElement(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Image") == 0) {
		element = new UIElement(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Container") == 0) {
		element = new UIContainer(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Button") == 0) {
		element = new UIElementButton(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Checkbox") == 0) {
		element = new UIElementCheckbox(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Thumbstick") == 0) {
		element = new UIElementThumbstick(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "Icon") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineIcon());
	} else if (SDL_strcasecmp(type, "Sprite") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineSprite());
	} else if (SDL_strcasecmp(type, "Title") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineTitle());
	} else if (SDL_strcasecmp(type, "DialogLabel") == 0) {
		element = new UIElement(parent, name, new MacDialogDrawEngine());
	} else if (SDL_strcasecmp(type, "DialogContainer") == 0) {
		element = new UIContainer(parent, name, new MacDialogDrawEngine());
	} else if (SDL_strcasecmp(type, "DialogButton") == 0) {
		element = new UIDialogButton(parent, name, new MacDialogButton());
	} else if (SDL_strcasecmp(type, "DialogCheckbox") == 0) {
		element = new UIElementCheckbox(parent, name, new MacDialogCheckbox());
	} else if (SDL_strcasecmp(type, "DialogRadioGroup") == 0) {
		element = new UIElementRadioGroup(parent, name, new UIDrawEngine());
	} else if (SDL_strcasecmp(type, "DialogRadioButton") == 0) {
		element = new UIElementRadioButton(parent, name, new MacDialogRadioButton());
	} else if (SDL_strcasecmp(type, "DialogEditbox") == 0) {
		element = new UIElementEditbox(parent, name, new MacDialogEditbox());
	} else {
		element = UIManager::CreateElement(parent, name, type);
	}
	return element;
}

//////////////////////////////////////////////////////////////////////////////
bool
UIDrawEngineIcon::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	if (!UIDrawEngine::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("id", 0, false);
	if (!attr) {
		error("Element '%s' missing attribute 'id'", node->name());
		return false;
	}

	UITexture *image = GetCIcon(m_screen, atoi(attr->value()));
	if (!image) {
		error("Unable to load icon %d", atoi(attr->value()));
		return false;
	}

	m_element->SetImage(image);

	return true;
}

//////////////////////////////////////////////////////////////////////////////
bool
UIDrawEngineSprite::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	if (!UIDrawEngine::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("id", 0, false);
	if (!attr) {
		error("Element '%s' missing attribute 'id'", node->name());
		return false;
	}

	UITexture *image = GetSprite(m_screen, atoi(attr->value()), true);
	if (!image) {
		error("Unable to load icon %d", atoi(attr->value()));
		return false;
	}
	m_element->SetImage(image);

	return true;
}

//////////////////////////////////////////////////////////////////////////////
bool
UIDrawEngineTitle::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	if (!UIDrawEngine::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("id", 0, false);
	if (!attr) {
		error("Element '%s' missing attribute 'id'", node->name());
		return false;
	}

	UITexture *image = Load_Title(m_screen, SDL_atoi(attr->value()));
	if (!image) {
		error("Unable to load icon %d", SDL_atoi(attr->value()));
		return false;
	}
	m_element->SetImage(image);

	return true;
}
