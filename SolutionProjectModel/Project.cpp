#include "Project.h"
#include <stdio.h>
#include <boost/uuid/detail/sha1.hpp>
#include <cguid.h>                                      //GUID_NULL
#include <objbase.h>                                    //StringFromCLSID
#include <filesystem>

using namespace pugi;
using namespace std;
//using namespace experimental;                           //filesystem
using namespace filesystem;

//
// Expose over .dll boundary
//
template class __declspec(dllexport) std::allocator<char>;
template class __declspec(dllexport) std::basic_string<char, std::char_traits<char>, std::allocator<char> >;

Project::Project()
{
    New();
}

Project::Project( const wchar_t* _name )
{
    New();
    name = _name;
}

void Project::SetSaveDirectory(const wchar_t* dir)
{
    saveDir = dir;
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

wstring Project::GetSaveDirectory()
{
    if (!saveDir.empty())
        return saveDir;

    wchar_t dir[MAX_PATH] = { 0 };
    GetModuleFileNameW((HINSTANCE)&__ImageBase, dir, _countof(dir));
    *wcsrchr(dir, '\\') = 0;
    return dir;
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
        proj.insert_attribute_before(L"ToolsVersion", proj.last_attribute() ).set_value(toolsVersion);
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


void Project::AddPlatform(const char* platform)
{
    AddPlatforms({ platform });
}

void Project::AddPlatforms(initializer_list<string> _platforms)
{
    PlatformConfigurationsUpdated(_platforms, true, true);
}

void Project::AddConfiguration(const char* configuration)
{
    AddConfigurations( { configuration } );
}

void Project::AddConfigurations(std::initializer_list<std::string> _configurations)
{
    PlatformConfigurationsUpdated(_configurations, false, true);
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
                
                const wchar_t* xmltag[] = { L"PlatformToolset" , L"Keyword", L"WindowsTargetPlatformVersion" };
                string (Project::*func[])() = { &Project::GetToolset, &Project::GetKeyword, &Project::GetWindowsSDKVersion };

                for (int i = 0; i < _countof(xmltag); i++)
                    pg.append_child(xmltag[i]).text().set(as_wide( (this->*func[i])() ).c_str());
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
//  Adds files to the project.
//
void Project::AddFiles(std::initializer_list<std::wstring> fileList)
{
    for (wstring f : fileList)
        AddFile(f.c_str());
}

void Project::AddFile(const wchar_t* file)
{
    wstring projectDir = GetSaveDirectory();
    path pathFile(file);
    if( !pathFile.is_absolute() )
        pathFile = path(projectDir).append(file);

    pathFile = weakly_canonical(pathFile);
    wstring relativePath = relative(pathFile, projectDir);

    auto it = find_if(files.begin(), files.end(), [relativePath](ProjectFile& f) { return f.relativePath == relativePath; } );
    if( it != files.end() )
        return;

    xml_node proj = project();
    xml_node markInsert = markForPropertyGroup;
    wstring name;

    while( (name = markInsert.next_sibling().name()) == L"Import" || name == L"PropertyGroup" || name == L"PropertyGroup" || name == L"ItemDefinitionGroup")
        markInsert = markInsert.next_sibling();

    ItemType newType = ProjectFile::GetFromPath(relativePath.c_str());
    xml_node itemGroup;

    for(xml_node next = markInsert.next_sibling() ; (name = next.name() ) == L"ItemGroup"; )
    {
        ItemType type;

        if( StringToEnum( as_utf8(next.first_child().name()).c_str() , type))
        {
            if(newType > type)
            {
                markInsert = next; next = next.next_sibling();
                continue;
            }

            if(type == newType)
                itemGroup = next;
        }

        break;
    }

    if(itemGroup.empty())
        itemGroup = proj.insert_child_after(L"ItemGroup", markInsert);

    ProjectFile p;
    p.relativePath = relativePath;
    p.node = itemGroup.append_child(as_wide(EnumToString(newType)).c_str());
    p.node.append_attribute(L"Include").set_value(relativePath.c_str());
    files.push_back(p);
}


//
// Clears existing project
//
void Project::New()
{
    // Reset parsing variables.
    markForPropertyGroup = projectGlobals = xml_node();
    
    pugi::xml_document::reset();
    guid = GUID_NULL;
    SetVsVersion(2017);     // May change without further notice
}


//
// Loads .vcxproj file.
//
bool Project::Load(const wchar_t* file)
{
    New();
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

string Project::GetKeyword()
{
    if (Keyword.empty())
        return "Win32Proj";     //Windows project (32 or 64 bit)

    return Keyword;
}

string Project::GetWindowsSDKVersion()
{
    return "10.0.17134.0";      // Hardcoded - fixme
}

//
//  Creates project as .xml or returns existing.
//
pugi::xml_node Project::project()
{
    // Specify utf-8 encoding.
    pugi::xml_node decl;

    for (auto markInsert : children())
        if (markInsert.type() == pugi::node_declaration)
            decl = markInsert;

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
        proj.append_attribute(L"xmlns").set_value(L"http://schemas.microsoft.com/developer/msbuild/2003");
    }

    // Project configurations
    xml_node itemGroup = proj.first_child();
    if (itemGroup.empty())
    {
        itemGroup = proj.append_child(L"ItemGroup");
        itemGroup.append_attribute(L"Label").set_value(L"ProjectConfigurations");
    }

    // Project globals.
    if (projectGlobals.empty())
    {
        projectGlobals = itemGroup.next_sibling();
        if (projectGlobals.empty())
        {
            // Project globals (Guid, etc...)
            projectGlobals = proj.insert_child_after(PropertyGroup, itemGroup);
            projectGlobals.append_attribute(L"Label").set_value(L"Globals");
            projectGlobals.append_child(L"ProjectGuid");
        }
    }

    // Magical xml imports.
    if (markForPropertyGroup.empty())
    {
        markForPropertyGroup = proj.append_child(L"Import");
        markForPropertyGroup.append_attribute(L"Project").set_value(LR"($(VCTargetsPath)\Microsoft.Cpp.Default.props)");

        proj.append_child(L"Import").append_attribute(L"Project").set_value(LR"($(VCTargetsPath)\Microsoft.Cpp.props)");
        proj.append_child(L"Import").append_attribute(L"Project").set_value(LR"($(VCTargetsPath)\Microsoft.Cpp.targets)");
    }

    return proj;
}

//
// Saves project file
//
bool Project::Save(const wchar_t* file)
{
    wstring fpath;

    if (file)
    {
        fpath = file;

        // Update project name
        name = path(file).stem();
        guid = GUID_NULL;
    }
    else
        fpath = name + L".vcxproj";

    project();
    projectGlobals.child(L"ProjectGuid").text().set(GetGuid().c_str());
    
    bool b  = save_file(fpath.c_str(), L"  ", format_indent | format_save_file_text | format_write_bom, encoding_utf8);
    return b;
}



