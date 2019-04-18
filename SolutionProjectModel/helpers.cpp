#include "pch.h"
#include "helpers.h"
#include <atlstr.h>                     //CStringA

using namespace std;

void SPM_DLLEXPORT throwFormat(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    throw exception(svaFormat(format, args).c_str());
}

string GetLastErrorMessageA( DWORD code )
{
    char* buf = nullptr;
    DWORD len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
        NULL, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0, NULL);

    if (!len)
        return sFormat("Windows error: %d", code);

    string msg(buf, len);
    LocalFree(buf);
    return msg;
}

void ThrowLastError(DWORD code)
{
    throw exception(GetLastErrorMessageA(code).c_str());
}

