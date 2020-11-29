//
//  Platform_win.cpp
//  DispatchCpp
//
//  Created by Katekov Anton on 29/11/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#include "Platform_win.h"
#include <mutex>



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
    p = new Platform_win();
    m.unlock();
    return p;
}

float Platform_win::GetCPUTemperature()
{
    // TODO
    return 0;
}

