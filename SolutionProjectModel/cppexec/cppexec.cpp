    #include "..\Project.h"
    #include <filesystem>
    using namespace std;
    using namespace filesystem;

    int wmain(int argc, wchar_t** argv)
    {
        path exePath(argv[0]);
        auto dir = weakly_canonical(exePath).parent_path();
        path scriptToRun;

        for (auto& pit : directory_iterator(dir))
        {
            auto& filePath = pit.path();
            auto ext = filePath.extension();

            if( ext == ".cpp")
            {
                if(scriptToRun.empty())
                {
                    scriptToRun = filePath;
                }
                else
                {
                    scriptToRun.clear();
                    break;
                }
            }
        } //for

        if(scriptToRun.empty())
        {
            printf("Usage: %S <.cpp script to run>\r\n", exePath.stem().c_str());
            return -2;
        }

        return 0;
    }

