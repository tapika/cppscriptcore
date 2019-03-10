#include "Project.h"
#include <filesystem>

using namespace std;
using namespace filesystem;

void main(void)
{
    Project p(L"test1");

    p.SetSaveDirectory(path(__FILE__).parent_path().c_str());

    //p.Load(L"testCppApp.vcxproj");
    //printf("%s", p.configurations[0].c_str());

    p.AddPlatforms( { "Win32" } );
    p.AddPlatforms({ "x64" });

    p.AddFiles({ L"Solution.h" });
    p.AddFiles({ path(p.GetSaveDirectory()).append(L"Solution.cpp").wstring() });

    p.VisitConfigurations( 
        [](VCConfiguration& c)
        {
            c.Linker.System.SubSystem = subsystem_Console;
            c.Linker.Debugging.GenerateDebugInformation = debuginfo_DebugFastLink;
        }
    );

    p.Save();
    
    p.VisitConfigurations(
        [](VCConfiguration& c)
        {
            c.Linker.System.SubSystem = subsystem_Windows;
            c.Linker.Debugging.GenerateDebugInformation = debuginfo_true;
        }
    );

    p.Save(L"test2.vcxproj");


}

