#pragma once
#include "VCConfiguration.h"                //VCConfiguration
#include <list>
#include <initializer_list>
#include <functional>                       //std::functional
#include <guiddef.h>                        //GUID

//---------------------------------------------------------
//  Project
//---------------------------------------------------------
class SPM_DLLEXPORT Project : public ReflectClassT<Project>
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
    std::wstring GetProjectSaveLocation();

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

    virtual void OnAfterSetProperty(ReflectPath& path);

    REFLECTABLE(Project,
        (ProjectGlobalConf)Globals
    );

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
    void AddPlatform(const wchar_t* platform);
    void AddPlatforms(std::initializer_list<std::wstring> _platforms);

    void AddConfiguration(const wchar_t* configuration);
    void AddConfigurations(std::initializer_list<std::wstring> _configuration);

    //
    // Queries for currently selected toolset, if none is selected, tries to determine from visual studio format version
    //
    std::string GetToolset();

    //
    //  Adds files to the project.
    //
    void AddFiles(std::initializer_list<std::wstring> fileList);

    //
    //  Queries if project contains given file, adds it if add is true
    //
    ProjectFile* File(const wchar_t* file, bool add);

    //
    // Visits each project configuration, if configurationName & platformName - uses additional filtering, otherwise visits all configurations.
    //
    void VisitConfigurations( std::function<void (VCConfiguration&)> visitConf, const wchar_t* configurationName = nullptr, const wchar_t* platformName = nullptr );


protected:
    // Project name, typically used to identify project within solution or specify saved filename if file is not specified during save.
    std::wstring name;

    // Directory where project shall be saved.
    std::wstring saveDir;

    //  "Win32", "х64", ...
    std::vector<std::wstring> platforms;

    //  "Debug", "Release", user defined
    std::vector<std::wstring> configurationNames;

    //
    //  Gets current configuration names, if not initialized yet, returns default "Debug" / "Release" set.
    //
    std::vector<std::wstring>& GetConfigurationNames();

    // Project settings
    std::vector< std::shared_ptr<VCConfiguration> > configurations;

    //  List of files within a project.
    std::list< std::shared_ptr<ProjectFile> >  files;

    // Project guid
    GUID guid;

    //
    //  Visual studio version, in year. e.g. 2017, 2019, ...
    //
    int vsVersion;

    // Platform Toolset, e.g. "v141" (for vs2017), "142" (for vs2019), "Clang_5_0" ...
    std::string toolset;

    // xml document.
    pugi::xml_document xmldoc;

    pugi::xml_node project( );
    pugi::xml_node projectGlobals;
    pugi::xml_node markForPropertyGroup;

public:
    //
    // Selects project node with specific name / label, of specific confName/platform, creates if does not exists
    //
    pugi::xml_node selectProjectNodes(const wchar_t* name, const wchar_t* label, const wchar_t* confName, const wchar_t* platform);

protected:
    //
    // Platforms or Configurations arrays updated, bPlatforms == true - platforms false = configurations, bAdd = true - added, bAdd = false - removed.
    //
    void PlatformConfigurationsUpdated(std::initializer_list<std::wstring> items, bool bPlatforms, bool bAdd);
};

pugi::xml_node LocateInsert(pugi::xml_node current, bool asChild, const wchar_t* name2select,
    const wchar_t* confName, const wchar_t* platform, const wchar_t* label = nullptr, bool bLabelAfterCondition = true);


