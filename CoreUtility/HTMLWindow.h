#pragma once

#include <windows.h>
#include <ExDisp.h>
#include <ExDispid.h>
#include <mshtml.h>
#include <mshtmhst.h>
#include <MsHtmdid.h>
#include <oaidl.h>
#pragma warning(push)
#pragma warning(pop)
#pragma warning(disable: 4995)
#include <atlbase.h>
#include <atlsafe.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <comutil.h>
#pragma comment(lib, "comsuppw.lib")

#include "scope_guard.h"
#include "value.h"
#include "noncopyable.h"
#include "com_utility.h"
#include "error_handling_utility.h"
#include <Shellapi.h>

#define MUST_BE_IMPLEMENTED(s) return E_NOTIMPL;

//
// Modal HTML Window Message
//
static const UINT HTMLWIN_MODAL_RESPONSE = (WM_USER + 100);

using namespace ATL;

class HTMLWindow;

namespace detail
{
    class OleClientSite : public IOleClientSite, IServiceProvider, IInternetSecurityManager
    {
        IOleInPlaceSite    *in_place_;
        IDocHostUIHandler  *doc_host_ui_handler_;
        DWebBrowserEvents2 *web_browser_events_;

    public:

        OleClientSite (
            IOleInPlaceSite*    in_place,
            IDocHostUIHandler*  doc_host_ui_handler,
            DWebBrowserEvents2* web_browser_events)
            : in_place_ (in_place), doc_host_ui_handler_(doc_host_ui_handler), web_browser_events_ (web_browser_events ) 
        { }

        HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, _Deref_out_ void ** ppvObject)
        {
            if(IsEqualIID(riid, IID_IUnknown))
            {
                *ppvObject = static_cast<IUnknown*>(static_cast<IOleClientSite*>(this));
                return S_OK;
            } 
            else if(IsEqualIID(riid, IID_IOleClientSite))
            {
                *ppvObject = static_cast<IOleClientSite*>(this);
                return S_OK;
            }
            else if(IsEqualIID(riid, IID_IOleInPlaceSite)) 
            {
                *ppvObject = in_place_;
                return S_OK;
            }
            else if(IsEqualIID(riid, IID_IDocHostUIHandler)) 
            {
                *ppvObject = doc_host_ui_handler_;
                return S_OK;
            }
            else if(IsEqualIID(riid, DIID_DWebBrowserEvents2)) 
            {
                *ppvObject = web_browser_events_;
                return S_OK;
            }
            else if(IsEqualIID(riid, IID_IDispatch))
            {
                *ppvObject = static_cast<IDispatch*>(web_browser_events_);
                return S_OK;
            }
            else if(IsEqualIID(riid, IID_IServiceProvider))
            {
                *ppvObject = static_cast<IServiceProvider*>(this);
                return S_OK;
            }
            else 
            {
                *ppvObject = 0;
                return E_NOINTERFACE;
            }
        }

        HRESULT STDMETHODCALLTYPE QueryService( 
            /* [in] */ __in REFGUID guidService,
            /* [in] */ __in REFIID riid,
            /* [out] */ __deref_out void __RPC_FAR *__RPC_FAR *ppvObject)
		{
			int hr = E_NOINTERFACE;
			*ppvObject = 0;

			if (guidService == SID_SInternetSecurityManager && riid == IID_IInternetSecurityManager)
			{
				*ppvObject = static_cast<IInternetSecurityManager*>(this);
				hr = S_OK;
			}

			return hr;
		}

        //
        // IInternetSecurityManager
        //
        HRESULT STDMETHODCALLTYPE SetSecuritySite( 
            /* [unique][in] */ __RPC__in_opt IInternetSecurityMgrSite *)
        {
		    return INET_E_DEFAULT_ACTION;
        }
        
        HRESULT STDMETHODCALLTYPE GetSecuritySite( 
            /* [out] */ __RPC__deref_out_opt IInternetSecurityMgrSite **)
        {
            return INET_E_DEFAULT_ACTION;
        }
        
        HRESULT STDMETHODCALLTYPE MapUrlToZone( 
            /* [in] */ __RPC__in LPCWSTR,
            /* [out] */ __RPC__out DWORD *,
            /* [in] */ DWORD)
        {
		    return INET_E_DEFAULT_ACTION;
        }
        
        HRESULT STDMETHODCALLTYPE GetSecurityId( 
            /* [in] */ __RPC__in LPCWSTR,
            /* [size_is][out] */ __RPC__out_ecount_full(*pcbSecurityId) BYTE *,
            /* [out][in] */ __RPC__inout DWORD *,
            /* [in] */ DWORD_PTR)
        {
		    return INET_E_DEFAULT_ACTION;
        }
        
        HRESULT STDMETHODCALLTYPE ProcessUrlAction( 
            /* [in] */ __RPC__in LPCWSTR url,
            /* [in] */ DWORD dwAction,
            /* [size_is][out] */ __RPC__out_ecount_full(cbPolicy) BYTE *pPolicy,
            /* [in] */ DWORD cbPolicy,
            /* [unique][in] */ __RPC__in_opt BYTE * pContext,
            /* [in] */ DWORD cbContext,
            /* [in] */ DWORD,
            /* [in] */ DWORD)
        {
            UNREFERENCED_PARAMETER(url);
            UNREFERENCED_PARAMETER(dwAction);
            UNREFERENCED_PARAMETER(pContext);
            UNREFERENCED_PARAMETER(cbContext);
            UNREFERENCED_PARAMETER(pPolicy);

            if(cbPolicy < sizeof(DWORD))
            {
                return E_INVALIDARG;
            }
#ifdef _DEBUG
            //if(false)
#else
            if(dwAction == URLACTION_ACTIVEX_RUN && !IsXMLHTTP(pContext, cbContext))
            {
                *((DWORD*)pPolicy) = URLPOLICY_DISALLOW;
                return S_FALSE;
            }
#endif
            
            return INET_E_DEFAULT_ACTION;
        }

        bool IsXMLHTTP(BYTE* pContext, DWORD cbContext)
        {
            CLSID clsid_xmlhttp = { 0 };
            if(FAILED(CLSIDFromProgID(_T("Microsoft.XMLHTTP"), &clsid_xmlhttp)))
            {
                return false;
            }

            if(pContext == NULL)
            {
                return false;
            }

            if(cbContext < sizeof(CLSID))
            {
                return false;
            }

            return IsEqualCLSID(clsid_xmlhttp, *((CLSID*)pContext)) != 0;
        }
        
        HRESULT STDMETHODCALLTYPE QueryCustomPolicy( 
            /* [in] */ __RPC__in LPCWSTR,
            /* [in] */ __RPC__in REFGUID,
            /* [size_is][size_is][out] */ __RPC__deref_out_ecount_full_opt(*pcbPolicy) BYTE **,
            /* [out] */ __RPC__out DWORD *,
            /* [in] */ __RPC__in BYTE *,
            /* [in] */ DWORD,
            /* [in] */ DWORD)
        {
		    return INET_E_DEFAULT_ACTION;
        }
        
