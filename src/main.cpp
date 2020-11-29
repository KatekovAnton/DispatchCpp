#include <stdio.h>
#include <iostream>
#include <math.h>
#include <assert.h>
#include "Dispatch.hpp"
#include "DispatchPrivate.hpp"
#include "DispatchThread.hpp"
#include "Platform.h"



Dispatch *dispatch = nullptr;
int complexityMultiplier = 1;

void createDispatch()
{
    if (DispatchPrivate::GetSharedDispatch() != NULL) {
        dispatch = DispatchPrivate::GetSharedDispatch();
    }
    else {
        DispatchImpl *d = new DispatchImpl();
        dispatch = d;
        DispatchPrivate::SetSharedDispatch(d);
    }
}

int internal(int number)
{
    int result = 0;
    for (int i = 0; i < number; i++) {
        result += i;
    }
    return result;
}

void heavyCalculation(int count) {
    std::vector<float> data;
    for (int i = 0; i < count; i++) {
        float b = sqrtf(sqrtf(static_cast<float>(rand() % 1000 + i)));
        data.push_back(internal(b));
    }
}

class Timer {
    
    std::chrono::high_resolution_clock::time_point _startTime;
    double _duration = -1;
    
public:
    
    void start();
    void stop();
    
    double duration() const {
        return _duration;
    }
    
};

void Timer::start()
{
    _startTime = std::chrono::high_resolution_clock::now();
}

void Timer::stop()
{
    auto nowTime = std::chrono::high_resolution_clock::now();
    double time = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - _startTime).count();
    _duration = time / 1000.0;
}



class Counter {
    
    std::mutex _m;
    int _value;
    
public:
    
    Counter()
    :_value(0)
    {}
    
    void increase()
    {
        _m.lock();
        _value++;
        _m.unlock();
    }
    
    int get() {
        return _value; 
    }
    
};



void testSingleOperation() 
{
    Timer t;
    t.start();
    
    
    bool finished = false;
    dispatch->PerformOnBackgroundQueue([&finished](DispatchOperation *operation) {
        heavyCalculation(100000);
        finished = true;
    });
    
    while (!finished) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    t.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    printf("single operation way took %d\n", (int)(t.duration() * 1000));
}


void testSimple() 
{
    int amount = 100 * complexityMultiplier;

    Timer t;
    t.start();
    DispatchOperationGroupP group;
    Counter completed;
    std::vector<DispatchWork> works;
    for (int i = 0; i < amount; i++) {
        works.push_back([i, &completed](DispatchOperation *operation){
            if (operation->IsCancelled()) {
                return;
            }
            Dispatch::SharedDispatch()->PerformAndWait({}, nullptr);
            heavyCalculation(1000000);
            completed.increase();
            printf("complete %d!\n", i);
        });
    }
    bool finished = false;
    group = dispatch->PerformGroup(works, [&finished](DispatchOperation *operation) {
        finished = true;
    });
    
    int singleCount = 10 * complexityMultiplier;
    Counter singleFinished;
    for (int i = 0; i < singleCount; i++) {
        dispatch->PerformOnBackgroundQueue([&singleFinished, i](DispatchOperation *operation) {
            heavyCalculation(100000);
            singleFinished.increase();
            printf("completed single %d\n", i);
        });
    }
    
    while (group->IsExecuting() || singleFinished.get() != singleCount) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    t.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    printf("concurrent way took %d\n", (int)(t.duration() * 1000));
}


