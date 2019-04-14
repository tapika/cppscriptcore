#include "pch.h"
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
template class __declspec(dllexport) std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>;

const wchar_t* Microsoft_Cpp_Default_props = LR"($(VCTargetsPath)\Microsoft.Cpp.Default.props)";
const wchar_t* Microsoft_Cpp_props = LR"($(VCTargetsPath)\Microsoft.Cpp.props)";


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


void Project::AddPlatform(const wchar_t* platform)
{
    AddPlatforms({ platform });
}

void Project::AddPlatforms(initializer_list<wstring> _platforms)
{
    PlatformConfigurationsUpdated(_platforms, true, true);
}

void Project::AddConfiguration(const wchar_t* configuration)
{
    AddConfigurations( { configuration } );
}

void Project::AddConfigurations(std::initializer_list<std::wstring> _configurations)
{
    PlatformConfigurationsUpdated(_configurations, false, true);
}

const wchar_t* PropertyGroup = L"PropertyGroup";


vector<wstring>& Project::GetConfigurationNames()
{
    if (configurationNames.size() == 0)
    {
        static vector<wstring> dummyDefaults;

        if (dummyDefaults.size() == 0)
        {
            dummyDefaults.push_back(L"Debug");
            dummyDefaults.push_back(L"Release");
        }

        return dummyDefaults;
    }

    return configurationNames;
}


pugi::xml_node Project::selectProjectNodes(const wchar_t* _name2select, const wchar_t* _label, const wchar_t* confName, const wchar_t* platform)
{
    xml_node next, current = project().first_child();
    wstring name2select = _name2select;

    wstring name;
    bool bLabelAfterCondition = false;

    if( name2select == L"ImportGroup")
    {
        current = select_node(L"/Project/ImportGroup[@Label='ExtensionSettings']").node();
        bLabelAfterCondition = true;
    }
    else
    {
        // Locate target point where nodes should be
        for (next = current.next_sibling(); !next.empty(); current = next, next = next.next_sibling())
        {
            name = next.name();
            if (name == L"ItemGroup" && !next.attribute(L"Label").empty())
                continue;

            if (name == L"ImportGroup")
                continue;

            if (name == L"PropertyGroup")
                continue;

            if (name == L"Import" && wcscmp(next.attribute(L"Project").value(), Microsoft_Cpp_Default_props) == 0)
                continue;

            if (name == L"Import" && *_label == 0 && wcscmp(next.attribute(L"Project").value(), Microsoft_Cpp_props) == 0)
                continue;

            break;
        }
    }

    return LocateInsert(current, false, _name2select, confName, platform, _label, bLabelAfterCondition);
}


//
//  Locates xml node for specific configuration / platform, and appends new xml code if not found.
//  name2select specifies xml node name, 
//  label specifies additional selector / label xml attribute.
//
xml_node LocateInsert( xml_node current, bool asChild, const wchar_t* name2select,
    const wchar_t* confName, const wchar_t* platform, const wchar_t* label, bool bLabelAfterCondition )
{
    xml_node next, parent;
    wstring name;
    xml_attribute attr;
    xml_node selected;

    if(label && *label == 0)
        label = nullptr;

    if( asChild )
    {
        parent = current;
        next = current.first_child();
        current = xml_node();
    }
    else
        next = current.next_sibling();

    for (; !next.empty(); current = next, next = next.next_sibling())
    {
        name = next.name();
        if (name != name2select)
            break;

        if( label )
        {
            auto attr = next.attribute(L"Label");
            if (wcscmp(label,attr.value()) != 0)
                break;
        }

        attr = next.attribute(L"Condition");
        if (attr.empty())
        {
            selected = next;
            break;
        }

        static wregex reEqual(L"'(.*?)'=='(.*)'");
        wstring attrValue(attr.value());
        wsmatch sm;
        if (!regex_search(attrValue, sm, reEqual))
            continue;
    }

    // Insert new node if does not exists already.
    if (selected.empty())
    {
        if (asChild)
        {
            if(current.empty())
                selected = parent.append_child(name2select);
            else
                selected = parent.insert_child_after(name2select, current);
        }
        else
            selected = current.parent().insert_child_after(name2select, current);
        selected.append_attribute(L"Condition").set_value(wformat(L"'$(Configuration)|$(Platform)'=='%s|%s'", confName, platform).c_str());

        if (label)
        {
            if (bLabelAfterCondition)
                selected.prepend_attribute(L"Label").set_value(label);
            else
                selected.append_attribute(L"Label").set_value(label);
        }
    }

    return selected;
}



