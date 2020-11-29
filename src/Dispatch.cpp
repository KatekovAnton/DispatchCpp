//
//  Dispatch.cpp
//  DispatchCpp
//
//  Created by Katekov Anton on 24/10/20.
//  Copyright © 2020 AntonKatekov. All rights reserved.
//

#include "Dispatch.hpp"
#include "DispatchPrivate.hpp"
#include "DispatchQueue.hpp"
#include "Thread.h"
#include <cassert>
#include <string>


using namespace std;



DispatchReadWriteLock::DispatchReadWriteLock()
:_shared()
,_readerQ()
,_writerQ()
,_activeReaders(0)
,_waitingWriters(0)
,_activeWriters(0)
{}

void DispatchReadWriteLock::ReadLock() {
    std::unique_lock<std::mutex> lk(_shared);
    while (_waitingWriters != 0 ) {
        _readerQ.wait(lk);
    }
    ++_activeReaders;
    lk.unlock();
}

void DispatchReadWriteLock::ReadUnlock() {
    std::unique_lock<std::mutex> lk(_shared);
    --_activeReaders;
    lk.unlock();
    _writerQ.notify_one();
}

void DispatchReadWriteLock::WriteLock() {
    std::unique_lock<std::mutex> lk(_shared);
    ++_waitingWriters;
    while (_activeReaders != 0 || _activeWriters != 0) {
        _writerQ.wait(lk);
    }
    ++_activeWriters;
    lk.unlock();
}

void DispatchReadWriteLock::WriteUnlock() {
    std::unique_lock<std::mutex> lk(_shared);
    --_waitingWriters;
    --_activeWriters;
    if(_waitingWriters > 0) {
        _writerQ.notify_one();
    }
    else {
        _readerQ.notify_all();
    }
    lk.unlock();
}



DispatchLock::DispatchLock()
:_locked(false)
,_processed(false)
{}

DispatchLock::~DispatchLock()
{
    // stupid situation but ok...
    if (_locked && !_processed) {
        Unlock();
    }
}

void DispatchLock::Lock()
{
    _locked = true;
    _processed = false;
    std::unique_lock<std::mutex> lk(_m);
    _cv.wait(lk, [this] {
        return this->_processed;
    });
}

void DispatchLock::Unlock()
{
    if (_locked == false) {
        return;
    }
    _locked = false;
    _processed = true;
    _cv.notify_one();
}



DispatchOperation::DispatchOperation(const DispatchWork &work)
:_work(work)
,_state(DispatchOperationState_Created)
{}

void DispatchOperation::SetState(DispatchOperationState state)
{
     _state = state;
    if (OnStateChanged != nullptr) {
        OnStateChanged();
    }
 }

void DispatchOperation::Cancel()
{
    SetState(DispatchOperationState_Cancelled);
}



DispatchOperationGroup::DispatchOperationGroup(const std::vector<DispatchWork> &works, DispatchWork completionWork)
:_executing(false)
,_cancelled(false)
,_haveMoreToRun(works.size() > 0 || completionWork != nullptr)
,_readyToRunMore(works.size() > 0 || completionWork != nullptr)
{
    for (size_t i = 0; i < works.size(); i++) {
        _pendingWorks.push(works[i]);
    }
    _pendingCompletionWork = completionWork;
}

void DispatchOperationGroup::ExecuteOne(DispatchQueue *queue, Dispatch *owner, bool queueLocked)
{
    if (_cancelled) {
        return;
    }
    _mutex.lock();
    if (_pendingWorks.size() == 0) {
        if (_executingOperations.size() == 0) {
            if (_executingCompletion == nullptr) {
                _executingCompletion = DispatchOperationP(new DispatchOperation([this, owner](DispatchOperation *operation) {
                    if (!this->IsCancelled()) {
                        this->_pendingCompletionWork(operation);
                    }
                    this->ExecutionOfCompletionFinished(operation);
                    owner->GroupExecutionFinished(this);
                }));
                _haveMoreToRun = false;
                _readyToRunMore = false;
                _mutex.unlock();
                queue->AddOperation(_executingCompletion, !queueLocked);
                return;
            }
        }
        _mutex.unlock();
        return;
    }
    
    auto work = _pendingWorks.front();
    _pendingWorks.pop();
    _haveMoreToRun = _pendingWorks.size() > 0 || _pendingCompletionWork != nullptr;
    _readyToRunMore = _pendingWorks.size() > 0 || (_executingOperations.size() == 0 && _pendingCompletionWork != nullptr);
    
    auto operation = DispatchOperationP(new DispatchOperation([work, this, owner](DispatchOperation *operation) {
        if (!this->IsCancelled()) {
            work(operation);
        }
        this->ExecutionOfWorkFinished(operation); 
        owner->ExecuteNextPendingGroup(nullptr, false);
    }));
    _executingOperations.push_back(operation);
    _executing = true;
    _mutex.unlock();
    
    queue->AddOperation(operation, !queueLocked);
}

