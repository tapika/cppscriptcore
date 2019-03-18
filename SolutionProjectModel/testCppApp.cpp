#include "Project.h"
#include <filesystem>

using namespace std;
using namespace filesystem;

void main(void)
{
    Project p(L"HelloWorld");

    p.SetSaveDirectory(path(__FILE__).parent_path().c_str());

    p.Globals.Keyword = projecttype_Win32Proj;

    p.AddPlatforms( { "Win32", "x64" } );
    p.AddPlatforms({ "x64" });

    p.AddFiles({ L"helloWorld.cpp" });

    //p.AddFiles({ path(p.GetSaveDirectory()).append(L"Solution.cpp").wstring() });

    p.VisitConfigurations( 
        [](VCConfiguration& c)
        {
            c.General.IntDir = LR"(obj\$(ProjectName)_$(Configuration)_$(Platform)\)";
            c.General.OutDir = LR"(bin\$(Configuration)_$(Platform)\)";
            //c.General.TargetName = L"$(ProjectName)";
            //c.General.TargetExt = ".dll";
            c.General.UseDebugLibraries = true;
            c.General.LinkIncremental = true;

            c.CCpp.Optimization = optimization_Disabled;

            c.Linker.System.SubSystem = subsystem_Console;
            c.Linker.Debugging.GenerateDebugInformation = debuginfo_true;
        }
    );

    p.Save();
    
}

