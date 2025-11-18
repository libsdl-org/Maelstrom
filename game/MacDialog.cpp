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

#include "../screenlib/SDL_FrameBuf.h"
#include "../screenlib/UIDialogButton.h"
#include "../screenlib/UIElementCheckbox.h"
#include "../screenlib/UIElementEditbox.h"
#include "../screenlib/UIElementRadio.h"

#include "Maelstrom_Globals.h"
#include "MacDialog.h"

#define CLASSIC_DIALOG_FONT	"Chicago"
#define CLASSIC_DIALOG_FONT_SIZE 12
#define MODERN_DIALOG_FONT	"FreeSansBold"
#define MODERN_DIALOG_FONT_SIZE 12

#define EXPAND_STEPS	30

#define BUTTON_WIDTH	75
#define BUTTON_HEIGHT	19

#define CHECKBOX_SIZE	12

#define RADIOBUTTON_SIZE	20

UIElementType MacDialog::s_elementType;


MacDialog::MacDialog(UIManager *ui, const char *name) :
	UIDialog(ui, name)
{
	m_colors[COLOR_BLACK] = m_screen->MapRGB(0x00, 0x00, 0x00);
	m_colors[COLOR_DARK] = m_screen->MapRGB(0x66, 0x66, 0x99);
	m_colors[COLOR_MEDIUM] = m_screen->MapRGB(0xBB, 0xBB, 0xBB);
	m_colors[COLOR_LIGHT] = m_screen->MapRGB(0xCC, 0xCC, 0xFF);
	m_colors[COLOR_WHITE] = m_screen->MapRGB(0xFF, 0xFF, 0xFF);
#ifdef FAST_ITERATION
	m_expand = false;
#else
	m_expand = true;
#endif
	m_step = 0;
}

bool
MacDialog::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	if (!UIDialog::Load(node, templates)) {
		return false;
	}

	LoadBool(node, "expand", m_expand);

	return true;
}

void
MacDialog::Show()
{
	if (m_expand) {
		m_step = 0;
	} else {
		m_step = EXPAND_STEPS;
	}

	UIDialog::Show();
}

void
MacDialog::Draw(DRAWLEVEL drawLevel)
{
	if (drawLevel == DRAWLEVEL_BACKGROUND) {
		int x, y, w, h;
		int maxx, maxy;

		if (m_step < EXPAND_STEPS) {
			w = (Width()*m_step)/EXPAND_STEPS;
			h = (Height()*m_step)/EXPAND_STEPS;
			x = X() + Width()/2 - (w/2);
			y = Y() + Height()/2 - (h/2);
		} else {
			w = Width();
			h = Height();
			x = X();
			y = Y();
		}

		/* The border is 4 pixels around the area of the dialog */
		w += 8;
		h += 8;
		x -= 4;
		y -= 4;
		maxx = x+w-1;
		maxy = y+h-1;

		/* Draw the dialog border and background color */
		m_screen->DrawLine(x, y, maxx, y, m_colors[COLOR_LIGHT]);
		m_screen->DrawLine(x, y, x, maxy, m_colors[COLOR_LIGHT]);
		m_screen->DrawLine(x, maxy, maxx, maxy, m_colors[COLOR_DARK]);
		m_screen->DrawLine(maxx, y, maxx, maxy, m_colors[COLOR_DARK]);
		m_screen->DrawRect(x+1, y+1, w-2, h-2, m_colors[COLOR_MEDIUM]);
		m_screen->DrawLine(x+2, y+2, maxx-2, y+2, m_colors[COLOR_DARK]);
		m_screen->DrawLine(x+2, y+2, x+2, maxy-2, m_colors[COLOR_DARK]);
		m_screen->DrawLine(x+3, maxy-2, maxx-2, maxy-2, m_colors[COLOR_LIGHT]);
		m_screen->DrawLine(maxx-2, y+3, maxx-2, maxy-2, m_colors[COLOR_LIGHT]);
		m_screen->DrawRect(x+3, y+3, w-6, h-6, m_colors[COLOR_BLACK]);
		m_screen->FillRect(x+4, y+4, w-8, h-8, m_colors[COLOR_WHITE]);
	}

	/* Don't draw the controls until we've finished expanding */
	if (m_step < EXPAND_STEPS) {
		if (drawLevel == NUM_DRAWLEVELS-1) {
			++m_step;
		}
		return;
	}

	UIDialog::Draw(drawLevel);
}