void testCancel()
{
    int amount = 120 * complexityMultiplier;
    Timer t;
    t.start();
    DispatchOperationGroupP group;
    Counter completed;
    
    static const int completeToCancel = 40;
    int completedWhenCanceled = 0;
    std::vector<DispatchWork> works;
    for (int i = 0; i < amount; i++) {
        works.push_back([i, &group, &completed, &completedWhenCanceled](DispatchOperation *operation){
            heavyCalculation(1000000);
            completed.increase();
            printf("complete %d!\n", i);
            
            if (i == completeToCancel) {
                completedWhenCanceled = completed.get();
                group->Cancel();
            }
        });
    }
    bool finished = false;
    group = dispatch->PerformGroup(works, [&finished](DispatchOperation *operation) {
        finished = true;
    });
    
    int singleCount = 0;
    Counter singleFinished;
    for (int i = 0; i < singleCount; i++) {
        dispatch->PerformOnBackgroundQueue([&singleFinished, i](DispatchOperation *operation) {
            heavyCalculation(100000);
            printf("completed single %d\n", i);
            singleFinished.increase();
        });
    }
    
    while (group->IsExecuting()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    t.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    printf("concurrent way took %d\n", (int)(t.duration() * 1000));
}




void testSingleNestedWait() 
{
    Timer t;
    t.start();
    
    Counter completed;
    Counter completedNested;
    
    Dispatch::SharedDispatch()->PerformOnBackgroundQueue([&completed, &completedNested](DispatchOperation *operation){
        if (operation->IsCancelled()) {
            return;
        }
        
        int nestedCount = 10 * complexityMultiplier;
        Counter nestedCompletedCount;
        std::vector<DispatchWork> worksNested;
        for (int in = 0; in < nestedCount; in++) {
            worksNested.push_back([in, &completedNested, &nestedCompletedCount](DispatchOperation *operation){
                heavyCalculation(100000);
                completedNested.increase();
                nestedCompletedCount.increase();
                printf("complete nested (%d)!\n", in);
            });
        }
        
        
        Dispatch::SharedDispatch()->PerformAndWait(worksNested, nullptr);
        completed.increase();
        assert(nestedCompletedCount.get() == nestedCount);
        printf("complete all %d!\n", nestedCompletedCount.get());
    });
    
    
    while (completed.get() != 1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    t.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    printf("concurrent way with nested took %d\n", (int)(t.duration() * 1000));
}



void testDoubleNestedWait() 
{
    Timer t;
    t.start();
    
    Counter completed;
    Counter completedNested;
    int count = 2 * complexityMultiplier;
    
    for (int i = 0; i < count; i++) {
        
        
        Dispatch::SharedDispatch()->PerformOnBackgroundQueue([i, &completed, &completedNested](DispatchOperation *operation){
            if (operation->IsCancelled()) {
                return;
            }
            
            int nestedCount = 2 * complexityMultiplier;
            Counter nestedCompletedCount;
            std::vector<DispatchWork> worksNested;
            for (int in = 0; in < nestedCount; in++) {
                worksNested.push_back([i, in, &completedNested, &nestedCompletedCount](DispatchOperation *operation){
                    heavyCalculation(100000);
                    completedNested.increase();
                    nestedCompletedCount.increase();
                    printf("complete nested %d (%d)!\n", i, in);
                });
            }
            
            
            Dispatch::SharedDispatch()->PerformAndWait(worksNested, nullptr);
            completed.increase();
            assert(nestedCompletedCount.get() == nestedCount);
            printf("complete all %d (%d)!\n", i, nestedCompletedCount.get());
        
        });
    }
    
    
    while (completed.get() != count) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    t.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    printf("concurrent way with nested took %d\n", (int)(t.duration() * 1000));
}



void testNestedWaitsFromGroup() 
{
    static const int amount = 500 * complexityMultiplier;
    static const int complexity = 1000;

    Timer t;
    t.start();
    DispatchOperationGroupP group;
    
    Counter completed;
    Counter completedNested;
    
    std::vector<DispatchWork> works;
    for (int i = 0; i < amount; i++) {
        bool isNested = i % 5 == 0;
        works.push_back([i, isNested, &completed, &completedNested](DispatchOperation *operation){
            if (operation->IsCancelled()) {
                return;
            }
            
            if (isNested) {
                const int nestedCount = 1000;
                 
                Counter nestedCompletedCount;
                
                std::vector<DispatchWork> worksNested;
                for (int in = 0; in < nestedCount; in++) {
                    worksNested.push_back([i, in, &completedNested, &nestedCompletedCount](DispatchOperation *operation){
                        heavyCalculation(complexity);
                        completedNested.increase();
                        nestedCompletedCount.increase();
                        printf("complete nested %d (%d)!\n", i, in);
                    });
                }
                
                Dispatch::SharedDispatch()->PerformAndWait(worksNested, nullptr);
                completed.increase();
                assert(nestedCompletedCount.get() == nestedCount);
                printf("complete %d (all %d)!\n", i, nestedCompletedCount.get());
            }
            else {
                heavyCalculation(complexity);
                completed.increase();
                printf("complete %d!\n", i);
            }
        });
    }
    bool finished = false;
    group = dispatch->PerformGroup(works, [&finished](DispatchOperation *operation) {
        finished = true;
    });
    
    while (group->IsExecuting()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    t.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    printf("concurrent way with nested from group took %d\n", (int)(t.duration() * 1000));
}




#if defined PROJECTN_TARGET_OS_WIN
int main()
#else
int main(int argc, char **argv)
#endif
{
    printf("--- Temperature: %f°C\n", Platform::CurrentPlatform()->GetCPUTemperature());
    createDispatch();
    
    printf("--- Test single operation\n");
    testSingleOperation();
    printf("--- Temperature: %f°C\n", Platform::CurrentPlatform()->GetCPUTemperature());
    
    printf("--- Test group and simple operations\n");
    testSimple();
    printf("--- Temperature: %f°C\n", Platform::CurrentPlatform()->GetCPUTemperature());
    
    printf("--- Test group with cancelling\n");
    testCancel();
    printf("--- Temperature: %f°C\n", Platform::CurrentPlatform()->GetCPUTemperature());
    
    printf("--- Test with single nested wait\n");
    testSingleNestedWait();
    printf("--- Temperature: %f°C\n", Platform::CurrentPlatform()->GetCPUTemperature());
    
    printf("--- Test with double nested waits\n");
    testDoubleNestedWait();
    printf("--- Temperature: %f°C\n", Platform::CurrentPlatform()->GetCPUTemperature());
    
    printf("--- Test with mix of groups and waits\n");
    testNestedWaitsFromGroup();
    printf("--- Temperature: %.03f°C\n", Platform::CurrentPlatform()->GetCPUTemperature());
    
    Dispatch *d = Dispatch::SharedDispatch();
    int threadsSpawned = DispatchThread::GetStatistic().GetSpawned();
    int a = 0;
    a++;
    
//    {
//        Timer t;
//        t.start();
//        std::vector<DispatchWork> works;
//        for (int i = 0; i < amount; i++) {   
//            heavyCalculation(1000000);
//            printf("complete %d!\n", i);
//        }
//        t.stop();
//        printf("sequantial way took %d\n", (int)(t.duration() * 1000));
//    }
    
    return 0;
}