        HRESULT STDMETHODCALLTYPE SetZoneMapping( 
            /* [in] */ DWORD,
            /* [in] */ __RPC__in LPCWSTR,
            /* [in] */ DWORD)
        {
		    return INET_E_DEFAULT_ACTION;
        }
        
        HRESULT STDMETHODCALLTYPE GetZoneMappings( 
            /* [in] */ DWORD,
            /* [out] */ __RPC__deref_out_opt IEnumString **,
            /* [in] */ DWORD)
        {
		    return INET_E_DEFAULT_ACTION;
        }

        ULONG STDMETHODCALLTYPE AddRef() { return(1); }
        ULONG STDMETHODCALLTYPE Release() { return(1); }

        HRESULT STDMETHODCALLTYPE SaveObject() { MUST_BE_IMPLEMENTED("SaveObject") }
        HRESULT STDMETHODCALLTYPE GetMoniker( DWORD /*dwAssign*/, DWORD /*dwWhichMoniker*/, IMoniker ** /*ppmk*/) { MUST_BE_IMPLEMENTED("GetMoniker") }
        HRESULT STDMETHODCALLTYPE GetContainer( LPOLECONTAINER FAR* ppContainer)
        {
            *ppContainer = 0;
            return(E_NOINTERFACE);
        }

