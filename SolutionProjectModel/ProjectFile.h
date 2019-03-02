#pragma once
#include "../pugixml/pugixml.hpp"

#ifdef SPM_EXPORT
#define SPM_DLLEXPORT __declspec(dllexport)
#else
#define SPM_DLLEXPORT __declspec(dllimport)
#endif


class SPM_DLLEXPORT ProjectFile
{
public:


protected:
};

