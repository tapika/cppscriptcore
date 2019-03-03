#pragma once
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

//
// Generic autoprobe - file extension to guessed type.
//
ItemType ProjectFile::GetFromPath(const wchar_t* file)
{
    string ext = lowercased(path(file).extension().string().substr(1));

    struct TypeInfoStr
    {
        const char* ext;
        ItemType type;
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
        {"?", None}
    };

    TypeInfoStr* end = types + _countof(types) - 1;
    return find_if(types, end, [ext](TypeInfoStr& ti) { return ti.ext == ext; } )->type;
}
