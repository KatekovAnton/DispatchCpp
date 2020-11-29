//
//  Platform_mac.h
//  DispatchCpp
//
//  Created by Katekov Anton on 29/11/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#ifndef Platform_mac_h
#define Platform_mac_h

#include "Platform.h"


class Platform_mac: public Platform {
    
public:
    
    float GetCPUTemperature() override;
    
};

#endif /* Platform_mac_h */
