#pragma once
#include "ProjectFile.h"
#include <string>
#include <vector>

class Project;


class SPM_DLLEXPORT VCConfiguration : ReflectClassT<VCConfiguration>
{
public:
    Project* project;
    pugi::xml_node idgConfNode;
    pugi::xml_node pgNode;
    pugi::xml_node pgConfigurationNode;

    VCConfiguration() :
        project(nullptr)
    {
        Init();
    }

    VCConfiguration(const VCConfiguration& clone) :
        project(clone.project),
        configurationName(clone.configurationName),
        platform(clone.platform)
    {
        Init();
    }

    void Init()
    {
        // Define configuration "category" (will be used when serializing / restoring)
        Linker.fieldName = "Link";
        CCpp.fieldName = "ClCompile";
        ReflectConnectChildren(nullptr);
    }


    virtual void OnAfterSetProperty(ReflectPath& path);

    // Configuration name / platform of specific configuration
    std::wstring configurationName;
    std::wstring platform;

    //
    // Individual tools settings, depending on project type (static library, dynamic library) individual tool configuration is not necessarily used.
    //
    REFLECTABLE(VCConfiguration,
        (GeneralConf)General,
        (CCppConf)CCpp,
        (LinkerConf)Linker
    );
};

void ReflectCopy(ReflectPath& path, pugi::xml_node toNode);

