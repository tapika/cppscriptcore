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
    p.AddFiles({ "Solution.h" });
    p.AddFiles({ path(p.GetSaveDirectory()).append("Solution.cpp").string() });
    p.Save();
    
    p.AddPlatforms({ "x64" });
    p.Save(L"test2.vcxproj");


}

