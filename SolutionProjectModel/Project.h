#pragma once
#include "../pugixml/pugixml.hpp"
#include <string>
#include <vector>

#ifdef SPM_EXPORT
#define SPM_DLLEXPORT __declspec(dllexport)
#else
#define SPM_DLLEXPORT __declspec(dllimport)
#endif

// warning C4251: ... needs to have dll-interface to be used by clients of class ...
#pragma warning( disable: 4251 )
// warning C4275: non dll-interface class 'pugi::xml_document' used as base for dll-interface class 'Solution'
#pragma warning( disable: 4275 )

//---------------------------------------------------------
//  Project configuration
//---------------------------------------------------------
class SPM_DLLEXPORT Configuration
{
public:
    //  "Debug", "Release", user defined
    std::string ConfigurationName;

    //  "Win32", "х64", ...
    std::string PlatformName;
};


//---------------------------------------------------------
//  Project
//---------------------------------------------------------
class SPM_DLLEXPORT Project : pugi::xml_document
{
public:
    std::vector<Configuration> Configurations;

    bool Load(const wchar_t* file);
};