void Project::PlatformConfigurationsUpdated(initializer_list<wstring> items, bool bPlatforms, bool bAdd)
{
    vector<wstring>* pConfigurations = &configurationNames;

    if (configurationNames.size() == 0)
    {
        if (bPlatforms)
            pConfigurations = &GetConfigurationNames();
        else
            PlatformConfigurationsUpdated({ L"Debug" , L"Release" }, false, false);
    }
    
    vector<wstring>* listMain = (bPlatforms) ? &platforms : pConfigurations;
    vector<wstring>* list2 = (bPlatforms) ? pConfigurations : &platforms;

    for (initializer_list<wstring>::iterator i = items.begin(); i != items.end(); i++)
    {
        const wstring& name = *i;
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
            int to;         // Index where to insert / from where to remove within single configuration array
            wstring platform;
            wstring configuration;

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

            wstring platformConfiguration = configuration + L"|" + platform;
            shared_ptr<VCConfiguration> conf = nullptr;

            if (bAdd)
            {
                conf.reset(new VCConfiguration());
                configurations.insert(configurations.begin() + to, conf);
                conf->project = this;
                conf->configurationName = configuration;
                conf->platform = platform;
                to--;   //Xml node backshift - previous node after which to insert.
            }

            //
            //  <ItemGroup Label="ProjectConfigurations">
            //    <ProjectConfiguration Include="Debug|Win32">
            //      <Configuration>Debug</Configuration>
            //
            xml_node c;
            const wchar_t* ProjectConfiguration = L"ProjectConfiguration";

            if(to != -1)
                c = itemGroup.select_nodes(ProjectConfiguration)[to].node();
            
            if (bAdd)
            {
                xml_node pc;

                if (!c.empty())
                    pc = itemGroup.insert_child_after(ProjectConfiguration, c);
                else
                    pc = itemGroup.append_child(ProjectConfiguration);

                pc.append_child(L"Configuration").text().set(configuration.c_str());
                pc.append_child(L"Platform").text().set(platform.c_str());
                pc.append_attribute(L"Include").set_value( platformConfiguration.c_str() );
            }
            else
                itemGroup.remove_child(c);

            xml_node node = selectProjectNodes(L"PropertyGroup", L"Configuration", configuration.c_str(), platform.c_str());

            if( bAdd )
            {
                conf->pgConfigurationNode = node;
                // New configuration defaults
                auto& general = conf->General;
                general.ConfigurationType = conftype_Application;
                general.PlatformToolset = GetToolset().c_str();
                general.CharacterSet = charset_Unicode;
            }

            node = selectProjectNodes(L"ImportGroup", L"PropertySheets", configuration.c_str(), platform.c_str());

            if (bAdd)
            {
                xml_node impNode;
                impNode = node.append_child(L"Import");
                impNode.append_attribute(L"Project").set_value(LR"($(UserRootDir)\Microsoft.Cpp.$(Platform).user.props)");
                impNode.append_attribute(L"Condition").set_value(LR"(exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props'))");
                impNode.append_attribute(L"Label").set_value(L"LocalAppDataPlatform");
            }
        }

        if (bAdd)
            listMain->push_back(name);
        else
            listMain->erase(it);

    } //for

} //PlatformConfigurationsUpdated


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
        File(f.c_str(), true);
}

