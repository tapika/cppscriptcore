#include "Project.h"
#include <filesystem>

using namespace std;
using namespace filesystem;

void main(void)
{
    Project p(L"HelloWorld");

    p.SetSaveDirectory(L".");
    p.AddPlatform(L"x64");
    p.AddFiles({ L"helloWorld.cpp" });

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

    auto f = p.File(L"..\\SolutionProjectModel.dll", true);
    f->General.ItemType = CustomBuild;
    f->VisitTool(
        [](PlatformConfigurationProperties* props)
        {
            CustomBuildToolProperties& custtool= *((CustomBuildToolProperties*)props);
            custtool.Command = "echo Hello world";
        }
    , &CustomBuildToolProperties::GetType());



    p.Save();
}

