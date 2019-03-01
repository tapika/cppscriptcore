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
    SetVsVersion(2017);     // May change without further notice
}


void Project::SetVsVersion(int _vsVersion)
{
    vsVersion = _vsVersion;

    const wchar_t* toolsVersion = nullptr;
    switch (vsVersion)
    {
        case 2010:
        case 2012: toolsVersion = L"4.0";  break;
        case 2013: toolsVersion = L"12.0"; break;
        case 2015: toolsVersion = L"14.0"; break;
        case 2017: toolsVersion = L"15.0"; break;
            // From vs2019 not required anymore.
        default:                          break;
    }

    auto proj = project();
    if (toolsVersion)
        proj.append_attribute(L"ToolsVersion").set_value(toolsVersion);
    else
        proj.remove_attribute(L"ToolsVersion");

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
    AddPlatforms({ platform });
    //if (!vectorContains(platforms, platform))
    //    platforms.push_back(platform);
}


void Project::AddPlatforms(initializer_list<string> _platforms)
{
    PlatformConfigurationsUpdated(_platforms, true, true);
    //platforms.reserve(_platforms.size());
    //for (initializer_list<string>::iterator i = _platforms.begin(); i != _platforms.end(); i++)
    //    AddPlatform(i->c_str());
}

void Project::AddConfiguration(const char* configuration)
{
    AddConfigurations( { configuration } );
    //if (!vectorContains(configurations, configuration))
    //    configurations.push_back(configuration);
}

void Project::AddConfigurations(std::initializer_list<std::string> _configurations)
{
    PlatformConfigurationsUpdated(_configurations, false, true);
    //configurations.reserve(_configurations.size());
    //for (initializer_list<string>::iterator i = _configurations.begin(); i != _configurations.end(); i++)
    //    AddConfiguration(i->c_str());
}

const wchar_t* PropertyGroup = L"PropertyGroup";


void Project::PlatformConfigurationsUpdated(initializer_list<string> items, bool bPlatforms, bool bAdd)
{
    vector<string>* pConfigurations = &configurations;

    if (configurations.size() == 0)
    {
        static vector<string> dummyDefaults;

        if (bPlatforms)
        {
            if (dummyDefaults.size() == 0)
            {
                dummyDefaults.push_back("Debug");
                dummyDefaults.push_back("Release");
            }

            pConfigurations = &dummyDefaults;
        }
        else
        {
            PlatformConfigurationsUpdated({ "Debug" , "Release" }, false, false);
        }
    }
    
    vector<string>* listMain = (bPlatforms) ? &platforms : pConfigurations;
    vector<string>* list2 = (bPlatforms) ? pConfigurations : &platforms;

    for (initializer_list<string>::iterator i = items.begin(); i != items.end(); i++)
    {
        const string& name = *i;
        const auto& it = find(listMain->begin(), listMain->end(), name);
        bool found = it != listMain->end();

        // Already have that name or don't have (nothing to remove)
        if ((bAdd && found) || (!bAdd && !found))
            continue;

        int index = 0;
        int from = 0, to = (int)list2->size(), inc = 1;
        
        if (bAdd)
        {
            index = (int)listMain->size();
        }
        else
        {
            index = (int)distance(listMain->begin(), it);
            from = to; to = 0; inc = -1;
        }

        xml_node proj = project();
        xml_node itemGroup = proj.first_child();

        for (int j = from; j != to; j += inc)
        {
            int to;
            string platform;
            string configuration;

            if (bPlatforms)
            {
                to = index * (int)pConfigurations->size() + j;
                platform = name;
                configuration = pConfigurations->at(j);
            }
            else
            {
                to = (int)platforms.size() * j + index;
                platform = platforms[j];
                configuration = name;
            }

            if (bAdd) to--;

            //
            //  <ItemGroup Label="ProjectConfigurations">
            //    <ProjectConfiguration Include="Debug|Win32">
            //      <Configuration>Debug</Configuration>
            //
            xml_node c;
            const wchar_t* ProjectConfiguration = L"ProjectConfiguration";

            if(to != -1)
                c = itemGroup.select_nodes(ProjectConfiguration)[to].node();
            
            wstring platformConfiguration = as_wide(configuration + "|" + platform);
            
            if (bAdd)
            {
                xml_node pc;

                if (!c.empty())
                    pc = itemGroup.insert_child_after(ProjectConfiguration, c);
                else
                    pc = itemGroup.append_child(ProjectConfiguration);

                pc.append_child(L"Configuration").text().set(as_wide(configuration).c_str());
                pc.append_child(L"Platform").text().set(as_wide(platform).c_str());
                pc.append_attribute(L"Include").set_value( platformConfiguration.c_str() );
            }
            else
                itemGroup.remove_child(c);

            c = xml_node();
            if (to != -1)
                c = select_nodes(L"/Project/PropertyGroup[@Label='Configuration']")[to].node();

            if (bAdd)
            {
                xml_node pg;
                if (!c.empty())
                    pg = proj.insert_child_after(PropertyGroup, c);
                else
                    pg = proj.insert_child_after(PropertyGroup, markForPropertyGroup);

                pg.append_attribute(L"Condition").set_value((wstring(L"'$(Configuration)|$(Platform)'=='") + platformConfiguration + L"'").c_str());
                pg.append_attribute(L"Label").set_value(L"Configuration");
                pg.append_child(L"PlatformToolset").text().set(as_wide(GetToolset()).c_str());
            }
            else
            {
                proj.remove_child(c);
            }
        }

        if (bAdd)
            listMain->push_back(name);
        else
            listMain->erase(it);

    } //for
} //PlatformConfigurationsUpdated


