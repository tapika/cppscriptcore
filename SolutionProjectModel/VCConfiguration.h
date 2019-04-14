#pragma once
#include "ProjectFile.h"
#include <string>
#include <vector>

class Project;


class SPM_DLLEXPORT VCConfiguration : public PlatformConfigurationProperties, public ReflectClassT<VCConfiguration>
{
public:
    Project* project;
    pugi::xml_node idgConfNode;
    pugi::xml_node pgNode;
    pugi::xml_node pgConfigurationNode;

    VCConfiguration() :
        project(nullptr)
    {
        // Define configuration "category" (will be used when serializing / restoring)
        Linker.propertyName = "Link";
        CCpp.propertyName = "ClCompile";
        ReflectConnectChildren(nullptr);
    }

    virtual void OnAfterSetProperty(ReflectPath& path);

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
void ReflectCopyValue(ReflectPath& path, pugi::xml_node node);


