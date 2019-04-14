#pragma once
#include "../pugixml/pugixml.hpp"
#include <string>
#include <functional>
#include "ProjectFileTypes.h"

class Project;

//
// Information about that particular file.
//
class SPM_DLLEXPORT ProjectFile : ReflectClassT<ProjectFile>
{
public:
    ProjectFile();

    REFLECTABLE(ProjectFile,
        (ProjectItemGeneralConf)General
    );

    void VisitTool(std::function<void(BuildToolProperties&)> visitConf, const char* configurationName = nullptr, const char* platformName = nullptr);
    
    // Generic autoprobe - file extension to guessed type.
    static EItemType GetFromPath(const wchar_t* file);

    // Relative path to file (from project path perspective)
    std::wstring relativePath;

    virtual void OnAfterSetProperty(ReflectPath& path);

    // Configuration/platform specific tools
    std::vector< std::shared_ptr<BuildToolProperties> > tools;

    // Xml node containing child xml data.
    pugi::xml_node node;

    //Project owning this file
    Project* project;
};

