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

#ifndef _UIElement_h
#define _UIElement_h

#include "UIBaseElement.h"
#include "UIDrawEngine.h"
#include "UIFontInterface.h"

class UITexture;

class UIClickCallback
{
public:
	virtual ~UIClickCallback() { }

	virtual void operator()() = 0;
};

template <class C>
class UIObjectClickCallback : public UIClickCallback
{
public:
	UIObjectClickCallback(C *obj, void (C::*callback)(void*), void *param) : UIClickCallback() {
		m_obj = obj;
		m_callback = callback;
		m_param = param;
	}

	virtual void operator()() {
		(m_obj->*m_callback)(m_param);
	}

protected:
	C *m_obj;
	void (C::*m_callback)(void*);
	void *m_param;
};

class UIFunctionClickCallback : public UIClickCallback
{
public:
	UIFunctionClickCallback(void (*callback)(void*), void *param) : UIClickCallback() {
		m_callback = callback;
		m_param = param;
	}

	virtual void operator()() {
		(*m_callback)(m_param);
	}

protected:
	void (*m_callback)(void*);
	void *m_param;
};

// This is the basic thing you see on the screen.
// It consists of an area, a parent, children, text, an image, and a
// drawing engine which is notified of state changes.


class UIElement : public UIBaseElement
{
DECLARE_TYPESAFE_CLASS(UIBaseElement)
public:
	UIElement(UIBaseElement *parent, const char *name, UIDrawEngine *drawEngine);
	virtual ~UIElement();

	override bool Load(rapidxml::xml_node<> *node, const UITemplates *templates);
	override bool FinishLoading();

	// Bind any preferences variables to the preferences manager
	override void LoadData(Prefs *prefs);
	override void SaveData(Prefs *prefs);

	override void OnRectChanged() {
		UIBaseElement::OnRectChanged();

		m_imageArea.AutoSize(Width(), Height(), true);
	}

	// Set the draw engine for this element
	// This should be called before Load() so the draw engine can load too.
	// Once set, the element owns the draw engine and will free it.
	void SetDrawEngine(UIDrawEngine *drawEngine);

	// Border and fill information
	void SetBorder(bool enabled);
	bool HasBorder() const {
		return m_border;
	}
	void SetFill(bool enabled);
	bool HasFill() const {
		return m_fill;
	}
	void SetFillColor(Uint8 R, Uint8 G, Uint8 B);
	void SetFillColor(Uint32 color);
	Uint32 GetFillColor() const {
		return m_fillColor;
	}

	// Color information
	void SetColor(Uint8 R, Uint8 G, Uint8 B);
	void SetColor(Uint32 color);
	Uint32 GetColor() const {
		return m_color;
	}
	void SetDisabledColor(Uint8 R, Uint8 G, Uint8 B);
	void SetDisabledColor(Uint32 color);
	Uint32 GetDisabledColor() const {
		return m_disabledColor;
	}
	Uint32 GetCurrentColor() const {
		return IsDisabled() ? GetDisabledColor() : GetColor();
	}

	// Text information
	void SetFont(const char *fontName, int fontSize, UIFontStyle fontStyle);
	bool HasFont() const {
		return m_fontName != NULL;
	}
	const char *GetFontName() const {
		return m_fontName;
	}
	int GetFontSize() const {
		return m_fontSize;
	}
	UIFontStyle GetFontStyle() const {
		return m_fontStyle;
	}

	virtual void SetText(const char *text);
	const char *GetText() const {
		return m_text;
	}
	UIArea *GetTextArea() {
		return &m_textArea;
	}
	void GetTextOffset(int *x, int *y) const {
		*x = m_textOffset.x;
		*y = m_textOffset.y;
	}
	void SetTextShadowOffset(int x, int y);
	bool GetTextShadowOffset(int *x, int *y) const;
	void SetTextShadowColor(Uint8 R, Uint8 G, Uint8 B);
	void SetTextShadowColor(Uint32 color);
	Uint32 GetTextShadowColor() const {
		return m_textShadowColor;
	}

	// Background
	bool SetBackground(const char *name);
	void SetBackground(UITexture *background);
	UITexture *GetBackground() const {
		return m_background;
	}

	// Image information
	bool SetImage(const char *name);
	void SetImage(UITexture *image);
	UITexture *GetImage() const {
		return m_image;
	}
	UIArea *GetImageArea() {
		return &m_imageArea;
	}

	// Draw!
	override void Draw(DRAWLEVEL drawLevel);

	// Events
	override bool HandleEvent(const SDL_Event &event);

	// Once set, the element owns the click callback and will free it.
	template <class C>
	void SetClickCallback(C *obj, void (C::*callback)(void*), void *param = 0) {
		SetClickCallback(new UIObjectClickCallback<C>(obj, callback, param));
	}
	void SetClickCallback(void (*callback)(void*), void *param = 0) {
		SetClickCallback(new UIFunctionClickCallback(callback, param));
	}
	void SetClickCallback(UIClickCallback *callback);
	void SetAction(const char *action);

	// These can be overridden by inheriting classes
	virtual void OnMouseEnter();
	virtual void OnMouseLeave();
	virtual void OnMouseDown();
	virtual void OnMouseUp();
	virtual void OnClick();

protected:
	UIDrawEngine *m_drawEngine;
	bool m_border;
	bool m_fill;
	Uint32 m_fillColor;
	Uint32 m_color;
	Uint32 m_disabledColor;
	char *m_fontName;
	int m_fontSize;
	UIFontStyle m_fontStyle;
	char *m_text;
	char *m_textBinding;
	UIArea m_textArea;
	SDL_Point m_textOffset;
	SDL_Point m_textShadowOffset;
	Uint32 m_textShadowColor;
	UITexture *m_background;
	UITexture *m_image;
	UIArea m_imageArea;
	bool m_mouseEnabled;
	bool m_mouseInside;
	bool m_mousePressed;
	UIClickCallback *m_clickCallback;
	char *m_action;
	char *m_actionPressed;
	char *m_actionReleased;

protected:
	bool LoadColor(rapidxml::xml_node<> *node, const char *name, Uint32 &value);
	bool ParseFont(char *text);

	override void UpdateDisabledState();
};

#endif // _UIElement_h
