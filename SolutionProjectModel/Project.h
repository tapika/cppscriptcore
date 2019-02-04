#pragma once
#pragma once
#include "../pugixml/pugixml.hpp"

#ifdef SPM_EXPORT
#define SPM_DLLEXPORT __declspec(dllexport)
#else
#define SPM_DLLEXPORT __declspec(dllimport)
#endif

class SPM_DLLEXPORT Project : protected pugi::xml_document
{
public:
    bool Load(const wchar_t* file);

};

