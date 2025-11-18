
#include "../game/store.h"
#include "../utils/array.h"
#include "../utils/prefs.h"

#import <StoreKit/StoreKit.h>


enum StoreEvent
{
    STORE_EVENT_SHOW_PURCHASE_DIALOG,
    STORE_EVENT_PRODUCT_QUERY_FAILED,
    STORE_EVENT_UPDATED_TRANSACTIONS,
    NUM_STORE_EVENTS
};

extern Prefs *prefs;

//
// The StoreManagerStoreKit class implements the game side interface and
// the StoreManagerDelegate handles the Objective C interaction with Store Kit.
// Each class has a reference to the other so they can call back and forth.
//
class StoreManagerStoreKit;

@interface StoreManagerDelegate : NSObject <SKProductsRequestDelegate,
SKPaymentTransactionObserver> {
@private
    StoreManagerStoreKit *m_manager;
}

- (id)initWithStoreManager:(StoreManagerStoreKit *)manager;
- (void)cleanup;

// StoreManager interfaces
- (void)promptPurchase:(NSString *)productIdentifier;
- (void)completePurchase:(SKProduct *)product;
- (void)handleEvent:(int)eventCode withData:(void*)data1 andData:(void*)data2;

- (void)completeTransaction:(SKPaymentTransaction *)transaction;
- (void)restoreTransaction:(SKPaymentTransaction *)transaction;
- (void)failedTransaction:(SKPaymentTransaction *)transaction;
- (void)recordTransaction:(SKPaymentTransaction *)transaction;

- (NSString *)getProductPriceString:(SKProduct *)product;

@end // StoreManagerDelegate


static struct {
    const char *feature;
    const char *product;
} SupportedFeatures[] = {
    { FEATURE_KIDMODE, "maelstrom_kid_mode" },
    { FEATURE_NETWORK, "maelstrom_network_play" },
};

class StoreManagerStoreKit : public StoreManager
{
public:
    StoreManagerStoreKit();
    virtual ~StoreManagerStoreKit();
    
    virtual bool IsAvailable() { return true; }
    virtual bool HasFeature(const char *feature);
    virtual void GrantFeature(const char *feature);
    virtual void ActivateFeature(const char *feature,
                                 const char *action_activate,
                                 const char *action_canceled);
    
    virtual void PurchaseDialogResult(void *context, bool shouldPurchase);
    virtual void HandleEvent(int eventCode, void *data1, void *data2);
    
    // Called by the Store Kit if a purchase was canceled
    void ProductsCanceled(const char *message);
    void ProductCanceled(NSString *productIdentifier, const char *message);
    
    // Called by the Store Kit delegate when a product is made available
    void ProductPurchased(NSString *productIdentifier);
    
protected:
    NSString *GetProductIDForFeature(const char *feature);
    const char *GetFeatureForProductID(NSString *productID);
    const char *GetPreferencesKeyForFeature(const char *feature, char *key, size_t maxlen);
    
protected:
    StoreManagerDelegate *m_delegate;
    
    struct PendingRequest {
        PendingRequest(NSString *productID,
                       const char *action_activate,
                       const char *action_canceled) {
            this->productID = [productID retain];
            if (action_activate) {
                this->action_activate = SDL_strdup(action_activate);
            } else {
                this->action_activate = NULL;
            }
            if (action_canceled) {
                this->action_canceled = SDL_strdup(action_canceled);
            } else {
                this->action_canceled = NULL;
            }
        }
        ~PendingRequest() {
            [this->productID release];
            if (this->action_activate) {
                SDL_free(this->action_activate);
            }
            if (this->action_canceled) {
                SDL_free(this->action_canceled);
            }
        }
        
        NSString *productID;
        char *action_activate;
        char *action_canceled;
    };
    array<PendingRequest*> m_requests;
};

StoreManager *
StoreManager::Create()
{
    return new StoreManagerStoreKit;
}

StoreManagerStoreKit::StoreManagerStoreKit() : StoreManager()
{
    m_delegate = [[StoreManagerDelegate alloc] initWithStoreManager:this];
}

StoreManagerStoreKit::~StoreManagerStoreKit()
{
    [m_delegate cleanup];
    [m_delegate release];
}

bool
StoreManagerStoreKit::HasFeature(const char *feature)
{
    char key[BUFSIZ];
    return prefs->GetBool(GetPreferencesKeyForFeature(feature, key, sizeof(key)));
}

void
StoreManagerStoreKit::GrantFeature(const char *feature)
{
    char key[BUFSIZ];
    prefs->SetBool(GetPreferencesKeyForFeature(feature, key, sizeof(key)), true);
    prefs->Save();
}

