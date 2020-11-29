//
//  DispatchPrivate.cpp
//  DispatchCpp
//
//  Created by Katekov Anton on 24/10/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#include "DispatchPrivate.hpp"



Dispatch *staticDispatch = NULL;



void DispatchImpl::OnAssigned()
{
    Dispatch::OnAssigned();
}



void DispatchPrivate::SetSharedDispatch(DispatchImpl *dispatch)
{
    staticDispatch = dispatch;
    dispatch->OnAssigned();
}

Dispatch *DispatchPrivate::GetSharedDispatch()
{
    return staticDispatch;
}
