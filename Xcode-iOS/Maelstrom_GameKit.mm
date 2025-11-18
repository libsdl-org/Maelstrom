
#import "GameKit/GameKit.h"

#include "Maelstrom_GameKit.h"

static BOOL s_hasGameCenter = NO;

static BOOL isGameCenterAPIAvailable()
{
   // check for presence of GKLocalPlayer API
   Class gcClass = NSClassFromString(@"GKLocalPlayer");
   // check if the device is running iOS 4.1 or later
   BOOL osVersionSupported = ([[[UIDevice currentDevice] systemVersion]
        compare:@"4.1" options:NSNumericSearch] != NSOrderedAscending);
   return (gcClass && osVersionSupported);
}

void InitGameCenter(void)
{
    if (isGameCenterAPIAvailable()) 
    {
        GKLocalPlayer *localPlayer = [GKLocalPlayer localPlayer];
        [localPlayer authenticateWithCompletionHandler:^(NSError *err) {
            if (localPlayer.authenticated) {
                // Authentication Successful
		s_hasGameCenter = YES;
            } else {
                // Disable Game Center features
		s_hasGameCenter = NO;
		
		NSLog(@"Unable to log into Game Center: %@", [err localizedDescription]);
            }
        }];
    }
}
