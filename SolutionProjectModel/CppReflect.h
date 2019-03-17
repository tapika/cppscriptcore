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

    //
    // Gets field by name, nullptr if not found.
    //
    FieldInfo* GetField(const char* name);

    //
    // Get field index, -1 if not found.
    //
    int GetFieldIndex(const char* name);
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
    fi.offset = offsetof(_className, ARGNAME_UNDERSCORE(x));    \
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
                    (fieldType)fieldName
                              ^ no space in between, otherwise
defines wont expand correctly.
*/
#define REFLECTABLE(className, ...)                             \
    /* Dump field types and names with underscore */            \
    DOFOREACH_SEMICOLON(ARGPAIR_FIELD_UNDERSCORED,__VA_ARGS__)  \
    /* Dump accessor functions (Get/Set<field name> */          \
    DOFOREACH_SEMICOLON(ARGPAIR_ACCESSOR,__VA_ARGS__)           \
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

class ReflectClass;

//
//  One step in whole <main class, sub-class, sub-class, ....> scenario
//
class ReflectPathStep
{
public:
    //
    //  reflectable class <this> pointer converted to ReflectClass. Restore original pointer by calling instance[x]->ReflectGetInstance(). 
    //
    ReflectClass* instance;

    //
    // Field name
    //
    const char*   field;

    //
    // Type information of instance
    //
    CppTypeInfo*  typeInfo;
};


//
//  Path to highlight property set / get.
//
class ReflectPath
{
public:
    ReflectPath(CppTypeInfo& type, const char* field);
    
    void Init(ReflectClass* instance);
    
    std::vector<ReflectPathStep>  steps;
};


//
//  All classes which use C++ reflection should inherit from this base class.
//
class ReflectClass
{
protected:
    // Parent class, nullptr if don't have parent class.
    ReflectClass*   _parent;

public:
    // Field name under assignment. If empty - can be used to bypass structure (exists on API level, does not exists in file format level), if non-empty -
    // specifies fieldname to be registered on parent.
    std::string  fieldName;

    // Map field name to index (used when sorting fields)
    std::map<CStringA, int> mapFieldToIndex;

    ReflectClass();

    //
    //  Use current class instance provided as parent to replicate <_parent> pointer
    //  of all children, recursively. parent can be also nullptr if topmost class,
    //
    void ReflectConnectChildren(ReflectClass* parent);

    //  Gets parent's ReflectClass which contains this type.
    inline ReflectClass* GetParent()
    {
        return _parent;
    }

    virtual CppTypeInfo& GetType() = 0;
    virtual void* ReflectGetInstance() = 0;

    //  By default set / get property rebroadcats event to parent class
    void PushPathStep(ReflectPath& path);
    virtual void OnBeforeGetProperty(ReflectPath& path);
    virtual void OnAfterSetProperty(ReflectPath& path);
};

template <class T>
class ReflectClassT : public ReflectClass
{
public:
    virtual CppTypeInfo& GetType()
    {
        return T::GetType();
    }

    virtual void* ReflectGetInstance()
    {
        return (T*) this;
    }
};

