#pragma once
#include <string>
#include <functional>
#include "ProjectFileTypes.h"

class Project;

//
// Information about that particular file.
//
class SPM_DLLEXPORT ProjectFile : public ReflectClassT<ProjectFile>
{
public:
    ProjectFile();

    REFLECTABLE(ProjectFile,
        (ProjectItemGeneralConf)General
    );

    void VisitTool(
        std::function<void(PlatformConfigurationProperties*)> visitConf, 
        CppTypeInfo* confType,      // Type to create, null if just to locate
        const wchar_t* configurationName = nullptr, 
        const wchar_t* platform = nullptr
    );
    
    // Generic autoprobe - file extension to guessed type.
    static EItemType GetFromPath(const wchar_t* file);

    // Relative path to file (from project path perspective)
    std::wstring relativePath;

    virtual void OnAfterSetProperty(ReflectPath& path);

    // Configuration/platform specific tools
    std::vector< std::shared_ptr<PlatformConfigurationProperties> > tools;

    // Xml node containing child xml data.
    pugi::xml_node node;

    //Project owning this file
    Project* project;
};

