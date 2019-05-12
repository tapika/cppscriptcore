#include "Project.h"
#include <filesystem>

using namespace std;
using namespace filesystem;

void __declspec(dllexport) main(void)
{
    Project p(L"sharedItems");
    p.New(projecttype_CppSharedItemsProject);

    p.SetSaveDirectory(L".");
    p.Save();

    //Project p(L"HelloWorld");

    //p.SetSaveDirectory(L".");
    //p.AddPlatform(L"x64");
    //p.AddFiles({ L"helloWorld.cpp" });

    //p.VisitConfigurations( 
    //    [](VCConfiguration& c)
    //    {
    //        c.General.IntDir = LR"(obj\$(ProjectName)_$(Configuration)_$(Platform)\)";
    //        c.General.OutDir = LR"(bin\$(Configuration)_$(Platform)\)";
    //        c.General.UseDebugLibraries = true;
    //        c.General.LinkIncremental = true;
    //        c.CCpp.Optimization.Optimization = optimization_Disabled;
    //        c.Linker.System.SubSystem = subsystem_Console;
    //        c.Linker.Debugging.GenerateDebugInformation = debuginfo_true;
    //    }
    //);

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

    //p.Save();

    //Project p(L"emptyProject");
    //p.SetSaveDirectory(L".");
    //p.AddPlatform(L"x64");
    //p.Save();

}

