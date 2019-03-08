#pragma once
#include "ProjectFile.h"
#include <string>
#include <vector>
#include <list>
#include <initializer_list>
#include <guiddef.h>                        //GUID

//---------------------------------------------------------
//  Project
//---------------------------------------------------------
class SPM_DLLEXPORT Project : 
    pugi::xml_document,
    LinkerConfiguration
{
public:
    Project();
    Project(const wchar_t* _name);

    //
    //  It's good to set save directory before adding files to project, as all paths will be relative to project.
    //
    void SetSaveDirectory(const wchar_t* dir);

    //
    //  Gets project save directory.
    //
    std::wstring GetSaveDirectory();

    //
    // Clears existing project
    //
    void New();

    //
    // Loads .vcxproj file.
    //
    bool Load(const wchar_t* file);

    //
    // Saves project file
    //
    bool Save(const wchar_t* file = nullptr);

    //
    //  Sets visual studio version, in year. e.g. 2017, 2019, ...
    //
    void SetVsVersion(int vsVersion);

    //
    // Gets project guid, initialized if it's not initialized yet
    //
    std::wstring GetGuid(void);

    //
    // Add support for platform or configuration, if not yet added.
    //
    void AddPlatform(const char* platform);
    void AddPlatforms(std::initializer_list<std::string> _platforms);

    void AddConfiguration(const char* configuration);
    void AddConfigurations(std::initializer_list<std::string> _configuration);

    //
    //  Gets list of currently supported configurations, in form "<configuration>|<platform>"
    //
    std::vector<std::string> GetConfigurations();

    //
    // Queries for currently selected toolset, if none is selected, tries to determine from visual studio format version
    //
    std::string GetToolset();

    //
    //  Adds files to the project.
    //
    void AddFiles(std::initializer_list<std::wstring> fileList);
    void AddFile(const wchar_t* file);


    LinkerConfiguration& GetLinker()
    {
        return *this;
    }

    __declspec(property(get = GetLinker)) LinkerConfiguration& linker;

protected:
    // Project name, typically used to identify project within solution or specify saved filename if file is not specified during save.
    std::wstring name;

    // Directory where project shall be saved.
    std::wstring saveDir;

    //  "Win32", "х64", ...
    std::vector<std::string> platforms;

    //  "Debug", "Release", user defined
    std::vector<std::string> configurations;

    //  List of files within a project.
    std::list<ProjectFile>  files;

    // Project guid
    GUID guid;

    //
    //  Visual studio version, in year. e.g. 2017, 2019, ...
    //
    int vsVersion;

    // Platform Toolset, e.g. "v141" (for vs2017), "142" (for vs2019), "Clang_5_0" ...
    std::string toolset;

    //
    //  Typically in .vcxproj this is used to mark where project is targetted upon, e.g. "Win32Proj" - win32 or win64, "Android", "Linux", "Clang",
    //  but division of whether it's os, compiler or just .dll inside VS is not so clear. Keeping as string for timebeing.
    //
    std::string Keyword;

    std::string GetKeyword();
    std::string GetWindowsSDKVersion();


    pugi::xml_node project( );
    pugi::xml_node projectGlobals;
    pugi::xml_node markForPropertyGroup;

    //
    // Platforms or Configurations arrays updated, bPlatforms == true - platforms false = configurations, bAdd = true - added, bAdd = false - removed.
    //
    void PlatformConfigurationsUpdated(std::initializer_list<std::string> items, bool bPlatforms, bool bAdd);
};

