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
#include "../screenlib/UIDialogButton.h"
#include "../utils/hashtable.h"


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

	/* First see if we can find it in our cache */
	keysize = strlen(fontName)+1+2+1+1+1+8+1+strlen(text)+1;
	key = SDL_stack_alloc(char, keysize);
	SDL_snprintf(key, keysize, "%s:%d:%c:%8.8x:%s", fontName, fontSize, '0'+fontStyle, color, text);
	if (hash_find(m_strings, key, (const void**)&texture)) {
		SDL_stack_free(key);
		return new UITexture(texture);
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

	return new UITexture(texture);
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

void
MaelstromUI::PlaySound(int soundID)
{
	sound->PlaySound(soundID, 5);
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
	} else if (SDL_strcasecmp(type, "Icon") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineIcon());
	} else if (SDL_strcasecmp(type, "Sprite") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineSprite());
	} else if (SDL_strcasecmp(type, "Title") == 0) {
		element = new UIElement(parent, name, new UIDrawEngineTitle());
	} else if (SDL_strcasecmp(type, "ControlButton") == 0) {
		element = new UIElementControlButton(parent, name, new UIDrawEngine());
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
UIElementControlButton::UIElementControlButton(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIElement(parent, name, drawEngine)
{
#ifndef __IPHONEOS__
	// Use the mouse if touch control isn't available
	m_mouseEnabled = true;
#endif
}

bool
UIElementControlButton::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_attribute<> *attr;

	if (!UIElement::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("action", 0, false);
	if (!attr) {
		error("Element '%s' missing attribute 'action'", node->name());
		return false;
	}

	if (SDL_strcasecmp(attr->value(), "THRUST") == 0) {
		m_control = THRUST_KEY;
	} else if (SDL_strcasecmp(attr->value(), "RIGHT") == 0) {
		m_control = RIGHT_KEY;
	} else if (SDL_strcasecmp(attr->value(), "LEFT") == 0) {
		m_control = LEFT_KEY;
	} else if (SDL_strcasecmp(attr->value(), "SHIELD") == 0) {
		m_control = SHIELD_KEY;
	} else if (SDL_strcasecmp(attr->value(), "FIRE") == 0) {
		m_control = FIRE_KEY;
	} else if (SDL_strcasecmp(attr->value(), "PAUSE") == 0) {
		m_control = PAUSE_KEY;
	} else if (SDL_strcasecmp(attr->value(), "ABORT") == 0) {
		m_control = ABORT_KEY;
	} else {
		error("Element '%s' has unknown action '%s'", node->name(), attr->value());
		return false;
	}

	return true;
}

bool
UIElementControlButton::HandleEvent(const SDL_Event &event)
{
	if (UIElement::HandleEvent(event)) {
		return true;
	}

	if (event.type == SDL_FINGERDOWN) {
		// Convert the touch coordinate into something useful here
		int x, y;

		if (!m_screen->ConvertTouchCoordinates(event.tfinger, &x, &y)) {
			return false;
		}
		if (ContainsPoint(x, y)) {
			m_finger = event.tfinger.fingerId;
			OnMouseDown();
			return true;
		}
	}
	if (event.type == SDL_FINGERUP && event.tfinger.fingerId == m_finger) {
		m_finger = 0;
		OnMouseUp();
		return true;
	}

	return false;
}

void
UIElementControlButton::OnMouseDown()
{
	if (m_control == PAUSE_KEY) {
		return;
	}
	if (m_control == ABORT_KEY) {
		return;
	}

	Player *player = GetControlPlayer(CONTROL_TOUCH);
	if (player) {
		player->SetControl(m_control, true);
	}
}

void
UIElementControlButton::OnMouseUp()
{
	if (m_control == PAUSE_KEY) {
		gGameInfo.ToggleLocalState(STATE_PAUSE);
		return;
	}
	if (m_control == ABORT_KEY) {
		gGameInfo.SetLocalState(STATE_ABORT, true);
		return;
	}

	Player *player = GetControlPlayer(CONTROL_TOUCH);
	if (player) {
		player->SetControl(m_control, false);
	}
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
