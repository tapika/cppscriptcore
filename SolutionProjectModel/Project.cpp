#include "Project.h"
#include <stdio.h>
using namespace pugi;
using namespace std;

//
// Expose over .dll boundary
//
template class __declspec(dllexport) std::allocator<char>;
template class __declspec(dllexport) std::basic_string<char, std::char_traits<char>, std::allocator<char> >;


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
// Saves project file
//
bool Project::Save(const wchar_t* file)
{
    xml_node proj = child(L"Project");
    if (proj.empty())
        proj = append_child(L"Project");
    
    
    xml_node confs = proj.select_node(L"ItemGroup[@Label='ProjectConfigurations']").node();
    if (confs.empty())
    {
        confs = proj.append_child(L"ItemGroup");
        confs.append_attribute(L"Label").set_value(L"ProjectConfigurations");
    }

    if (!configurations.size())
        AddConfigurations({"Debug", "Release" });

    for (auto n : confs.children()) confs.remove_child(n);

    for( auto p: platforms )
        for (auto c : configurations)
        {
            xml_node n = confs.append_child(L"ProjectConfiguration");
            n.append_child(L"Configuration").text().set( as_wide(p).c_str() );
            n.append_child(L"Platform").text().set( as_wide(c).c_str() );
        }

    bool b  = save_file(file, L"  ", format_indent | format_save_file_text, encoding_utf8);
    return b;
}



