#pragma once
#include "../pugixml/pugixml.hpp"           //pugi::xml_node
#include "EnumReflect.h"
#include "CppReflect.h"

/// <summary>
/// Defines what needs to be done with given item. Not all project types support all enumerations - for example
/// packaging projects / C# projects does not support CustomBuild.
/// 
/// Order of EItemType must be the same as appear in .vcxproj (first comes first)
/// </summary>
DECLARE_ENUM(EItemType, "",

    //
    // C# references to .net assemblies
    //
    Reference,

    //
    // Header file (.h)
    //
    ClInclude,

    //
    // Source codes (.cpp) files
    //
    ClCompile,

    //
    // .rc / resource files.
    //
    ResourceCompile,

    //
    // Any custom file with custom build step
    //
    CustomBuild,

    //
    // .def / .bat
    //
    None,

    //
    // .ico files.
    //
    Image,

    //
    // .txt files.
    //
    Text,

    // Following enumerations are used in android packaging project (.androidproj)
    Content,
    AntBuildXml,
    AndroidManifest,
    AntProjectPropertiesFile,

    //
    // For Android package project: Reference to another project, which needs to be included into package.
    //
    ProjectReference,

    //
    // Intentionally not valid value, so can be replaced with correct one. (Visual studio does not supports one)
    //
    Invalid,

    //
    // C# - source codes to compile
    //
    Compile,

    //
    // Android / Gradle project, *.template files.
    //
    GradleTemplate,

    //
    // .java - source codes to compile
    //
    JavaCompile,

    //
    //  Native Visualization files (*.natvis)
    //
    Natvis
);

class SPM_DLLEXPORT ProjectItemGeneralConf : public ReflectClassT<ProjectItemGeneralConf>
{
public:
    REFLECTABLE(ProjectItemGeneralConf,
        (EItemType)ItemType
    );
};



//
//  Visual Studio project tagging.
//
DECLARE_ENUM(EKeyword, "projecttype_",
    //
    // For sub-folders for example (Also default value). Also for utility projects.
    //
    projecttype_None = 0,

    //
    // Windows project (32 or 64 bit)
    //
    projecttype_Win32Proj,

    //
    // Same as Win32Proj, for some reason exists as separate value
    //
    projecttype_ManagedCProj,

    //
    // Android project
    //
    projecttype_Android,

    //
    // Windows application with MFC support
    //
    projecttype_MFCProj,

    //
    // Android packaging project (does not exists on file format level)
    //
    projecttype_AntPackage,

    /// <summary>
    /// Typically set for Android packaging project. (does not exists on file format level)
    /// </summary>
    projecttype_GradlePackage
);

class SPM_DLLEXPORT ProjectGlobalConf : public ReflectClassT<ProjectGlobalConf>
{
public:
    REFLECTABLE(ProjectGlobalConf,
        // This is typically non-configurable by end-user.
        (CStringA)ProjectGuid,
        (EKeyword)Keyword,
        (CStringW)WindowsTargetPlatformVersion
    );
};


//
// Project type
//
DECLARE_ENUM(EConfigurationType, "conftype_",
    //
    // .exe
    //
    conftype_Application = 0,

    //
    // .dll
    //
    conftype_DynamicLibrary,

    //
    // .lib or .a
    //
    conftype_StaticLibrary,

    //
    // Android gradle project: Library (.aar/.jar)
    //
    conftype_Library,

    //
    // Utility project
    //
    conftype_Utility,

    //
    // This value does not physically exists in serialized form in .vcxproj, used only for generation of C# script.
    //
    conftype_ConsoleApplication
);



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


class SPM_DLLEXPORT PlatformConfigurationProperties
{
public:
    // So can safely delete by base pointer.
    virtual ~PlatformConfigurationProperties() { }

    // Configuration name / platform of specific configuration
    std::wstring configurationName;
    std::wstring platform;

    pugi::xml_node node;

};

