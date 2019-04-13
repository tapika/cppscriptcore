#include "pch.h"
#include "ProjectFile.h"
#include <filesystem>
#include <algorithm>

using namespace std;
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