//////////////////////////////////////////////////////////////////////////////

void
MacDialogDrawEngine::Init(UIElement *element)
{
	UIDrawEngine::Init(element);

	// Set the default colors and font for dialog elements
	m_element->SetFillColor(0xFF, 0xFF, 0xFF);
	m_element->SetColor(0x00, 0x00, 0x00);
	if (gClassic) {
		m_element->SetFont(CLASSIC_DIALOG_FONT, CLASSIC_DIALOG_FONT_SIZE, UIFONT_STYLE_NORMAL);
	} else {
		m_element->SetFont(MODERN_DIALOG_FONT, MODERN_DIALOG_FONT_SIZE, UIFONT_STYLE_NORMAL);
	}
}

//////////////////////////////////////////////////////////////////////////////

void
MacDialogButton::Init(UIElement *element)
{
	MacDialogDrawEngine::Init(element);

	m_element->SetFill(true);
	m_element->SetSize(BUTTON_WIDTH, BUTTON_HEIGHT);
}

void
MacDialogButton::OnLoad()
{
	MacDialogDrawEngine::OnLoad();

	m_colors[0] = m_element->GetFillColor();
	m_colors[1] = m_element->GetColor();
}

void
MacDialogButton::OnDraw()
{
	Uint32 color;
	int x, y, maxx, maxy;

	// Do the normal drawing
	MacDialogDrawEngine::OnDraw();

	color = m_element->GetCurrentColor();

	// Draw the beveled edge
	x = m_element->X();
	maxx = x+m_element->Width()-1;
	y = m_element->Y();
	maxy = y+m_element->Height()-1;

	// Top and upper corners
	m_screen->DrawLine(x+3, y, maxx-3, y, color);
	m_screen->DrawLine(x+1, y+1, x+2, y+1, color);
	m_screen->DrawLine(maxx-2, y+1, maxx-1, y+1, color);
	m_screen->DrawLine(x+1, y+2, x+1, y+2, color);
	m_screen->DrawLine(maxx-1, y+2, maxx-1, y+2, color);

	// Sides
	m_screen->DrawLine(x, y+3, x, maxy-3, color);
	m_screen->DrawLine(maxx, y+3, maxx, maxy-3, color);

	// Bottom and lower corners
	m_screen->DrawLine(x+1, maxy-2, x+1, maxy-2, color);
	m_screen->DrawLine(maxx-1, maxy-2, maxx-1, maxy-2, color);
	m_screen->DrawLine(x+1, maxy-1, x+2, maxy-1, color);
	m_screen->DrawLine(maxx-2, maxy-1, maxx-1, maxy-1, color);
	m_screen->DrawLine(x+3, maxy, maxx-3, maxy, color);

	if (m_element->IsA(UIDialogButton::GetType()) &&
	    static_cast<UIDialogButton*>(m_element)->IsDefault()) {
		// Show the thick edge
		x = m_element->X()-4;
		maxx = x+4+m_element->Width()+4-1;
		y = m_element->Y()-4;
		maxy = y+4+m_element->Height()+4-1;

		// The edge always uses the real foreground color
		if (m_element->IsDisabled()) {
			color = m_element->GetDisabledColor();
		} else {
			color = m_colors[1];
		}

		m_screen->DrawLine(x+5, y, maxx-5, y, color);
		m_screen->DrawLine(x+3, y+1, maxx-3, y+1, color);
		m_screen->DrawLine(x+2, y+2, maxx-2, y+2, color);
		m_screen->DrawLine(x+1, y+3, x+5, y+3, color);
		m_screen->DrawLine(maxx-5, y+3, maxx-1, y+3, color);
		m_screen->DrawLine(x+1, y+4, x+3, y+4, color);
		m_screen->DrawLine(maxx-3, y+4, maxx-1, y+4, color);
		m_screen->DrawLine(x, y+5, x+3, y+5, color);
		m_screen->DrawLine(maxx-3, y+5, maxx, y+5, color);

		m_screen->DrawLine(x, y+6, x, maxy-6, color);
		m_screen->DrawLine(maxx, y+6, maxx, maxy-6, color);
		m_screen->DrawLine(x+1, y+6, x+1, maxy-6, color);
		m_screen->DrawLine(maxx-1, y+6, maxx-1, maxy-6, color);
		m_screen->DrawLine(x+2, y+6, x+2, maxy-6, color);
		m_screen->DrawLine(maxx-2, y+6, maxx-2, maxy-6, color);

		m_screen->DrawLine(x, maxy-5, x+3, maxy-5, color);
		m_screen->DrawLine(maxx-3, maxy-5, maxx, maxy-5, color);
		m_screen->DrawLine(x+1, maxy-4, x+3, maxy-4, color);
		m_screen->DrawLine(maxx-3, maxy-4, maxx-1, maxy-4, color);
		m_screen->DrawLine(x+1, maxy-3, x+5, maxy-3, color);
		m_screen->DrawLine(maxx-5, maxy-3, maxx-1, maxy-3, color);
		m_screen->DrawLine(x+2, maxy-2, maxx-2, maxy-2, color);
		m_screen->DrawLine(x+3, maxy-1, maxx-3, maxy-1, color);
		m_screen->DrawLine(x+5, maxy, maxx-5, maxy, color);
	}
}

