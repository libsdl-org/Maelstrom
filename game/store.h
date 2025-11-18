
/* ----------------------------------------------------------------- */
/* -- Store */

#ifndef _STORE_H
#define _STORE_H

// This file defines the interface to the store functionality

#include "ServiceManager.h"

// Paid features
#define FEATURE_KIDMODE	"Feature.KidMode"
#define FEATURE_NETWORK	"Feature.Network"

class StoreManager : public ServiceManager
{
public:
    static StoreManager *Create();

public:
    StoreManager();
    virtual ~StoreManager() { }

    // Returns true if a feature has been purchased
    virtual bool HasFeature(const char *feature) = 0;

    // Grant a feature as though it had been purchased
    virtual void GrantFeature(const char *feature) = 0;

    // If the feature has already been purchased, no UI should be shown,
    // and the action should be executed immediately.
    //
    // If the feature has not been purchased, UI should be shown allowing
    // the user to purchase the feature.
    //
    // If the feature can not be purchased or the purchase is cancelled,
    // the appropriate UI should be shown and no action should be taken.
    virtual void ActivateFeature(const char *feature,
                                 const char *action_activate,
                                 const char *action_canceled = NULL) = 0;

    // The function called when the purchase dialog returns
    virtual void PurchaseDialogResult(void *context, bool shouldPurchase) = 0;

    //
    // API called by platform specific code
    //

    // Show the feature purchase dialog (Would you like to buy this?)
    void ShowPurchaseDialog(void *context, const char *title, const char *description, const char *price);

    // Show an optional message and perform the cancel action
    // The message should be NULL if the action was canceled by the user,
    // and set to an error message if the purchase failed for some reason.
    void ShowPurchaseCanceled(const char *message, const char *action);

    // Show a thank you note and perform the activate action
    void ShowPurchaseComplete(const char *action);

    // Perform an action once a feature is available
    void PerformAction(const char *action);
};

#endif // _STORE_H

// vim: ts=4:sw=4:expandtab
