
#include "screenlib/SDL_FrameBuf.h"
#include "MacDialogButton.h"
#include "MacDialogLabel.h"

/* Default dialog button size */
#define BUTTON_WIDTH	75
#define BUTTON_HEIGHT	19


UIElementType MacDialogButton::s_elementType;


MacDialogButton::MacDialogButton(UIBaseElement *parent, const char *name) :
	UIDialogButton(parent, name)
{
	m_colors[0] = m_screen->MapRGB(0xFF, 0xFF, 0xFF);
	m_colors[1] = m_screen->MapRGB(0x00, 0x00, 0x00);

	SetSize(BUTTON_WIDTH, BUTTON_HEIGHT);
}

UIElementLabel *
MacDialogButton::CreateLabel()
{
	return new MacDialogLabel(this, "label");
}

void
MacDialogButton::Draw()
{
	Uint32 bg, fg;
	int x, y, maxx, maxy;

	/* The colors are inverted when the mouse is pressed */
	bg = m_colors[m_mousePressed];
	fg = m_colors[!m_mousePressed];

	/* First draw the background */
	m_screen->FillRect(X(), Y(), Width(), Height(), bg);

	/* Draw the beveled edge */
	x = X();
	maxx = x+Width()-1;
	y = Y();
	maxy = y+Height()-1;

	/* Top and upper corners */
	m_screen->DrawLine(x+3, y, maxx-3, y, fg);
	m_screen->DrawLine(x+1, y+1, x+2, y+1, fg);
	m_screen->DrawLine(maxx-2, y+1, maxx-1, y+1, fg);
	m_screen->DrawLine(x+1, y+2, x+1, y+2, fg);
	m_screen->DrawLine(maxx-1, y+2, maxx-1, y+2, fg);

	/* Sides */
	m_screen->DrawLine(x, y+3, x, maxy-3, fg);
	m_screen->DrawLine(maxx, y+3, maxx, maxy-3, fg);

	/* Bottom and lower corners */
	m_screen->DrawLine(x+1, maxy-2, x+1, maxy-2, fg);
	m_screen->DrawLine(maxx-1, maxy-2, maxx-1, maxy-2, fg);
	m_screen->DrawLine(x+1, maxy-1, x+2, maxy-1, fg);
	m_screen->DrawLine(maxx-2, maxy-1, maxx-1, maxy-1, fg);
	m_screen->DrawLine(x+3, maxy, maxx-3, maxy, fg);

	if (m_default) {
		/* Show the thick edge */
		x = X()-4;
		maxx = x+4+Width()+4-1;
		y = Y()-4;
		maxy = y+4+Height()+4-1;

		/* The edge always uses the real foreground color */
		fg = m_colors[1];

		m_screen->DrawLine(x+5, y, maxx-5, y, fg);
		m_screen->DrawLine(x+3, y+1, maxx-3, y+1, fg);
		m_screen->DrawLine(x+2, y+2, maxx-2, y+2, fg);
		m_screen->DrawLine(x+1, y+3, x+5, y+3, fg);
		m_screen->DrawLine(maxx-5, y+3, maxx-1, y+3, fg);
		m_screen->DrawLine(x+1, y+4, x+3, y+4, fg);
		m_screen->DrawLine(maxx-3, y+4, maxx-1, y+4, fg);
		m_screen->DrawLine(x, y+5, x+3, y+5, fg);
		m_screen->DrawLine(maxx-3, y+5, maxx, y+5, fg);

		m_screen->DrawLine(x, y+6, x, maxy-6, fg);
		m_screen->DrawLine(maxx, y+6, maxx, maxy-6, fg);
		m_screen->DrawLine(x+1, y+6, x+1, maxy-6, fg);
		m_screen->DrawLine(maxx-1, y+6, maxx-1, maxy-6, fg);
		m_screen->DrawLine(x+2, y+6, x+2, maxy-6, fg);
		m_screen->DrawLine(maxx-2, y+6, maxx-2, maxy-6, fg);

		m_screen->DrawLine(x, maxy-5, x+3, maxy-5, fg);
		m_screen->DrawLine(maxx-3, maxy-5, maxx, maxy-5, fg);
		m_screen->DrawLine(x+1, maxy-4, x+3, maxy-4, fg);
		m_screen->DrawLine(maxx-3, maxy-4, maxx-1, maxy-4, fg);
		m_screen->DrawLine(x+1, maxy-3, x+5, maxy-3, fg);
		m_screen->DrawLine(maxx-5, maxy-3, maxx-1, maxy-3, fg);
		m_screen->DrawLine(x+2, maxy-2, maxx-2, maxy-2, fg);
		m_screen->DrawLine(x+3, maxy-1, maxx-3, maxy-1, fg);
		m_screen->DrawLine(x+5, maxy, maxx-5, maxy, fg);
	}

	UIDialogButton::Draw();
}

void
MacDialogButton::OnMouseDown()
{
	SetElementColor(m_colors[0]);

	UIDialogButton::OnMouseDown();
}

void
MacDialogButton::OnMouseUp()
{
	SetElementColor(m_colors[1]);

	UIDialogButton::OnMouseUp();
}

void
MacDialogButton::SetElementColor(Uint32 color)
{
	Uint8 R, G, B;

	m_screen->GetRGB(color, &R, &G, &B);
	for (unsigned i = 0; i < m_elements.length(); ++i) {
		if (m_elements[i]->IsA(UIElementLabel::GetType())) {
			static_cast<UIElementLabel*>(m_elements[i])->SetTextColor(R, G, B);
		}
	}
}