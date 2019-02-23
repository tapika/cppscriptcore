#pragma once
#include "../pugixml/pugixml.hpp"
#include <string>
#include <vector>
#include <initializer_list>

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
//  Project
//---------------------------------------------------------
class SPM_DLLEXPORT Project : pugi::xml_document
{
public:
    //  "Win32", "х64", ...
    std::vector<std::string> platforms;

    //  "Debug", "Release", user defined
    std::vector<std::string> configurations;

    //
    // Add support for platform or configuration, if not yet added.
    //
    void AddPlatform(const char* platform);
    void AddPlatforms(std::initializer_list<std::string> _platforms);

    void AddConfiguration(const char* configuration);
    void AddConfigurations(std::initializer_list<std::string> _configuration);

    //
    // Loads .vcxproj file.
    //
    bool Load(const wchar_t* file);
};

