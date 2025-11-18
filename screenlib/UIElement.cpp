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

#include "SDL_FrameBuf.h"
#include "UIManager.h"
#include "UIElement.h"

UIElementType UIElement::s_elementType;


UIElement::UIElement(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine) :
	UIBaseElement(parent, name),
	m_textArea(this),
	m_imageArea(this)
{
	m_drawEngine = NULL;
	m_border = false;
	m_fill = false;
	m_fillColor = m_screen->MapRGB(0x00, 0x00, 0x00);
	m_color = m_screen->MapRGB(0xFF, 0xFF, 0xFF);
	m_disabledColor = m_screen->MapRGB(0x80, 0x80, 0x80);
	m_fontName = NULL;
	m_fontSize = 0;
	m_fontStyle = UIFONT_STYLE_NORMAL;
	m_text = NULL;
	m_textBinding = NULL;
	m_textOffset.x = 0;
	m_textOffset.y = 0;
	m_textShadowOffset.x = 0;
	m_textShadowOffset.y = 0;
	m_textShadowColor = m_screen->MapRGB(0x00, 0x00, 0x00);
	m_background = NULL;
	m_image = NULL;
	m_mouseEnabled = false;
	m_mouseInside = false;
	m_mousePressed = false;
	m_clickCallback = NULL;
	m_action = NULL;
	m_actionPressed = NULL;
	m_actionReleased = NULL;

	SetDrawEngine(drawEngine);
}

UIElement::~UIElement()
{
	if (m_drawEngine) {
		delete m_drawEngine;
	}
	if (m_fontName) {
		SDL_free(m_fontName);
	}
	if (m_text) {
		SDL_free(m_text);
	}
	if (m_textBinding) {
		SDL_free(m_textBinding);
	}
	if (m_background && !m_background->IsLocked()) {
		m_ui->FreeBackground(m_background);
	}
	if (m_image && !m_image->IsLocked()) {
		m_ui->FreeImage(m_image);
	}
	if (m_clickCallback) {
		delete m_clickCallback;
	}
	if (m_action) {
		SDL_free(m_action);
	}
	if (m_actionPressed) {
		SDL_free(m_actionPressed);
	}
	if (m_actionReleased) {
		SDL_free(m_actionReleased);
	}
}

bool
UIElement::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_node<> *child;
	rapidxml::xml_attribute<> *attr;

	if (!UIBaseElement::Load(node, templates)) {
		return false;
	}

	LoadString(node, "name", m_name);

	LoadString(node, "action", m_action);
	LoadString(node, "actionPressed", m_actionPressed);
	LoadString(node, "actionReleased", m_actionReleased);

	bool border;
	if (LoadBool(node, "border", border)) {
		SetBorder(border);
	}

	bool fill;
	if (LoadBool(node, "fill", fill)) {
		SetFill(fill);
	}

	Uint32 color;
	if (LoadColor(node, "fillColor", color)) {
		SetFillColor(color);
	}

	if (LoadColor(node, "color", color)) {
		SetColor(color);
	}

	if (LoadColor(node, "disabledColor", color)) {
		SetDisabledColor(color);
	}

	attr = node->first_attribute("font", 0, false);
	if (attr) {
		if (!ParseFont(attr->value())) {
			SDL_Log("Warning: Couldn't parse font '%s'", attr->value());
			return false;
		}
	}

	attr = node->first_attribute("text", 0, false);
	if (attr) {
		SetText(attr->value());
	}

	LoadString(node, "bindText", m_textBinding);

	child = node->first_node("TextArea", 0, false);
	if (child) {
		if (!m_textArea.Load(child)) {
			SDL_Log("Warning: Couldn't load TextArea");
			return false;
		}
	}

	child = node->first_node("TextShadow", 0, false);
	if (child) {
		char *x = NULL;
		char *y = NULL;

		attr = child->first_attribute("offset", 0, false);
		if (attr) {
			x = attr->value();
			y = SDL_strchr(x, ',');
		}
		if (x && y) {
			*y++ = '\0';
			SetTextShadowOffset(SDL_atoi(x), SDL_atoi(y));
			*--y = ',';
		}

		if (LoadColor(child, "color", color)) {
			SetTextShadowColor(color);
		}
	}

	attr = node->first_attribute("background", 0, false);
	if (attr) {
		if (!SetBackground(attr->value())) {
			SDL_Log("Warning: Couldn't load background '%s'", attr->value());
			return false;
		}
	}

	attr = node->first_attribute("image", 0, false);
	if (attr) {
		if (!SetImage(attr->value())) {
			SDL_Log("Warning: Couldn't load image '%s'", attr->value());
			return false;
		}
	}

	child = node->first_node("ImageArea", 0, false);
	if (child) {
		if (!m_imageArea.Load(child)) {
			SDL_Log("Warning: Couldn't load ImageArea");
			return false;
		}
	}

	if (m_drawEngine) {
		if (!m_drawEngine->Load(node, templates)) {
			return false;
		}
	}

	return true;
}

