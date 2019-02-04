//css_ref syncproj.exe
using System;

partial class Builder : SolutionProjectBuilder
{
    static void Main(String[] args)
    {
        project("testCppApp");
        platforms("Win32", "x64");
        vsver(2017);
        kind("ConsoleApp");
        projectScript("testCppApp.cs");
        files("testCppApp.cpp");
        referencesProject("SolutionProjectModel.vcxproj");
        targetdir(@"$(ProjectDir)\bin\$(Platform)_$(Configuration)\");
        objdir(@"$(ProjectDir)\bin\$(Platform)_$(Configuration)\");
        symbols("on");
        optimize("off");
    }
};

