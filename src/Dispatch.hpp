//
//  Dispatch.hpp
//  DispatchCpp
//
//  Created by Katekov Anton on 24/10/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#ifndef Dispatch_hpp
#define Dispatch_hpp

#include <stdio.h>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <set>
#include "DispatchThread.hpp"



class Dispatch;
class DispatchQueue;
class DispatchBackgroundQueue;
class DispatchOperation;
class DispatchOperationGroup;

typedef std::shared_ptr<DispatchOperationGroup> DispatchOperationGroupP;
typedef std::shared_ptr<DispatchOperation> DispatchOperationP;
typedef std::weak_ptr<DispatchOperation> DispatchOperationWP;
typedef std::shared_ptr<DispatchQueue> DispatchQueueP;
typedef std::weak_ptr<DispatchQueue> DispatchQueueWP;
typedef std::shared_ptr<DispatchBackgroundQueue> DispatchBackgroundQueueP;
typedef std::function<void(DispatchOperation *operation)> DispatchWork;
typedef std::function<void(DispatchOperationGroupP group)> DispatchGroupCallback;



typedef enum {
    
    DispatchOperationState_Created,
    DispatchOperationState_Running,
    DispatchOperationState_Finished,
    DispatchOperationState_Cancelled
    
} DispatchOperationState;



typedef struct _DispatchBackgroundQueueSearchResult {
    
    DispatchBackgroundQueueP _queue;
    size_t _queueIndex = 0;
    
    _DispatchBackgroundQueueSearchResult()
    :_queue(nullptr)
    ,_queueIndex(0)
    {}
    
} DispatchBackgroundQueueSearchResult;



class DispatchLock {

public:
    bool _locked;
    bool _processed;
    std::condition_variable _cv;
    std::mutex _m;
    
    DispatchLock();
    ~DispatchLock();
    
    void Lock();
    void Unlock();
    
};



class DispatchReadWriteLock {
    
    std::mutex              _shared;
    std::condition_variable _readerQ;
    std::condition_variable _writerQ;
    int                     _activeReaders;
    int                     _waitingWriters;
    int                     _activeWriters;
    
public:
    
    DispatchReadWriteLock();

    void ReadLock();
    void ReadUnlock();

    void WriteLock();
    void WriteUnlock();
    
};



class DispatchOperation {
    
    friend class DispatchQueue;
    
    DispatchWork _work;
    std::atomic<DispatchOperationState> _state;
    
    void SetState(DispatchOperationState state);
    
public:
    
    std::function<void()> OnStateChanged;
    
    DispatchOperation(const DispatchWork &work);

    DispatchOperationState GetState() { return _state; };
    bool IsCancelled() { return _state == DispatchOperationState::DispatchOperationState_Cancelled; }
    
    void Cancel();
    
};



class DispatchOperationGroup {
    
    friend class Dispatch;
    
    std::mutex _mutex;
    
    std::queue<DispatchWork> _pendingWorks;
    DispatchWork _pendingCompletionWork;
    
    std::vector<DispatchOperationP> _executingOperations;
    DispatchOperationP _executingCompletion;
    
    bool _executing;
    bool _cancelled;
    bool _haveMoreToRun;
    bool _readyToRunMore;
    
    bool HaveMoreToRun();
    
    void ExecuteOne(DispatchQueue *queue, Dispatch *owner, bool queueLocked);
    void ExecutionOfWorkFinished(DispatchOperation *operation);
    void ExecutionOfCompletionFinished(DispatchOperation *operation);
    
public:
    
    DispatchOperationGroup(const std::vector<DispatchWork> &works, DispatchWork completionWork);
    
    bool IsExecuting() { return _executing; }
    bool IsCancelled() { return _cancelled; }
    void Cancel();
    
};



class Dispatch {
    
    friend class DispatchQueue;
    friend class DispatchOperationGroup;
    
    void QueueHaveNoTasks(DispatchQueue *emptyQueue);
//    void QueueHaveNoTasksExternal(DispatchQueue *emptyQueue);
    void GroupExecutionFinished(DispatchOperationGroup *group);
    void ExecuteGroup(DispatchOperationGroupP group);
    void ExecuteNextPendingGroup(DispatchQueue *emptyQueue, bool queueLocked);
    
protected:
    
    std::vector<DispatchBackgroundQueueP> _backgroundQueuesToRemove;
    std::mutex _mutexBackgroundQueuesToRemove;
    
    std::vector<DispatchBackgroundQueueP> _backgroundQueuesPaused;
    std::vector<DispatchBackgroundQueueP> _backgroundQueuesSpecial;
    std::vector<DispatchBackgroundQueueP> _backgroundQueues;
    std::mutex _mutexBackgroundQueuesPaused;
    
    DispatchMutex _mutexBackgroundQueues;
    int _backgroundQueuesCount;
    
    int _backgroundQueuesDesiredCount;
    
    std::mutex _mutexGroups;
    std::vector<DispatchOperationGroupP> _groups;
    
    std::thread::id _mainThreadId;
    DispatchQueueP _mainQueue;
    
    virtual void OnAssigned();
    DispatchBackgroundQueueSearchResult GetCurrentBackgroundQueue(bool lockUnlock);
    
    void BackgroundQueueWaitDone(DispatchBackgroundQueueP pausedQueue, const std::shared_ptr<DispatchLock> &sync);
    void BackgroundQueueEvacuate(DispatchBackgroundQueueP queue, bool lockUnlockQueue, bool lockUnlockDispatch);
    void BackgroundQueueAssignEvacuated(const DispatchOperationP &operation, DispatchQueue *from, bool lockUnlock);
    
    DispatchOperationP InternalPerformOnBackgroundQueue(const DispatchWork &work);
    
public:
    
    Dispatch();
    static Dispatch *SharedDispatch();
    
    DispatchQueueP GetFreeQueue(bool lock, DispatchQueue *exceptQueue, bool lockUnlockDispatch);
    
    void FlushMainThread();
    bool IsMainThread();
    
    DispatchOperationGroupP PerformGroup(const std::vector<DispatchWork> &works, const DispatchWork &completionWork);
    void PerformAndWait(const std::vector<DispatchWork> &works, DispatchGroupCallback groupCallback);
    
    void FlushQueues();
    void BackgroundQueueRemoving(DispatchBackgroundQueue *sender);
    
    static DispatchOperationP PerformOnMainQueue(const DispatchWork &work);
    static DispatchOperationP PerformOnBackgroundQueue(const DispatchWork &work);
    
    static DispatchOperationP PerformSynchronousOnMainQueue(const DispatchWork &work);
    static DispatchOperationP PerformSynchronousOnBackgroundQueue(const DispatchWork &work);
    static DispatchOperationP PerformSynchronousOnBackgroundQueue(const DispatchWork &work, DispatchQueueP queue);
    
    static void Sleep(unsigned long long miliseconds);
    
};
#endif /* Dispatch_hpp */