bool
UIElement::FinishLoading()
{
	if (m_drawEngine) {
		m_drawEngine->OnLoad();
	}
	return UIBaseElement::FinishLoading();
}

void
UIElement::LoadData(Prefs *prefs)
{
	if (m_textBinding) {
		SetText(prefs->GetString(m_textBinding, GetText()));
	}
	UIBaseElement::LoadData(prefs);
}

void
UIElement::SaveData(Prefs *prefs)
{
	if (m_textBinding) {
		prefs->SetString(m_textBinding, GetText());
	}
	UIBaseElement::SaveData(prefs);
}

bool
UIElement::LoadColor(rapidxml::xml_node<> *node, const char *name, Uint32 &value)
{
	rapidxml::xml_node<> *child;

	child = node->first_node(name, 0, false);
	if (child) {
		int r = 0xFF, g = 0xFF, b = 0xFF;

		LoadNumber(child, "r", r);
		LoadNumber(child, "g", g);
		LoadNumber(child, "b", b);
		value = m_screen->MapRGB(r, g, b);
		return true;
	}
	return false;
}

bool
UIElement::ParseFont(char *text)
{
	char *fontName;
	char *fontSizeText = NULL;
	int fontSize = m_fontSize;
	char *fontStyleText = NULL;
	UIFontStyle fontStyle = m_fontStyle;
	char *next;

 	text = SDL_strdup(text);
	fontName = text;
	next = SDL_strchr(text, ':');
	if (next) {
		*next++ = '\0';
		fontSizeText = next;
		next = SDL_strchr(next, ':');
	}
	if (next) {
		*next++ = '\0';
		fontStyleText = next;
	}

	if (fontSizeText) {
		fontSize = SDL_atoi(fontSizeText);
		if (!fontSize) {
			SDL_free(text);
			return false;
		}
	}
	if (fontStyleText) {
		if (SDL_strcasecmp(fontStyleText, "BOLD") == 0) {
			fontStyle = UIFONT_STYLE_BOLD;
		} else if (SDL_strcasecmp(fontStyleText, "UNDERLINE") == 0) {
			fontStyle = UIFONT_STYLE_UNDERLINE;
		} else if (SDL_strcasecmp(fontStyleText, "ITALIC") == 0) {
			fontStyle = UIFONT_STYLE_UNDERLINE;
		} else if (SDL_strcasecmp(fontStyleText, "NORMAL") == 0) {
			fontStyle = UIFONT_STYLE_NORMAL;
		} else {
			SDL_free(text);
			return false;
		}
	}

	SetFont(fontName, fontSize, fontStyle);
	SDL_free(text);

	return true;
}

void
UIElement::SetDrawEngine(UIDrawEngine *drawEngine)
{
	assert(!m_drawEngine);
	m_drawEngine = drawEngine;
	if (m_drawEngine) {
		m_drawEngine->Init(this);
	}
}

