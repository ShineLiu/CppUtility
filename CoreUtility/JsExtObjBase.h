#pragma once

#include <windows.h>
#pragma warning(push)
#pragma warning(disable: 4995)
#include <atlcomcli.h>
#include <comutil.h>
#pragma warning(pop)

#include <map>
#include <utility>
#include <functional>
#include <string>
#include <type_traits>
#include "error_handling_utility.h"

#pragma comment(lib, "comsuppw.lib")

namespace BingWallPaper { namespace detail {

    class CComVariantExtended : public CComVariant
    {
    public:
        CComVariantExtended(std::wstring const& str)
            : CComVariant(str.c_str())
        { }

        CComVariantExtended(std::string const& str)
            : CComVariant(str.c_str())
        { }

        template <typename T>
        CComVariantExtended(T const& t)
            : CComVariant(t)
        { }
    };

    template <typename R, typename Func>
    inline void CallAndReturn(VARIANT* result, Func func, R*)
    {
        if(result == NULL)
        {
            func();
        }
        else
        {
            VariantInit(result);
            CComVariantExtended(func()).Detach(result);
        }
    }

    template <typename Func>
    inline void CallAndReturn(VARIANT* result, Func func, void*)
    {
		UNREFERENCED_PARAMETER(result);
        func();
    } 

} }

#define DISPATCH_THUNK_IMPL(num, args)   \
    try                                  \
    {                                    \
        if(params->cArgs != num)         \
        {                                \
            return E_FAIL;               \
        }                                \
                                         \
        detail::CallAndReturn(result, [=] { return (obj->*pmf)args; }, (R*)0);    \
        return S_OK;                     \
    }                                    \
    catch(...)                           \
    {                                    \
        return E_FAIL;                   \
    }        

namespace BingWallPaper { namespace detail {

    struct _extend_variant_t : _variant_t
    {
        template <typename T>
        _extend_variant_t(T const& t)
            : _variant_t(t)
        { }

        operator std::wstring() const
        {
            return (wchar_t*)(_bstr_t)*this;
        }
    };

