#pragma once
#include <atlstr.h>
#include "TypeTraits.h"
#include "MacroHelpers.h"             //DOFOREACH_SEMICOLON
#include <memory>                     //shared_ptr
#include <vector>


class FieldInfo;
class CppTypeInfo
{
public:
    //  Type (class) name
    CStringA name;
    std::vector<FieldInfo> fields;
};


class TypeTraits;
class FieldInfo
{
public:
    CStringA name;

    void SetName( const char* fieldName )
    {
        if( fieldName[0] == ' ' ) fieldName++;      // Result of define macro expansion, we fix it here.
        name = fieldName;
    }

    int offset;                                     // Field offset within a class instance
    std::shared_ptr<TypeTraits> fieldType;          // Class for field conversion to string / back from string. We must use 'new' otherwise virtual table does not gets initialized.
};


#define PUSH_FIELD_INFO(x)                                      \
    fi.SetName( ARGNAME_AS_STRING(x) );                         \
    fi.offset = offsetof(_className, ARGNAME(x));               \
    fi.fieldType.reset(new TypeTraitsT< ARGTYPE(x) >());        \
    t.fields.push_back(fi);                                     \

/*
Before using this macro, you must define your own types conversion
classes, for example see template class TypeTraitsT.

If you get compilation error, then it makes sense to try out first
without REFLECTABLE define, so you can specify normal C++ field
in class first, then adapt it under REFLECTABLE.

Also if your field does not needs to be serialized, declare it outside
of REFLECTABLE define.

While declaring REFLECTABLE(className, 
                    (fieldType) fieldName
                               ^ keep a space in between
fieldType <> fieldName otherwise intellisense might not work.
*/
#define REFLECTABLE(className, ...)                             \
    /* Dump field types and names */                            \
    DOFOREACH_SEMICOLON(ARGPAIR,__VA_ARGS__)                    \
    /* typedef is accessable from PUSH_FIELD_INFO define */     \
    typedef className _className;                               \
                                                                \
    static CppTypeInfo& GetType()                               \
    {                                                           \
        static CppTypeInfo t;                                   \
        if( t.name.GetLength() ) return t;                      \
        t.name = #className;                                    \
        FieldInfo fi;                                           \
        /* Dump offsets and field names */                      \
        DOFOREACH_SEMICOLON(PUSH_FIELD_INFO,__VA_ARGS__)        \
        return t;                                               \
    }                                                           \

CStringA ToXML_UTF8( void* pclass, CppTypeInfo& type );
CStringW ToXML( void* pclass, CppTypeInfo& type );

//
//  Serializes class instance to xml string.
//
template <class T>
CStringA ToXML_UTF8( T* pclass )
{
    CppTypeInfo& type = T::GetType();
    return ToXML_UTF8(pclass, type);
}

template <class T>
CStringW ToXML( T* pclass )
{
    CppTypeInfo& type = T::GetType();
    return ToXML( pclass, type );
}

bool FromXml( void* pclass, CppTypeInfo& type, const wchar_t* xml, CStringW& error );

//
//  Deserializes class instance from xml data. pclass must be valid instance where to fetch data.
//
template <class T>
bool FromXml( T* pclass, const wchar_t* xml, CStringW& error )
{
    CppTypeInfo& type = T::GetType();
    return FromXml(pclass, type, xml, error);
}


