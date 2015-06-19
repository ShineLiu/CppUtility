#pragma once

#include <Guiddef.h>
#include <map>
#include <functional>
#include <algorithm>

template <typename T>
struct InterfaceQuery
{
    InterfaceQuery(T* p, REFIID iid)
        : p_(p), iid_(&iid)
    { }

    operator void*()
    {
        auto found = std::find_if(map_.begin(), map_.end(), [&](TMap::const_reference p) { return IsEqualIID(*p.first, *iid_); });
        if(found != map_.end())
        {
            return (found->second)();
        }
        return nullptr;
    }

    template <typename TInterface>
    InterfaceQuery<T>& Support(REFIID iid)
    {
        map_.insert(std::make_pair(&iid, [=] { return static_cast<TInterface*>(p_); }));
        return *this;
    }

    template <typename TInterface>
    InterfaceQuery<T>& Support()
    {
        return Support<TInterface>(__uuidof(TInterface));
    }

private:
    T* p_;
    IID const* iid_;
    typedef std::map<IID const*, std::function<void*()>> TMap;
    TMap map_;
};

template <typename T>
inline InterfaceQuery<T> QueryInterface(_In_ T* this_, REFIID iid)
{
    return InterfaceQuery<T>(this_, iid);
}