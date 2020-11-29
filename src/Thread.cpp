//
//  Thread.cpp
//  DispatchCpp
//
//  Created by Katekov Anton on 24/10/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//


#include "Thread.h"
#include <assert.h>



static void runThread(void* arg)
{
    ((Thread*)arg)->run();
}



Thread::Thread()
:m_running(0)
,m_detached(0)
{
    
}

Thread::~Thread()
{
    if (m_running == 1 && m_detached == 0) {
        detach();
        m_running = false;
    }
}

void Thread::start()
{
    printf("Threads: starting thread\n");
    m_running = 1;
    m_thread = std::thread(runThread, this);
}

void Thread::join()
{
    if (m_running == 1)
    {
        m_thread.join();
        m_detached = 0;
    }
}

void Thread::detach()
{
    if (m_running == 1 && m_detached == 0)
    {
        m_thread.detach();
        m_detached = 1;
    }
}

void Thread::waitForUnblock()
{
    if (m_running == 0) {
        return;
    }
    
    m_SuspendMutex.lock();
    m_SuspendMutex.lock();
    m_SuspendMutex.unlock();
}

void Thread::unblock()
{
    if (m_running == 0) {
        return;
    }

    m_SuspendMutex.unlock();
}

std::thread::id Thread::getId()
{
    assert(m_running == 1);
    return m_threadId;
}

void Thread::run()
{
    m_threadId = std::this_thread::get_id();
}