void
StoreManagerStoreKit::ActivateFeature(const char *feature,
                                      const char *action_activate,
                                      const char *action_canceled)
{
    if (HasFeature(feature)) {
        StoreManager::PerformAction(action_activate);
        return;
    }
    
    NSString *productIdentifier = GetProductIDForFeature(feature);
    if (productIdentifier == nil) {
        char message[BUFSIZ];
        SDL_snprintf(message, sizeof(message), TEXT("Store requested unknown feature: %s"), feature);
        ShowMessage(message);
        PerformAction(action_canceled);
        return;
    }
    
    for (int i = 0; i < m_requests.length(); ++i) {
        if ([m_requests[i]->productID isEqualToString:productIdentifier] &&
            SDL_strcmp(action_activate, m_requests[i]->action_activate) == 0) {
            // We're on it!
            return;
        }
    }
    
    ShowMessage(TEXT("Connecting to App Store..."), NULL);
    
    m_requests.add(new PendingRequest(productIdentifier, action_activate, action_canceled));
    [m_delegate promptPurchase:productIdentifier];
}

void
StoreManagerStoreKit::PurchaseDialogResult(void *context, bool shouldPurchase)
{
    SKProduct *product = (SKProduct *)context;
    if (shouldPurchase) {
        [m_delegate completePurchase:product];
    } else {
        ProductCanceled(product.productIdentifier, NULL);
    }
    [product release];
}

void
StoreManagerStoreKit::HandleEvent(int eventCode, void *data1, void *data2)
{
    [m_delegate handleEvent:eventCode withData:data1 andData:data2];
}

void
StoreManagerStoreKit::ProductsCanceled(const char *message)
{
    while (m_requests.length() > 0) {
        PendingRequest *request = m_requests[0];
        ProductCanceled(request->productID, message);
    }
}

void
StoreManagerStoreKit::ProductCanceled(NSString *productIdentifier, const char *message)
{
    for (int i = 0; i < m_requests.length();) {
        if ([m_requests[i]->productID isEqualToString:productIdentifier]) {
            PendingRequest *request = m_requests[i];
            m_requests.remove(request);
            if (m_requests.length() == 0) {
                HideMessage();
            }
            ShowPurchaseCanceled(message, request->action_canceled);
            delete request;
        } else {
            ++i;
        }
    }
}

void
StoreManagerStoreKit::ProductPurchased(NSString *productIdentifier)
{
    const char *feature = GetFeatureForProductID(productIdentifier);
    if (!feature) {
        NSLog(@"Error: Couldn't find feature for product %@", productIdentifier);
        return;
    }
    
    // Note that we now own this feature and save immediately!
    GrantFeature(feature);
    
    for (int i = 0; i < m_requests.length();) {
        if ([m_requests[i]->productID isEqualToString:productIdentifier]) {
            PendingRequest *request = m_requests[i];
            m_requests.remove(request);
            if (m_requests.length() == 0) {
                HideMessage();
            }
            ShowPurchaseComplete(request->action_activate);
            delete request;
        } else {
            ++i;
        }
    }
}

static NSString *GetProductIdentifier(const char *product)
{
    return [NSString stringWithUTF8String:product];
}

NSString *
StoreManagerStoreKit::GetProductIDForFeature(const char *feature)
{
    for (int i = 0; i < SDL_arraysize(SupportedFeatures); ++i) {
        if (SDL_strcasecmp(feature, SupportedFeatures[i].feature) == 0) {
            return GetProductIdentifier(SupportedFeatures[i].product);
        }
    }
    return nil;
}

const char *
StoreManagerStoreKit::GetFeatureForProductID(NSString *productID)
{
    for (int i = 0; i < SDL_arraysize(SupportedFeatures); ++i) {
        if ([productID isEqualToString:GetProductIdentifier(SupportedFeatures[i].product)]) {
            return SupportedFeatures[i].feature;
        }
    }
    return nil;
}

const char *
StoreManagerStoreKit::GetPreferencesKeyForFeature(const char *feature, char *key, size_t maxlen)
{
    SDL_snprintf(key, maxlen, "Feature.%s", feature);
    return key;
}


@implementation StoreManagerDelegate

