#pragma once
#include <string>
#include <vector>
#include "helpers.h"

typedef struct
{
    int             version;
    std::wstring    InstallPath;
}VisualStudioInfo;


std::vector<VisualStudioInfo> SPM_DLLEXPORT getInstalledVisualStudios(void);

