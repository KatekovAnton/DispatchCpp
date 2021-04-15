//
//  Platform_lin.h
//  DispatchCpp
//
//  Created by Katekov Anton on 15/04/21.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#ifndef Platform_lin_h
#define Platform_lin_h

#include "Platform.h"


class Platform_lin: public Platform {
    
public:
    
    float GetCPUTemperature() override;
    
};

#endif /* Platform_lin_h */