        HRESULT STDMETHODCALLTYPE ShowObject() { return(NOERROR); }
        HRESULT STDMETHODCALLTYPE OnShowWindow( BOOL /*fShow*/) { MUST_BE_IMPLEMENTED("OnShowWindow") }
        HRESULT STDMETHODCALLTYPE RequestNewObjectLayout() { MUST_BE_IMPLEMENTED("RequestNewObjectLayout") }
    };

    class Storage : public IStorage 
    {
        HRESULT STDMETHODCALLTYPE QueryInterface( REFIID /*riid*/, LPVOID FAR* /*ppvObj*/) { MUST_BE_IMPLEMENTED("QueryInterface") }
        ULONG STDMETHODCALLTYPE AddRef() { return(1); }
        ULONG STDMETHODCALLTYPE Release() { return(1); }
        HRESULT STDMETHODCALLTYPE SetClass( REFCLSID /*clsid*/) { return(S_OK); }

        HRESULT STDMETHODCALLTYPE CreateStream( const WCHAR * /*pwcsName*/, DWORD /*grfMode*/, DWORD /*reserved1*/, DWORD /*reserved2*/, IStream ** /*ppstm*/) { MUST_BE_IMPLEMENTED("CreateStream") }
        HRESULT STDMETHODCALLTYPE OpenStream( const WCHAR * /*pwcsName*/, void * /*reserved1*/, DWORD /*grfMode*/, DWORD /*reserved2*/, IStream ** /*ppstm*/) { MUST_BE_IMPLEMENTED("OpenStream") }
        HRESULT STDMETHODCALLTYPE CreateStorage( const WCHAR * /*pwcsName*/, DWORD /*grfMode*/, DWORD /*reserved1*/, DWORD /*reserved2*/, IStorage ** /*ppstg*/) { MUST_BE_IMPLEMENTED("CreateStorage") }
        HRESULT STDMETHODCALLTYPE OpenStorage( const WCHAR * /*pwcsName*/, IStorage * /*pstgPriority*/, DWORD /*grfMode*/, SNB /*snbExclude*/, DWORD /*reserved*/, IStorage ** /*ppstg*/) { MUST_BE_IMPLEMENTED("OpenStorage") }
        HRESULT STDMETHODCALLTYPE CopyTo( DWORD /*ciidExclude*/, IID const * /*rgiidExclude*/, SNB /*snbExclude*/,IStorage * /*pstgDest*/) { MUST_BE_IMPLEMENTED("CopyTo") }
        HRESULT STDMETHODCALLTYPE MoveElementTo( const OLECHAR * /*pwcsName*/,IStorage * /*pstgDest*/, const OLECHAR * /*pwcsNewName*/, DWORD /*grfFlags*/) { MUST_BE_IMPLEMENTED("MoveElementTo") }
        HRESULT STDMETHODCALLTYPE Commit( DWORD /*grfCommitFlags*/) { MUST_BE_IMPLEMENTED("Commit") }
        HRESULT STDMETHODCALLTYPE Revert() { MUST_BE_IMPLEMENTED("Revert") }
        HRESULT STDMETHODCALLTYPE EnumElements( DWORD /*reserved1*/, void * /*reserved2*/, DWORD /*reserved3*/, IEnumSTATSTG ** /*ppenum*/) { MUST_BE_IMPLEMENTED("EnumElements") }
        HRESULT STDMETHODCALLTYPE DestroyElement( const OLECHAR * /*pwcsName*/) { MUST_BE_IMPLEMENTED("DestroyElement") }
        HRESULT STDMETHODCALLTYPE RenameElement( const WCHAR * /*pwcsOldName*/, const WCHAR * /*pwcsNewName*/) { MUST_BE_IMPLEMENTED("RenameElement") }
        HRESULT STDMETHODCALLTYPE SetElementTimes( const WCHAR * /*pwcsName*/, FILETIME const * /*pctime*/, FILETIME const * /*patime*/, FILETIME const * /*pmtime*/) { MUST_BE_IMPLEMENTED("SetElementTimes") }
        HRESULT STDMETHODCALLTYPE SetStateBits( DWORD /*grfStateBits*/, DWORD /*grfMask*/) { MUST_BE_IMPLEMENTED("SetStateBits") }
        HRESULT STDMETHODCALLTYPE Stat( STATSTG * /*pstatstg*/, DWORD /*grfStatFlag*/) { MUST_BE_IMPLEMENTED("Stat") }
    };

    class OleInPlaceSite : public IOleInPlaceSite 
    {
    private:
        IOleClientSite*   ole_client_site_;
        IOleInPlaceFrame* ole_in_place_frame_;
        IOleObject*       browser_object_;
        HWND              hwnd_;

    public:
        OleInPlaceSite(IOleInPlaceFrame* ole_in_place_frame, HWND h)
            : ole_client_site_(0), ole_in_place_frame_ (ole_in_place_frame), browser_object_(0), hwnd_(h) 
        { }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
        {
            return ole_client_site_->QueryInterface(riid, ppvObj);
        }

        ULONG STDMETHODCALLTYPE AddRef() { return(1); }
        ULONG STDMETHODCALLTYPE Release() { return(1); }

        HRESULT STDMETHODCALLTYPE GetWindow(HWND FAR* lphwnd) 
        {
            *lphwnd = hwnd_;
            return(S_OK);
        }

        HRESULT STDMETHODCALLTYPE CanInPlaceActivate() { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnInPlaceActivate() { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnUIActivate() { return(S_OK); }
        HRESULT STDMETHODCALLTYPE OnUIDeactivate( BOOL /*fUndoable*/) { return(S_OK); }
        HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate() { return(S_OK); }

        HRESULT STDMETHODCALLTYPE GetWindowContext(
            LPOLEINPLACEFRAME FAR* lplpFrame, LPOLEINPLACEUIWINDOW FAR* lplpDoc, 
            LPRECT /*lprcPosRect*/, LPRECT /*lprcClipRect*/, LPOLEINPLACEFRAMEINFO lpFrameInfo) 
        {
            *lplpFrame = ole_in_place_frame_;

            // We have no OLEINPLACEUIWINDOW
            *lplpDoc = 0;

            // Fill in some other info for the browser
            lpFrameInfo->fMDIApp = FALSE;
            lpFrameInfo->hwndFrame = hwnd_;
            lpFrameInfo->haccel = 0;
            lpFrameInfo->cAccelEntries = 0;

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL /*fEnterMode*/) { MUST_BE_IMPLEMENTED("ContextSensitiveHelp") }
        HRESULT STDMETHODCALLTYPE Scroll(SIZE /*scrollExtent*/) { MUST_BE_IMPLEMENTED("Scroll") }
        HRESULT STDMETHODCALLTYPE DiscardUndoState() { MUST_BE_IMPLEMENTED("DiscardUndoState") }
        HRESULT STDMETHODCALLTYPE DeactivateAndUndo() { MUST_BE_IMPLEMENTED("DeactivateAndUndo") }

        // Called when the position of the browser object is changed, 
        // such as when we call the IWebBrowser2's put_Width(), put_Right().
        HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT lprcPosRect) 
        {
            IOleInPlaceObject	*inplace;
            if (browser_object_->QueryInterface(IID_IOleInPlaceObject, (void**)&inplace) == S_OK) 
            {
                inplace->SetObjectRects(lprcPosRect, lprcPosRect);
            }

            return S_OK;
        }

        void BrowserObject(IOleObject* o)
        {
            browser_object_ = o;
        }

        void ClientSite(IOleClientSite* o) { ole_client_site_ = o; }
    };

    class OleInPlaceFrame : public IOleInPlaceFrame 
    {
        HWND hHostWnd_;
    public:
        OleInPlaceFrame(HWND hHostWnd) : hHostWnd_(hHostWnd)
        { }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID /*riid*/, LPVOID FAR* /*ppvObj*/) { MUST_BE_IMPLEMENTED("QueryInterface") }
        ULONG STDMETHODCALLTYPE AddRef() { return 1; }
        ULONG STDMETHODCALLTYPE Release() { return 1; }

        HRESULT STDMETHODCALLTYPE SetActiveObject( IOleInPlaceActiveObject * /*pActiveObject*/, LPCOLESTR /*pszObjName*/) { return S_OK; }
        HRESULT STDMETHODCALLTYPE SetMenu( HMENU /*hmenuShared*/, HOLEMENU /*holemenu*/, HWND /*hwndActiveObject*/) { return(S_OK); }
        HRESULT STDMETHODCALLTYPE SetStatusText( LPCOLESTR /*pszStatusText*/) { return S_OK; }
        HRESULT STDMETHODCALLTYPE EnableModeless( BOOL /*fEnable*/) { return S_OK; }

        HRESULT STDMETHODCALLTYPE GetWindow(HWND FAR* lphwnd)
        {
            *lphwnd = hHostWnd_;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE ContextSensitiveHelp( BOOL /*fEnterMode*/) { MUST_BE_IMPLEMENTED("ContextSensitiveHelp") }
        HRESULT STDMETHODCALLTYPE GetBorder( LPRECT /*lprectBorder*/) { MUST_BE_IMPLEMENTED("GetBorder") }
        HRESULT STDMETHODCALLTYPE RequestBorderSpace( LPCBORDERWIDTHS /*pborderwidths*/) { MUST_BE_IMPLEMENTED("RequestBorderSpace") }
        HRESULT STDMETHODCALLTYPE SetBorderSpace( LPCBORDERWIDTHS /*pborderwidths*/) { MUST_BE_IMPLEMENTED("SetBorderSpace") }
        HRESULT STDMETHODCALLTYPE InsertMenus( HMENU /*hmenuShared*/, LPOLEMENUGROUPWIDTHS /*lpMenuWidths*/) { MUST_BE_IMPLEMENTED("InsertMenus") }
        HRESULT STDMETHODCALLTYPE RemoveMenus( HMENU /*hmenuShared*/) { MUST_BE_IMPLEMENTED("RemoveMenus") }
        HRESULT STDMETHODCALLTYPE TranslateAccelerator( LPMSG /*lpmsg*/, WORD /*wID*/) { MUST_BE_IMPLEMENTED("TranslateAccelerator") }
    };

    class DocHostUiHandler : public IDocHostUIHandler 
    {
        IOleClientSite* ole_client_site_;
        HTMLWindow*     html_window_;

    public:
        DocHostUiHandler(HTMLWindow* w)
            : ole_client_site_(0), html_window_(w)
        { }

        void ClientSite(IOleClientSite* o)
        {
            ole_client_site_ = o;
        }

        virtual ~DocHostUiHandler() { } 

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID FAR* ppvObj) 
        {
            if (ole_client_site_) 
            {
                return ole_client_site_->QueryInterface(riid, ppvObj);
            }

            return E_NOINTERFACE;
        }

        ULONG STDMETHODCALLTYPE AddRef() { return 1; }

        ULONG STDMETHODCALLTYPE Release() { return 1; }

        HRESULT STDMETHODCALLTYPE ShowContextMenu(
            DWORD                 /*dwID*/, 
            POINT     __RPC_FAR * /*ppt*/, 
            IUnknown  __RPC_FAR * /*pcmdtReserved*/, 
            IDispatch __RPC_FAR * /*pdispReserved*/)
        {
            // S_FALSE for showing context menu
            // see http://msdn.microsoft.com/en-us/library/aa753264.aspx
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE ShowUI(
            DWORD                               /*dwID*/, 
            IOleInPlaceActiveObject __RPC_FAR * /*pActiveObject*/, 
            IOleCommandTarget       __RPC_FAR * /*pCommandTarget*/, 
            IOleInPlaceFrame        __RPC_FAR * /*pFrame*/,
            IOleInPlaceUIWindow     __RPC_FAR * /*pDoc*/) 
        {
            return S_OK ;
        }

        HRESULT STDMETHODCALLTYPE GetHostInfo(DOCHOSTUIINFO __RPC_FAR *pInfo)
        {
            pInfo->cbSize = sizeof(DOCHOSTUIINFO);

            pInfo->dwFlags = 
                // DOCHOSTUIFLAG_DIALOG                     |
                // DOCHOSTUIFLAG_DISABLE_HELP_MENU          |
                DOCHOSTUIFLAG_NO3DBORDER                    |  
                (enableScrollBar_ ? 0 : DOCHOSTUIFLAG_SCROLL_NO) |  
                // DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE    |
                // DOCHOSTUIFLAG_OPENNEWWIN                 |
                // DOCHOSTUIFLAG_DISABLE_OFFSCREEN          |
                // DOCHOSTUIFLAG_FLAT_SCROLLBAR             |
                // DOCHOSTUIFLAG_DIV_BLOCKDEFAULT           |
                // DOCHOSTUIFLAG_ACTIVATE_CLIENTHIT_ONLY    |
                // DOCHOSTUIFLAG_OVERRIDEBEHAVIORFACTORY    |
                // DOCHOSTUIFLAG_CODEPAGELINKEDFONTS        |
                // DOCHOSTUIFLAG_URL_ENCODING_DISABLE_UTF8  |
                // DOCHOSTUIFLAG_URL_ENCODING_ENABLE_UTF8   |
                // DOCHOSTUIFLAG_ENABLE_FORMS_AUTOCOMPLETE  |
                // DOCHOSTUIFLAG_ENABLE_INPLACE_NAVIGATION  | 
                // DOCHOSTUIFLAG_IME_ENABLE_RECONVERSION    |
                // DOCHOSTUIFLAG_THEME                      |
                // DOCHOSTUIFLAG_NOTHEME                    |
                // DOCHOSTUIFLAG_NOPICS                     |
                DOCHOSTUIFLAG_NO3DOUTERBORDER               |
                0                                           ;

            // What happens if user double clicks?
            pInfo->dwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

            return S_OK ;
        }

        HRESULT STDMETHODCALLTYPE HideUI() { return S_OK; }
        HRESULT STDMETHODCALLTYPE UpdateUI() { return S_OK; }
        HRESULT STDMETHODCALLTYPE EnableModeless(BOOL /*fEnable*/) { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL /*fActivate*/) { return S_OK; }
        HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL /*fActivate*/) { return S_OK; }
        HRESULT STDMETHODCALLTYPE ResizeBorder(LPCRECT /*prcBorder*/, IOleInPlaceUIWindow __RPC_FAR * /*pUIWindow*/, BOOL /*fRameWindow*/) { return S_OK; }

        HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG /*lpMsg*/, const GUID __RPC_FAR * /*pguidCmdGroup*/, DWORD /*nCmdID*/)
        {
            return S_FALSE;
        }

        HRESULT STDMETHODCALLTYPE GetOptionKeyPath(LPOLESTR __RPC_FAR * /*pchKey*/, DWORD /*dw*/)
        {
            return S_FALSE;
        }

        HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget __RPC_FAR * /*pDropTarget*/, IDropTarget __RPC_FAR *__RPC_FAR * /*ppDropTarget*/) 
        {
            /*
            Return our IDropTarget object associated with this IDocHostUIHandler object. I don't
            know why we don't do this via QueryInterface(), but we don't.

            NOTE: If we want/need an IDropTarget interface, then we would have had to setup our own
            IDropTarget functions, IDropTarget VTable, and create an IDropTarget object. We'd want to put
            a pointer to the IDropTarget object in our own custom IDocHostUIHandlerEx object (like how
            we may add an HWND field for the use of ShowContextMenu). So when we created our
            IDocHostUIHandlerEx object, maybe we'd add a 'idrop' field to the end of it, and
            store a pointer to our IDropTarget object there. Then we could return this pointer as so:

            *pDropTarget = ((IDocHostUIHandlerEx FAR *)This)->idrop;
            return S_OK;

            But for our purposes, we don't need an IDropTarget object, so we'll tell whomever is calling
            us that we don't have one. */
            return S_FALSE;
        }

        HRESULT STDMETHODCALLTYPE GetExternal(IDispatch __RPC_FAR *__RPC_FAR *ppDispatch)
        {
            *ppDispatch = 0;
            return S_FALSE;
        }

        HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD /*dwTranslate*/, OLECHAR __RPC_FAR * /*pchURLIn*/, OLECHAR __RPC_FAR *__RPC_FAR * /*ppchURLOut*/)
        {
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE FilterDataObject(IDataObject __RPC_FAR * /*pDO*/, IDataObject __RPC_FAR *__RPC_FAR *ppDORet) 
        {
            /*
            Return our IDataObject object associated with this IDocHostUIHandler object. I don't
            know why we don't do this via QueryInterface(), but we don't.

            NOTE: If we want/need an IDataObject interface, then we would have had to setup our own
            IDataObject functions, IDataObject VTable, and create an IDataObject object. We'd want to put
            a pointer to the IDataObject object in our custom _IDocHostUIHandlerEx object (like how
            we may add an HWND field for the use of ShowContextMenu). So when we defined our
            _IDocHostUIHandlerEx object, maybe we'd add a 'idata' field to the end of it, and
            store a pointer to our IDataObject object there. Then we could return this pointer as so:

            *ppDORet = ((_IDocHostUIHandlerEx FAR *)This)->idata;
            return S_OK;

            But for our purposes, we don't need an IDataObject object, so we'll tell whomever is calling
            us that we don't have one. Note: We must set ppDORet to 0 if we don't return our own
            IDataObject object. */
            *ppDORet = 0;
            return S_FALSE;
        }

        void EnableScrollBar(bool enable)
        {
            enableScrollBar_ = enable;
        }

    private:
        value<bool, true> enableScrollBar_;
    };
}