void DispatchOperationGroup::ExecutionOfWorkFinished(DispatchOperation *operation)
{
    _mutex.lock();
    for (size_t i = 0; i < _executingOperations.size(); i++) {
        if (_executingOperations[i].get() == operation) {
            _executingOperations.erase(_executingOperations.begin() + i);
            break;
        }
    }
    if (!_cancelled) {
        _haveMoreToRun = _pendingWorks.size() > 0 || _pendingCompletionWork != nullptr;
        _readyToRunMore = _pendingWorks.size() > 0 || (_executingOperations.size() == 0 && _pendingCompletionWork != nullptr);
    }
    else {
        _executing = _executingOperations.size() != 0 || _executingCompletion != nullptr;
    }
    _mutex.unlock();
}

void DispatchOperationGroup::ExecutionOfCompletionFinished(DispatchOperation *operation)
{
    _pendingCompletionWork = nullptr;
    _executing = false;
    _haveMoreToRun = false;
    _readyToRunMore = false;
}

void DispatchOperationGroup::Cancel()
{
    _cancelled = true;
    _haveMoreToRun = false;
    _readyToRunMore = false;
}



Dispatch::Dispatch()
:_backgroundQueuesCount(0)
,_backgroundQueuesDesiredCount(_backgroundQueuesDesiredCount = thread::hardware_concurrency() - 1)
,_mainQueue(DispatchQueueP(new DispatchMainQueue(this)))
{
    {
        auto queue = DispatchBackgroundQueueP(new DispatchBackgroundQueue(this));
        _backgroundQueues.push_back(queue);
        _backgroundQueuesCount = _backgroundQueues.size();
    }
}

Dispatch *Dispatch::SharedDispatch()
{
    return DispatchPrivate::GetSharedDispatch();
}

void Dispatch::OnAssigned()
{
    _mainThreadId = std::this_thread::get_id();
}

DispatchQueueP Dispatch::GetFreeQueue(bool lock, DispatchQueue *exceptQueue)
{
    DispatchBackgroundQueueP q = nullptr;
    DispatchBackgroundQueueP qlowest = nullptr;
    size_t qlowestCount = 0xffffffff;
    
    // schedule only to first threads, reasonable for cpu
    // last threads will finish and die
    int total = _backgroundQueuesCount > _backgroundQueuesDesiredCount ? _backgroundQueuesDesiredCount : _backgroundQueuesCount;
    for (size_t i = 0; i < total; i++) 
    {
        auto qt = _backgroundQueues[i];
        if (qt.get() == exceptQueue) {
            continue;
        }
        if (qt->_disabled) {
            continue;
        }
        assert(!qt->_sleep);
        size_t qtc = qt->_operationsCount;
        if (qtc == 0) {
            if (qt->GetIsReady()) {
                // locks
                if (lock) {
                    qt->Lock("GetFreeQueue 1");
                    assert(!qt->_sleep);
                }
                return qt;
            }
            q = qt;
        }
        else if (qtc < qlowestCount) {
            qlowestCount = qtc;
            qlowest = qt;
        }
    }
    
    if (q == nullptr) {
        if (_backgroundQueuesCount < _backgroundQueuesDesiredCount) {
            auto queue = DispatchBackgroundQueueP(new DispatchBackgroundQueue(this));
            _backgroundQueues.push_back(queue);
            _backgroundQueuesCount = _backgroundQueues.size();
            
            // locks
            if (lock) {
                queue->Lock("GetFreeQueue 2");
            }
            assert(!queue->_sleep);
            return queue;
        }
        else {
            q = qlowest;
        }
    }
    // locks
    if (lock) {
        if (q != nullptr) {
            q->Lock("GetFreeQueue 3");
            assert(!q->_sleep);
        }
    }
    assert(!q->_sleep);
    return q;
}

void Dispatch::FlushMainThread()
{
#if DEBUG
    if (!IsMainThread()) {
        throw "Dispatch: FlushMainThread in wrong thread!";
    }
#endif
    _mainQueue->LaunchExecution();
}

bool Dispatch::IsMainThread()
{
    return _mainThreadId == std::this_thread::get_id();
}

