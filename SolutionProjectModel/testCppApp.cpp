#include "Project.h"

void main(void)
{
    Project p;

    //p.Load(L"testCppApp.vcxproj");
    //printf("%s", p.configurations[0].c_str());

    p.name = L"test1";
    p.AddPlatforms( { "Win32" } );
    p.Save();
    
    p.AddPlatforms({ "x64" });
    p.Save(L"test2.vcxproj");


}

