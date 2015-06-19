#pragma once

template <typename T>
class Atomic
{
public:
    Atomic() : value_()
    {
        InitializeCriticalSection(&guard_);
    }

    explicit Atomic(T val)
        : value_(val)
    {
        InitializeCriticalSection(&guard_);
    }

    ~Atomic()
    {
        DeleteCriticalSection(&guard_);
    }

    operator T() const
    {
        T copy;
        EnterCriticalSection(&guard_);
        copy = value_;
        LeaveCriticalSection(&guard_);
        return std::move(copy);
    }

    Atomic& operator = (T const& newVal)
    {
        EnterCriticalSection(&guard_);
        value_ = newVal;
        LeaveCriticalSection(&guard_);
        
        return *this;
    }
private:
    mutable CRITICAL_SECTION guard_;
    T value_;

private:
    Atomic(Atomic const&);
    void operator = (Atomic const&);
};