void Dispatch::FlushQueues()
{
    _mutexBackgroundQueuesToRemove.lock();
    _backgroundQueuesToRemove.clear();
    _mutexBackgroundQueuesToRemove.unlock();
}

void Dispatch::QueueHaveNoTasks(DispatchQueue *emptyQueue)
{
    emptyQueue->_disabled = true;
    assert(!emptyQueue->_sleep);
    assert(emptyQueue->_operationsCount == 0);
    for (size_t i = 0; i < _backgroundQueues.size(); i++) {
        if (_backgroundQueues[i].get() == emptyQueue) {
            _mutexBackgroundQueuesToRemove.lock();
            _backgroundQueuesToRemove.push_back(_backgroundQueues[i]);
#if defined DEBUG
            printf("Move queue to paused 2 %p\n", (void *) emptyQueue);
#endif
            _backgroundQueues.erase(_backgroundQueues.begin() + i);
            _backgroundQueuesCount = _backgroundQueues.size();
            _mutexBackgroundQueuesToRemove.unlock();
            return;
        }
    }
}

//void Dispatch::QueueHaveNoTasksExternal(DispatchQueue *emptyQueue)
//{
//    _mutexBackgroundQueues.lock();
//    for (size_t i = 0; i < _backgroundQueues.size(); i++) {
//        if (_backgroundQueues[i].get() == emptyQueue) {
//            _mutexBackgroundQueuesToRemove.lock();
//            _backgroundQueuesToRemove.push_back(_backgroundQueues[i]);
//            _backgroundQueues.erase(_backgroundQueues.begin() + i);
//            _backgroundQueuesCount = _backgroundQueues.size();
//            _mutexBackgroundQueuesToRemove.unlock();
//            _mutexBackgroundQueues.unlock();
//            return;
//        }
//    }
//    _mutexBackgroundQueues.unlock();
//}

void Dispatch::GroupExecutionFinished(DispatchOperationGroup *group)
{
    DispatchOperationGroupP groupP = nullptr;
    
    _mutexGroups.lock();    
    for (size_t i = 0; i < _groups.size(); i++) {
        if (_groups[i].get() == group) {
            groupP = _groups[i];
            _groups.erase(_groups.begin() + i);
            break;
        }
    }
    _mutexGroups.unlock();
    
    if (groupP != nullptr) {
//        groupP->...
    }
}

// todo: can execute more
void Dispatch::ExecuteGroup(DispatchOperationGroupP group)
{
    _mutexBackgroundQueues.Lock("ExecuteGroup");
    DispatchQueue *q = GetFreeQueue(true, nullptr).get();
    while (q != nullptr) {
        group->ExecuteOne(q, this, true);
        q->Unlock();
        q = nullptr;
        if (!group->_readyToRunMore) {
            break;
        }
        q = GetFreeQueue(true, nullptr).get();
    }
    _mutexBackgroundQueues.Unlock();
}

// todo: can execute more
void Dispatch::ExecuteNextPendingGroup(DispatchQueue *emptyQueue, bool queueLocked)
{
#if defined DEBUG
    if (emptyQueue) {
        assert(!emptyQueue->_disabled);
    }
#endif
    _mutexGroups.lock();
    if (_groups.size() == 0) {
        if (emptyQueue) {
            if (!queueLocked) {
                emptyQueue->Lock("ExecuteNextPendingGroup2");
                if (emptyQueue->_operationsCount != 0) {
                    emptyQueue->Unlock();
                    _mutexGroups.unlock();
                    return;
                }
            }
            
            if (_backgroundQueuesCount > _backgroundQueuesDesiredCount) {
                _mutexBackgroundQueues.Lock("ExecuteNextPendingGroup1");
                QueueHaveNoTasks(emptyQueue);
                _mutexBackgroundQueues.Unlock();
            }
            if (!queueLocked) {
                emptyQueue->Unlock();
            }
        }
        _mutexGroups.unlock();
        return;
    }
    
    
    int index = 0;
    DispatchOperationGroupP g;
    while (true) {
        g = _groups[index];
        if (g->IsCancelled()) {
            _groups.erase(_groups.begin() + index);
            index--;
            if (_groups.size() >= index) {
                g = _groups[index];
            }
            else {
                g = nullptr;
                break;
            }
        }
        if (!g->_readyToRunMore) {
            index++;
            g = nullptr;
            if (_groups.size() == index) {
                break;
            }
            continue;
        }
        break;
    }
    _mutexGroups.unlock();
    if (g == nullptr) {
        return;
    }
    if (emptyQueue) {
        g->ExecuteOne(emptyQueue, this, queueLocked);
    }
    else {
        ExecuteGroup(g);
    }
}

