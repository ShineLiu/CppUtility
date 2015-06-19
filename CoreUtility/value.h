#pragma once

// simple value-initialization wrapper class
// so that when you declare value<int> m_data
// instead of just "naked" int m_data, you don't
// have to worry about it's uninitialization
template <typename T, T initVal = 0>
class value
{
public:
    value()
      : val(initVal)
    { }

    value(T t)
      : val(t)
    { }

    operator T() const
    { 
        return val; 
    }

    value& operator=(T t) 
    { 
        val = t; 
        return *this; 
    }

    T val;
};