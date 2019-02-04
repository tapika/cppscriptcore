#include "Project.h"
#include <stdio.h>
using namespace pugi;


bool Project::Load(const wchar_t* file)
{
    xml_parse_result res = load_file(file, parse_default | parse_declaration | parse_ws_pcdata_single);

    if (res.status != status_ok)
        return false;

    xml_node node = select_node(L"/Project/ItemGroup[@Label='ProjectConfigurations']").node();

    for (xml_node conf : node.children())
    {
        xml_attribute attrib = conf.attribute(L"Include");
        printf("%S\r\n", attrib.value());
        printf("Configuration = %S , ", conf.child(L"Configuration").text().get());
        printf("Platform = %S\r\n", conf.child(L"Platform").text().get());
    }

    return true;
}

