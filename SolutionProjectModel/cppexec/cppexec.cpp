#include "..\Project.h"
#include "expdef.h"                 //printPeExports
#include "..\VisualStudioInfo.h"    //getInstalledVisualStudios
#include <filesystem>
#include <atlconv.h>                //CW2A
#include <algorithm>                //transform
#include <time.h>                   //time
#include "spdlog/spdlog.h"

using namespace std;
using namespace filesystem;
using namespace spdlog;

class CommandLineArguments : public ReflectClassT<CommandLineArguments>
{
public:
    CommandLineArguments(): 
        _autorun(false),
        _local(false),
        _location(false),
        _time(false),
        _vs(0)
    {
    
    }

    REFLECTABLE(CommandLineArguments,
        (bool)autorun,
        (bool)local,
        (bool)location,
        (bool)time,
        (int)vs
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


int _wmain(int argc, wchar_t** argv)
{
    path exePath = weakly_canonical(argv[0]);
    auto exeDir = exePath.parent_path();
    path scriptToRun;

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
                scriptToRun = absolute(arg);
            } else if (ext == ".dll")
            {
                return printPeExports(arg);
            }

            continue;
        }

        string cmdarg(CW2A(arg + 1));

        if( cmdarg == "?" || cmdarg == "h")
        {
            scriptToRun.clear();
            break;
        }

        int findex = type.GetFieldIndex(cmdarg.c_str());
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

    if (scriptToRun.empty() && cmdargs.autorun)
    {
        for (auto& pit : directory_iterator(exeDir))
        {
            auto& filePath = pit.path();
            auto ext = getFileExtension(filePath.c_str());

            if (ext == ".cpp")
            {
                if (scriptToRun.empty())
                {
                    scriptToRun = absolute(filePath);
                }
                else
                {
                    scriptToRun.clear();
                    break;
                }
            }
        } //for
    }

    if(scriptToRun.empty() && !cmdargs.location)
    {
        info("Usage: {0} [options] <.cpp script to run>", exePath.filename().u8string().c_str());
        info("where options could be:");
        info("");
        info("    -local          - Keep generated project next to script.");
        info("    -vs <version>   - use specific Visual studio (2019, 2017...)");
        info("    -location       - Only display where Visual studio is located");
        return -2;
    }

    time_t start, end;
    time(&start);

    path projectDir;

    if (cmdargs.local)
        projectDir = scriptToRun.parent_path();
    else
        projectDir = temp_directory_path().append(L"cppscript").append(scriptToRun.stem().c_str());

    Project p(scriptToRun.stem().c_str());
    p.SetSaveDirectory(projectDir.c_str());

    if (!cmdargs.location)
    {
        if(!exists(projectDir))
            create_directories(projectDir);

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
    } //if

    vector<VisualStudioInfo> instances = getInstalledVisualStudios();
    vector<VisualStudioInfo>::iterator it;

    if(cmdargs.vs != 0)
    {
        it = find_if(instances.begin(), instances.end(), [&](auto vsinfo) { return vsinfo.version == cmdargs.vs; });

        if (it == instances.end())
            throwFormat("Visual studio %d was not found on this machine", cmdargs.vs);

        if (cmdargs.location)
        {
            wprintf(L"%s\n", it->InstallPath.c_str());
            return 0;
        }
    }
    else
    {
        it = max_element(instances.begin(), instances.end(), [](auto e1, auto e2) { return e1.version < e2.version; });

        if (it == instances.end())
            throw exception("No Visual Studio installation found");

        if(cmdargs.location)
        {
            for( auto vsinfo: instances)
            {
                wprintf(L"Visual studio %d:\n", vsinfo.version);
                wprintf(L"location: %s\n\n", vsinfo.InstallPath.c_str());
            }

            return 0;
        }
    }

    auto devenv = path(it->InstallPath).append("Common7\\IDE\\devenv.com");

    auto quoted = [](wstring f) -> wstring
    {
        return wstring(L"\"") + f + L"\"";
    };

    wprintf(L"%s: compile... ", scriptToRun.filename().c_str());
    wstring cmd = wstring(L"cmd /C ") + quoted( quoted(devenv.c_str()) + L" /nologo /build " + quoted(L"Debug^|x64") + L" " + quoted(p.GetProjectSaveLocation()) ) + L" 2>&1";
    // printf("%S\n", cmd.c_str());
    ExecCmd(cmd.c_str());

    auto dllPath = path(projectDir).append(scriptToRun.stem().wstring() + L".dll");
    if( !exists(dllPath) )
        throw exception(sFormat("Compilation failed: dll does not exists: '%s'", dllPath.u8string().c_str()).c_str());

    HMODULE h = LoadLibraryW(dllPath.c_str());
    if (!h)
        ThrowLastError();

    FARPROC proc = GetProcAddress(h, "main");

    if (!proc)
        throwFormat("Dll entry point function 'main' not found.");

    printf("execute:\n\n");
    int r = (int)proc();
    FreeLibrary(h);

    if(cmdargs.time)
    {
        time(&end);
        double diff = difftime(end, start);
        printf("\n\nElapsed time: %.2lf seconds\n", diff);
    }

    return r;
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

