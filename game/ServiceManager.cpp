
/* ----------------------------------------------------------------- */
/* -- ServiceManager */

#include "MaelstromUI.h"
#include "ServiceManager.h"


ServiceManager::ServiceManager()
{
    m_serviceEvent = SDL_RegisterEvents(1);
}

bool
ServiceManager::HandleEvent(const SDL_Event &event)
{
    if (event.type == m_serviceEvent) {
        HandleEvent(event.user.code, event.user.data1, event.user.data2);
        return true;
    }
    return false;
}

void
ServiceManager::PushEvent(int eventCode, void *data1, void *data2)
{
    SDL_Event event;

    event.type = m_serviceEvent;
    event.user.code = eventCode;
    event.user.data1 = data1;
    event.user.data2 = data2;
    SDL_PushEvent(&event);
}

void
ServiceManager::ShowMessage(const char *message, const char *button1Text, const char *button1Action)
{
    ::ShowMessage(message, button1Text, button1Action);
}

void
ServiceManager::HideMessage()
{
    ::HideMessage();
}

// vim: ts=4:sw=4:expandtab
