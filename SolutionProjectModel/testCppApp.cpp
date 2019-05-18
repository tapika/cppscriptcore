#include "Project.h"
#include <filesystem>
#include <algorithm>                //transform

using namespace std;
using namespace filesystem;

void __declspec(dllexport) main(void)
{
    //Project p(L"sharedItems");
    //p.New(projecttype_CppSharedItemsProject);

    //p.SetSaveDirectory(L".");
    //p.Save();

    Project p(L"ninja");

    p.SetSaveDirectory(LR"(C:\PrototypingQuick\CrashPad\crashpad3\ninja\src)");
    //p.SetVsVersion(2019);
    p.AddPlatform(L"x64");
    
    for (auto& pit : directory_iterator(p.GetSaveDirectory()))
    {
        auto& filePath = pit.path();

        if (is_directory(filePath))
            continue;

        auto fname = lowercase(filePath.stem().u8string());
        auto ext = lowercase(filePath.extension().u8string());
        transform(ext.begin(), ext.end(), ext.begin(), tolower);

        if (fname == "browse" || fname == "subprocess-posix" || fname == "test" /*|| fname == "msvc_helper_main-win32"*/)
            continue;

        if(fname.find("test") != -1 || fname.find(".in") != -1 || fname.find("bench") != -1)
            continue;
        
        if (ext == ".h" || ext == ".cc" || ext == ".c")
            p.AddFile(filePath.c_str());
    }

    p.VisitConfigurations( 
        [](VCConfiguration& c)
        {
            c.General.IntDir = LR"(obj\$(ProjectName)_$(Configuration)_$(Platform)\)";
            c.General.OutDir = LR"(bin\$(Configuration)_$(Platform)\)";
            c.General.UseDebugLibraries = true;
            c.General.LinkIncremental = true;
            c.CCpp.Optimization.Optimization = optimization_Disabled;
            c.Linker.System.SubSystem = subsystem_Console;
            c.Linker.Debugging.GenerateDebugInformation = debuginfo_true;
        }
    );

    //auto f = p.File(L"..\\SolutionProjectModel.dll", true);
    //f->General.ItemType = CustomBuild;
    //f->VisitTool(
    //    [](PlatformConfigurationProperties* props)
    //    {
    //        CustomBuildToolProperties& custtool= *((CustomBuildToolProperties*)props);
    //        CStringW cmd = "..\\cppexec.exe %(FullPath) >$(IntermediateOutputPath)%(Filename).def";
    //        cmd += "\n";
    //        cmd += "lib /nologo /def:$(IntermediateOutputPath)%(Filename).def /machine:$(Platform) /out:$(IntermediateOutputPath)%(Filename)_lib.lib";
    //        custtool.Message = "Generating static library for %(Identity)...";
    //        custtool.Command = cmd;
    //        custtool.Outputs = "$(IntermediateOutputPath)%(Filename)_lib.lib";
    //    }
    //, &CustomBuildToolProperties::GetType());

    p.Save();

    //Project p(L"emptyProject");
    //p.SetSaveDirectory(L".");
    //p.AddPlatform(L"x64");
    //p.Save();

}