class HTMLWindow : public DWebBrowserEvents2, BingWallPaper::noncopyable
{
public:
    // isMainWindow dictates whether this window will PostQuitMessage when closed
	HTMLWindow(HINSTANCE instance, std::wstring const& wndClsName, DWORD dwExStyle, DWORD dwStyle, HWND hwndOwner = HWND_DESKTOP, bool isMainWindow = true, bool isModalWindow = false, WNDPROC proc = NULL);
    HTMLWindow(HINSTANCE, std::wstring const& wndClsName, DWORD dwExStyle, DWORD dwStyle, bool isMainWindow, 
               HWND hwndOwner, bool isModalWindow, std::wstring const& title, int width, int height);
	~HTMLWindow();

public:
    void SetNavigationLoading(std::vector<HICON> rotationVec);
    void SetNavigationComplete(HICON icon);

    void HTML(std::wstring const& html_txt);
    void URL (std::wstring const& url);

	bool CallJsMethod(std::wstring strMethod, CComVariant* pVarResult = NULL);
	bool CallJsMethod(std::wstring strMethod, std::wstring strPara1, CComVariant* pVarResult = NULL);
	bool CallJsMethod(std::wstring strMethod, std::wstring strPara1, std::wstring strPara2, CComVariant* pVarResult = NULL);
	bool CallJsMethod(std::wstring strMethod, std::wstring strPara1, std::wstring strPara2, std::wstring strPara3, CComVariant* pVarResult = NULL);
	bool CallJsMethod(std::wstring strMethod, std::vector<std::wstring> paraArray, CComVariant* pVarResult = NULL);

