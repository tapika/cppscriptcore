#pragma once
#include "../pugixml/pugixml.hpp"
#include <string>
#include "ProjectFileTypes.h"

// warning C4251: ... needs to have dll-interface to be used by clients of class ...
#pragma warning( disable: 4251 )

//
// Information about that particular file.
//
class SPM_DLLEXPORT ProjectFile
{
public:
    //
    // Generic autoprobe - file extension to guessed type.
    //
    static ItemType GetFromPath(const wchar_t* file);

    // 
    // Relative path to file (from project path perspective)
    // 
    std::wstring relativePath;

    //
    // Xml node containing child xml data.
    //
    pugi::xml_node node;
};

