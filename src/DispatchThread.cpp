//
//  DispatchThread.cpp
//  DispatchCpp
//
//  Created by Katekov Anton on 24/10/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#include "DispatchThread.hpp"


DispatchThreadStatistic &DispatchThread::GetStatistic()
{
    static DispatchThreadStatistic s;
    return s;
}

DispatchThread::DispatchThread(DispatchThreadDelegate *delegate)
:_delegate(delegate)
{}

void DispatchThread::run()
{
    DispatchThread::GetStatistic().ThreadSpawned();
    
    Thread::run();
    _delegate->ThreadExecuting(this);
    _delegate->ThreadDidFinish(this);
}



bool DispatchMutexBase::TryLock(const std::string &from)
{
    _locked = true;
    _lockedFrom = from;
    return true;
}

void DispatchMutexBase::Lock(const std::string &from)
{
    _locked = true;
    _lockedFrom = from;
}

void DispatchMutexBase::Unlock()
{
    _locked = false;
}



DispatchMutex::DispatchMutex()
{}

bool DispatchMutex::TryLock(const std::string &from)
{
    if (_mutex.try_lock()) {
        _locked = true;
        _lockedFrom = from;
        return true;
    }
    return false;
}

void DispatchMutex::Lock(const std::string &from)
{
    _mutex.lock();
    _locked = true;
    _lockedFrom = from;
}

void DispatchMutex::Unlock()
{
    _locked = false;
    _mutex.unlock();
}
