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
        CString f(L"C:\\PrototypingQuick\\ConsoleApplication1\\ConsoleApplication1.vcxproj");
        doc.load_file(f);
        doc.save_file((f + L"2.xml").GetBuffer(), L"  ", format_indent | format_save_file_text, encoding_utf8);
    }
    time(&end);
    double dif = difftime(end, start);
    printf("Elasped time is %.2lf seconds.\r\n", dif);

    return 0;
}

