#include "Project.h"
#include <filesystem>

using namespace std;
using namespace filesystem;

void main(void)
{
    Project p(L"HelloWorld");

    p.SetSaveDirectory(L".");
    p.AddPlatform("x64");
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

    p.Save();
}

