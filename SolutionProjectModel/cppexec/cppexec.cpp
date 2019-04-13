#include "..\Project.h"
#include "expdef.h"             //printPeExports
#include <filesystem>
#include <atlconv.h>            //CW2A
#include <algorithm>            //transform
using namespace std;
using namespace filesystem;


class CommandLineArguments : ReflectClassT<CommandLineArguments>
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


int _wmain(int argc, wchar_t** argv)
{
    path exePath(argv[0]);
    auto exeDir = weakly_canonical(exePath).parent_path();
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
    p.AddPlatform("x64");
    p.File(scriptToRun.c_str(), true);

    p.VisitConfigurations(
        [&](VCConfiguration& c)
        {
            c.General.IntDir = LR"(obj\$(ProjectName)_$(Configuration)_$(Platform)\)";
            c.General.OutDir = LR"(.\)";
            c.General.UseDebugLibraries = true;
            c.General.LinkIncremental = true;
            c.CCpp.Optimization.Optimization = optimization_Disabled;
            c.CCpp.General.AdditionalIncludeDirectories = exeDir.append("SolutionProjectModel").c_str();
            c.CCpp.Language.LanguageStandard = cpplang_stdcpp17;
            c.Linker.System.SubSystem = subsystem_Windows;
            c.Linker.Debugging.GenerateDebugInformation = debuginfo_true;
        }
    );

    if( !p.Save() )
    {
        printf("Error: Could not save project file '%S'", scriptToRun.stem().c_str() );
        return -4;
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

