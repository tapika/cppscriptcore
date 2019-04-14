#include "pch.h"
#include "Project.h"
#include "ProjectFileTypes.h"
#include "VCConfiguration.h"

using namespace pugi;
using namespace std;

void ProjectToolProperties::OnAfterSetProperty(ReflectPath& path)
{
    USES_CONVERSION;

    if( !projectFile )
        return;

    xml_node node = LocateInsert(projectFile->node, true, CA2W(path.steps.front().propertyName), configurationName.c_str(), platform.c_str());
    ReflectCopyValue(path, node);
}

