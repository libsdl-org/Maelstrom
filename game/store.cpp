
/* ----------------------------------------------------------------- */
/* -- Store */

#include "Maelstrom_Globals.h"
#include "MaelstromUI.h"
#include "store.h"

#ifndef USING_STOREKIT
#define STORE_SIMPLE
#endif


struct PurchaseInfo {
    PurchaseInfo(StoreManager *manager, void *context,
                 const char *title, const char *description, const char *price)
        : manager(manager),
          context(context),
          title(title ? SDL_strdup(title) : NULL),
          description(description ? SDL_strdup(description) : NULL),
          price(price ? SDL_strdup(price) : NULL)
    { }

    ~PurchaseInfo() {
        if (this->title) {
            SDL_free(this->title);
        }
        if (this->description) {
            SDL_free(this->description);
        }
        if (this->price) {
            SDL_free(this->price);
        }
    }

    StoreManager *manager;
    void *context;
    char *title;
    char *description;
    char *price;
};

static void PurchaseDialogInit(void *param, UIDialog *dialog)
{
    PurchaseInfo *info = (PurchaseInfo *)param;

    UIElement *title = dialog->GetElement<UIElement>("title");
    if (title) {
        title->SetText(info->title);
    }

    UIElement *description = dialog->GetElement<UIElement>("description");
    if (description) {
        description->SetText(info->description);
    }

    UIElement *price = dialog->GetElement<UIElement>("price");
    if (price) {
        price->SetText(info->price);
    }
}

static void PurchaseDialogDone(void *param, UIDialog *dialog, int status)
{
    PurchaseInfo *info = (PurchaseInfo *)param;

    bool shouldPurchase = (status > 0);
    info->manager->PurchaseDialogResult(info->context, shouldPurchase);
    delete info;
}


#ifdef STORE_SIMPLE

class StoreManagerSimple : public StoreManager
{
public:
    StoreManagerSimple() : StoreManager() { }

    virtual bool IsAvailable() {
        return false;
    }
    virtual bool HasFeature(const char *feature);
    virtual void GrantFeature(const char *feature);
    virtual void ActivateFeature(const char *feature,
                                 const char *action_activate,
                                 const char *action_canceled);
    virtual void PurchaseDialogResult(void *context, bool shouldPurchase) { }
};

bool
StoreManagerSimple::HasFeature(const char *feature)
{
    return true;
}

void
StoreManagerSimple::GrantFeature(const char *feature)
{
}

void
StoreManagerSimple::ActivateFeature(const char *feature,
                                    const char *action_activate,
                                    const char *action_canceled)
{
    PerformAction(action_activate);
}

StoreManager *
StoreManager::Create()
{
    return new StoreManagerSimple;
}

#endif // STORE_SIMPLE


StoreManager::StoreManager() : ServiceManager()
{
}

void
StoreManager::ShowPurchaseDialog(void *context, const char *title, const char *description, const char *price)
{
    UIDialog *dialog = ui->GetPanel<UIDialog>(DIALOG_FEATURE);
    if (dialog) {
        PurchaseInfo *info = new PurchaseInfo(this, context,
                                              title, description, price);
        dialog->SetDialogInitHandler(PurchaseDialogInit, info);
        dialog->SetDialogDoneHandler(PurchaseDialogDone, info);
        ui->ShowPanel(dialog);
    }
}

void
StoreManager::ShowPurchaseCanceled(const char *message, const char *action)
{
    if (message && *message) {
        ::ShowMessage(message, TEXT("Okay"), action);
    } else {
        PerformAction(action);
    }
}

void
StoreManager::ShowPurchaseComplete(const char *action)
{
    ::ShowMessage(TEXT("Thank you for your purchase!"), TEXT("Okay"), action);
}

void
StoreManager::PerformAction(const char *action)
{
    UIPanel *panel = ui->GetCurrentPanel();
    if (panel && action) {
        panel->Action(NULL, action);
    }
}

// vim: ts=4:sw=4:expandtab