void
MacDialogButton::OnMouseDown()
{
	MacDialogDrawEngine::OnMouseDown();

	// Invert the colors
	m_element->SetFillColor(m_colors[1]);
	m_element->SetColor(m_colors[0]);
}

void
MacDialogButton::OnMouseUp()
{
	MacDialogDrawEngine::OnMouseUp();

	// Restore the colors
	m_element->SetFillColor(m_colors[0]);
	m_element->SetColor(m_colors[1]);
}

//////////////////////////////////////////////////////////////////////////////

void
MacDialogCheckbox::Init(UIElement *element)
{
	UIArea *area;

	MacDialogDrawEngine::Init(element);

	m_element->SetSize(CHECKBOX_SIZE, CHECKBOX_SIZE);

	area = m_element->GetTextArea();
	area->SetAnchor(TOPLEFT, TOPLEFT, m_element, CHECKBOX_SIZE+3, -2);

	area = m_element->GetImageArea();
	area->SetAnchor(LEFT, LEFT, m_element, CHECKBOX_SIZE+3, 0);
}

void
MacDialogCheckbox::OnDraw()
{
	Uint32 color;
	int x, y;

	MacDialogDrawEngine::OnDraw();

	color = m_element->GetCurrentColor();
	x = m_element->X();
	y = m_element->Y() + (m_element->Height() - CHECKBOX_SIZE)/2;

	m_screen->DrawRect(x, y, CHECKBOX_SIZE, CHECKBOX_SIZE, color);

	if (m_element->IsA(UIElementCheckbox::GetType()) &&
	    static_cast<UIElementCheckbox*>(m_element)->IsChecked()) {
		m_screen->DrawLine(x, y, x+CHECKBOX_SIZE-1,
					y+CHECKBOX_SIZE-1, color);
		m_screen->DrawLine(x, y+CHECKBOX_SIZE-1,
					x+CHECKBOX_SIZE-1, y, color);
	}
}

//////////////////////////////////////////////////////////////////////////////

void
MacDialogRadioButton::Init(UIElement *element)
{
	UIArea *area;

	MacDialogDrawEngine::Init(element);

	m_element->SetSize(RADIOBUTTON_SIZE, RADIOBUTTON_SIZE);

	area = m_element->GetTextArea();
	area->SetAnchor(TOPLEFT, TOPLEFT, m_element, RADIOBUTTON_SIZE+1, 3);

	area = m_element->GetImageArea();
	area->SetAnchor(LEFT, LEFT, m_element, RADIOBUTTON_SIZE+1, 0);
}

