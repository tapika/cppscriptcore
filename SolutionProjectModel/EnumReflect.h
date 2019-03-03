#pragma once
#include <string>
#include <map>
#include <regex>

template <class Enum>
class EnumReflect
{
public:
    static const char* getEnums() { return ""; }
};

//
//  Just a container for each enumeration type.
//
template <class Enum>
class EnumReflectBase
{
public:
    static std::map<std::string, int> enum2int;
    static std::map<int, std::string> int2enum;

    static void EnsureEnumMapReady( const char* enumsInfo )
    {
        if (*enumsInfo == 0 || enum2int.size() != 0 )
            return;

        // Should be called once per each enumeration.
        std::string senumsInfo(enumsInfo);
        std::regex re("^([a-zA-Z_][a-zA-Z0-9_]+) *=? *([^,]*)(,|$) *");     // C++ identifier to optional " = <value>"
        std::smatch sm;
        int value = 0;

        for (; regex_search(senumsInfo, sm, re); senumsInfo = sm.suffix(), value++)
        {
            string enumName = sm[1].str();
            string enumValue = sm[2].str();

            if (enumValue.length() != 0)
                value = atoi(enumValue.c_str());

            enum2int[enumName] = value;
            int2enum[value] = enumName;
        }
    }
};

template <class Enum>
std::map<std::string, int> EnumReflectBase<Enum>::enum2int;

template <class Enum>
std::map<int, std::string> EnumReflectBase<Enum>::int2enum;


#define DECLARE_ENUM(name, ...)                                         \
    enum name { __VA_ARGS__ };                                          \
    template <>                                                         \
    class EnumReflect<##name>: public EnumReflectBase<##name> {         \
    public:                                                             \
        static const char* getEnums() { return #__VA_ARGS__; }          \
    };




/*
    Basic usage:

    Declare enumeration:

DECLARE_ENUM( enumName,

    enumValue1,
    enumValue2,
    enumValue3 = 5,

    // comment
    enumValue4
);

    Conversion logic:

    From enumeration to string:

        printf( EnumToString(enumValue3).c_str() );

    From string to enumeration:

       enumName value;

       if( !StringToEnum("enumValue4", value) )
            printf("Conversion failed...");
*/

//
//  Converts enumeration to string, if not found - empty string is returned.
//
template <class T>
std::string EnumToString(T t)
{
    EnumReflect<T>::EnsureEnumMapReady(EnumReflect<T>::getEnums());
    auto& int2enum = EnumReflect<T>::int2enum;
    auto it = int2enum.find(t);
    
    if (it == int2enum.end())
        return "";

    return it->second;
}

//
//  Converts string to enumeration, if not found - false is returned.
//
template <class T>
bool StringToEnum(const char* enumName, T& t)
{
    EnumReflect<T>::EnsureEnumMapReady(EnumReflect<T>::getEnums());
    auto& enum2int = EnumReflect<T>::enum2int;
    auto it = enum2int.find(enumName);

    if (it == enum2int.end())
        return false;

    t = (T) it->second;
    return true;
}


