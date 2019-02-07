#include "Project.h"
#include <stdio.h>
using namespace pugi;

//
// Expose over .dll boundary
//
template class __declspec(dllexport) std::allocator<char>;
template class __declspec(dllexport) std::basic_string<char, std::char_traits<char>, std::allocator<char> >;


bool Project::Load(const wchar_t* file)
{
    xml_parse_result res = load_file(file, parse_default | parse_declaration | parse_ws_pcdata_single);

    if (res.status != status_ok)
        return false;

    xml_node node = select_node(L"/Project/ItemGroup[@Label='ProjectConfigurations']").node();

    for (xml_node conf : node.children())
    {
        Configuration c;
        c.ConfigurationName = as_utf8(conf.child(L"Configuration").text().get());
        c.PlatformName = as_utf8(conf.child(L"Platform").text().get());
        Configurations.push_back(c);
    }

    return true;
}

