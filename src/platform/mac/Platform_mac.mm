//
//  Platform_mac.mm
//  DispatchCpp
//
//  Created by Katekov Anton on 29/11/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#include "Platform_mac.h"
#include <mutex>
#import <Foundation/Foundation.h>
#import "SMCWrapper.h"



Platform *Platform::CurrentPlatform()
{
    static std::mutex m;
    static Platform *p = nullptr;
    if (p) {
        return p;
    }
    m.lock();
    if (p) {
        m.unlock();
        return p;
    }
    p = new Platform_mac();
    m.unlock();
    return p;
}

float Platform_mac::GetCPUTemperature()
{
    SMCWrapper *smc = [SMCWrapper sharedWrapper];
    char key[10];
    memset(key, 0, sizeof(key));
    
    NSNumber *temp;
    if ( [smc readKey:@"TC0P" intoNumber:&temp] ){
        return [temp floatValue];
    }
    return 0;
}

