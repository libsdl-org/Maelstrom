
/* ----------------------------------------------------------------- */
/* -- Service */

#ifndef _SERVICE_MANAGER_H
#define _SERVICE_MANAGER_H

// This file defines the interface to custom platform services

#include "SDL_events.h"

#include "Localization.h"

class ServiceManager
{
public:
    ServiceManager();
    virtual ~ServiceManager() { }

    // Return true if the platform supports this service
    // If the service is temporarily unavailable, this function should return
    // true, and the appropriate service functions should show an info message.
    virtual bool IsAvailable() = 0;

    // Threaded event handling
    // PushEvent() can be called from any thread
    // HandleEvent() gets called on main thread
    bool HandleEvent(const SDL_Event &event);
    void PushEvent(int eventCode, void *data1 = NULL, void *data2 = NULL);
    virtual void HandleEvent(int eventCode, void *data1, void *data2) { }

    // Do any per-frame processing
    virtual void Tick() { }

    //
    // API called by platform specific code
    //

    // Show a message in the UI
    void ShowMessage(const char *message, const char *button1Text = TEXT("Okay"), const char *button1Action = NULL);
    void HideMessage();

protected:
    Uint32 m_serviceEvent;
};

#endif // _SERVICE_MANAGER_H

// vim: ts=4:sw=4:expandtab
