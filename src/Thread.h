//
//  Thread.h
//  DispatchCpp
//
//  Created by Katekov Anton on 24/10/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#ifndef __Thread__
#define __Thread__

#include <thread>
#include <mutex>



class Thread
{
    
    std::thread  m_thread;
    std::thread::id m_threadId;
    int        m_running;
    int        m_detached;
    
public:
    
    Thread();
    virtual ~Thread();
    
    void start();
    void join();
    void detach();
    
    std::thread::id getId();
    
    virtual void run();
    
};

#endif /* defined(__Thread__) */
