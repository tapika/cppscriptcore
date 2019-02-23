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

