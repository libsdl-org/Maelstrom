
#include "SDL_FrameBuf.h"
#include "UIElementRect.h"

UIElementType UIElementRect::s_elementType;


UIElementRect::UIElementRect(UIBaseElement *parent, const char *name) :
	UIElement(parent, name)
{
	m_fill = false;
	m_color = m_screen->MapRGB(0xFF, 0xFF, 0xFF);
}

bool
UIElementRect::Load(rapidxml::xml_node<> *node, const UITemplates *templates)
{
	rapidxml::xml_node<> *child;
	rapidxml::xml_attribute<> *attr;

	if (!UIElement::Load(node, templates)) {
		return false;
	}

	attr = node->first_attribute("fill", 0, false);
	if (attr) {
		const char *value = attr->value();

		if (*value == '1' || *value == 't' || *value == 'T') {
			m_fill = true;
		}
	}

	child = node->first_node("color", 0, false);
	if (child) {
		m_color = LoadColor(child);
	}

	return true;
}

void
UIElementRect::Draw()
{
	if (m_fill) {
		m_screen->FillRect(X(), Y(), Width(), Height(), m_color);
	} else {
		m_screen->DrawRect(X(), Y(), Width(), Height(), m_color);
	}
}
