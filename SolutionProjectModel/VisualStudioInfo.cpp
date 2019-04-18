#include "pch.h"
#include "VisualStudioInfo.h"
#include <windows.h>            //DWORD
#include <functional>           //function
#include <atlconv.h>            //CW2A
#include <comdef.h>             //_COM_SMARTPTR_TYPEDEF
#include <Setup.Configuration.h>

_COM_SMARTPTR_TYPEDEF(ISetupConfiguration, __uuidof(ISetupConfiguration));
_COM_SMARTPTR_TYPEDEF(IEnumSetupInstances, __uuidof(IEnumSetupInstances));
_COM_SMARTPTR_TYPEDEF(ISetupInstance, __uuidof(ISetupInstance));
_COM_SMARTPTR_TYPEDEF(ISetupInstanceCatalog, __uuidof(ISetupInstanceCatalog));
_COM_SMARTPTR_TYPEDEF(ISetupPropertyStore, __uuidof(ISetupPropertyStore));

using namespace std;


vector<VisualStudioInfo> getInstalledVisualStudios(void)
{
    vector<VisualStudioInfo> instances;
    // Idea copied from vswhere, except whole implementation was re-written from scratch.
    {
        ISetupConfigurationPtr setupCfg;
        IEnumSetupInstancesPtr enumInstances;

        auto lcid = GetUserDefaultLCID();

        function<void(HRESULT)> hrc = [](HRESULT hr)
        {
            if (FAILED(hr))
            {
                USES_CONVERSION;
                throw exception(CW2A(_com_error(hr).ErrorMessage()));
            }
        };

        hrc(CoInitialize(nullptr));
        hrc(setupCfg.CreateInstance(__uuidof(SetupConfiguration)));
        hrc(setupCfg->EnumInstances(&enumInstances));

        while (true)
        {
            ISetupInstance* p = nullptr;
            unsigned long ul = 0;
            HRESULT hr = enumInstances->Next(1, &p, &ul);
            if (hr != S_OK)
                break;

            ISetupInstancePtr setupi(p, false);
            ISetupInstanceCatalogPtr instanceCatalog;
            ISetupPropertyStorePtr store;
            hrc(setupi->QueryInterface(&instanceCatalog));
            hrc(instanceCatalog->GetCatalogInfo(&store));

            CComVariant v;
            hrc(store->GetValue(L"productLineVersion", &v));

            VisualStudioInfo vsinfo;
            vsinfo.version = atoi(CStringA(v).GetBuffer());
            CComBSTR instpath;
            // GetDisplayName can be used to get name of instance.
            hrc(setupi->GetInstallationPath(&instpath));
            vsinfo.InstallPath = instpath;
            instances.push_back(vsinfo);
        }
    }
    CoUninitialize();

    return instances;
}


