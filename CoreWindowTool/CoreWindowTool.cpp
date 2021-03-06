// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "CoreWindowTool.h"
#include <ole2.h>
#include <uiautomation.h>
#include <strsafe.h>

template <class T>
class MyComPtr
{
public:
    typedef T InterfaceType;

    static_assert(__is_base_of(IUnknown, InterfaceType), "Invalid cast: InterfaceType does not derive from IUnknown");

public:

    MyComPtr() : _pUnk(nullptr)
    {
    }

    MyComPtr(T* pUnk) : _pUnk(pUnk)
    {
        InternalAdd();
    }

    MyComPtr(const MyComPtr& myComp)
    {
        _pUnk = myComp->_pUnk;
        InternalAdd();
    }

    MyComPtr& operator= (const MyComPtr& other)
    {
        if (this != &other)
        {
            InternalRelease();
            _pUnk = other._pUnk;
            InternalAdd();
        }
        return *this;
    }

    ~MyComPtr()
    {
        InternalRelease();
    }

    InterfaceType* operator-> () throw()
    {
        return _pUnk;
    }

    InterfaceType** operator& () throw()
    {
        return &_pUnk;
    }

    InterfaceType** GetAddressOf() throw()
    {
        return &_pUnk;
    }

    InterfaceType* Detach() throw()
    {
        InterfaceType* local = _pUnk;
        _pUnk = nullptr;
        return local;
    }

    operator InterfaceType*()
    {
        return _pUnk;
    }

private:
    void InternalRelease()
    {
        if (_pUnk != nullptr)
        {
            _pUnk->Release();
            _pUnk = nullptr;
        }
    }

    void InternalAdd()
    {
        if (_pUnk != nullptr)
        {
            _pUnk->AddRef();
        }
    }

private:
    InterfaceType* _pUnk;
};

MyComPtr<IUIAutomation> g_automation;

// Will search an element itself and all its children and descendants for the first element that supports Text Pattern 2
HRESULT FindCoreWindow(_In_ IUIAutomationElement *element, _Outptr_result_maybenull_ IUIAutomationElement **coreWindowElement)
{
    HRESULT hr = S_OK;

    // Create a condition that will be true for anything that supports Text Pattern 2
    VARIANT trueVar;
    trueVar.vt = VT_BOOL;
    trueVar.boolVal = VARIANT_TRUE;
    // hr = g_automation->CreatePropertyCondition(UIA_IsTextPattern2AvailablePropertyId, trueVar, &textPatternCondition);

    MyComPtr<IUIAutomationTreeWalker> rawWalker;

    hr = g_automation->get_RawViewWalker(&rawWalker);

    if (SUCCEEDED(hr))
    {
        MyComPtr<IUIAutomationElement> node = element;

        while (node)
        {
            MyComPtr<IUIAutomationElement> parent;

            hr = rawWalker->GetParentElement(node, &parent);

            if (SUCCEEDED(hr))
            {
                BSTR className = nullptr;

                hr = node->get_CurrentClassName(&className);
                if (SUCCEEDED(hr) && className != nullptr)
                {
                    if (_wcsicmp(className, L"Windows.UI.Core.CoreWindow") == 0)
                    {
                        break;
                    }
                    SysFreeString(className);
                }

                node = parent;
            }
        }

        *coreWindowElement = node.Detach();
        if (*coreWindowElement == nullptr)
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

void Usage()
{
    wprintf(L"Usage:\n\n");
    wprintf(L"CoreWindowTool -dpi \n");
    wprintf(L"    Show DPI and bounds information by using Win32 APIs for CoreWindow \n");
    wprintf(L"CoreWindowTool -message <Hex message id, 0x10> \n");
    wprintf(L"    Send a given message, no params, to the CoreWindow \n");
}

enum class Argument
{
    None = 0,
    Dpi = 0x01,
    Message = 0x02
};

int _cdecl wmain(_In_ int argc, _In_reads_(argc) WCHAR* argv[])
{
    UNREFERENCED_PARAMETER(argv);

    if (argc < 2)
    {
        Usage();
        return 0;
    }

    PCWSTR arg = argv[1];

    // Initialize COM before using UI Automation
    HRESULT hr = CoInitialize(nullptr);
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(__uuidof(CUIAutomation8), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&g_automation));
        if (SUCCEEDED(hr))
        {
            Argument argument = Argument::None;
            if (_wcsicmp(arg, L"-dpi") == 0)
            {
                argument = Argument::Dpi;
            }
            else if (_wcsicmp(arg, L"-message") == 0)
            {
                argument = Argument::Message;
            }
            else
            {
                Usage();
                return 1;
            }

            wprintf(L"Getting element at cursor in 3 seconds...\n");
            Sleep(3000);

            MyComPtr<IUIAutomationElement> element;

            POINT pt;
            GetCursorPos(&pt);
            hr = g_automation->ElementFromPoint(pt, &element);
            if (SUCCEEDED(hr))
            {
                MyComPtr<IUIAutomationElement> coreWindowElement;
                hr = FindCoreWindow(element, &coreWindowElement);
                if (SUCCEEDED(hr) && coreWindowElement != nullptr)
                {
                    UIA_HWND hwnd;

                    hr = coreWindowElement->get_CurrentNativeWindowHandle(&hwnd);
                    if (SUCCEEDED(hr))
                    {
                        switch (argument)
                        {
                        case Argument::Dpi:
                            DumpDpi((HWND)hwnd);
                            break;

                        case Argument::Message:
                            if (argc > 2)
                            {
                                int message = wcstoul(argv[2], nullptr, 16);
                                SendMessage((HWND)hwnd, message, 0, 0);
                            }
                            else
                            {
                                Usage();
                            }
                            break;
                        }
                    }
                }
            }
        }
		else
		{
			wprintf(L"Failed to create a CUIAutomation8, HR: 0x%08x\n", hr);
		}

        CoUninitialize();
    }
    else
    {
        wprintf(L"CoInitialize failed, HR:0x%08x\n", hr);
    }

    return 0;
}

