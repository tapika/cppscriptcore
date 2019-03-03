#pragma once
#include "EnumReflect.h"

/// <summary>
/// Defines what needs to be done with given item. Not all project types support all enumerations - for example
/// packaging projects / C# projects does not support CustomBuild.
/// 
/// Order of ItemType must be the same as appear in .vcxproj (first comes first)
/// </summary>
DECLARE_ENUM(ItemType,

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