    HWND GetEmbeddedBrowserHWND() const;
    CComPtr<IWebBrowser2> TryGetWebBrowser2() const;
    CComPtr<IWebBrowser2> GetWebBrowser2() const;
    std::wstring GetURL() const;
    std::wstring URLModal(std::wstring const& url);

    void EnableScrollBar(bool enable);
    void SupressScriptErrors(bool supress);
    READYSTATE GetReadyState();
    CComPtr<IHTMLDocument2> GetIHTMLDocument2();
	void InjectScriptObj(IDispatch* pScriptObj, std::wstring const& name);

    std::function<void()> OnNavigationComplete;
    std::function<void()> OnDocumentComplete;
    std::function<void()> OnNavigationFail;

	void NewWindow3(IDispatch** ppDisp, VARIANT_BOOL* Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl);

	HWND GetHWND() const;
	void SetWindowSize(int width, int height);
	void SetWindowText(std::wstring const& title);
	void SetIcon(HICON icon);
	void SetProp(std::wstring const& prop);
	void Show() const;
	void Hide() const;
	void MoveWindow(RECT);
	void CenterWindow(HWND hwndRelative = NULL);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static HTMLWindow* This(HWND hWnd);
private:
    void CommonInit(std::wstring const& wndClsName, DWORD dwExStyle, DWORD dwStyle, HWND hwndOwner, std::wstring const& title, int width, int height, WNDPROC proc = NULL);
    void EmbedBrowserObject();
    void SupressNavigationSound();
    void AddSink();
    void WriteBlankPageOnce();
    bool about_blank_written_once_;

    HINSTANCE           instance_;
    bool                isMainWindow_;
    bool                isModalWindow_;
    HWND				hwndOwner_;
    std::wstring		modalWinResult_;
    bool				modalWinOpen_;

    std::unique_ptr<detail::Storage>          storage_;
    std::unique_ptr<detail::OleInPlaceFrame>  ole_in_place_frame_;
    std::unique_ptr<detail::OleInPlaceSite>   ole_in_place_site_;
    std::unique_ptr<detail::DocHostUiHandler> doc_host_ui_handler_;
    std::unique_ptr<detail::OleClientSite>    ole_client_site_;
    CComPtr<IOleObject>                       browserObject_;

    std::vector<HICON>                        rotationVec_;
    value<int, -1>                            currRotation_;
    value<HICON, NULL>                        navComplete_;

private:
    void ResizeBrowser(HWND hwnd, DWORD width, DWORD height);
    HWND hwnd_;

private:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);
    ULONG STDMETHODCALLTYPE AddRef()  {return 1;}
    ULONG STDMETHODCALLTYPE Release() {return 1;}

    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(unsigned int FAR* pctinfo) 
    {
        *pctinfo = 0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTypeInfo(unsigned int /*iTInfo*/, LCID /*lcid*/, ITypeInfo FAR* FAR* ppTInfo) 
    {
        *ppTInfo = NULL;
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID /*riid*/, OLECHAR FAR* FAR* /*rgszNames*/, unsigned int /*cNames*/, LCID /*lcid*/, DISPID FAR* /*rgDispId*/)
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID, LCID, WORD wFlags, DISPPARAMS FAR*, VARIANT FAR* pVarResult,  
        EXCEPINFO    FAR*  pExcepInfo,  
        unsigned int FAR*  puArgErr);
};

inline HTMLWindow::HTMLWindow(HINSTANCE instance, std::wstring const& wndClsName, DWORD dwExStyle, DWORD dwStyle, HWND hwndOwner/* = HWND_DESKTOP*/, bool isMainWindow/* = true*/, bool isModalWindow/* = false*/, WNDPROC proc/* = NULL*/)
    : instance_(instance)
	, hwndOwner_(hwndOwner)
    , isMainWindow_(isMainWindow)
    , isModalWindow_(isModalWindow)
    , browserObject_(0)
    , about_blank_written_once_(false)
    , OnNavigationComplete([]{})
    , OnDocumentComplete([]{})
    , OnNavigationFail([]{})
{
    CommonInit(wndClsName, dwExStyle, dwStyle, hwndOwner, _T(""), 0, 0, proc);
#ifndef _DEBUG
    SupressScriptErrors(true);
#endif
}

inline HTMLWindow::HTMLWindow(HINSTANCE instance, std::wstring const& wndClsName, DWORD dwExStyle, DWORD dwStyle, bool isMainWindow, 
                              HWND hwndOwner, bool isModalWindow, std::wstring const& title, int width, int height)
    : instance_(instance)
    , isMainWindow_(isMainWindow)
    , isModalWindow_(isModalWindow)
    , hwndOwner_(hwndOwner)
    , browserObject_(0)
    , about_blank_written_once_(false)
    , OnNavigationComplete([]{})
    , OnDocumentComplete([]{})
    , OnNavigationFail([]{})
{
    CommonInit(wndClsName, dwExStyle, dwStyle, hwndOwner, title, width, height);
#ifndef _DEBUG
    SupressScriptErrors(true);
#endif
}

inline void HTMLWindow::CommonInit(std::wstring const& wndClsName, DWORD dwExStyle, DWORD dwStyle, HWND hwndOwner, std::wstring const& title, int width, int height, WNDPROC proc/* = NULL*/)
{
    WNDCLASSEX wc;
    {
        ::ZeroMemory (&wc, sizeof(WNDCLASSEX));
        wc.cbSize        = sizeof(WNDCLASSEX);
        wc.hInstance     = instance_;
        wc.lpfnWndProc   = proc != NULL? proc : WindowProc;
        wc.lpszClassName = wndClsName.c_str();
    }

    RegisterClassEx(&wc);
    
    hwnd_ = ::CreateWindowEx(dwExStyle, wndClsName.c_str(), title.c_str(), dwStyle, 
                             CW_USEDEFAULT, 0, width, height, hwndOwner, 0, instance_, 0);
	ENSURE(hwnd_ != NULL);

    SetWindowLongPtr(hwnd_, GWLP_USERDATA, (LONG_PTR)this);	
    
    EmbedBrowserObject();
    AddSink();

    ShowWindow(hwnd_, SW_HIDE);		
    UpdateWindow(hwnd_);

    SupressNavigationSound();
}

inline HTMLWindow::~HTMLWindow() 
{
    DestroyWindow(hwnd_);
}

inline HRESULT HTMLWindow::QueryInterface(REFIID riid, void ** ppvObject) 
{
    *ppvObject = ::QueryInterface(this, riid)
        .Support<IUnknown>()
        .Support<IDispatch>()
        .Support<DWebBrowserEvents2>();

    if(*ppvObject == 0)
    {
        *ppvObject = ::QueryInterface(doc_host_ui_handler_.get(), riid).Support<IDocHostUIHandler>();
    }

    if(*ppvObject)
    {
        return S_OK;
    }

    return E_NOINTERFACE;
}

