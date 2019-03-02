#pragma once
#include <string>

template <class Enum>
class EnumReflect
{
public:
    static const char* getEnums() { return ""; }
};

#define DECLARE_ENUM(name, ...)                                         \
    enum name { __VA_ARGS__ };                                          \
    template <>                                                         \
    class EnumReflect<##name> {                                         \
    public:                                                             \
        static const char* getEnums() { return #__VA_ARGS__; }          \
    };

/*
    Basic usage:

    Declare enumeration:

DECLARE_ENUM( enumName,

    enumValue1,
    enumValue2,
    enumValue3,

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

    WARNING: At the moment assigning enum value to specific number is not supported.
*/

//
//  Converts enumeration to string, if not found - empty string is returned.
//
template <class T>
std::string EnumToString(T t)
{
    const char* enums = EnumReflect<T>::getEnums();
    const char *token, *next = enums - 1;
    int id = (int)t;

    do
    {
        token = next + 1;
        if (*token == ' ') token++;
        next = strchr(token, ',');
        if (!next) next = token + strlen(token);

        if (id == 0)
            return std::string(token, next);
        id--;
    } while (*next != 0);

    return std::string();
}

//
//  Converts string to enumeration, if not found - false is returned.
//
template <class T>
bool StringToEnum(const char* enumName, T& t)
{
    const char* enums = EnumReflect<T>::getEnums();
    const char *token, *next = enums - 1;
    int id = 0;

    do
    {
        token = next + 1;
        if (*token == ' ') token++;
        next = strchr(token, ',');
        if (!next) next = token + strlen(token);

        if (strncmp(token, enumName, next - token) == 0)
        {
            t = (T)id;
            return true;
        }

        id++;
    } while (*next != 0);

    return false;
}


