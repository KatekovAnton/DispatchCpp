//
//  DispatchQueue.hpp
//  DispatchCpp
//
//  Created by Katekov Anton on 24/10/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#ifndef DispatchQueue_hpp
#define DispatchQueue_hpp

#include <stdio.h>
#include <mutex>
#include <vector>
#include <atomic>
#include <functional>
#include "DispatchThread.hpp"



class Dispatch;
class DispatchQueue;
class DispatchBackgroundQueue;
class DispatchOperation;

typedef std::shared_ptr<DispatchQueue> DispatchQueueP;
typedef std::shared_ptr<DispatchBackgroundQueue> DispatchBackgroundQueueP;
typedef std::shared_ptr<DispatchOperation> DispatchOperationP;
typedef std::function<void(DispatchOperation *operation)> DispatchWork;



class DispatchQueue {
    
    bool _removing;
    bool _working;
    bool _disabled;
    
    friend class Dispatch;
    
protected:
    
    bool GetRemoving() const { return _removing; }
    bool GetDisabled() const { return _disabled; }
    
    Dispatch *_owner; 
    
    std::atomic<size_t> _operationsCount;
    std::vector<DispatchOperationP> _operations;
    DispatchMutex _mutexOperations;
    
    size_t GetOpertaionsCount();
    bool ExecuteNext();
    virtual void LaunchExecution();
    virtual bool GetIsBusy();
    
public:
    
    bool _sleep = false;
    
    typedef void (*Scheduler)(DispatchOperationP, DispatchQueue *from, bool lockUnlockDispatch);
    
    DispatchQueue(Dispatch *owner);
    virtual ~DispatchQueue();
    
    virtual bool GetIsReady();
    bool GetIsRemoving() { return _removing; }
    
    bool TryLock(const std::string &from);
    void Lock(const std::string &from);
    void Unlock();
    
    bool AddOperation(DispatchOperationP operation, bool lockUnlock);
    void RemoveOperation(DispatchOperationP operation);
    void Reschedule(Scheduler scheduler, bool lockUnlockQueue, bool lockUnlockDispatch);
    
    DispatchOperationP Perform(const DispatchWork &work);
    
};



class DispatchMainQueue : public DispatchQueue {
    
protected:
    
    void LaunchExecution() override;
    
public:
    
    DispatchMainQueue(Dispatch *owner);
    
};



class DispatchBackgroundQueue : public DispatchQueue, public DispatchThreadDelegate {
    
    friend class Dispatch;
    
    std::mutex _mutexThread;
    std::thread::id _threadId;
    DispatchThread *_thread;
    
protected:
    
    void LaunchExecution() override;
    bool GetIsBusy() override;
    bool GetIsThreadWithId(const std::thread::id threadId);
    
public:
    
    DispatchBackgroundQueue(Dispatch *owner);
    ~DispatchBackgroundQueue();
    
    bool GetIsReady() override;
    
    
#pragma mark - DispatchThreadDelegate
    void ThreadExecuting(DispatchThread *sender) override;
    void ThreadDidFinish(DispatchThread *sender) override;
    
};

typedef std::shared_ptr<DispatchQueue> DispatchQueueP;



#endif /* DispatchQueue_hpp */
