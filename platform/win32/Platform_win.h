//
//  Platform_win.h
//  DispatchCpp
//
//  Created by Katekov Anton on 29/11/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#ifndef Platform_win_h
#define Platform_win_h

#include "Platform.h"


class Platform_win: public Platform {
    
public:
    
    float GetCPUTemperature() override;
    
};

#endif /* Platform_win_h */
