#include "Project.h"

void main(void)
{
    Project p;
    p.Load(L"testCppApp.vcxproj");

    printf("%s", p.Configurations[0].ConfigurationName.c_str());
    p.Configurations[0].ConfigurationName = "x64";
}

