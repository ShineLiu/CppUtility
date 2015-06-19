#pragma once

#include <Windows.h>
#include <string>
#include <map>
#include <memory>
#include "error_handling_utility.h"
#include "noncopyable.h"

class Mutex : BingWallPaper::noncopyable
{
public:
    Mutex(std::wstring const& name)
    {
        hMutex_ = CreateMutex(NULL, FALSE, name.c_str());
    }

    HANDLE Handle()
    {
        return hMutex_;
    }

    void Lock()
    {
        ENSURE(::WaitForSingleObject(hMutex_, INFINITE) == WAIT_OBJECT_0);
    }

    void Unlock()
    {
        ::ReleaseMutex(hMutex_);
    }

private:
    HANDLE hMutex_;
};

class CriticalSection : BingWallPaper::noncopyable
{
public:
    CriticalSection()
    {
        InitializeCriticalSection(&cs_);
    }

    void Lock()
    {
        EnterCriticalSection(&cs_);
    }

    void Unlock()
    {
        LeaveCriticalSection(&cs_);
    }

    ~CriticalSection()
    {
        DeleteCriticalSection(&cs_);
    }

private:
    CRITICAL_SECTION cs_;
};

template <typename TLock>
class LockGuard : BingWallPaper::noncopyable
{
public:
    LockGuard(TLock& lock)
        : lock_(lock)
    {
        lock_.Lock();
    }

    ~LockGuard()
    {
        lock_.Unlock();
    }

private:
    TLock& lock_;
};

template <typename TLock>
inline void Lock(TLock& lock, std::function<void()> func)
{
    LockGuard<TLock> guard(lock);
    func();
}