void
MacDialogRadioButton::OnDraw()
{
	Uint32 color;
	int x, y;

	MacDialogDrawEngine::OnDraw();

	color = m_element->GetCurrentColor();
	x = m_element->X() + 5;
	y = m_element->Y() + 5 + (m_element->Height() - RADIOBUTTON_SIZE)/2;

	/* Draw the circle */
	m_screen->DrawLine(x+4, y, x+7, y, color);
	m_screen->DrawLine(x+2, y+1, x+3, y+1, color);
	m_screen->DrawLine(x+8, y+1, x+9, y+1, color);
	m_screen->DrawLine(x+1, y+2, x+1, y+3, color);
	m_screen->DrawLine(x+10, y+2, x+10, y+3, color);
	m_screen->DrawLine(x, y+4, x, y+7, color);
	m_screen->DrawLine(x+11, y+4, x+11, y+7, color);
	m_screen->DrawLine(x+1, y+8, x+1, y+9, color);
	m_screen->DrawLine(x+10, y+8, x+10, y+9, color);
	m_screen->DrawLine(x+2, y+10, x+3, y+10, color);
	m_screen->DrawLine(x+8, y+10, x+9, y+10, color);
	m_screen->DrawLine(x+4, y+11, x+7, y+11, color);

	if (m_element->IsA(UIElementRadioButton::GetType()) &&
	    static_cast<UIElementRadioButton*>(m_element)->IsChecked()) {
		/* Draw the spot in the center */
		x += 3;
		y += 3;

		m_screen->DrawLine(x+1, y, x+4, y, color);
		++y;
		m_screen->DrawLine(x, y, x+5, y, color);
		++y;
		m_screen->DrawLine(x, y, x+5, y, color);
		++y;
		m_screen->DrawLine(x, y, x+5, y, color);
		++y;
		m_screen->DrawLine(x, y, x+5, y, color);
		++y;
		m_screen->DrawLine(x+1, y, x+4, y, color);
	}
}

//////////////////////////////////////////////////////////////////////////////

void
MacDialogEditbox::Init(UIElement *element)
{
	MacDialogDrawEngine::Init(element);

	UIArea *area = m_element->GetTextArea();
	area->SetAnchor(LEFT, LEFT, m_element, 3, 0);
}

void
MacDialogEditbox::OnLoad()
{
	MacDialogDrawEngine::OnLoad();

	m_colors[0] = m_element->GetFillColor();
	m_colors[1] = m_element->GetColor();
}

void
MacDialogEditbox::OnDraw()
{
	bool highlight = false;
	bool hasfocus = false;

	if (m_element->IsA(UIElementEditbox::GetType())) {
		UIElementEditbox *editbox;

		editbox = static_cast<UIElementEditbox*>(m_element);
		highlight = editbox->IsHighlighted();
		hasfocus = editbox->HasFocus();
	}

	/* The colors are inverted when the editbox is highlighted */
	m_element->SetFillColor(m_colors[highlight]);
	m_element->SetColor(m_colors[!highlight]);

	// Draw the outline, always in the real foreground color
	m_screen->DrawRect(m_element->X(), m_element->Y(), m_element->Width(), m_element->Height(), m_colors[1]);

	if (highlight) {
		// Draw the highlight
		m_screen->FillRect(m_element->X()+3, m_element->Y()+3, m_element->Width()-6, m_element->Height()-6, m_element->GetFillColor());
	}

	MacDialogDrawEngine::OnDraw();

	if (hasfocus && !highlight) {
		// Draw the cursor
		int x = m_element->GetTextArea()->X() + m_element->GetTextArea()->Width();

		m_screen->DrawLine(x, m_element->Y()+3, x, m_element->Y()+3+m_element->Height()-6-1, m_element->GetCurrentColor());
	}
}

//////////////////////////////////////////////////////////////////////////////
