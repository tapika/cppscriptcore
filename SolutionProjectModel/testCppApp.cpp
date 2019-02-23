#include "Project.h"

void main(void)
{
    Project p;

    //p.Load(L"testCppApp.vcxproj");
    //printf("%s", p.configurations[0].c_str());

    p.AddPlatforms( { "Win32", "x64", "x64" } );

}

