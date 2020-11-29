//
//  DispatchPrivate.hpp
//  DispatchCpp
//
//  Created by Katekov Anton on 24/10/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#ifndef DispatchPrivate_hpp
#define DispatchPrivate_hpp

#include <stdio.h>
#include <mutex>
#include <condition_variable>
#include "Dispatch.hpp"



class DispatchImpl: public Dispatch {
    
public:
    void OnAssigned() override;
    
};



class DispatchPrivate {
    
public:
    static void SetSharedDispatch(DispatchImpl *dispatch);
    static Dispatch *GetSharedDispatch();
};

#endif /* DispatchPrivate_hpp */
