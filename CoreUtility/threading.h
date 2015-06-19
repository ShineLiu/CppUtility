#pragma once

#include <stdexcept>
#include <process.h>
#include <memory>
#include <Windows.h>
#include "error_handling_utility.h"
#include "noncopyable.h"

class thread
{
public:
    typedef std::function<void()> callback_t;

    thread(callback_t const& f)
    {
        thrd_ = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &thread::Run, new callback_t(f), 0, NULL));
        ENSURE(thrd_ != 0);
    }

    void join()
    {
        ENSURE(joinable());
        WaitForSingleObject(thrd_, INFINITE);
        CloseHandle(thrd_);
        thrd_ = 0;
    }

    bool joinable()
    {
        return thrd_ != 0;
    }

    void detach()
    {
        CloseHandle(thrd_);
        thrd_ = 0;
    }

    ~thread()
    {
        ENSURE(!joinable());
    }

private:
    static unsigned __stdcall Run(_In_ void* arg)
    {
        std::unique_ptr<callback_t> f(static_cast<callback_t*>(arg));
        (*f)();

        _endthreadex(0);
        return 0;
    }

private:
    HANDLE thrd_;

private:
    thread(thread const&);
    thread& operator=(thread const&);
};

inline void async(std::function<void()> callback)
{
    thread t(callback);
    t.detach();
}