void
UIElement::SetBorder(bool enabled)
{
	if (enabled == m_border) {
		return;
	}

	m_border = enabled;

	if (m_drawEngine) {
		m_drawEngine->OnBorderChanged();
	}
}

void
UIElement::SetFill(bool enabled)
{
	if (enabled == m_fill) {
		return;
	}

	m_fill = enabled;

	if (m_drawEngine) {
		m_drawEngine->OnFillChanged();
	}
}

void
UIElement::SetFillColor(Uint8 R, Uint8 G, Uint8 B)
{
	SetFillColor(m_screen->MapRGB(R, G, B));
}

void
UIElement::SetFillColor(Uint32 color)
{
	if (color == m_fillColor) {
		return;
	}

	m_fillColor = color;

	if (m_drawEngine) {
		m_drawEngine->OnFillColorChanged();
	}
}

void
UIElement::SetColor(Uint8 R, Uint8 G, Uint8 B)
{
	SetColor(m_screen->MapRGB(R, G, B));
}

void
UIElement::SetColor(Uint32 color)
{
	if (color == m_color) {
		return;
	}

	m_color = color;

	if (!IsDisabled() && m_drawEngine) {
		m_drawEngine->OnColorChanged();
	}
}

void
UIElement::SetDisabledColor(Uint8 R, Uint8 G, Uint8 B)
{
	SetDisabledColor(m_screen->MapRGB(R, G, B));
}

void
UIElement::SetDisabledColor(Uint32 color)
{
	if (color == m_disabledColor) {
		return;
	}

	m_disabledColor = color;

	if (IsDisabled() && m_drawEngine) {
		m_drawEngine->OnColorChanged();
	}
}

void
UIElement::UpdateDisabledState()
{
	UIBaseElement::UpdateDisabledState();

	if (m_drawEngine) {
		m_drawEngine->OnColorChanged();
	}
}

void
UIElement::SetFont(const char *fontName, int fontSize, UIFontStyle fontStyle)
{
	if (fontName && m_fontName && SDL_strcmp(fontName, m_fontName) == 0 &&
	    fontSize == m_fontSize &&
	    fontStyle == m_fontStyle) {
		return;
	}

	if (m_fontName) {
		SDL_free(m_fontName);
	}
	assert(fontName);
	assert(fontSize > 0);
	m_fontName = SDL_strdup(fontName);
	m_fontSize = fontSize;
	m_fontStyle = fontStyle;

	if (m_drawEngine) {
		m_drawEngine->OnFontChanged();
	}
}

void
UIElement::SetText(const char *text)
{
	if (!text && !m_text) {
		return;
	}
	if (text && m_text && SDL_strcmp(text, m_text) == 0) {
		return;
	}

	if (m_text) {
		SDL_free(m_text);
		m_text = NULL;
	}
	if (text) {
		m_text = SDL_strdup(text);
	}

	if (m_drawEngine) {
		m_drawEngine->OnTextChanged();
	}
}

void
UIElement::SetTextShadowOffset(int x, int y)
{
	m_textShadowOffset.x = x;
	m_textShadowOffset.y = y;
}

bool
UIElement::GetTextShadowOffset(int *x, int *y) const
{
	if (m_textShadowOffset.x || m_textShadowOffset.y) {
		*x = m_textShadowOffset.y;
		*y = m_textShadowOffset.y;
		return true;
	} else {
		*x = 0;
		*y = 0;
		return false;
	}
}

void
UIElement::SetTextShadowColor(Uint8 R, Uint8 G, Uint8 B)
{
	SetTextShadowColor(m_screen->MapRGB(R, G, B));
}

void
UIElement::SetTextShadowColor(Uint32 color)
{
	m_textShadowColor = color;
}

bool
UIElement::SetBackground(const char *name)
{
	UITexture *background;

	background = m_ui->CreateBackground(name);
	if (!background) {
		return false;
	}

	SetBackground(background);

	return true;
}

