//css_ref syncproj.exe
using System;

partial class Builder : SolutionProjectBuilder
{
    static void Main(String[] args)
    {
        project("HelloWorld2");
        platforms("Win32", "x64");
        vsver(2017);
        kind("ConsoleApp");
        files("HelloWorld.cpp");
        targetdir(@"$(ProjectDir)\bin\$(Platform)_$(Configuration)\");
        objdir(@"$(ProjectDir)\obj\$(Platform)_$(Configuration)\");
        //symbols("on");
        //optimize("off");
    }
};

