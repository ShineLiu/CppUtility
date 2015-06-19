#pragma once

#include <memory>
#include "lock.h"
#include "noncopyable.h"

template <class T>
class ThreadLocal 
{
public:
    typedef T* (*pfnCreate)();

    T& Value(pfnCreate creator)
    {
        LockGuard<CriticalSection> lock(cs_);

        auto currThreadId = GetCurrentThreadId();

        if(threadLocalObjMap_.find(currThreadId) == threadLocalObjMap_.end())
        {
            threadLocalObjMap_[currThreadId].reset(creator());
        }

        return *threadLocalObjMap_[currThreadId];
    }

    CriticalSection  cs_;
    std::map<DWORD /* thread id */, std::unique_ptr<T>> threadLocalObjMap_;;
};

template <typename Derived>
class PerThreadSingleton : BingWallPaper::noncopyable
{
public:
    static Derived& Instance()
    {
        return threadSingletons_.Value(&PerThreadSingleton::Creator);
    }

protected:
    PerThreadSingleton() { }

    static Derived* Creator()
    {
        return new Derived;
    }

private:
    static ThreadLocal<Derived> threadSingletons_;
};

template <typename Derived>
ThreadLocal<Derived> PerThreadSingleton<Derived>::threadSingletons_;

template <typename Derived>
class Singleton : BingWallPaper::noncopyable
{
public:
    static Derived& Instance()
    {
        LockGuard<CriticalSection> guard(cs_);
        if(singleton_ == nullptr)
        {
            singleton_.reset(new Derived);
        }
        return *singleton_;
    }

private:
    static CriticalSection cs_;
    static std::unique_ptr<Derived> singleton_;
};

template <typename Derived>
CriticalSection Singleton<Derived>::cs_;

template <typename Derived>
std::unique_ptr<Derived> Singleton<Derived>::singleton_;
