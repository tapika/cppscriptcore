#include "pch.h"
#define _WIN32_WINNT  _WIN32_WINNT_WIN7 /*0x0601*/
#include <SDKDDKVer.h>
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