    template <typename T, typename R>
    inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)())
    {
        return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
        {
            DISPATCH_THUNK_IMPL(0, ())
        };
    }

    template <typename T, typename R, typename A1>
    inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1))
    {
        return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
        {
            DISPATCH_THUNK_IMPL(1, (_extend_variant_t(params->rgvarg[0])))
        };
    }

    template <typename T, typename R, typename A1, typename A2>
    inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2))
    {
        return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
        {
            // Arguments are stored in rgvarg in reverse order
            // see MSDN doc on IDispatch::Invoke
            DISPATCH_THUNK_IMPL(2, (_extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
        };
    }

    template <typename T, typename R, typename A1, typename A2, typename A3>
    inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3))
    {
        return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
        {
            // Arguments are stored in rgvarg in reverse order
            // see MSDN doc on IDispatch::Invoke
            DISPATCH_THUNK_IMPL(3, (_extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
        };
    }

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(4, (_extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(5, (_extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5, A6))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(6, (_extend_variant_t(params->rgvarg[5]), _extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5, A6, A7))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(7, (_extend_variant_t(params->rgvarg[6]), _extend_variant_t(params->rgvarg[5]), _extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5, A6, A7, A8))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(8, (_extend_variant_t(params->rgvarg[7]), _extend_variant_t(params->rgvarg[6]), _extend_variant_t(params->rgvarg[5]), _extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5, A6, A7, A8, A9))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(9, (_extend_variant_t(params->rgvarg[8]), _extend_variant_t(params->rgvarg[7]), _extend_variant_t(params->rgvarg[6]), _extend_variant_t(params->rgvarg[5]), _extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(10, (_extend_variant_t(params->rgvarg[9]),_extend_variant_t(params->rgvarg[8]), _extend_variant_t(params->rgvarg[7]), _extend_variant_t(params->rgvarg[6]), _extend_variant_t(params->rgvarg[5]), _extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(11, (_extend_variant_t(params->rgvarg[10]),_extend_variant_t(params->rgvarg[9]),_extend_variant_t(params->rgvarg[8]), _extend_variant_t(params->rgvarg[7]), _extend_variant_t(params->rgvarg[6]), _extend_variant_t(params->rgvarg[5]), _extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(12, (_extend_variant_t(params->rgvarg[11]),_extend_variant_t(params->rgvarg[10]),_extend_variant_t(params->rgvarg[9]),_extend_variant_t(params->rgvarg[8]), _extend_variant_t(params->rgvarg[7]), _extend_variant_t(params->rgvarg[6]), _extend_variant_t(params->rgvarg[5]), _extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(13, (_extend_variant_t(params->rgvarg[12]),_extend_variant_t(params->rgvarg[11]),_extend_variant_t(params->rgvarg[10]),_extend_variant_t(params->rgvarg[9]),_extend_variant_t(params->rgvarg[8]), _extend_variant_t(params->rgvarg[7]), _extend_variant_t(params->rgvarg[6]), _extend_variant_t(params->rgvarg[5]), _extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(14, (_extend_variant_t(params->rgvarg[13]),_extend_variant_t(params->rgvarg[12]),_extend_variant_t(params->rgvarg[11]),_extend_variant_t(params->rgvarg[10]),_extend_variant_t(params->rgvarg[9]),_extend_variant_t(params->rgvarg[8]), _extend_variant_t(params->rgvarg[7]), _extend_variant_t(params->rgvarg[6]), _extend_variant_t(params->rgvarg[5]), _extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

	template <typename T, typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9, typename A10, typename A11, typename A12, typename A13, typename A14, typename A15>
	inline std::function<HRESULT(DISPPARAMS*, VARIANT* /* result */)> MakeThunk(T* obj, R (T::*pmf)(A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15))
	{
		return [=](DISPPARAMS* params, VARIANT* result) -> HRESULT
		{
			// Arguments are stored in rgvarg in reverse order
			// see MSDN doc on IDispatch::Invoke
			DISPATCH_THUNK_IMPL(15, (_extend_variant_t(params->rgvarg[14]),_extend_variant_t(params->rgvarg[13]),_extend_variant_t(params->rgvarg[12]),_extend_variant_t(params->rgvarg[11]),_extend_variant_t(params->rgvarg[10]),_extend_variant_t(params->rgvarg[9]),_extend_variant_t(params->rgvarg[8]), _extend_variant_t(params->rgvarg[7]), _extend_variant_t(params->rgvarg[6]), _extend_variant_t(params->rgvarg[5]), _extend_variant_t(params->rgvarg[4]), _extend_variant_t(params->rgvarg[3]), _extend_variant_t(params->rgvarg[2]), _extend_variant_t(params->rgvarg[1]), _extend_variant_t(params->rgvarg[0])))
		};
	}

} }

#define DISP_FUNC(methodName) \
    AddDispatchEntry(L###methodName, [this](DISPPARAMS* params, VARIANT* result) { return this->methodName(params, result); })

#define DISP_FUNC2(className, methodName) \
	AddDispatchEntry(L###methodName, BingWallPaper::detail::MakeThunk(this, &className::methodName))

class JsExtObjBase : public IDispatch
{
protected:
    JsExtObjBase()
        : nextAvailDispId_(1)
    { }

protected:
    typedef std::function<HRESULT(DISPPARAMS* /* params */, VARIANT* /* result */)> TMethod;
    
    std::map<std::wstring, int> mapMethodNameToDispId_;
    std::map<int, TMethod> mapDispIdToMethod_;
    int nextAvailDispId_;

    void AddDispatchEntry(std::wstring const& methodName, TMethod pMemFunc)
    {
        AddDispatchEntry(methodName, nextAvailDispId_++, pMemFunc);
    }

    void AddDispatchEntry(std::wstring const& methodName, int dispId, TMethod pMemFunc)
    {
        ENSURE(mapMethodNameToDispId_.find(methodName) == mapMethodNameToDispId_.end())(dispId)(methodName);
        ENSURE(mapDispIdToMethod_.find(dispId) == mapDispIdToMethod_.end())(dispId)(methodName);
        mapMethodNameToDispId_[methodName] = dispId;
        mapDispIdToMethod_[dispId] = pMemFunc;
    }

private: // IDispatch & IUnknown
    STDMETHODIMP_(DWORD) AddRef()
    {
        return 1;
    }

    STDMETHODIMP_(DWORD) Release()
    {
        return 1;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        if(riid == IID_IUnknown)
        {
            *ppv = (IUnknown*)this;
            return S_OK;
        }
        else if(riid == IID_IDispatch)
        {
            *ppv = (IDispatch*)this;
            return S_OK;
        }
        else
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }
    }

    STDMETHODIMP GetTypeInfoCount( UINT *pctinfo)
    {
        *pctinfo = 0;
        return S_OK;
    }

    STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
    {
		UNREFERENCED_PARAMETER(lcid);
		UNREFERENCED_PARAMETER(iTInfo);
        *ppTInfo = NULL;
        return E_NOTIMPL;
    }

    STDMETHODIMP GetIDsOfNames( REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
    {
		UNREFERENCED_PARAMETER(lcid);
		UNREFERENCED_PARAMETER(riid);
        // we only support one name at a time.
        if(cNames > 1)
            return E_INVALIDARG;

        if(mapMethodNameToDispId_.find(*rgszNames) != mapMethodNameToDispId_.end())
        {
            *rgDispId = mapMethodNameToDispId_[*rgszNames];
            return S_OK;
        }
        else
        {
            return DISP_E_UNKNOWNNAME;
        }
    }

    STDMETHODIMP Invoke(
        DISPID dispIdMember,          // DISPID of dispinterface member.
        REFIID riid,                  // Reserved.
        LCID lcid,                    // Locality.
        WORD wFlags,                  // Are we calling a method or property?
        DISPPARAMS *pDispParams,      // Any arguments for the method.
        VARIANT *pVarResult,          // A place to hold the logical return value.
        EXCEPINFO *pExcepInfo,        // Any error information?
        UINT *puArgErr)               // Again, any error information?
    {
		UNREFERENCED_PARAMETER(puArgErr);
		UNREFERENCED_PARAMETER(pExcepInfo);
		UNREFERENCED_PARAMETER(wFlags);
		UNREFERENCED_PARAMETER(lcid);
		UNREFERENCED_PARAMETER(riid);
        auto found = mapDispIdToMethod_.find(dispIdMember);
        if(found != mapDispIdToMethod_.end())
        {
            return (found->second)(pDispParams, pVarResult);
        }
        else
        {
            return DISP_E_UNKNOWNINTERFACE;
        }
    }
};