ProjectFile* Project::File(const wchar_t* file, bool add)
{
    wstring projectDir = GetSaveDirectory();
    path pathFile(file);
    if( !pathFile.is_absolute() )
        pathFile = path(projectDir).append(file);

    pathFile = weakly_canonical(pathFile);
    wstring relativePath = relative(pathFile, projectDir);

    auto it = find_if(files.begin(), files.end(), [relativePath](auto& f) { return f->relativePath == relativePath; } );
    if( it != files.end() )
        return it->get();

    if( !add )
        return nullptr;

    xml_node proj = project();
    xml_node markInsert = markForPropertyGroup;
    xml_node next;
    wstring name;

    // Skip nodes with specific names.
    for (next = markForPropertyGroup.next_sibling(); !next.empty(); markInsert = next, next = next.next_sibling())
    {
        name = next.name();
        if( name == L"PropertyGroup" || name == L"PropertyGroup" || name == L"ItemDefinitionGroup")
            continue;

        break;
    }

    EItemType newType = ProjectFile::GetFromPath(relativePath.c_str());
    xml_node itemGroup;

    for(xml_node next = markInsert.next_sibling() ; (name = next.name() ) == L"ItemGroup"; )
    {
        EItemType type;

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

    shared_ptr<ProjectFile> p(new ProjectFile());
    p->relativePath = relativePath;
    p->project = this;
    p->node = itemGroup.append_child(as_wide(EnumToString(newType)).c_str());
    p->node.append_attribute(L"Include").set_value(relativePath.c_str());
    files.push_back(p);
    return p.get();
}

//
// Visits each project configuration, if configurationName & platformName - uses additional filtering, otherwise visits all configurations.
//
void Project::VisitConfigurations(std::function<void (VCConfiguration&)> visitConf, const wchar_t* configurationName, const wchar_t* platformName)
{
    vector<wstring>& confNames = GetConfigurationNames();
    for (size_t p = 0; p < platforms.size(); p++)
        for( size_t c = 0; c < confNames.size(); c++)
        {
            if(configurationName != nullptr && confNames[c] != configurationName)
                continue;

            if(platformName != nullptr && platforms[p] != platformName)
                continue;

            size_t i = platforms.size() * p + c;
            visitConf(*configurations[i]);
        }
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

    ReflectConnectChildren(nullptr);
    Globals.Keyword = projecttype_Win32Proj;
    Globals.WindowsTargetPlatformVersion = "10.0.17134.0";
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
        void (Project::*func [])(const wchar_t*) = { &Project::AddConfiguration, &Project::AddPlatform };

        for( int i = 0; i < _countof(xmltag); i++)
            (this->*func[i])( conf.child(xmltag[i]).text().get() );
    }

    return true;
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
        proj.append_attribute(L"DefaultTargets").set_value(L"Build");
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
        }
    }

    // Magical xml imports.
    if (markForPropertyGroup.empty())
    {
        proj.append_child(L"Import").append_attribute(L"Project").set_value(Microsoft_Cpp_Default_props);
        proj.append_child(L"Import").append_attribute(L"Project").set_value(Microsoft_Cpp_props);

        auto addImportGroup = [&](auto label)
        {
            xml_node impGroup;
            (impGroup = proj.append_child(L"ImportGroup")).append_attribute(L"Label").set_value(label);
            impGroup.text().set(L"\r\n  ");
        };

        addImportGroup(L"ExtensionSettings");
        (markForPropertyGroup = proj.append_child(L"PropertyGroup")).append_attribute(L"Label").set_value(L"UserMacros");
        proj.append_child(L"Import").append_attribute(L"Project").set_value(LR"($(VCTargetsPath)\Microsoft.Cpp.targets)");
        addImportGroup(L"ExtensionTargets");
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
    Globals.ProjectGuid = GetGuid().c_str();
    
    fpath = GetSaveDirectory() + L"\\" + fpath;

    if(filesystem::exists(fpath))
        copy(fpath, path(fpath + L".bkp"), copy_options::overwrite_existing);

    bool b  = save_file(fpath.c_str(), L"  ", format_indent | format_save_file_text | format_write_bom, encoding_utf8);
    return b;
}

void Project::OnAfterSetProperty(ReflectPath& path)
{
    ReflectCopy(path, projectGlobals);
}

