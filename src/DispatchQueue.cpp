//
//  DispatchQueue.cpp
//  DispatchCpp
//
//  Created by Katekov Anton on 24/10/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#include "DispatchQueue.hpp"
#include "Dispatch.hpp"
#include <assert.h>



static std::thread::id emptyThreadId;



DispatchQueue::DispatchQueue(Dispatch *owner)
:_removing(false)
,_working(false)
,_owner(owner)
,_operationsCount(0)
,_disabled(false)
{}

DispatchQueue::~DispatchQueue()
{
    Lock("destructor 1");
    assert(_operations.size() == 0);
    assert(_operationsCount == 0);
    _removing = true;
    for (int i = 0; i < _operations.size(); i++) {
        _operations[i]->Cancel();
        _operations[i]->_work(_operations[i].get());
    }
    Unlock();
}

bool DispatchQueue::GetIsReady()
{
    return true;
}

bool DispatchQueue::TryLock(const std::string &from)
{
    return _mutexOperations.TryLock(from);
}

void DispatchQueue::Lock(const std::string &from)
{
    _mutexOperations.Lock(from);
}

void DispatchQueue::Unlock()
{
    _mutexOperations.Unlock();
}

size_t DispatchQueue::GetOpertaionsCount()
{
    return _operationsCount;
}

bool DispatchQueue::ExecuteNext()
{
    Lock("ExecuteNext() 1");
    if (_disabled) {
        Unlock();
        return false;
    }
    assert(!_sleep);
    if (_operations.size() == 0)
    {
        // report to owner
        _owner->ExecuteNextPendingGroup(this, true);
        
        if (_operations.size() == 0)
        {
            Unlock();
            return false;
        }
        
    }
    int operationIndex = 0;
    DispatchOperationP operation = _operations[operationIndex];
    assert(operation != nullptr);
    _operations.erase(_operations.begin() + operationIndex);
    _operationsCount = _operations.size();
    _working = operation->GetState() == DispatchOperationState_Created;
    Unlock();
    
    if (operation->GetState() == DispatchOperationState_Created)
    {
        operation->SetState(DispatchOperationState_Running);
        operation->_work(operation.get());
        _working = false;
    }
    operation->SetState(DispatchOperationState_Finished);
    
    return true;
}

void DispatchQueue::LaunchExecution()
{}

bool DispatchQueue::GetIsBusy()
{
    return GetOpertaionsCount() > 0;
}

bool DispatchQueue::AddOperation(DispatchOperationP operation, bool lockUnlock)
{
    assert(!_removing);
    assert(!_sleep);
    assert(operation);
    assert(!_disabled);
    
    if (lockUnlock) {
        Lock("AddOperation 1");
    }
    
    assert(!_removing);
    assert(!_sleep);
    
    _operations.push_back(operation);
    _operationsCount = _operations.size();
    
    if (lockUnlock) {
        Unlock();
    }
    
    if (GetIsReady()) {
        return true;
    }
    LaunchExecution();
    return true;
}

void DispatchQueue::RemoveOperation(DispatchOperationP operation)
{
    assert(false);// not implemented
}

void DispatchQueue::Reschedule(DispatchQueue::Scheduler schedule, bool lockUnlockQueue, bool lockUnlockDispatch)
{
    if (lockUnlockQueue) {
        Lock("Reschedule 1");
    }
    std::vector<DispatchOperationP> operations = _operations;
    _operations.clear();
    _operationsCount = 0;
    if (lockUnlockQueue) {
        Unlock();
    }
    
    for (int i = 0; i < operations.size(); i++) {
        schedule(operations[i], this, lockUnlockDispatch);
    }
}

DispatchOperationP DispatchQueue::Perform(const DispatchWork &work)
{
    DispatchOperationP p = DispatchOperationP(new DispatchOperation(work));
    AddOperation(p, true);
    return p;
}



DispatchMainQueue::DispatchMainQueue(Dispatch *owner)
:DispatchQueue(owner)
{}

void DispatchMainQueue::LaunchExecution()
{
    while (ExecuteNext())
    {}
}



DispatchBackgroundQueue::DispatchBackgroundQueue(Dispatch *owner)
:DispatchQueue(owner)
,_threadId(emptyThreadId)
,_thread(NULL)
{
    
}

DispatchBackgroundQueue::~DispatchBackgroundQueue()
{
    _mutexThread.lock();
    if (_thread != NULL) {
        delete _thread;
        _thread = NULL;
        _threadId = emptyThreadId;
    }
    _mutexThread.unlock();
}

bool DispatchBackgroundQueue::GetIsReady() 
{
    return _thread != nullptr;
}

void DispatchBackgroundQueue::LaunchExecution()
{
    _mutexThread.lock();
    if (_thread == NULL)
    {
        _thread = new DispatchThread(this);
        _thread->start();
    }
    _mutexThread.unlock();
}

bool DispatchBackgroundQueue::GetIsBusy()
{
    _mutexThread.lock();
    bool result = GetOpertaionsCount() > 0;
    if (!result) {
        _mutexThread.unlock();
        return false;
    }
    // do we need a lock here?
    result = _thread != NULL;
    _mutexThread.unlock();
    return result;
}

bool DispatchBackgroundQueue::GetIsThreadWithId(const std::thread::id threadId)
{
    return GetIsReady() && _threadId == threadId;
}

void DispatchBackgroundQueue::ThreadExecuting(DispatchThread *sender)
{
    assert(_threadId == emptyThreadId);
    _threadId = sender->getId();
    while (ExecuteNext())
    {}
}

void DispatchBackgroundQueue::ThreadDidFinish(DispatchThread *sender)
{
    _mutexThread.lock();
    DispatchThread *t = _thread;
    _thread = nullptr;
    _threadId = emptyThreadId;
    _mutexThread.unlock();
    delete t;
    
    if (_operationsCount != 0) {
        assert(!GetRemoving());
        assert(!GetDisabled());
        LaunchExecution();
    }
}
