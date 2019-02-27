#include "Project.h"
#include <stdio.h>
#include <boost/uuid/detail/sha1.hpp>
#include <cguid.h>                                      //GUID_NULL
#include <objbase.h>                                    //StringFromCLSID
#include <filesystem>

using namespace pugi;
using namespace std;
using namespace experimental;                           //filesystem

//
// Expose over .dll boundary
//
template class __declspec(dllexport) std::allocator<char>;
template class __declspec(dllexport) std::basic_string<char, std::char_traits<char>, std::allocator<char> >;

Project::Project()
{
    guid = GUID_NULL;
}

//
// Gets project guid, initialized if it's not initialized yet
//
wstring Project::GetGuid(void)
{
    if (guid == GUID_NULL)
    {
        //
        // Generates Guid based on String. Key assumption for this algorithm is that name is unique (across where it it's being used)
        // we compute sha - 1 hash from string and then pass it to guid.
        //
        boost::uuids::detail::sha1 sha1;
        auto pname = as_utf8(name);
        sha1.process_bytes(&pname[0], pname.length());
        unsigned hash[5] = { 0 };
        sha1.get_digest(hash);

        // Hash is 20 bytes, but we need 16. We loose some of "uniqueness", but I doubt it will be fatal
        memcpy(&guid, hash, sizeof(GUID));
    }

    wchar_t* gs = nullptr;
    wstring r;

    if( StringFromCLSID(guid, &gs) == S_OK )
        r = gs;

    CoTaskMemFree(gs);
    return r;
}



template <class T, class I >
bool vectorContains(const vector<T>& v, I& t)
{
    bool found = (std::find(v.begin(), v.end(), t) != v.end());
    return found;
}


void Project::AddPlatform(const char* platform)
{
    if (!vectorContains(platforms, platform))
        platforms.push_back(platform);
}


void Project::AddPlatforms(initializer_list<string> _platforms)
{
    platforms.reserve(_platforms.size());
    for (initializer_list<string>::iterator i = _platforms.begin(); i != _platforms.end(); i++)
        AddPlatform(i->c_str());
}

void Project::AddConfiguration(const char* configuration)
{
    if (!vectorContains(configurations, configuration))
        configurations.push_back(configuration);
}

void Project::AddConfigurations(std::initializer_list<std::string> _configurations)
{
    configurations.reserve(_configurations.size());
    for (initializer_list<string>::iterator i = _configurations.begin(); i != _configurations.end(); i++)
        AddConfiguration(i->c_str());
}


//
// Loads .vcxproj file.
//
bool Project::Load(const wchar_t* file)
{
    xml_parse_result res = load_file(file, parse_default | parse_declaration | parse_ws_pcdata_single);

    if (res.status != status_ok)
        return false;

    xml_node node = select_node(L"/Project/ItemGroup[@Label='ProjectConfigurations']").node();

    for (xml_node conf : node.children())
    {
        const wchar_t* xmltag[] = { L"Configuration" , L"Platform" };
        void (Project::*func [])(const char*) = { &Project::AddConfiguration, &Project::AddPlatform };

        for( int i = 0; i < _countof(xmltag); i++)
            (this->*func[i])( as_utf8(conf.child(xmltag[i]).text().get()).c_str() );
    }

    return true;
}

//
//  selects node with specific label attribute or creates required child and set's it's attribute to required value.
//
xml_node GetLabelledNode(xml_node node, const wchar_t* elemName, const wchar_t* attrValue)
{
    wstring q = wstring(elemName) + L"[@Label='" + attrValue + L"']";
    xml_node r = node.select_node(q.c_str()).node();
    if (r.empty())
    {
        r = node.append_child(elemName);
        r.append_attribute(L"Label").set_value(attrValue);
    }

    return r;
}

//
//  Selects specific child node or creates it.
//
xml_node GetOrCreate(xml_node& node, const wchar_t* name)
{
    xml_node r = node.child(name);
    if (r.empty())
        r = node.append_child(name);

    return r;
}

//
//  Formats wstring according to format.
//
wstring wformat(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    int size = _vsnwprintf(nullptr, 0, format, args);
    size++; // Zero termination
    wstring ws;
    ws.reserve(size);
    _vsnwprintf(&ws[0], size, format, args);
    va_end(args);
    return ws;
}

void checkImportNode(xml_node& node, const wchar_t* text)
{
    xml_node r = node.select_node(wformat(L"Import[@Project='%s']", text).c_str() ).node();
    if (!r.empty())
        return;

    node.append_child(L"Import").append_attribute(L"Project").set_value(text);
}


//
// Saves project file
//
bool Project::Save(const wchar_t* file)
{
    wstring path;

    if (file)
    {
        path = file;

        // Update project name
        name = filesystem::path(file).stem();
        guid = GUID_NULL;
    }
    else
        path = name + L".vcxproj";


    // Specify utf-8 encoding.
    pugi::xml_node decl;

    for (auto n : children())
        if (n.type() == pugi::node_declaration)
            decl = n;

    if (decl.empty())
    {
        decl = prepend_child(pugi::node_declaration);
        decl.append_attribute(L"version") = L"1.0";
        decl.append_attribute(L"encoding") = L"utf-8";
    }

    xml_node proj = GetOrCreate(*this, L"Project");
    
    xml_node confs = GetLabelledNode(proj, L"ItemGroup", L"ProjectConfigurations");
    xml_node nGlobals = GetLabelledNode(proj, L"PropertyGroup", L"Globals");
    GetOrCreate(nGlobals,L"ProjectGuid").text().set(GetGuid().c_str());

    checkImportNode(proj, LR"($(VCTargetsPath)\Microsoft.Cpp.Default.props)");

    if (!configurations.size())
        AddConfigurations({"Debug", "Release" });

    for (auto n : confs.children()) confs.remove_child(n);

    for( auto _p: platforms )
        for (auto _c : configurations)
        {
            xml_node n = confs.append_child(L"ProjectConfiguration");
            auto p = as_wide(_p);
            auto c = as_wide(_c);
            n.append_attribute(L"Include").set_value( (c + L"|" + p).c_str() );
            n.append_child(L"Configuration").text().set( c.c_str() );
            n.append_child(L"Platform").text().set( p.c_str() );
        }

    bool b  = save_file(path.c_str(), L"  ", format_indent | format_save_file_text, encoding_utf8);
    return b;
}



