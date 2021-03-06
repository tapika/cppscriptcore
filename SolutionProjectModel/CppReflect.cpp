#include "pch.h"
#include <regex>
#include "CppReflect.h"
#include "../pugixml/pugixml.hpp"          //pugi::xml_node
#include "boolinq.h"

using namespace pugi;
using namespace boolinq;
using namespace std;

std::map< std::string, pfuncGetClassInfo > CppTypeInfo::classNameToClassInfo;


FieldInfo* CppTypeInfo::GetField(const char* name)
{
    for( auto& f: fields )
        if(f.name == name)
            return &f;

    return nullptr;
}

int CppTypeInfo::GetFieldIndex(const char* name)
{
    for( size_t i = 0; i < fields.size(); i++ )
        if (fields[i].name == name )
            return (int)i;

    return -1;
}

ReflectClassTypeNameInfo::ReflectClassTypeNameInfo(pfuncGetClassInfo func, const char* className)
{
    if (strncmp(className, "class ", 6) == 0)
        className += 6;

    ClassTypeName = className;
    CppTypeInfo::classNameToClassInfo[className] = func;
}

//
//  Serializes class instance to xml node.
//
void DataToNode( xml_node& _node, void* pclass, CppTypeInfo& type )
{
    USES_CONVERSION;
    xml_node node = _node.append_child( CA2W( type.name ) );

    for (FieldInfo fi : type.fields)
    {
        void* p = ((char*)pclass) + fi.offset;
        CppTypeInfo* arrayType = nullptr;

        if( !fi.fieldType->GetArrayElementType( arrayType ) )
        {
            // Simple type, append as attribute.
            CStringW s = fi.fieldType->ToString( p );
            if( s.GetLength() )     // Don't serialize empty values.
                node.append_attribute( CA2W( fi.name ) ) = s;
            continue;
        }

        if( !arrayType )
            continue;

        xml_node fieldNode = node.append_child( CA2W( fi.name ) );

        size_t size = fi.fieldType->ArraySize( p );
        for( size_t i = 0; i < size; i++ )
        {
            void* pstr2 = fi.fieldType->ArrayElement( p, i );
            DataToNode( fieldNode, pstr2, *arrayType );
        }
    } //for each
} //DataToNode

//  Helper class.
struct xml_string_writer : xml_writer
{
    CStringA result;
    virtual void write( const void* data, size_t size )
    {
        result += CStringA( (const char*)data, (int)size );
    }
};

CStringA ToXML_UTF8( void* pclass, CppTypeInfo& type )
{
    xml_document doc;
    xml_node decl = doc.prepend_child( pugi::node_declaration );
    decl.append_attribute( _T( "version" ) ) = _T( "1.0" );
    decl.append_attribute( _T( "encoding" ) ) = _T( "utf-8" );
    DataToNode( doc, pclass, type );

    xml_string_writer writer;
    doc.save( writer );
    return writer.result;
}

CStringW ToXML( void* pclass, CppTypeInfo& type )
{
    xml_document doc;
    xml_node decl = doc.prepend_child( pugi::node_declaration );
    decl.append_attribute( _T( "version" ) ) = _T( "1.0" );
    decl.append_attribute( _T( "encoding" ) ) = _T( "utf-8" );
    DataToNode( doc, pclass, type );

    xml_string_writer writer;
    doc.save( writer );
    return CStringW(CA2T(writer.result, CP_UTF8));
}


//
//  Deserializes xml to class structure, returns true if succeeded, false if fails.
//  error holds error information if any.
//
bool NodeToData( xml_node node, void* pclass, CppTypeInfo& type, CStringW& error )
{
    CStringA name = node.name();

    if( type.name != name )
    {
        error.AppendFormat( _T( "Expected xml tag '%S', but found '%S' instead" ), type.name.GetBuffer(), name.GetBuffer() );
        return false;
    }

    for (FieldInfo fi: type.fields)
    {
        void* p = ((char*)pclass) + fi.offset;
        CppTypeInfo* arrayType = nullptr;

        if( !fi.fieldType->GetArrayElementType( arrayType ) )
        {
            // Simple type, query from attribute value.
            xml_attribute attr = node.attribute( CA2W( fi.name ) );
            fi.fieldType->FromString( p, attr.value() );
            continue;
        }

        if( !arrayType )
            continue;

        xml_node fieldNode = node.child( CA2W( fi.name ) );
        if( fieldNode.empty() )
            continue;

        int size = 0;
        for( auto it = fieldNode.children().begin(); it != fieldNode.children().end(); it++ )
            size++;

        fi.fieldType->SetArraySize( p, size );
        int i = 0;
        for( auto it = fieldNode.children().begin(); it != fieldNode.children().end(); it++ )
        {
            void* pstr2 = fi.fieldType->ArrayElement( p, i );
            if( !NodeToData( *it, pstr2, *arrayType, error ) )
                return false;
            i++;
        }
    } //for each

    return true;
} //NodeToData

bool FromXml( void* pclass, CppTypeInfo& type, const wchar_t* xml, CStringW& error )
{
    xml_document doc2;

    xml_parse_result res = doc2.load_string( xml );
    if( !res )
    {
        error = L"Failed to load xml: ";
        error += res.description();
        return false;
    }

    return NodeToData( doc2.first_child(), pclass, type, error );
}


ReflectPath::ReflectPath(CppTypeInfo& type, const char* _propertyName)
{
    // Doubt that class hierarchy is more complex than 5 levels, but increase this size if it's.
    steps.reserve(5);
    ReflectPathStep step;
    step.typeInfo = &type;
    step.propertyName = _propertyName;
    step.instance = nullptr;
    steps.push_back(step);
}

void ReflectPath::Init(ReflectClass* instance)
{
    steps.resize(1);
    steps[0].instance = instance;
}

ReflectClass::ReflectClass():
    _parent(nullptr)
{
}

void ReflectClass::ReflectConnectChildren(ReflectClass* parent)
{
    CppTypeInfo& typeinfo = GetType();
    char* inst = nullptr;
    int idx = 0;
    
    for( auto& fi: typeinfo.fields )
    {
        mapFieldToIndex[fi.name] = idx;

        if( fi.fieldType->IsPrimitiveType() )
            continue;

        if( !inst )
            inst = (char*)ReflectGetInstance();

        ReflectClass* child = fi.fieldType->ReflectClassPtr(inst + fi.offset);
        child->_parent = this;

        // Reconnect children as well recursively.
        child->ReflectConnectChildren(this);

        if( child->propertyName.length() != 0 )
            mapFieldToIndex[child->propertyName.c_str()] = idx;

        idx++;

        if (child->propertyName.length() == 0)
        {
            for (auto fi : child->GetType().fields)
                mapFieldToIndex[fi.name] = idx++;
        }

    }
}

//
//  Pushes information about current path step
//
void ReflectClass::PushPathStep(ReflectPath& path)
{
    if (propertyName.length() == 0)
        return;

    ReflectPathStep step;
    step.typeInfo = &_parent->GetType();
    step.propertyName = propertyName.c_str();
    step.instance = _parent;
    path.steps.push_back(step);
}

void ReflectClass::OnBeforeGetProperty(ReflectPath& path)
{
    if(!_parent)
        return;

    PushPathStep(path);
    _parent->OnBeforeGetProperty(path);
}

void ReflectClass::OnAfterSetProperty(ReflectPath& path)
{
    if (!_parent)
        return;

    PushPathStep(path);
    _parent->OnAfterSetProperty(path);
}