class SPM_DLLEXPORT CustomBuildToolProperties : public PlatformConfigurationProperties, public ReflectClassT<CustomBuildToolProperties>
{
public:
    REFLECTABLE(CustomBuildToolProperties,
        // Command line
        (CStringW)Command,
        // Description. Use empty string to supress message printing.
        (CStringW)Message,
        // Outputs
        (CStringW)Outputs,
        // Additional dependencies
        (CStringW)AdditionalInputs,
        // Specify whether the inputs and outputs files with specific extension are passed to linker.
        (bool)LinkObjects,
        (bool)ExcludedFromBuild
    );
};



class SPM_DLLEXPORT LinkerSystemConf: public ReflectClassT<LinkerSystemConf>
{
public:
    REFLECTABLE(LinkerSystemConf,
        (ESubSystem)SubSystem
    );
};

class SPM_DLLEXPORT LinkerDebuggingConf : public ReflectClassT<LinkerDebuggingConf>
{
public:
    REFLECTABLE(LinkerDebuggingConf,
        (EGenerateDebugInformation)GenerateDebugInformation
    );
};

//
// Character set - unicode MBCS.
//
DECLARE_ENUM(ECharacterSet, "charset_",
    //
    // Unicode
    //
    charset_Unicode = 0,

    //
    // Ansi
    //
    charset_MultiByte
);


//
// How to optimize code ?
//
DECLARE_ENUM(EOptimization, "optimization_",
    optimization_Custom,

    //
    // No optimizations
    //
    optimization_Disabled,

    //
    // Minimize Size, in Windows projects
    //
    optimization_MinSpace,

    //
    // Minimize Size, In Android projects
    //
    optimization_MinSize,

    //
    // Maximize Speed
    //
    optimization_MaxSpeed,

    //
    // Full Optimization
    //
    optimization_Full,

    //
    // Not available in project file, but this is something we indicate that we haven't set value
    //
    optimization_ProjectDefault
);


DECLARE_ENUM(ELanguageStandard, "cpplang_",

    // ISO C++14 Standard (/std:c++14)
    cpplang_stdcpp14,

    // ISO C++17 Standard (/std:c++17)
    cpplang_stdcpp17,

    // ISO C++ Latest Draft Standard (/std:c++latest)
    cpplang_stdcpplatest
);



class SPM_DLLEXPORT CCppGeneralConf : public ReflectClassT<CCppGeneralConf>
{
public:
    REFLECTABLE(CCppGeneralConf,

        // Additional Include Directories, ';' separated list.
        (CStringW)AdditionalIncludeDirectories
    );
};

class SPM_DLLEXPORT CCppOptimizationConf : public ReflectClassT<CCppOptimizationConf>
{
public:
    REFLECTABLE(CCppOptimizationConf,
        (EOptimization)Optimization
    );
};

class SPM_DLLEXPORT CCppLanguageConf : public ReflectClassT<CCppLanguageConf>
{
public:
    REFLECTABLE(CCppLanguageConf,
        (ELanguageStandard)LanguageStandard
    );
};


class SPM_DLLEXPORT CCppConf: public ReflectClassT<CCppConf>
{
public:
    REFLECTABLE(CCppConf,
        (CCppGeneralConf)General,
        (CCppOptimizationConf)Optimization,
        (CCppLanguageConf)Language
    );
};


class SPM_DLLEXPORT GeneralConf : public ReflectClassT<GeneralConf>
{
public:
    REFLECTABLE(GeneralConf,
        (bool)LinkIncremental,
        //
        // Output directory
        //
        (CStringW)OutDir,
        //
        // Intermediate Directory
        //
        (CStringW)IntDir,
        (CStringW)TargetName,
        (CStringW)TargetExt,

        (EConfigurationType)ConfigurationType,

        //
        // Mysterious flag, which cannot be set from Visual studio properties, but it affects to some parameter's default values.
        //
        (bool)UseDebugLibraries,

        //
        // For example:
        //     'Clang_3_8'     - Clang 3.8
        //     'v141'          - for Visual Studio 2017.
        //     'v140'          - for Visual Studio 2015.
        //     'v120'          - for Visual Studio 2013.
        //
        (CStringW)PlatformToolset,
        (ECharacterSet)CharacterSet
    );
};


class SPM_DLLEXPORT LinkerConf: public ReflectClassT<LinkerConf>
{
public:
    REFLECTABLE(LinkerConf,
        (LinkerSystemConf)System,
        (LinkerDebuggingConf)Debugging
    );
};