inline void HTMLWindow::HTML(std::wstring const& html_txt) 
{
    WriteBlankPageOnce();

    CComPtr<IHTMLDocument2> htmlDoc2 = GetIHTMLDocument2();
	if (htmlDoc2 == NULL)
	{
		return;
	}
    ON_SCOPE_EXIT([=] { htmlDoc2->close(); });

    CComSafeArray<VARIANT> sa(/*elem count*/ 1);

    sa[0] = html_txt.c_str();

    htmlDoc2->write(sa);
}

inline void HTMLWindow::EmbedBrowserObject()
{
    const DWORD OLERENDER_DRAW = 1;

    storage_.reset(new detail::Storage);

    ole_in_place_frame_.reset(new detail::OleInPlaceFrame(hwnd_));
    ole_in_place_site_.reset(new detail::OleInPlaceSite(ole_in_place_frame_.get(), hwnd_));
    doc_host_ui_handler_.reset(new detail::DocHostUiHandler(this));

    ole_client_site_.reset(new detail::OleClientSite(
        ole_in_place_site_.get(), 
        doc_host_ui_handler_.get(),
        static_cast<DWebBrowserEvents2*>(this)));

    doc_host_ui_handler_->ClientSite(ole_client_site_.get());
    ole_in_place_site_->ClientSite(ole_client_site_.get());

    ENSURE(S_OK == ::OleCreate(CLSID_WebBrowser, IID_IOleObject,
        OLERENDER_DRAW, 0, ole_client_site_.get(), storage_.get(),
        (void**)&browserObject_)); 
	
    ScopeGuard closeBrowserOnException([&] { browserObject_->Close(OLECLOSE_NOSAVE); });
    {
        ole_in_place_site_->BrowserObject(browserObject_);
        browserObject_->SetHostNames(_T("BingWallPaperRichCandWnd"), 0);

        RECT rect;
        GetClientRect(hwnd_, &rect);

        CComPtr<IWebBrowser2> webBrowser2;
        // Let browser object know that it is embedded in an OLE container.
        ENSURE(OleSetContainedObject(browserObject_, TRUE) == S_OK &&
            browserObject_->DoVerb(OLEIVERB_SHOW, NULL, ole_client_site_.get(), -1, hwnd_, &rect) == S_OK &&
            browserObject_.QueryInterface(&webBrowser2) == S_OK);

        webBrowser2->put_Left  (0);
        webBrowser2->put_Top   (0);
        webBrowser2->put_Width (rect.right);
        webBrowser2->put_Height(rect.bottom);
    }
    closeBrowserOnException.Dismiss();
}

inline void HTMLWindow::SupressNavigationSound()
{
    /*const int FEATURE_DISABLE_NAVIGATION_SOUNDS = 21;
    const int SET_FEATURE_ON_PROCESS = 0x00000002;*/
    typedef HRESULT (STDAPICALLTYPE *fnptr_CoInternetSetFeatureEnabled)(int /* FeatureEntry */, DWORD /* dwFlags */, BOOL /* fEnable */);
	
	auto fnCoInternetSetFeatureEnabled = (fnptr_CoInternetSetFeatureEnabled) GetProcAddress(GetModuleHandle(_T("Urlmon")),"CoInternetSetFeatureEnabled");
	if (fnCoInternetSetFeatureEnabled != NULL)
	{
		fnCoInternetSetFeatureEnabled(21, 0x00000002, TRUE);
	}
}

inline void HTMLWindow::WriteBlankPageOnce()
{
    // this is necessary, otherwise the later get_Document(&lpDispatch) would fail
    if(!about_blank_written_once_)
    {
        URL(_T("about:blank"));
        about_blank_written_once_ = true;
    }
}

inline void HTMLWindow::ResizeBrowser(HWND /*hwnd*/, DWORD width, DWORD height) 
{
    CComPtr<IWebBrowser2> webBrowser2 = TryGetWebBrowser2();
    if(webBrowser2 == nullptr)
    {
        return;
    }

    webBrowser2->put_Width(width);
    webBrowser2->put_Height(height);
}

inline std::wstring HTMLWindow::GetURL() const
{
    CComPtr<IWebBrowser2> webBrowser2 = TryGetWebBrowser2();
    if(webBrowser2 == nullptr)
    {
        return nullptr;
    }
    BSTR url;
    webBrowser2->get_LocationURL(&url);
    std::wstring wstrVal((const WCHAR*) url);
    return wstrVal;
}