void
UIElement::SetBackground(UITexture *background)
{
	if (m_background && !m_background->IsLocked()) {
		m_ui->FreeBackground(m_background);
	}
	m_background = background;
}

bool
UIElement::SetImage(const char *name)
{
	UITexture *image;

	image = m_ui->CreateImage(name);
	if (!image) {
		return false;
	}

	SetImage(image);

	return true;
}

void
UIElement::SetImage(UITexture *image)
{
	if (m_image && !m_image->IsLocked()) {
		m_ui->FreeImage(m_image);
	}
	m_image = image;

	if (m_drawEngine) {
		m_drawEngine->OnImageChanged();
	}
}

void
UIElement::Draw(DRAWLEVEL drawLevel)
{
	if (m_drawEngine && drawLevel == GetDrawLevel()) {
		m_drawEngine->OnDraw();
	}

	UIBaseElement::Draw(drawLevel);
}

bool
UIElement::HandleEvent(const SDL_Event &event)
{
	if (m_mouseEnabled) {
		bool checkMouseLocation = false;
		int x, y;

		if (event.type == SDL_MOUSEMOTION) {
			x = event.motion.x;
			y = event.motion.y;
			checkMouseLocation = true;
		}
		if (event.type == SDL_MOUSEBUTTONDOWN ||
		    event.type == SDL_MOUSEBUTTONUP) {
			x = event.button.x;
			y = event.button.y;
			checkMouseLocation = true;
		}

		if (checkMouseLocation) {
			if (ContainsPoint(x, y)) {
				if (!m_mouseInside) {
					m_mouseInside = true;
					OnMouseEnter();
				}
			} else {
				if (m_mouseInside) {
					m_mouseInside = false;
					OnMouseLeave();
				}
			}
		}
		if (event.type == SDL_MOUSEMOTION) {
			return m_mouseInside;
		}

		if (event.type == SDL_MOUSEBUTTONDOWN &&
		    event.button.button == SDL_BUTTON_LEFT && m_mouseInside) {
			m_mousePressed = true;
			OnMouseDown();
			return true;
		}

		if (event.type == SDL_MOUSEBUTTONUP &&
		    event.button.button == SDL_BUTTON_LEFT && m_mousePressed) {
			m_mousePressed = false;
			OnMouseUp();
			if (m_mouseInside) {
				OnClick();
			}
			return true;
		}
	}

	return UIBaseElement::HandleEvent(event);
}

void
UIElement::SetClickCallback(UIClickCallback *callback)
{
	if (m_clickCallback) {
		delete m_clickCallback;
	}

	m_clickCallback = callback;

	if (m_clickCallback) {
		m_mouseEnabled = true;
	}
}

void
UIElement::SetAction(const char *action)
{
	if (m_action) {
		SDL_free(m_action);
	}
	if (action) {
		m_action = SDL_strdup(action);
	} else {
		m_action = NULL;
	}
}

// These can be overridden by inheriting classes
void
UIElement::OnMouseEnter()
{
	if (m_drawEngine) {
		m_drawEngine->OnMouseEnter();
	}
}

void
UIElement::OnMouseLeave()
{
	if (m_drawEngine) {
		m_drawEngine->OnMouseLeave();
	}
}

void
UIElement::OnMouseDown()
{
	if (m_drawEngine) {
		m_drawEngine->OnMouseDown();
	}
	if (m_actionPressed) {
		Action(this, m_actionPressed);
	}
}

void
UIElement::OnMouseUp()
{
	if (m_drawEngine) {
		m_drawEngine->OnMouseUp();
	}
	if (m_actionReleased) {
		Action(this, m_actionReleased);
	}
}

void
UIElement::OnClick()
{
	if (m_action) {
		Action(this, m_action);
	}
	if (m_clickCallback) {
		(*m_clickCallback)();
	}
}