DispatchOperationGroupP Dispatch::PerformGroup(const std::vector<DispatchWork> &works, const DispatchWork &completionWork)
{
    DispatchOperationGroupP g = DispatchOperationGroupP(new DispatchOperationGroup(works, completionWork));
    _mutexGroups.lock();
    _groups.push_back(g);
    _mutexGroups.unlock();
    
    ExecuteGroup(g);
    return g;
}

DispatchBackgroundQueueSearchResult Dispatch::GetCurrentBackgroundQueue(bool lockUnlock)
{
    auto threadId = std::this_thread::get_id();
    if (lockUnlock) {
        _mutexBackgroundQueues.Lock("GetCurrentBackgroundQueue");
    }

    for (int i = 0; i < _backgroundQueues.size(); i++) {
        if (_backgroundQueues[i]->GetIsThreadWithId(threadId)) {
            DispatchBackgroundQueueSearchResult result;
            result._queue = _backgroundQueues[i];
            result._queueIndex = i;
            if (lockUnlock) {
                _mutexBackgroundQueues.Unlock();
            }
            return result;
        }
    }
    
    if (lockUnlock) {
        _mutexBackgroundQueues.Unlock();
    }
    return DispatchBackgroundQueueSearchResult();
}

void Dispatch::BackgroundQueueWaitDone(DispatchBackgroundQueueP pausedQueue, const std::shared_ptr<DispatchLock> &sync)
{   
    // move paused queue back to executing
    _mutexBackgroundQueuesPaused.lock();
    {
        for (size_t i = 0; i < _backgroundQueuesPaused.size(); i++) {
            if (_backgroundQueuesPaused[i] == pausedQueue) {
                _backgroundQueuesPaused.erase(_backgroundQueuesPaused.begin() + i);
                break;
            }
        }
    }
    _mutexBackgroundQueuesPaused.unlock();
    
    DispatchBackgroundQueueSearchResult thisQueue;
    
    _mutexBackgroundQueues.Lock("BackgroundQueueWaitDone 1");
    {
        {
            thisQueue = GetCurrentBackgroundQueue(false);
            assert(thisQueue._queue != nullptr);
            if (thisQueue._queue != nullptr) {
                thisQueue._queue->Lock("BackgroundQueueWaitDone");
            }
        }
        {
            if (_backgroundQueuesCount < _backgroundQueuesDesiredCount) {
                _backgroundQueues.push_back(pausedQueue);
                _backgroundQueuesCount = _backgroundQueues.size();
            }
            else {
                assert(pausedQueue->_operationsCount == 0);
                pausedQueue->_disabled = true;
                _mutexBackgroundQueuesToRemove.lock();
                _backgroundQueuesToRemove.push_back(pausedQueue);
#if defined DEBUG
                printf("Move queue to paused 1 %p\n", (void *) pausedQueue.get());
#endif
                _mutexBackgroundQueuesToRemove.unlock();
            }
        }
        
        pausedQueue->_sleep = false;
        sync->Unlock();
        
        // free current queue
        if (thisQueue._queue != nullptr) {
            BackgroundQueueEvacuate(thisQueue._queue, false, false);
            thisQueue._queue->Unlock();
        }
    }
    _mutexBackgroundQueues.Unlock();
}

void Dispatch::BackgroundQueueEvacuate(DispatchBackgroundQueueP queue, bool lockUnlockQueue, bool lockUnlockDispatch)
{
    typedef void (*DispatchScheduler)(DispatchOperationP operation, DispatchQueue *from, bool lockUnlockDispatch);

    DispatchScheduler test = [](DispatchOperationP p, DispatchQueue *from, bool lockUnlockDispatch)->void {
        if (p->GetState() != DispatchOperationState_Created) {
            assert(p->GetState() == DispatchOperationState_Cancelled);
            return;
        }
        Dispatch::SharedDispatch()->BackgroundQueueAssignEvacuated(p, from, lockUnlockDispatch);
    };
    queue->Reschedule(test, lockUnlockQueue, lockUnlockDispatch);
}

void Dispatch::BackgroundQueueAssignEvacuated(const DispatchOperationP &operation, DispatchQueue *from, bool lockUnlock)
{
    // TODO GET LOCKED QUEUE
    if (lockUnlock) {
        _mutexBackgroundQueues.Lock("BackgroundQueueAssignEvacuated");
    }
    
    DispatchQueue *q = GetFreeQueue(true, from).get();
    q->AddOperation(operation, false);
    q->Unlock();
    
    if (lockUnlock) {
        _mutexBackgroundQueues.Unlock();
    }
}