inline std::wstring HTMLWindow::URLModal(std::wstring const& url)
{
    URL(url);
    modalWinResult_ = _T("");
    modalWinOpen_ = true;

    if (isModalWindow_)
    {
        EnableWindow(hwndOwner_, false);
    }

    CComPtr<IOleInPlaceActiveObject> iOle;
    GetWebBrowser2().QueryInterface(&iOle);

    MSG msg;
    while (modalWinOpen_ && GetMessage(&msg, NULL, 0, 0))
    {
        bool isTranslated = iOle && iOle->TranslateAccelerator(&msg) == S_OK;
        if(!isTranslated)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return modalWinResult_;
}

inline void HTMLWindow::SetNavigationLoading(std::vector<HICON> rotation)
{
    rotationVec_ = std::move(rotation);
}

inline void HTMLWindow::SetNavigationComplete(HICON icon)
{
    navComplete_ = icon;
}

inline bool HTMLWindow::CallJsMethod(std::wstring strMethod, CComVariant* pVarResult)
{
	std::vector<std::wstring> paraArray;
	return CallJsMethod(strMethod, paraArray, pVarResult);
}

inline bool HTMLWindow::CallJsMethod(std::wstring strMethod, std::wstring strPara1, CComVariant* pVarResult)
{
	std::vector<std::wstring> paraArray;
	paraArray.push_back(strPara1);
	return CallJsMethod(strMethod, paraArray, pVarResult);
}

inline bool HTMLWindow::CallJsMethod(std::wstring strMethod, std::wstring strPara1, std::wstring strPara2, CComVariant* pVarResult)
{
	std::vector<std::wstring> paraArray;
	paraArray.push_back(strPara1);
	paraArray.push_back(strPara2);
	return CallJsMethod(strMethod, paraArray, pVarResult);
}

inline bool HTMLWindow::CallJsMethod(std::wstring strMethod, std::wstring strPara1, std::wstring strPara2, std::wstring strPara3, CComVariant* pVarResult)
{
	std::vector<std::wstring> paraArray;
	paraArray.push_back(strPara1);
	paraArray.push_back(strPara2);
	paraArray.push_back(strPara3);
	return CallJsMethod(strMethod, paraArray, pVarResult);
}

inline bool HTMLWindow::CallJsMethod(std::wstring strMethod, std::vector<std::wstring> paraArray, CComVariant* pVarResult)
{
	//get script IDispatch
	CComPtr<IHTMLDocument2> pDoc = GetIHTMLDocument2();
	if (pDoc == NULL)
	{
		// Failed to call.
		return false;
	}
	ON_SCOPE_EXIT([=] { if(pDoc != NULL){pDoc->close();} });

	CComPtr<IDispatch> pScriptDisp;
	pDoc->get_Script(&pScriptDisp);

	//get DISPID of methodName
	DISPID dispId = 0;
	if(FAILED(pScriptDisp->GetIDsOfNames(IID_NULL, &CComBSTR(strMethod.c_str()), 1, LOCALE_SYSTEM_DEFAULT, &dispId)))
	{
		return false;
	}

	//init params and result
	DISPPARAMS params;
	memset(&params, 0, sizeof params);
	const int arraySize = (int)paraArray.size();
	params.cArgs = arraySize;
	params.rgvarg = new VARIANT[params.cArgs];
	ON_SCOPE_EXIT([=] { delete[] params.rgvarg; });
	for ( int i = 0; i < arraySize; i++)
	{
		CComBSTR bstr = paraArray[arraySize - 1 - i].c_str(); // back reading
		bstr.CopyTo(&params.rgvarg[i].bstrVal);
		params.rgvarg[i].vt = VT_BSTR;
	}
	params.cNamedArgs = 0;

	CComVariant vaResult;

	EXCEPINFO excepInfo;
	memset(&excepInfo, 0, sizeof excepInfo);

	UINT nArgErr = (UINT)-1;  //initialize to invalid arg
	
	if (FAILED(pScriptDisp->Invoke(dispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &params, &vaResult, &excepInfo, &nArgErr)))
	{
		return false;
	}

	if (pVarResult)
	{
		*pVarResult = vaResult;
	}
	return true;
}

inline void HTMLWindow::InjectScriptObj(IDispatch* pScriptObj, std::wstring const& name)
{
    CComPtr<IHTMLDocument2> pDoc = GetIHTMLDocument2();
	if (pDoc == NULL)
	{
		return;
	}
    ON_SCOPE_EXIT([=] { pDoc->close(); });

    CComPtr<IDispatch> pScriptDisp;
    pDoc->get_Script(&pScriptDisp);

    CComQIPtr<IDispatchEx> pScriptDispEx = pScriptDisp;

    // fdexNameEnsure forces the property to be generated on the Window object in the scripting engine
    // see http://msdn.microsoft.com/en-us/library/sky96ah7.aspx
    DISPID dispId = 0;

    CComBSTR bstrName(name.c_str());
    HRESULT hr = pScriptDispEx->GetDispID(bstrName, fdexNameEnsure | fdexNameCaseSensitive, &dispId);

    //  Add the object
    {
        DISPID dispIdPut = DISPID_PROPERTYPUT;
        CComVariant vtArg(pScriptObj);

        DISPPARAMS params;
        {
            ZeroMemory(&params, sizeof(DISPPARAMS));
            params.cArgs = 1;
            params.rgvarg = &vtArg;
            params.cNamedArgs = 1;
            params.rgdispidNamedArgs = &dispIdPut;
        }

        EXCEPINFO ei;

        hr = pScriptDispEx->InvokeEx(dispId, LOCALE_USER_DEFAULT, 
            DISPATCH_PROPERTYPUT, &params, NULL, &ei, NULL);
    }
}

inline void HTMLWindow::AddSink() 
{
    assert(browserObject_ != NULL);
    
	CComPtr<IConnectionPointContainer> cpc;
	ENSURE(browserObject_.QueryInterface(&cpc) == S_OK);

    CComPtr<IConnectionPoint> cp;
    ENSURE(cpc->FindConnectionPoint(DIID_DWebBrowserEvents2, &cp) == S_OK);

    unsigned long cookie = 1;
    ENSURE(cp->Advise(static_cast<IDispatch*>(this), &cookie) == S_OK);
}

inline void HTMLWindow::NewWindow3(IDispatch** ppDisp, VARIANT_BOOL* Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl)
{
	*Cancel = TRUE;
	::ShellExecute(NULL, L"open", L"iexplore.exe", bstrUrl, NULL, SW_SHOWNORMAL);
}

const int TIMER_ID_ROTATION_ICON = 1;
inline LRESULT CALLBACK HTMLWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HTMLWindow* win = This(hwnd);

    switch (uMsg) 
    {
    case HTMLWIN_MODAL_RESPONSE:
		ENSURE(win != NULL);
        win->modalWinResult_ = (wchar_t*)lParam;
        win->modalWinOpen_ = false;
        break;
    case WM_SIZE: 
        if(win != nullptr)
        {
            win->ResizeBrowser(hwnd, LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
        break;
    case WM_TIMER:
        if(win != nullptr && wParam == TIMER_ID_ROTATION_ICON && !win->rotationVec_.empty())
        {
            win->currRotation_ = (win->currRotation_ + 1) % win->rotationVec_.size();
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)win->rotationVec_.at(win->currRotation_));
        }
        break;
	case WM_CLOSE: 
		if (win->isModalWindow_) // to avoid flickering when click the red X
		{
			EnableWindow(win->hwndOwner_, TRUE);
		}
		break;
    case WM_DESTROY:
		ENSURE(win != NULL);
        KillTimer(hwnd, TIMER_ID_ROTATION_ICON); 
        win->browserObject_->Close(OLECLOSE_NOSAVE);
        win->browserObject_.Release();
        if (win->isModalWindow_)
        {
			win->modalWinOpen_ = false;
			// often it's too late to enable owner window here, you should enable it earlier to avoid flickering
            EnableWindow(win->hwndOwner_, true); 
            SetActiveWindow(win->hwndOwner_);
        }
        if(win->isMainWindow_)
        {
            PostQuitMessage(0);
        }
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

inline CComPtr<IWebBrowser2> HTMLWindow::TryGetWebBrowser2() const
{
    return CComQIPtr<IWebBrowser2>(browserObject_);
}

inline CComPtr<IWebBrowser2> HTMLWindow::GetWebBrowser2() const
{
    CComPtr<IWebBrowser2> webBrowser2 = TryGetWebBrowser2();
	ENSURE(webBrowser2 != nullptr);
    return webBrowser2;
}

inline HWND HTMLWindow::GetHWND() const
{
    return hwnd_;
}

inline void HTMLWindow::SetWindowSize(int width, int height)
{
    RECT rc;
	::GetWindowRect(hwnd_, &rc);
    ::MoveWindow(hwnd_, rc.left, rc.top, width, height, TRUE);
}

inline void HTMLWindow::SetWindowText(std::wstring const& title)
{
    ::SetWindowText(hwnd_, title.c_str());
}

inline void HTMLWindow::SetIcon(HICON icon)
{
	::PostMessage(hwnd_, WM_SETICON, ICON_BIG, (LPARAM)icon);
	::PostMessage(hwnd_, WM_SETICON, ICON_SMALL, (LPARAM)icon);
}

inline void HTMLWindow::SetProp(std::wstring const& prop)
{
	::SetProp(hwnd_, prop.c_str(), hwnd_);
}

inline void HTMLWindow::Show() const
{
    ::SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, 
        SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

inline void HTMLWindow::Hide() const
{
    ::SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0, 
        SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_HIDEWINDOW);
}

inline void HTMLWindow::MoveWindow(RECT rect)
{
    ::MoveWindow(hwnd_, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);
}

inline void HTMLWindow::CenterWindow(HWND hwndRelative/* = NULL*/)
{
	RECT rectWindow, rectRelative;

	::GetWindowRect(hwnd_, &rectWindow);
	::GetWindowRect(hwndRelative == NULL ? ::GetDesktopWindow() : hwndRelative, &rectRelative);

	int nWidth = rectWindow.right - rectWindow.left;
	int nHeight = rectWindow.bottom - rectWindow.top;

	int nX = (rectRelative.left + rectRelative.right - nWidth) / 2;
	int nY = (rectRelative.top + rectRelative.bottom - nHeight) / 2 ;

	::MoveWindow(hwnd_, nX, nY, nWidth, nHeight, FALSE);
}


inline HTMLWindow* HTMLWindow::This(HWND hwnd)
{ 
    return reinterpret_cast<HTMLWindow* >(::GetWindowLongPtr(hwnd, GWLP_USERDATA)); 
}

inline HWND HTMLWindow::GetEmbeddedBrowserHWND() const
{
    CComQIPtr<IOleWindow> pOWin = browserObject_;

    HWND hBWnd;
	ENSURE(pOWin != NULL);
    ENSURE(pOWin->GetWindow(&hBWnd) == S_OK);
    return hBWnd;
}

inline void HTMLWindow::URL(std::wstring const& url) 
{
    CComPtr<IWebBrowser2> webBrowser2 = GetWebBrowser2();

    CComVariant myURL(url.c_str());

    webBrowser2->Navigate2(&myURL, 0, 0, 0, 0);
}

inline CComPtr<IHTMLDocument2> HTMLWindow::GetIHTMLDocument2()
{
    CComPtr<IWebBrowser2> webBrowser2 = GetWebBrowser2();

    CComPtr<IDispatch> lpDispatch;
    webBrowser2->get_Document(&lpDispatch);
	if (lpDispatch == NULL)
		return NULL;
    CComPtr<IHTMLDocument2> htmlDoc2;
    lpDispatch.QueryInterface(&htmlDoc2);
	if (htmlDoc2 == NULL)
		return NULL;

    return htmlDoc2;
}

inline void HTMLWindow::EnableScrollBar(bool enable)
{
    doc_host_ui_handler_->EnableScrollBar(enable);
}

inline void HTMLWindow::SupressScriptErrors(bool supress)
{
    GetWebBrowser2()->put_Silent(supress ? VARIANT_TRUE : VARIANT_FALSE);
}

inline READYSTATE HTMLWindow::GetReadyState()
{
    READYSTATE state;
    GetWebBrowser2()->get_ReadyState(&state);
    return state;
}

inline HRESULT HTMLWindow::Invoke( 
    DISPID             dispIdMember,      
    REFIID             riid,              
    LCID               lcid,                
    WORD               wFlags,              
    DISPPARAMS FAR*    pDispParams,  
    VARIANT FAR*       pVarResult,  
    EXCEPINFO FAR*     pExcepInfo,  
    unsigned int FAR*  puArgErr)
{
    UNREFERENCED_PARAMETER(pExcepInfo);
    UNREFERENCED_PARAMETER(puArgErr);
    UNREFERENCED_PARAMETER(pVarResult);
    UNREFERENCED_PARAMETER(wFlags);
    UNREFERENCED_PARAMETER(lcid);
    UNREFERENCED_PARAMETER(riid);

    switch(dispIdMember)
    {
    case DISPID_DOWNLOADBEGIN:
        SetTimer(hwnd_, TIMER_ID_ROTATION_ICON, 50, NULL);
        break;
    case DISPID_NAVIGATECOMPLETE2:
        OnNavigationComplete();
        break;
    case DISPID_DOCUMENTCOMPLETE:
        KillTimer(hwnd_, TIMER_ID_ROTATION_ICON);
        if(navComplete_)
        {
            SendMessage(hwnd_, WM_SETICON, ICON_SMALL, (LPARAM)(HICON)navComplete_);
        }
        OnDocumentComplete();
        break;
    case DISPID_NAVIGATEERROR:
        {
            CComVariant cancel(true);
            cancel.Detach(&pDispParams->rgvarg[0]);
            OnNavigationFail();
        }
        break;
	case DISPID_NEWWINDOW3:
		{
			NewWindow3(
				pDispParams->rgvarg[4].ppdispVal,
				pDispParams->rgvarg[3].pboolVal,
				pDispParams->rgvarg[2].uintVal,
				pDispParams->rgvarg[1].bstrVal,
				pDispParams->rgvarg[0].bstrVal);
		}
		break;
	case DISPID_AMBIENT_DLCONTROL:
		// supress ActiveX
		V_VT(pVarResult) = VT_I4;
		V_I4(pVarResult) = DLCTL_DLIMAGES                  |  
                           DLCTL_VIDEOS                    |  
                           DLCTL_BGSOUNDS                  |  
                         //DLCTL_NO_SCRIPTS                |  
                           DLCTL_NO_JAVA                   |
#ifdef _DEBUG
                           0;
#else
                           DLCTL_NO_RUNACTIVEXCTLS         |  
                           DLCTL_NO_DLACTIVEXCTLS          ;
#endif
                         //DLCTL_DOWNLOADONLY              |  
                         //DLCTL_NO_FRAMEDOWNLOAD          |  
                         //DLCTL_RESYNCHRONIZE             |  
                         //DLCTL_PRAGMA_NO_CACHE           |  
                         //DLCTL_NO_BEHAVIORS              |  
                         //DLCTL_NO_METACHARSET            |  
                         //DLCTL_URL_ENCODING_DISABLE_UTF8 |  
                         //DLCTL_URL_ENCODING_ENABLE_UTF8  |  
                         //DLCTL_NOFRAMES                  |  
                         //DLCTL_FORCEOFFLINE              |  
                         //DLCTL_NO_CLIENTPULL             |  
                         //DLCTL_SILENT                    |  
                         //DLCTL_OFFLINEIFNOTCONNECTED     |  
                         //DLCTL_OFFLINE                   ;
		break;
	default:
		return DISP_E_MEMBERNOTFOUND;
    }
    return S_OK;
}
