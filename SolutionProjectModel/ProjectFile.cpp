#include "pch.h"
#include "ProjectFile.h"
#include "Project.h"
#include <filesystem>
#include <algorithm>

using namespace std;
using namespace pugi;
using namespace std::filesystem;

string lowercased( string s )
{
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

ProjectFile::ProjectFile()
{
    ReflectConnectChildren(nullptr);
}

void ProjectFile::VisitTool(
    std::function<void(PlatformConfigurationProperties&)> visitConf, 
    CppTypeInfo* confType,
    const wchar_t* _configurationName, const wchar_t* _platform)
{
    if(!project)
        return;

    function<void(const wchar_t*, const wchar_t*)> visitTool = [&]( const wchar_t* configurationName, const wchar_t* platform )
    {
        auto it = find_if(tools.begin(), tools.end(), [&](auto& t) { return t->platform == platform && t->configurationName == configurationName; });
        if (it != tools.end())
        {
            if(visitConf)
                visitConf(**it);

            return;
        }
        
        if(!confType)
            return;

        shared_ptr<PlatformConfigurationProperties> props;
        confType->ReflectCreateInstance(props);
        props->node = LocateInsert(node, true, as_wide(confType->name).c_str(), configurationName, platform);
        tools.push_back(props);

        if (visitConf)
            visitConf(*props);
    };

    if (_configurationName == nullptr || _platform == nullptr)
    {
        project->VisitConfigurations
        (
            [&](VCConfiguration& c)
            {
                visitTool(c.platform.c_str(), c.configurationName.c_str());
            },
            _configurationName, _platform
        );
    } 
    else
    {
        visitTool(_configurationName, _platform);
    }
}


//
// Generic autoprobe - file extension to guessed type.
//
EItemType ProjectFile::GetFromPath(const wchar_t* file)
{
    string ext = lowercased(path(file).extension().string().substr(1));

    struct TypeInfoStr
    {
        const char* ext;
        EItemType type;
    } types [] = 
    {
        {"properties", AntProjectPropertiesFile},
        {"h", ClInclude},
        {"c", ClCompile},
        {"cxx", ClCompile},
        {"cpp", ClCompile},
        {"java", JavaCompile},
        {"template", GradleTemplate},
        {"rc", ResourceCompile},
        {"ico", Image},
        {"txt", Text},
        {"natvis", Natvis},
        {"?", None}
    };

    TypeInfoStr* end = types + _countof(types) - 1;
    return find_if(types, end, [ext](TypeInfoStr& ti) { return ti.ext == ext; } )->type;
}

void ProjectFile::OnAfterSetProperty(ReflectPath& path)
{
    if(path.steps.size() == 1)
    {
        auto step = path.steps.front();

        if(strcmp(step.propertyName, "ItemType") == 0)
        {
            FieldInfo* fi = step.typeInfo->GetField(step.propertyName);
            CStringW newName = fi->fieldType->ToString((char*)step.instance->ReflectGetInstance() + fi->offset);

            if(newName != node.name())
            {
                for(auto childnode: node.children())
                    node.remove_child(childnode);

                node.set_name(newName);
            }
        }
    }
}

