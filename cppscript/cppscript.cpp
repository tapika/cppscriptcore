#include "..\pugixml\pugixml.hpp"
#include <atlstr.h>
#include <time.h>

using namespace pugi;

int main(void)
{
    time_t start, end;
    time(&start);
    {
        xml_document doc;
        CString f(L"\\PrototypingQuick\\ConsoleApplication1\\ConsoleApplication1.vcxproj");
        //doc.load_file(f, parse_default | parse_declaration | parse_ws_pcdata_single);

		auto root = doc.append_child(L"Project");

		pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
		decl.append_attribute(L"version") = L"1.0";
		decl.append_attribute(L"encoding") = L"utf-8";

		root.append_attribute(L"DefaultTargets").set_value(L"Build");
		root.append_attribute(L"ToolsVersion").set_value(L"15.0");
		root.append_attribute(L"xmlns").set_value(L"http://schemas.microsoft.com/developer/msbuild/2003");


		doc.save_file((f + L"2.xml").GetBuffer(), L"  ", format_indent | format_save_file_text, encoding_utf8);
	
	}
    time(&end);
    double dif = difftime(end, start);
    printf("Elasped time is %.2lf seconds.\r\n", dif);

    return 0;
}

