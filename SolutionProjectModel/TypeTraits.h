#pragma once
#include <vector>
#include <string>                       //std::vector

#pragma warning(push)
#pragma warning(disable: 4100)          // Unreferenced parameter

class CppTypeInfo;
//
//  Base class for performing field conversion to string / from string.
//
class TypeTraits
{
public:
    virtual bool GetArrayElementType( CppTypeInfo*& type )
    {
        // Not array type
        return false;
    }

    //
    // If GetArrayElementType() returns true, returns size of array.
    //
    virtual size_t ArraySize( void* p )
    {
        return 0;
    }

    virtual void SetArraySize( void* p, size_t size )
    {
    }

    //
    //  Gets field (at p) array element at position i.
    //
    virtual void* ArrayElement( void* p, size_t i )
    {
        return nullptr;     // Invalid operation, since not array
    }

    //
    // Converts specific data to String.
    //
    // Default implementation: Don't know how to print given field, ignore it
    //
    virtual CStringW ToString( void* pField ) { return CStringW(); }
    
    //
    // Converts from String to data.
    //
    // Default implementation: Value cannot be set from string.
    //
    virtual void FromString( void* pField, const wchar_t* value ) { }
};

//
//  Generic class definition, which can be applied to any class type. This implementation however does nothing -
//  does not serializes or deserializes your type. You must override with your type specific implementation
//  for each supported data type. For more examples - see below.
//
template <class T>
class TypeTraitsT : public TypeTraits
{
public:
    virtual CStringW ToString( void* pField ) { return CStringW(); }
};

template <>
class TypeTraitsT<CString> : public TypeTraits
{
public:
    virtual CStringW ToString( void* pField )
    { 
        CString* s = (CString*)pField;
        return *s;
    }
    
    virtual void FromString( void* pField, const wchar_t* value )
    { 
        CString* s = (CString*)pField;
        *s = value;
    }
};

template <>
class TypeTraitsT<int> : public TypeTraits
{
public:
    virtual CStringW ToString( void* pField )
    {
        int* p = (int*) pField;
        char buf[10];
        _itoa_s(*p, buf, 10); 
        return buf;
    }

    virtual void FromString( void* pField, const wchar_t* value )
    {
        int* p = (int*)pField;
        *p = _wtoi(value);
    }
};


template <>
class TypeTraitsT<bool> : public TypeTraits
{
public:
    virtual CStringW ToString( void* p )
    {
        if( *(bool*)p )
            return L"true";

        return L"false";
    }
    
    virtual void FromString( void* pField, const wchar_t* value )
    {
        bool* pb = (bool*)pField;
        if( _wcsicmp(value, L"true") == 0 )
        {
            *pb = true;
            return;
        }

        *pb = false;
    }
};

template <class E>
class TypeTraitsT< std::vector<E> > : public TypeTraits
{
public:
    virtual bool GetArrayElementType( CppTypeInfo*& type )
    {
        __if_exists(E::GetType)
        {
            type = &E::GetType();
            return true;
        }
        
        return true;
    }
    
    virtual size_t ArraySize( void* p )
    {
        std::vector<E>* v = (std::vector<E>*) p;
        return v->size();
    }

    virtual void SetArraySize( void* p, size_t size )
    {
        std::vector<E>* v = (std::vector<E>*) p;
        v->resize(size);
    }

    virtual void* ArrayElement( void* p, size_t i )
    {
        std::vector<E>* v = (std::vector<E>*) p;
        return &v->at( i );
    }

    virtual CStringW ToString( void* pField )
    {
        return CStringW();
    }
};


/*
    COLORREF is typedef'ed from DWORD. But it's possible that we will want to serialize DWORD as well as a number, but we want to threat 
    color separately and independently from DWORD. We define here extra clue class just to be able to not to mix COLORREF and DWORD.
*/
class ColorRef
{
public:
    COLORREF color;

    ColorRef() : color( 0 )
    {
    }

    ColorRef( COLORREF _color ) : color( _color )
    {
    }

    unsigned char GetR() { return GetRValue( color ); }
    unsigned char GetG() { return GetGValue( color ); }
    unsigned char GetB() { return GetBValue( color ); }

    operator COLORREF&() { return color; }
};


template <>
class TypeTraitsT<ColorRef> : public TypeTraits
{
public:

    virtual CStringW ToString( void* pField )
    {
        char buf[256];
        ColorRef clr = *(ColorRef*)pField;
        sprintf_s( buf, sizeof( buf ), "%u,%u,%u", clr.GetR(), clr.GetG(), clr.GetB() );
        return buf;
    }

    virtual void FromString( void* pField, const wchar_t* value )
    {
        ColorRef* pclr = (ColorRef*)pField;
        int clr[3];
        if( swscanf_s( value, L"%u,%u,%u", &clr[0], &clr[1], &clr[2] ) == 3 )
            *pclr = RGB( clr[0], clr[1], clr[2] );
    }
};

#pragma warning(pop)

