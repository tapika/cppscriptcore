#include "..\Project.h"
#include "expdef.h"             //printPeExports
#include <filesystem>
#include <atlconv.h>            //CW2A
#include <algorithm>            //transform
using namespace std;
using namespace filesystem;
#include <comdef.h>             //_COM_SMARTPTR_TYPEDEF
#include <Setup.Configuration.h>

_COM_SMARTPTR_TYPEDEF(ISetupConfiguration, __uuidof(ISetupConfiguration));
_COM_SMARTPTR_TYPEDEF(IEnumSetupInstances, __uuidof(IEnumSetupInstances));
_COM_SMARTPTR_TYPEDEF(ISetupInstance, __uuidof(ISetupInstance));
_COM_SMARTPTR_TYPEDEF(ISetupInstanceCatalog, __uuidof(ISetupInstanceCatalog));
_COM_SMARTPTR_TYPEDEF(ISetupPropertyStore, __uuidof(ISetupPropertyStore));

class CommandLineArguments : public ReflectClassT<CommandLineArguments>
{
public:
    CommandLineArguments(): 
        _local(false)
    {
    
    }

    REFLECTABLE(CommandLineArguments,
        (bool)local
    );
};

//
//  Gets file extension in lowercase
//
string getFileExtension( const wchar_t* filePath )
{
    USES_CONVERSION;
    auto ext = string(CW2A(path(filePath).extension().c_str()));
    transform(ext.begin(), ext.end(), ext.begin(), tolower);
    return ext;
}

typedef struct
{
    int     version;
    wstring InstallPath;
}VisualStudioInfo;



int _wmain(int argc, wchar_t** argv)
{
    path exePath = weakly_canonical(argv[0]);
    auto exeDir = exePath.parent_path();
    path scriptToRun;

    for (auto& pit : directory_iterator(exeDir))
    {
        auto& filePath = pit.path();
        auto ext = getFileExtension(filePath.c_str());

        if( ext == ".cpp")
        {
            if(scriptToRun.empty())
            {
                scriptToRun = filePath;
            }
            else
            {
                scriptToRun.clear();
                break;
            }
        }
    } //for

    auto& type = CommandLineArguments::GetType();
    CommandLineArguments cmdargs;

    for( int i = 1; i < argc; i++)
    {
        wchar_t* arg = argv[i];

        if( *arg != L'/' && *arg != L'-' )
        {
            auto ext = getFileExtension(arg);
            
            if (ext == ".cpp")
            {
                scriptToRun = arg;
            } else if (ext == ".dll")
            {
                return printPeExports(arg);
            }

            continue;
        }

        int findex = type.GetFieldIndex(CW2A(arg+1));
        if(findex < 0 )
            continue;

        FieldInfo fi = type.fields[findex];
        auto name = fi.fieldType->name();
        char* p = (char*)&cmdargs + fi.offset;
        if(strcmp(name, "bool") == 0)
        {
            bool& b = *((bool*)p);
            b = !b;
        }
        else
        {
            if (i == argc - 1)
                continue;

            fi.fieldType->FromString(p, argv[i+1]);
        }
    }

    if(scriptToRun.empty())
    {
        printf("Usage: %S [options] <.cpp script to run>\r\n", exePath.filename().c_str());
        printf("where options could be:\r\n");
        printf("    -local    - Keep generated project locally.\r\n");
        return -2;
    }

    path projectDir;

    if( cmdargs.local )
        projectDir = scriptToRun.parent_path();
    else
        projectDir = temp_directory_path().append(L"cppscript").append(scriptToRun.stem().c_str());

    if(!exists(projectDir))
        create_directories(projectDir);

    Project p(scriptToRun.stem().c_str());
    p.SetSaveDirectory(projectDir.c_str());
    p.AddPlatform(L"x64");
    p.File(scriptToRun.c_str(), true);

    p.VisitConfigurations(
        [&](VCConfiguration& c)
        {
            c.General.IntDir = LR"(obj\$(ProjectName)_$(Configuration)_$(Platform)\)";
            c.General.OutDir = LR"(.\)";
            c.General.UseDebugLibraries = true;
            c.General.LinkIncremental = true;
            c.General.ConfigurationType = conftype_DynamicLibrary;
            c.CCpp.Optimization.Optimization = optimization_Disabled;
            c.CCpp.General.AdditionalIncludeDirectories = path(exeDir).append("SolutionProjectModel").c_str();
            c.CCpp.Language.LanguageStandard = cpplang_stdcpp17;
            c.Linker.System.SubSystem = subsystem_Windows;
            c.Linker.Debugging.GenerateDebugInformation = debuginfo_true;
        }
    );

    auto dll = path(exeDir).append("SolutionProjectModel.dll");
    auto f = p.File(dll.c_str(), true);
    f->General.ItemType = CustomBuild;
    auto exePathRelative = relative(exePath, projectDir);
    f->VisitTool(
        [&](PlatformConfigurationProperties* props)
        {
            CustomBuildToolProperties& custtool = *((CustomBuildToolProperties*)props);
            CStringW cmd = CStringW("\"") + exePathRelative.c_str() + "\" %(FullPath) >$(IntermediateOutputPath)%(Filename).def";
            cmd += "\n";
            cmd += "lib /nologo /def:$(IntermediateOutputPath)%(Filename).def /machine:$(Platform) /out:$(IntermediateOutputPath)%(Filename)_lib.lib";
            custtool.Message = "Generating static library for %(Identity)...";
            custtool.Command = cmd;
            custtool.Outputs = "$(IntermediateOutputPath)%(Filename)_lib.lib";
        }
    , &CustomBuildToolProperties::GetType());

    if( !p.Save() )
    {
        printf("Error: Could not save project file '%S'", scriptToRun.stem().c_str() );
        return -4;
    }

    // Idea copied from vswhere, except whole implementation was re-written from scratch.
    ISetupConfigurationPtr setupCfg;
    IEnumSetupInstancesPtr enumInstances;

    vector<VisualStudioInfo> instances;
    auto lcid = GetUserDefaultLCID();

    function< void(HRESULT)> hrc = [](HRESULT hr)
    {
        if (FAILED(hr))
            throw _com_error(hr);
    };

    try {
        hrc(CoInitialize(nullptr));
        hrc(setupCfg.CreateInstance(__uuidof(SetupConfiguration)));
        hrc(setupCfg->EnumInstances(&enumInstances));

        while(true)
        {
            ISetupInstance* p = nullptr;
            unsigned long ul = 0;
            HRESULT hr = enumInstances->Next(1, &p, &ul);
            if (hr != S_OK )
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
        
        CoUninitialize();
    }                        
    catch (_com_error ce)
    {
        printf("Error: %S\n", ce.ErrorMessage());
    }

    return 0;
}

int wmain(int argc, wchar_t** argv)
{
    try
    {
        return _wmain(argc, argv);
    }
    catch (exception & ex)
    {
        fprintf(stderr, "Error: %s\r\n", ex.what());
        return -3;
    }
}

