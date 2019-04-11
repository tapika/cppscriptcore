#include <windows.h>        //DWORD
#include <WinNT.h>          //PIMAGE_DOS_HEADER
#include <map>

// Originated from:
// http://purefractalsolutions.com/show.php?a=utils/expdef

using namespace std;

#define PEEF_PRINT_ORDINALS              0x01
#define PEEF_ORDER_BY_ORDINALS           0x02
#define PEEF_MAKE_UNDERSCORE_ALIAS       0x04
#define PEEF_MAKE_NO_UNDERSCORE_ALIAS    0x08
#define PEEF_USE_FILENAME                0x10
#define PEEF_CALL_LIB                    0x20
#define PEEF_VERBOSE                     0x40

static void printPeExports(FILE *pf, DWORD *base, DWORD flags)
{
    PIMAGE_DOS_HEADER             pDOSHeader = (PIMAGE_DOS_HEADER)base;
    PIMAGE_NT_HEADERS             pNTHeaders = (PIMAGE_NT_HEADERS)((const char*)base + pDOSHeader->e_lfanew);
    PIMAGE_EXPORT_DIRECTORY       pExportDir = (PIMAGE_EXPORT_DIRECTORY)((const char*)base + pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
    PDWORD                        pAddr = (DWORD *)((const char*)base + pExportDir->AddressOfFunctions);
    PWORD                         pOrdinals = (WORD *)((const char*)base + pExportDir->AddressOfNameOrdinals);
    PDWORD                        pName = (DWORD *)((const char*)base + pExportDir->AddressOfNames);

    ::std::map< WORD, ::std::string > byOrdinal;

    const char* pModuleName = (const char*)((const char*)base + pExportDir->Name);

    //fprintf(pf, "LIBRARY %s\n", (flags&PEEF_USE_FILENAME) ? filename : (const char *)(pModuleName));
    fprintf(pf, "EXPORTS\n");

    for (DWORD i = 0; i < pExportDir->NumberOfNames; i++)
    {
        if (flags&PEEF_ORDER_BY_ORDINALS)
        {
            byOrdinal[(WORD)(pExportDir->Base) + pOrdinals[i]] = (const char*)base + pName[i];
        }
        else
        {
            const char* fnName = (const char*)((const char*)base + pName[i]);
            fprintf(pf, "    %s", fnName);
            if (flags&PEEF_PRINT_ORDINALS)
                fprintf(pf, "    @%d", pExportDir->Base + pOrdinals[i]);
            fprintf(pf, "\n");

            if (flags&PEEF_MAKE_NO_UNDERSCORE_ALIAS && *fnName == '_')
            {
                const char* newName = fnName;
                for (; *newName && *newName == '_'; ++newName) {}
                fprintf(pf, "    %s=%s\n", newName, fnName);
            }

            if (flags&PEEF_MAKE_UNDERSCORE_ALIAS)
                fprintf(pf, "    _%s=%s\n", fnName, fnName);

            fflush(pf);
        }
    }
    if (flags&PEEF_ORDER_BY_ORDINALS)
    {
        ::std::map< WORD, ::std::string >::const_iterator it = byOrdinal.begin();
        for (; it != byOrdinal.end(); ++it)
        {
            fprintf(pf, "    %s", (const char*)(it->second.c_str()));
            if (flags&PEEF_PRINT_ORDINALS)
                fprintf(pf, "    @%d", it->first);
            fprintf(pf, "\n");
            if (flags&PEEF_MAKE_NO_UNDERSCORE_ALIAS && *it->second.c_str() == '_')
            {
                const char* newName = it->second.c_str();
                for (; *newName && *newName == '_'; ++newName) {}
                fprintf(pf, "    %s=%s\n", newName, it->second.c_str());
            }
            if (flags&PEEF_MAKE_UNDERSCORE_ALIAS)
                fprintf(pf, "    _%s=%s\n", it->second.c_str(), it->second.c_str());
            fflush(pf);
        }
    }
}

int printPeExports(const wchar_t* dll)
{
    auto h = LoadLibraryExW(dll, 0, LOAD_LIBRARY_AS_DATAFILE);
    if(!h)
    {
        printf("Error: Failed to load dll '%S'", dll);
        return -3;
    }

    printPeExports(stdout, (DWORD*)h,0);
    FreeLibrary(h);

    return 0;
}

