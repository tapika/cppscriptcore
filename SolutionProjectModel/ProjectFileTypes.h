#pragma once
#include "EnumReflect.h"
#include "CppReflect.h"

#ifdef SPM_EXPORT
#define SPM_DLLEXPORT __declspec(dllexport)
#else
#define SPM_DLLEXPORT __declspec(dllimport)
#endif


/// <summary>
/// Defines what needs to be done with given item. Not all project types support all enumerations - for example
/// packaging projects / C# projects does not support CustomBuild.
/// 
/// Order of ItemType must be the same as appear in .vcxproj (first comes first)
/// </summary>
DECLARE_ENUM(ItemType, "",

    /// <summary>
    /// C# references to .net assemblies
    /// </summary>
    Reference,

    /// <summary>
    /// Header file (.h)
    /// </summary>
    ClInclude,

    /// <summary>
    /// Source codes (.cpp) files
    /// </summary>
    ClCompile,

    /// <summary>
    /// .rc / resource files.
    /// </summary>
    ResourceCompile,

    /// <summary>
    /// Any custom file with custom build step
    /// </summary>
    CustomBuild,

    /// <summary>
    /// .def / .bat
    /// </summary>
    None,

    /// <summary>
    /// .ico files.
    /// </summary>
    Image,

    /// <summary>
    /// .txt files.
    /// </summary>
    Text,

    // Following enumerations are used in android packaging project (.androidproj)
    Content,
    AntBuildXml,
    AndroidManifest,
    AntProjectPropertiesFile,

    /// <summary>
    /// For Android package project: Reference to another project, which needs to be included into package.
    /// </summary>
    ProjectReference,

    /// <summary>
    /// Intentionally not valid value, so can be replaced with correct one. (Visual studio does not supports one)
    /// </summary>
    Invalid,

    /// <summary>
    /// C# - source codes to compile
    /// </summary>
    Compile,

    /// <summary>
    /// Android / Gradle project, *.template files.
    /// </summary>
    GradleTemplate,

    /// <summary>
    /// .java - source codes to compile
    /// </summary>
    JavaCompile,

    //
    //  Native Visualization files (*.natvis)
    //
    Natvis
);

class SPM_DLLEXPORT CCppConfiguration
{
public:


};

//
// Binary image format / target
//
DECLARE_ENUM(ESubSystem, "subsystem_",
    //
    // Not specified
    //
    subsystem_NotSet,

    //
    // <summary>
    //
    subsystem_Windows,

    //
    // Console application
    //
    subsystem_Console,
    subsystem_Native,
    subsystem_EFI_Application,
    subsystem_EFI_Boot_Service_Driver,
    subsystem_EFI_ROM,
    subsystem_EFI_Runtime,
    subsystem_POSIX
);


//
// Generate debug information
//
DECLARE_ENUM(EGenerateDebugInformation, "debuginfo_",
    //
    // No
    //
    debuginfo_false,

    //
    // Optimize for debugging
    //
    debuginfo_true,

    //
    // Use fast linking
    //
    debuginfo_DebugFastLink,

    //
    // Generate Debug Information optimized for sharing and publishing (/DEBUG:FULL)
    //
    debuginfo_DebugFull
);



class SPM_DLLEXPORT LinkerSystemConf: ReflectClassT<LinkerSystemConf>
{
public:
    LinkerSystemConf(ReflectClass* parent):
        ReflectClassT<LinkerSystemConf>(parent)
    {
    }

    REFLECTABLE(LinkerSystemConf,
        (ESubSystem)SubSystem
    );
};

class SPM_DLLEXPORT LinkerDebuggingConf : ReflectClassT<LinkerDebuggingConf>
{
public:
    LinkerDebuggingConf(ReflectClass* parent):
        ReflectClassT<LinkerDebuggingConf>(parent)
    {
    }

    REFLECTABLE(LinkerDebuggingConf,
        (EGenerateDebugInformation)GenerateDebugInformation
    );
};



class SPM_DLLEXPORT LinkerConf: ReflectClassT<LinkerConf>
{
public:
    LinkerConf(ReflectClass* parent, const char* field): 
        ReflectClassT<LinkerConf>(parent, field),
        _System(this),
        _Debugging(this)
    {
    }

    REFLECTABLE(LinkerConf,
        (LinkerSystemConf)System,
        (LinkerDebuggingConf)Debugging
    );
};

