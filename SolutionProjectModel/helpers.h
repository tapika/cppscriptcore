#pragma once
#include <string>
#include <algorithm>
#include <windows.h>

#ifdef SPM_EXPORT
    #define SPM_DLLEXPORT __declspec(dllexport)
#else
    #define SPM_DLLEXPORT __declspec(dllimport)
#endif


template <typename T>
std::basic_string<T> svaFormat(const T* format, va_list args)
{
    int size;

    if constexpr (std::is_same_v<T, char>)
        size = vsnprintf(nullptr, 0, format, args);
    else
        size = _vsnwprintf(nullptr, 0, format, args);

    size++; // Zero termination
    std::basic_string<T> s;
    s.resize(size);

    if constexpr (std::is_same_v<T, char>)
        vsnprintf(&s[0], size, format, args);
    else
        _vsnwprintf(&s[0], size, format, args);

    return s;
}

//
//  Formats string/wstring according to format, if formatting fails (e.g. invalid %s pointer - returns empty string)
//
template <typename T>
std::basic_string<T> sFormat(const T* format, ...)
{
    va_list args;
    va_start(args, format);
    return svaFormat(format, args);
}

void SPM_DLLEXPORT throwFormat(const char* format, ...);

std::string SPM_DLLEXPORT GetLastErrorMessageA( DWORD code = GetLastError() );

void SPM_DLLEXPORT ThrowLastError( DWORD code = GetLastError() );

int SPM_DLLEXPORT ExecCmd( const wchar_t* cmd );

//
//  Lowercases string
//
template <typename T>
std::basic_string<T> lowercase(const std::basic_string<T>& s)
{
    std::basic_string<T> s2 = s;
    std::transform(s2.begin(), s2.end(), s2.begin(), tolower);
    return std::move(s2);
}

//
// Uppercases string
//
template <typename T>
std::basic_string<T> uppercase(const std::basic_string<T>& s)
{
    std::basic_string<T> s2 = s;
    std::transform(s2.begin(), s2.end(), s2.begin(), toupper);
    return std::move(s2);
}

