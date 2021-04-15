//
//  Platform_lin.cpp
//  DispatchCpp
//
//  Created by Katekov Anton on 15/04/21.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#include "Platform_lin.h"
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
    p = new Platform_lin();
    m.unlock();
    return p;
}

float Platform_lin::GetCPUTemperature()
{
    // TODO
    return 0;
}