//  Gets list of currently supported configurations, in form "<configuration>|<platform>"
vector<string> Project::GetConfigurations()
{
    vector<string> confs;

    for (auto p : platforms)
        for (auto c : configurations)
            confs.push_back(c + "|" + p);

    return confs;
}

// Queries for currently selected toolset, if none is selected, tries to determine from visual studio format version
std::string Project::GetToolset()
{
    string toolset = this->toolset;
    if (toolset.size() == 0)
    {
        switch (vsVersion)
        {
        case 2010: toolset = "v100"; break;
        case 2012: toolset = "v110"; break;
        case 2013: toolset = "v120"; break;
        case 2015: toolset = "v140"; break;
        case 2017: toolset = "v141"; break;
        case 2019: toolset = "v142"; break;
        default:
            // Try to guess the future. 2021 => "v160" ?
            toolset = "v" + to_string(((vsVersion - 2021) + 16) * 10);
            break;
        }
    }

    return toolset;
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
    ws.resize(size);
    _vsnwprintf(&ws[0], size, format, args);
    va_end(args);
    return ws;
}

//
//  Creates project as .xml or returns existing.
//
pugi::xml_node Project::project()
{
    // Specify utf-8 encoding.
    pugi::xml_node decl;

    for (auto n : children())
        if (n.type() == pugi::node_declaration)
            decl = n;

    // Xml declaration
    if (decl.empty())
    {
        decl = prepend_child(pugi::node_declaration);
        decl.append_attribute(L"version") = L"1.0";
        decl.append_attribute(L"encoding") = L"utf-8";
    }

    // Project itself
    xml_node proj = child(L"Project");
    if (proj.empty())
    {
        proj = append_child(L"Project");

        // Project configurations
        xml_node itemGroup = proj.append_child(L"ItemGroup");
        itemGroup.append_attribute(L"Label").set_value(L"ProjectConfigurations");

        // Project globals (Guid, etc...)
        xml_node propertyGroup = proj.append_child(PropertyGroup);
        propertyGroup.append_attribute(L"Label").set_value(L"Globals");
        propertyGroup.append_child(L"ProjectGuid");

        // Magical xml imports.
        markForPropertyGroup = proj.append_child(L"Import");
        markForPropertyGroup.append_attribute(L"Project").set_value(LR"($(VCTargetsPath)\Microsoft.Cpp.Default.props)");
     
        proj.append_child(L"Import").append_attribute(L"Project").set_value(LR"($(VCTargetsPath)\Microsoft.Cpp.targets)");
    }

    return proj;
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

    select_node(L"/Project/PropertyGroup[@Label='Globals']/ProjectGuid").node().text().set(GetGuid().c_str());

    bool b  = save_file(path.c_str(), L"  ", format_indent | format_save_file_text | format_write_bom, encoding_utf8);
    return b;
}