- (id)initWithStoreManager:(StoreManagerStoreKit *)manager
{
    self = [super init];
    if (self == nil) {
        return nil;
    }
    
    self->m_manager = manager;
    
    // Add ourselves as an observer so we can catch any payments being processed
    [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
    
    return self;
}

- (void)cleanup
{
}

//
// StoreManagerDelegate
//

- (void)promptPurchase:(NSString *)productIdentifier
{
    if ([SKPaymentQueue canMakePayments]) {
        SKProductsRequest *request= [[SKProductsRequest alloc] initWithProductIdentifiers:
                                     [NSSet setWithObject: productIdentifier]];
        request.delegate = self;
        [request start];
    } else {
        m_manager->ProductCanceled(productIdentifier, TEXT("In-App purchases are disabled in Settings > General > Restrictions. Please enable them to access this feature."));
    }
}

- (void)completePurchase:(SKProduct *)product
{
    SKPayment *payment = [SKPayment paymentWithProduct:product];
    [[SKPaymentQueue defaultQueue] addPayment:payment];
}

- (void)handleEvent:(int)eventCode withData:(void *)data1 andData:(void *)data2
{
    switch (eventCode) {
        case STORE_EVENT_SHOW_PURCHASE_DIALOG:
        {
            SKProduct *product = (SKProduct *)data1;
            m_manager->ShowPurchaseDialog(product,
                                          [product.localizedTitle UTF8String],
                                          [product.localizedDescription UTF8String],
                                          [[self getProductPriceString:product] UTF8String]);
            break;
        }
        case STORE_EVENT_PRODUCT_QUERY_FAILED:
        {
            NSString *productIdentifier = (NSString *)data1;
            if (productIdentifier) {
                char message[BUFSIZ];
                SDL_snprintf(message, sizeof(message), TEXT("Unknown product %s"), [productIdentifier UTF8String]);
                m_manager->ProductCanceled(productIdentifier, message);
                [productIdentifier release];
            } else {
                // Unfortunately we don't have any context here as to which product failed.
                // We could fix this by having a custom object acting as the request delegate instead this class.
                // Or we can just cancel all the pending requests. :)
                m_manager->ProductsCanceled(TEXT("Store unavailable"));
            }
            break;
        }
        case STORE_EVENT_UPDATED_TRANSACTIONS:
        {
            NSArray *transactions = (NSArray *)data1;
            for (SKPaymentTransaction *transaction in transactions) {
                switch (transaction.transactionState) {
                    case SKPaymentTransactionStatePurchased:
                        [self completeTransaction:transaction];
                        break;
                    case SKPaymentTransactionStateFailed:
                        [self failedTransaction:transaction];
                        break;
                    case SKPaymentTransactionStateRestored:
                        [self restoreTransaction:transaction];
                        break;
                }
            }
            [transactions release];
            break;
        }
        default:
            break;
    }
}

//
// SKProductsRequestDelegate
//

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
    for (SKProduct *product in response.products) {
        [product retain];
        m_manager->PushEvent(STORE_EVENT_SHOW_PURCHASE_DIALOG, product);
    }
    for (NSString *productIdentifier in response.invalidProductIdentifiers) {
        [productIdentifier retain];
        m_manager->PushEvent(STORE_EVENT_PRODUCT_QUERY_FAILED, productIdentifier);
    }
    [request release];
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
    m_manager->PushEvent(STORE_EVENT_PRODUCT_QUERY_FAILED, NULL);
    [request release];
}

/// end SKProductsRequestDelegate

//
// SKPaymentTransactionObserver
//

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions
{
    [transactions retain];
    m_manager->PushEvent(STORE_EVENT_UPDATED_TRANSACTIONS, transactions);
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedDownloads:(NSArray *)downloads
{
    // We don't download any content, so we shouldn't need to do anything
}

// end SKPaymentTransactionObserver

- (void)completeTransaction:(SKPaymentTransaction *)transaction
{
    [self recordTransaction:transaction];
    m_manager->ProductPurchased(transaction.payment.productIdentifier);
    [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
}

- (void)restoreTransaction:(SKPaymentTransaction *)transaction
{
    [self recordTransaction:transaction];
    m_manager->ProductPurchased(transaction.originalTransaction.payment.productIdentifier);
    [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
}

- (void)failedTransaction:(SKPaymentTransaction *)transaction
{
    char message[BUFSIZ];
    if (transaction.error.code == SKErrorPaymentCancelled) {
        message[0] = '\0';
    } else {
        SDL_snprintf(message, sizeof(message), TEXT("Failed purchase transaction for %s: %s"),
                     [transaction.payment.productIdentifier UTF8String],
                     [[transaction.error localizedDescription] UTF8String]);
    }
    m_manager->ProductCanceled(transaction.payment.productIdentifier, message);
    [[SKPaymentQueue defaultQueue] finishTransaction: transaction];
}

- (void)recordTransaction:(SKPaymentTransaction *)transaction
{
    // Nothing to do here, at least for now.
}

- (NSString *)getProductPriceString:(SKProduct *)product
{
    NSNumberFormatter *formatter = [[NSNumberFormatter alloc] init];
    [formatter setNumberStyle:NSNumberFormatterCurrencyStyle];
    [formatter setLocale:product.priceLocale];
    
    NSString *str = [formatter stringFromNumber:product.price];
    [formatter release];
    return str;
}

@end // StoreManagerDelegate
