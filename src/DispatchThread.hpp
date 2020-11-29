//
//  DispatchThread.hpp
//  DispatchCpp
//
//  Created by Katekov Anton on 24/10/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#ifndef DispatchThread_hpp
#define DispatchThread_hpp

#include "Thread.h"
#include <atomic>
#include <mutex>
#include <string>



class DispatchThread;



class DispatchThreadDelegate {
public:
    virtual void ThreadExecuting(DispatchThread *sender) = 0;
    virtual void ThreadDidFinish(DispatchThread *sender) = 0;
};


class DispatchThreadStatistic {
    
    std::mutex _m;
    int _spawned;
    
public:
    
    DispatchThreadStatistic()
    :_spawned(0)
    {}
    
    void ThreadSpawned()
    {
        _m.lock();
        _spawned++;
        _m.unlock();
    }
    
    int GetSpawned() {
        return _spawned; 
    }
    
};



class DispatchThread: public Thread {
public:
    
    static DispatchThreadStatistic &GetStatistic();
    
    DispatchThreadDelegate *_delegate;
    
    DispatchThread(DispatchThreadDelegate *delegate);
    void run() override;
};



class DispatchMutexBase {
    
public:
    
    bool _locked;
    std::string _lockedFrom;
    
    DispatchMutexBase()
    :_locked(false)
    {}
    
    virtual bool TryLock(const std::string &from);
    virtual void Lock(const std::string &from);
    virtual void Unlock();
    
};



class DispatchMutex: public DispatchMutexBase {
    
    std::mutex _mutex;
    
    DispatchMutex(const DispatchMutex &o){};
    void operator=(const DispatchMutex &o){}
    
public:
    
    DispatchMutex();
    
    bool TryLock(const std::string &from) override;
    void Lock(const std::string &from) override;
    void Unlock() override;
    
};

#endif /* DispatchThread_hpp */
