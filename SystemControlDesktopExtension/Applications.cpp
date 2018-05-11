//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "Applications.h"
#include <collection.h>
#include <windows.h>
#include <Shellapi.h>
#include <shlobj.h>
#include <propkey.h>
#include <string>
#include <sstream>     

using namespace Windows::Storage;
using namespace Windows::Foundation::Collections;
using namespace Platform::Collections;

std::mutex Applications::s_mutex;

HRESULT LaunchAppFromShortCut(IShellItem* psi)
{
    HRESULT hr;
    IShellLink* psl;

    StorageFolder^ localFolder = ApplicationData::Current->LocalFolder;
    Platform::String^ path = localFolder->Path + L"\\shortcut.lnk";

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
    // has already been called.
    hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hr))
    {
        IPersistFile* ppf = nullptr;

        PIDLIST_ABSOLUTE pidl;

        hr= SHGetIDListFromObject(psi, &pidl);
        if (SUCCEEDED(hr))
        {
            hr = psl->SetIDList(pidl);
            CoTaskMemFree(pidl);
        }

        // Query IShellLink for the IPersistFile interface, used for saving the 
        // shortcut in persistent storage. 
        if (SUCCEEDED(hr))
        {
            hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
            if (SUCCEEDED(hr))
            {
                // Save the link by calling IPersistFile::Save. 
                hr = ppf->Save(path->Data(), TRUE);
                ppf->Release();
                if (SUCCEEDED(hr))
                {
                    HINSTANCE result = ShellExecute(NULL, NULL, path->Data(), L"", NULL, SW_SHOWNORMAL);
                }
            }
        }
        psl->Release();
    }
    return hr;
}

Platform::String^ GetDisplayName(IShellItem *psi, SIGDN sigdn)
{
    LPWSTR pszName = nullptr;
    Platform::String^ result = ref new Platform::String();
    HRESULT hr = psi->GetDisplayName(sigdn, &pszName);
    if (SUCCEEDED(hr)) {
        result = ref new Platform::String(pszName);
        CoTaskMemFree(pszName);
    }

    return result;
}

Windows::Foundation::Collections::ValueSet^ Applications::LaunchApplication(Platform::String^ name)
{
    ValueSet^ result = ref new ValueSet;
    IShellItem *psiFolder;
    bool found = false;
    
    HRESULT hr = SHGetKnownFolderItem(FOLDERID_AppsFolder, KF_FLAG_DEFAULT, NULL, IID_PPV_ARGS(&psiFolder));
    if (SUCCEEDED(hr))
    {
        IEnumShellItems *pesi;
        hr = psiFolder->BindToHandler(NULL, BHID_EnumItems, IID_PPV_ARGS(&pesi));
        if (hr == S_OK)
        {
            IShellItem *psi;
            while (pesi->Next(1, &psi, NULL) == S_OK && !found)
            {
                IShellItem2 *psi2;
                if (SUCCEEDED(psi->QueryInterface(IID_PPV_ARGS(&psi2))))
                {
                    auto appName = GetDisplayName(psi2, SIGDN_NORMALDISPLAY);
                    if (!appName->IsEmpty())
                    {
                        if (appName == name)
                        {
                            hr = LaunchAppFromShortCut(psi);
                            found = true;
                        }
                    }
                    psi2->Release();
                }
                psi->Release();
            }
            psiFolder->Release();
        }

        if (SUCCEEDED(hr))
        {
            result->Insert("Status", "OK");
        }
        else
        {
            std::wstringstream errorMessage;
            errorMessage << L"LaunchApplication error: " << hr;
            result->Insert("Error", ref new Platform::String(errorMessage.str().c_str()));
        }
    }

    return result;
}

// This method returns an array of Application Names from the Windows Application folder
ValueSet^ Applications::GetApplications()
{
    ValueSet^ result = ref new ValueSet;
    Vector<Platform::String^>^ applications = ref new Vector<Platform::String^>();

    IShellItem *psiFolder;
    HRESULT hr = SHGetKnownFolderItem(FOLDERID_AppsFolder, KF_FLAG_DEFAULT, NULL, IID_PPV_ARGS(&psiFolder));
    if (SUCCEEDED(hr)) 
    {
        IEnumShellItems *pesi;
        hr = psiFolder->BindToHandler(NULL, BHID_EnumItems, IID_PPV_ARGS(&pesi));
        if (SUCCEEDED(hr))
        {
            IShellItem *psi;
            while (pesi->Next(1, &psi, NULL) == S_OK)
            {
                IShellItem2 *psi2;
                if (SUCCEEDED(psi->QueryInterface(IID_PPV_ARGS(&psi2)))) 
                {
                    auto name = GetDisplayName(psi2, SIGDN_NORMALDISPLAY);
                    if (!name->IsEmpty())
                    {
                        applications->Append(name);
                    }
                    psi2->Release();
                }
                psi->Release();
            }
            psiFolder->Release();
        }

        if (SUCCEEDED(hr))
        {
            auto temp = ref new Platform::Array<Platform::String^>(applications->Size);
            for (unsigned int i = 0; i < applications->Size; ++i)
            {
                temp[i] = applications->GetAt(i);
            }

            result->Insert("Applications", temp);
        }
    }

    if (SUCCEEDED(hr))
    {
        result->Insert("Status", "OK");
    }
    else
    {
        std::wstringstream errorMessage;
        errorMessage << L"LaunchApplication error: " << hr;
        result->Insert("Error", ref new Platform::String(errorMessage.str().c_str()));
    }

    return result;
}


