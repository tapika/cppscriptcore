#pragma once
#include <string>
#include <vector>
#include "helpers.h"

typedef struct
{
    int             version;
    std::wstring    InstallPath;
}VisualStudioInfo;

//
// Queries for all currently installed Visual Studio versions on given machine.
//
// Function throws exception is any COM error occurs.
//
std::vector<VisualStudioInfo> SPM_DLLEXPORT getInstalledVisualStudios(void);