void Dispatch::PerformAndWait(const std::vector<DispatchWork> &works, DispatchGroupCallback groupCallback)
{
    if (works.size() == 0) {
        return;
    }
    
    DispatchBackgroundQueueSearchResult q;
    if (!IsMainThread()) {
        _mutexBackgroundQueues.Lock("PerformAndWait");
        {
            q = GetCurrentBackgroundQueue(false);
            assert(q._queue != nullptr); // thread of main queue or other thread not managed by dispatch
            q._queue->Lock("PerformAndWait");
            q._queue->_sleep = true;
            
            _backgroundQueues.erase(_backgroundQueues.begin() + q._queueIndex);
            _backgroundQueuesCount = _backgroundQueues.size();
        }
        BackgroundQueueEvacuate(q._queue, false, false);
        q._queue->Unlock();
        _mutexBackgroundQueues.Unlock();
        
        _mutexBackgroundQueuesPaused.lock();
        _backgroundQueuesPaused.push_back(q._queue);
        _mutexBackgroundQueuesPaused.unlock();
    }
    
    std::shared_ptr<DispatchLock> sync = std::shared_ptr<DispatchLock>(new DispatchLock());
    auto result = SharedDispatch()->PerformGroup(works, [sync, &q](DispatchOperation *o)->void {
        if (q._queue) {
            Dispatch::SharedDispatch()->BackgroundQueueWaitDone(q._queue, sync);
        }
        else {
            sync->Unlock();
        }
        assert(!sync->_locked);
    });
    if (groupCallback != nullptr) {
        groupCallback(result);
    }
    sync->Lock();
}

void Dispatch::BackgroundQueueRemoving(DispatchBackgroundQueue *sender)
{
//    if (sender->_mutexOperations._locked) {
//        assert(false);
//    }
    if (_backgroundQueuesCount <= _backgroundQueuesDesiredCount) {
        sender->_removing = false;
        return;
    }
    _mutexBackgroundQueues.Lock("BackgroundQueueRemoving");
    for (int i = 0; i < _backgroundQueues.size(); i++) {
        if (_backgroundQueues[i].get() == sender) {
            _backgroundQueues.erase(_backgroundQueues.begin() + i);
            _backgroundQueuesCount = _backgroundQueues.size(); 
            break;
        }
    }
    _mutexBackgroundQueues.Unlock();
    
}

DispatchOperationP Dispatch::PerformOnMainQueue(const DispatchWork &work)
{
    return SharedDispatch()->_mainQueue->Perform(work);
}

DispatchOperationP Dispatch::InternalPerformOnBackgroundQueue(const DispatchWork &work)
{
    _mutexBackgroundQueues.Lock("InternalPerformOnBackgroundQueue");
    DispatchQueue *q = GetFreeQueue(true, nullptr).get();
    DispatchOperationP p = DispatchOperationP(new DispatchOperation(work));
    q->AddOperation(p, false);
    q->Unlock();
    _mutexBackgroundQueues.Unlock();
    return p;
}

DispatchOperationP Dispatch::PerformOnBackgroundQueue(const DispatchWork &work)
{
    return Dispatch::SharedDispatch()->InternalPerformOnBackgroundQueue(work);
}

DispatchOperationP Dispatch::PerformSynchronousOnMainQueue(const DispatchWork &work)
{
#if DEBUG
    if (SharedDispatch()->IsMainThread()) {
        throw "Dispatch: PerformSynchronousOnMainQueue from main queue cause deadlock!";
    }
#endif
    std::shared_ptr<DispatchLock> sync = std::shared_ptr<DispatchLock>(new DispatchLock());
    auto result = SharedDispatch()->_mainQueue->Perform([sync, &work](DispatchOperation *o)->void {
        work(o);
        sync->Unlock();
    });
    sync->Lock();
    return result;
}

DispatchOperationP Dispatch::PerformSynchronousOnBackgroundQueue(const DispatchWork &work)
{
    std::shared_ptr<DispatchLock> sync = std::shared_ptr<DispatchLock>(new DispatchLock());
    auto result = SharedDispatch()->_backgroundQueues[0]->Perform([sync, &work](DispatchOperation *o)->void {
        work(o);
        sync->Unlock();
    });
    sync->Lock();
    return result;
}

void Dispatch::Sleep(unsigned long long miliseconds)
{
    std::this_thread::sleep_for(chrono::milliseconds(miliseconds));
}
